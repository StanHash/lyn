#include "event_object.h"

#include <iomanip>
#include <algorithm>

#include "../ea/event_section.h"
#include "util/hex_write.h"

namespace lyn {

void event_object::append_from_elf(const elf_file& elfFile) {
	std::vector<section_data> newSections(elfFile.sections().size());
	std::vector<bool> outMap(elfFile.sections().size(), false);

	auto getLocalSymbolName = [this] (int section, int index) -> std::string {
		std::string result;
		result.reserve(4 + 8 + 4 + 4);

		result.append("_L");

		util::append_hex(result, size());
		result.append("_");
		util::append_hex(result, section);
		result.append("_");
		util::append_hex(result, index);

		return result;
	};

	auto getGlobalSymbolName = [this] (const char* name) -> std::string {
		std::string result(name);
		int find = 0;

		while ((find = result.find('.')) != std::string::npos)
			result[find] = '_';

		return result;
	};

	// Initializing written Sections from relevant elf sections

	for (int i=0; i<newSections.size(); ++i) {
		auto& elfSection = elfFile.sections()[i];

		if ((elfSection.sh_type == elf::SHT_PROGBITS)) {
			if ((elfSection.sh_flags & elf::SHF_ALLOC) && !(elfSection.sh_flags & elf::SHF_WRITE)) {
				auto& sectionData = newSections[i];

				sectionData.set_name(elfFile.get_section_name(elfSection));
				outMap[i] = true;
				sectionData.load_from_other(elfFile, elfSection.sh_offset, elfSection.sh_size);
			}
		}
	}

	// Initializing labels, mappings & relocations from other elf sections

	for (int sectionIndex=0; sectionIndex<elfFile.sections().size(); ++sectionIndex) {
		auto& elfSection = elfFile.sections()[sectionIndex];

		if (elfSection.sh_type == elf::SHT_SYMTAB) {
			int symbolCount = elfSection.sh_size / elfSection.sh_entsize;

			auto& nameElfSection = elfFile.sections()[elfSection.sh_link];

			for (int i=0; i<symbolCount; ++i) {
				auto sym = elfFile.symbol(elfSection, i);

				if (sym.st_shndx >= newSections.size()) {
					if ((sym.st_shndx == elf::SHN_ABS) && (sym.bind() == elf::STB_GLOBAL))
						mAbsoluteSymbols.push_back({ elfFile.string(nameElfSection, sym.st_name), sym.st_value, false });
					continue;
				}

				if (!outMap[sym.st_shndx])
					continue;

				auto& sectionData = newSections[sym.st_shndx];

				std::string symbolName = elfFile.string(nameElfSection, sym.st_name);

				if (sym.type() == elf::STT_NOTYPE && sym.bind() == elf::STB_LOCAL) {
					std::string subString = symbolName.substr(0, 3);

					if ((symbolName == "$t") || (subString == "$t.")) {
						sectionData.set_mapping(sym.st_value, mapping::Thumb);
						continue;
					} else if ((symbolName == "$a") || (subString == "$a.")) {
						sectionData.set_mapping(sym.st_value, mapping::ARM);
						continue;
					} else if ((symbolName == "$d") || (subString == "$d.")) {
						sectionData.set_mapping(sym.st_value, mapping::Data);
						continue;
					}
				}

				if (sym.bind() == elf::STB_LOCAL)
					symbolName = getLocalSymbolName(sectionIndex, i);
				else
					symbolName = getGlobalSymbolName(symbolName.c_str());

				sectionData.symbols().push_back({ symbolName, sym.st_value, (sym.bind() == elf::STB_LOCAL) });
			}
		} else if (elfSection.sh_type == elf::SHT_REL) {
			int relCount = elfSection.sh_size / elfSection.sh_entsize;

			auto& symbolSection = elfFile.sections()[elfSection.sh_link];
			auto& symbolNameSection = elfFile.sections()[symbolSection.sh_link];

			auto& sectionData = newSections[elfSection.sh_info];

			for (int i=0; i<relCount; ++i) {
				auto rel  = elfFile.rel(elfSection, i);
				auto sym  = elfFile.symbol(symbolSection, rel.symId());
				std::string name = elfFile.string(symbolNameSection, sym.st_name);

				if (sym.bind() == elf::STB_LOCAL)
					name = getLocalSymbolName(elfSection.sh_link, rel.symId());
				else
					name = getGlobalSymbolName(name.c_str());

				sectionData.relocations().push_back({ std::string(name), 0, rel.type(), rel.r_offset });
			}
		} else if (elfSection.sh_type == elf::SHT_RELA) {
			int relCount = elfSection.sh_size / elfSection.sh_entsize;

			auto& symbolSection = elfFile.sections()[elfSection.sh_link];
			auto& symbolNameSection = elfFile.sections()[symbolSection.sh_link];

			auto& sectionData = newSections[elfSection.sh_info];

			for (int i=0; i<relCount; ++i) {
				auto rela = elfFile.rela(elfSection, i);
				auto sym  = elfFile.symbol(symbolSection, rela.symId());
				std::string name = elfFile.string(symbolNameSection, sym.st_name);

				if (sym.bind() == elf::STB_LOCAL)
					name = getLocalSymbolName(elfSection.sh_link, rela.symId());
				else
					name = getGlobalSymbolName(name.c_str());

				sectionData.relocations().push_back({ std::string(name), rela.r_addend, rela.type(), rela.r_offset });
			}
		}
	}

	// Remove empty sections

	newSections.erase(std::remove_if(newSections.begin(), newSections.end(), [] (const section_data& sectionData) {
		return (sectionData.size()==0);
	}), newSections.end());

	// Create the ultimate lifeform

	for (auto& newSection : newSections) {
		combine_with(std::move(newSection));
		ensure_aligned(4);
	}
}

void event_object::try_transform_relatives() {
	section_data trampolineData;

	for (auto& relocation : relocations()) {
		if (auto relocatelet = mRelocator.get_relocatelet(relocation.type)) {
			if (!relocatelet->is_absolute() && relocatelet->can_make_trampoline()) {
				std::string renamed;
				renamed.reserve(4 + relocation.symbolName.size());

				renamed.append("_LP_"); // local proxy
				renamed.append(relocation.symbolName);

				bool exists = false;

				for (auto& sym : symbols())
					if (sym.name == renamed)
						exists = true;

				if (!exists) {
					section_data newData = relocatelet->make_trampoline(relocation.symbolName, relocation.addend);
					newData.symbols().push_back({ renamed, (newData.mapping_type_at(0) == section_data::mapping::Thumb), true });

					trampolineData.combine_with(std::move(newData));
				}

				relocation.symbolName = renamed;
				relocation.addend = 0;
			}
		}
	}

	combine_with(std::move(trampolineData));
}

void event_object::try_relocate_relatives() {
	relocations().erase(
		std::remove_if(
			relocations().begin(),
			relocations().end(),
			[this] (const section_data::relocation& relocation) -> bool {
				for (auto& symbol : symbols()) {
					if (symbol.name != relocation.symbolName)
						continue;

					if (auto relocatelet = mRelocator.get_relocatelet(relocation.type)) {
						if (!relocatelet->is_absolute()) {
							relocatelet->apply_relocation(*this, relocation.offset, symbol.offset, relocation.addend);
							return true;
						}
					}

					return false;
				}

				return false;
			}
		),
		relocations().end()
	);
}

void event_object::try_relocate_absolutes() {
	relocations().erase(
		std::remove_if(
			relocations().begin(),
			relocations().end(),
			[this] (const section_data::relocation& relocation) -> bool {
				for (auto& symbol : mAbsoluteSymbols) {
					if (symbol.name != relocation.symbolName)
						continue;

					if (auto relocatelet = mRelocator.get_relocatelet(relocation.type)) {
						if (relocatelet->is_absolute()) {
							relocatelet->apply_relocation(*this, relocation.offset, symbol.offset, relocation.addend);
							return true;
						}
					}

					return false;
				}

				return false;
			}
		),
		relocations().end()
	);
}

void event_object::remove_unnecessary_symbols() {
	symbols().erase(
		std::remove_if(
			symbols().begin(),
			symbols().end(),
			[this] (const section_data::symbol& symbol) -> bool {
				if (!symbol.isLocal)
					return false; // symbol may be used outside of the scope of this object

				for (auto& reloc : relocations())
					if (reloc.symbolName == symbol.name)
						return false; // a relocation is dependant on this symbol

				return true; // symbol is local and unused, we can remove it safely (hopefully)
			}
		),
		symbols().end()
	);
}

void event_object::cleanup() {
	std::sort(
		symbols().begin(),
		symbols().end(),
		[] (const section_data::symbol& a, const section_data::symbol& b) -> bool {
			return a.offset < b.offset;
		}
	);
}

std::vector<event_object::hook> event_object::get_hooks() const {
	std::vector<hook> result;

	for (auto& absSymbol : mAbsoluteSymbols) {
		if (!(absSymbol.offset & 0x8000000))
			continue; // Not in ROM

		for (auto& locSymbol : symbols()) {
			if (absSymbol.name != locSymbol.name)
				continue; // Not same symbol

			result.push_back({ (absSymbol.offset & (~0x8000000)), absSymbol.name });
		}
	}

	return result;
}

void event_object::write_events(std::ostream& output) const {
	event_section events = make_events();

	for (auto& relocation : relocations()) {
		if (auto relocatelet = mRelocator.get_relocatelet(relocation.type))
			events.set_code(relocation.offset, relocatelet->make_event_code(
				*this,
				relocation.offset,
				relocation.symbolName,
				relocation.addend
			));
		else if (relocation.type != 40) // R_ARM_V4BX
			throw std::runtime_error(std::string("unhandled relocation type #").append(std::to_string(relocation.type)));
	}

	events.compress_codes();
	events.optimize();

	if (std::any_of(symbols().begin(), symbols().end(), [] (const section_data::symbol& sym) { return !sym.isLocal; })) {
		output << "PUSH" << std::endl;
		int currentOffset = 0;

		for (auto& symbol : symbols()) {
			if (symbol.isLocal)
				continue;

			output << "ORG (CURRENTOFFSET+$" << std::hex << (symbol.offset - currentOffset) << "); "
				   << symbol.name << ":" << std::endl;
			currentOffset = symbol.offset;
		}

		output << "POP" << std::endl;
	}

	if (std::any_of(symbols().begin(), symbols().end(), [] (const section_data::symbol& sym) { return sym.isLocal; })) {
		output << "{" << std::endl;
		output << "PUSH" << std::endl;

		int currentOffset = 0;

		for (auto& symbol : symbols()) {
			if (!symbol.isLocal)
				continue;

			output << "ORG (CURRENTOFFSET+$" << std::hex << (symbol.offset - currentOffset) << "); "
				   << symbol.name << ":" << std::endl;
			currentOffset = symbol.offset;
		}

		output << "POP" << std::endl;

		events.write_to_stream(output);

		output << "}" << std::endl;
	} else {
		events.write_to_stream(output);
	}
}

} // namespace lyn
