#include "event_object.h"

#include <fstream>

#include <iomanip>
#include <algorithm>

#include "elfcpp/elfcpp_file.h"

#include "ea/event_section.h"
#include "util/hex_write.h"

namespace elfcpp {

// for some reason this is required in order to make the whole thing work

template<int Size, bool BigEndian, typename File>
const int Elf_file<Size, BigEndian, File>::ehdr_size;

template<int Size, bool BigEndian, typename File>
const int Elf_file<Size, BigEndian, File>::phdr_size;

template<int Size, bool BigEndian, typename File>
const int Elf_file<Size, BigEndian, File>::shdr_size;

template<int Size, bool BigEndian, typename File>
const int Elf_file<Size, BigEndian, File>::sym_size;

template<int Size, bool BigEndian, typename File>
const int Elf_file<Size, BigEndian, File>::rel_size;

template<int Size, bool BigEndian, typename File>
const int Elf_file<Size, BigEndian, File>::rela_size;

} // namespace elfcpp

namespace lyn {

void event_object::append_from_elf(const char* fileName) {
	binary_file file; {
		std::ifstream fileStream;

		fileStream.open(fileName, std::ios::in | std::ios::binary);

		if (!fileStream.is_open())
			throw std::runtime_error(std::string("Couldn't open file for read: ").append(fileName)); // TODO: better error

		file.load_from_stream(fileStream);
		fileStream.close();
	}

	elfcpp::Elf_file<32, false, lyn::binary_file> elfFile(&file);

	auto readString = [&file] (elfcpp::Shdr<32, false> section, unsigned offset) -> std::string {
		return file.cstr_at(section.get_sh_offset() + offset);
	};

	std::vector<section_data> newSections(elfFile.shnum());
	std::vector<bool> outMap(elfFile.shnum(), false);

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

	for (unsigned i = 0; i < elfFile.shnum(); ++i) {
		auto flags = elfFile.section_flags(i);

		if ((flags & elfcpp::SHF_ALLOC) && !(flags & elfcpp::SHF_WRITE)) {
			auto& section = newSections.at(i);
			auto  loc     = elfFile.section_contents(i);

			section.set_name(elfFile.section_name(i));
			section.load_from_other(file, loc.file_offset, loc.data_size);

			outMap[i] = true;
		}
	}

	for (unsigned si = 0; si < elfFile.shnum(); ++si) {
		elfcpp::Shdr<32, false> header(&file, elfFile.section_header(si));

		switch (header.get_sh_type()) {

		case elfcpp::SHT_SYMTAB: {
			const unsigned count = header.get_sh_size() / header.get_sh_entsize();
			const elfcpp::Shdr<32, false> nameShdr(&file, elfFile.section_header(header.get_sh_link()));

			for (unsigned i = 0; i < count; ++i) {
				elfcpp::Sym<32, false> sym(&file, binary_file::Location(
					header.get_sh_offset() + i * header.get_sh_entsize(),
					header.get_sh_entsize()
				));

				switch (sym.get_st_shndx()) {

				case elfcpp::SHN_ABS: {
					if (sym.get_st_bind() != elfcpp::STB_GLOBAL)
						break;

					mAbsoluteSymbols.push_back(symbol {
						readString(nameShdr, sym.get_st_name()),
						sym.get_st_value(),
						false
					});

					break;
				} // case elfcpp::SHN_ABS

				default: {
					if (!outMap.at(sym.get_st_shndx()))
						break;

					auto& section = newSections.at(sym.get_st_shndx());

					std::string name = readString(nameShdr, sym.get_st_name());

					if (sym.get_st_type() == elfcpp::STT_NOTYPE && sym.get_st_bind() == elfcpp::STB_LOCAL) {
						std::string subString = name.substr(0, 3);

						if ((name == "$t") || (subString == "$t.")) {
							section.set_mapping(sym.get_st_value(), mapping::Thumb);
							break;
						} else if ((name == "$a") || (subString == "$a.")) {
							section.set_mapping(sym.get_st_value(), mapping::ARM);
							break;
						} else if ((name == "$d") || (subString == "$d.")) {
							section.set_mapping(sym.get_st_value(), mapping::Data);
							break;
						}
					}

					if (sym.get_st_bind() == elfcpp::STB_LOCAL)
						name = getLocalSymbolName(si, i);
					else
						name = getGlobalSymbolName(name.c_str());

					section.symbols().push_back(symbol {
						name,
						sym.get_st_value(),
						(sym.get_st_bind() == elfcpp::STB_LOCAL)
					});

					break;
				} // default

				} // switch (sym.get_st_shndx())
			}

			break;
		} // case elfcpp::SHT_SYMTAB

		case elfcpp::SHT_REL: {
			if (!outMap.at(header.get_sh_info()))
				break;

			const unsigned count = header.get_sh_size() / header.get_sh_entsize();

			const elfcpp::Shdr<32, false> symShdr(&file, elfFile.section_header(header.get_sh_link()));
			const elfcpp::Shdr<32, false> symNameShdr(&file, elfFile.section_header(symShdr.get_sh_link()));

			auto& section = newSections.at(header.get_sh_info());

			for (unsigned i = 0; i < count; ++i) {
				const elfcpp::Rel<32, false> rel(&file, binary_file::Location(
					header.get_sh_offset() + i * header.get_sh_entsize(),
					header.get_sh_entsize()
				));

				const elfcpp::Sym<32, false> sym(&file, binary_file::Location(
					symShdr.get_sh_offset() + elfcpp::elf_r_sym<32>(rel.get_r_info()) * symShdr.get_sh_entsize(),
					symShdr.get_sh_entsize()
				));

				const std::string name = (sym.get_st_bind() == elfcpp::STB_LOCAL)
					? getLocalSymbolName(header.get_sh_link(), elfcpp::elf_r_sym<32>(rel.get_r_info()))
					: getGlobalSymbolName(readString(symNameShdr, sym.get_st_name()).c_str());

				section.relocations().push_back(relocation {
					name,
					0,
					elfcpp::elf_r_type<32>(rel.get_r_info()),
					rel.get_r_offset()
				});
			}

			break;
		} // case elfcpp::SHT_REL

		case elfcpp::SHT_RELA: {
			if (!outMap.at(header.get_sh_info()))
				break;

			const unsigned count = header.get_sh_size() / header.get_sh_entsize();

			const elfcpp::Shdr<32, false> symShdr(&file, elfFile.section_header(header.get_sh_link()));
			const elfcpp::Shdr<32, false> symNameShdr(&file, elfFile.section_header(symShdr.get_sh_link()));

			auto& section = newSections.at(header.get_sh_info());

			for (unsigned i = 0; i < count; ++i) {
				const elfcpp::Rela<32, false> rela(&file, binary_file::Location(
					header.get_sh_offset() + i * header.get_sh_entsize(),
					header.get_sh_entsize()
				));

				const elfcpp::Sym<32, false> sym(&file, binary_file::Location(
					symShdr.get_sh_offset() + elfcpp::elf_r_sym<32>(rela.get_r_info()) * symShdr.get_sh_entsize(),
					symShdr.get_sh_entsize()
				));

				const std::string name = (sym.get_st_bind() == elfcpp::STB_LOCAL)
					? getLocalSymbolName(header.get_sh_link(), elfcpp::elf_r_sym<32>(rela.get_r_info()))
					: getGlobalSymbolName(readString(symNameShdr, sym.get_st_name()).c_str());

				section.relocations().push_back(relocation {
					name,
					rela.get_r_addend(),
					elfcpp::elf_r_type<32>(rela.get_r_info()),
					rela.get_r_offset()
				});
			}

			break;
		} // case elfcpp::SHT_RELA

		} // switch (header.get_sh_type())
	}

	// Remove empty sections

	newSections.erase(std::remove_if(newSections.begin(), newSections.end(), [] (const section_data& section) {
		return (section.size()==0);
	}), newSections.end());

	// Create the ultimate lifeform

	for (auto& section : newSections) {
		combine_with(std::move(section));
		ensure_aligned(4);
	}
}

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
