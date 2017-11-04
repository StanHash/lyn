#include "event_object.h"

#include <iomanip>
#include <algorithm>

#include "core/event_section.h"

namespace lyn {

void event_object::append_from_elf(const elf_file& elfFile) {
	std::vector<section_data> newSections(elfFile.sections().size());
	std::vector<bool> outMap(elfFile.sections().size(), false);

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

	for (auto& elfSection : elfFile.sections()) {
		if (elfSection.sh_type == elf::SHT_SYMTAB) {
			int symbolCount = elfSection.sh_size / 0x10;

			auto& nameElfSection = elfFile.sections()[elfSection.sh_link];

			for (int i=0; i<symbolCount; ++i) {
				auto sym = elfFile.symbol(elfSection, i);

				if (sym.st_shndx >= newSections.size()) {
					if ((sym.st_shndx == elf::SHN_ABS) && (sym.bind() == elf::STB_GLOBAL))
						mAbsoluteSymbols.push_back({ elfFile.string(nameElfSection, sym.st_name), sym.st_value });
					continue;
				}

				if (!outMap[sym.st_shndx])
					continue;

				auto& sectionData = newSections[sym.st_shndx];

				std::string symbolName = elfFile.string(nameElfSection, sym.st_name);

				switch (sym.type()) {
				case elf::STT_FUNC:
					// sym.st_value &= ~1; // remove eventual thumb bit
				case elf::STT_OBJECT: {
					if (sym.bind() == elf::STB_GLOBAL)
						sectionData.symbols().push_back({ symbolName, sym.st_value });

					break;
				}

				case elf::STT_NOTYPE: {
					if (sym.bind() != elf::STB_LOCAL)
						break;

					std::string subString = symbolName.substr(0, 3);

					if ((symbolName == "$t") || (subString == "$t."))
						sectionData.set_mapping(sym.st_value, mapping::Thumb);
					else if ((symbolName == "$a") || (subString == "$a."))
						sectionData.set_mapping(sym.st_value, mapping::ARM);
					else if ((symbolName == "$d") || (subString == "$d."))
						sectionData.set_mapping(sym.st_value, mapping::Data);

					break;
				}

				}
			}
		} else if (elfSection.sh_type == elf::SHT_REL) {
			int relCount = elfSection.sh_size / 0x08;

			auto& symbolSection = elfFile.sections()[elfSection.sh_link];
			auto& symbolNameSection = elfFile.sections()[symbolSection.sh_link];

			auto& sectionData = newSections[elfSection.sh_info];

			for (int i=0; i<relCount; ++i) {
				auto rel  = elfFile.rel(elfSection, i);
				auto name = elfFile.string(symbolNameSection, elfFile.symbol(symbolSection, rel.symId()).st_name);

				sectionData.relocations().push_back({ std::string(name), 0, rel.type(), rel.r_offset });
			}
		} else if (elfSection.sh_type == elf::SHT_RELA) {
			int relCount = elfSection.sh_size / 0x0C;

			auto& symbolSection = elfFile.sections()[elfSection.sh_link];
			auto& symbolNameSection = elfFile.sections()[symbolSection.sh_link];

			auto& sectionData = newSections[elfSection.sh_info];

			for (int i=0; i<relCount; ++i) {
				auto rela = elfFile.rela(elfSection, i);
				auto name = elfFile.string(symbolNameSection, elfFile.symbol(symbolSection, rela.symId()).st_name);

				sectionData.relocations().push_back({ std::string(name), rela.r_addend, rela.type(), rela.r_offset });
			}
		}
	}

	// Remove empty sections

	newSections.erase(std::remove_if(newSections.begin(), newSections.end(), [] (const section_data& sectionData) {
		return (sectionData.size()==0);
	}), newSections.end());

	// Create the ultimate lifeform

	for (auto& newSection : newSections)
		combine_with(std::move(newSection));
}

void event_object::link() {
	relocations().erase(
		std::remove_if(
			relocations().begin(),
			relocations().end(),
			[this] (const section_data::relocation& relocation) -> bool {
				return try_relocate(relocation);
			}
		),
		relocations().end()
	);
}

void event_object::write_events(std::ostream& output) const {
	event_section events = make_events();

	for (auto& relocation : relocations()) {
		if (auto relocatelet = mRelocator.get_relocatelet(relocation.type))
			events.set_code(relocation.offset, relocatelet->make_event_code(relocation.symbolName, relocation.addend));
		else
			throw std::runtime_error(std::string("RELOC ERROR: Unhandled relocation type ").append(std::to_string(relocation.type)));
	}

	events.compress_codes();
	events.optimize();

	if (!symbols().empty()) {
		output << "PUSH" << std::endl;
		int currentOffset = 0;

		for (auto& symbol : symbols()) {
			output << "ORG (CURRENTOFFSET+$" << std::hex << (symbol.offset - currentOffset) << "); "
				   << symbol.name << ":" << std::endl;
			currentOffset = symbol.offset;
		}

		output << "POP" << std::endl << std::endl;
	}

	events.write_to_stream(output);
}

bool event_object::try_relocate(const section_data::relocation& relocation) {
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

} // namespace lyn
