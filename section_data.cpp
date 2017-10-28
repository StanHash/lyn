#include "section_data.h"

#include "core/event_section.h"

#include <algorithm>

namespace lyn {

std::string::value_type toHexDigit(std::uint32_t value) {
	value = (value & 0xF);

	if (value < 10)
		return '0' + value;
	return 'A' + (value - 10);
}

std::string toHexDigits(std::uint32_t value, int digits) {
	std::string result;
	result.resize(digits);

	for (int i=0; i<digits; ++i)
		result[digits-i-1] = toHexDigit(value >> (4*i));

	return result;
}

void section_data::write_events(std::ostream& output) const {
	lyn::event_section eventSection;
	eventSection.resize(mData.size());

	// TODO: assert(!sectionData.mappings.empty());

	// TODO: make_relocation_string but better
	auto make_relocation_string = [] (const std::string& symbol, int addend) {
		if (addend == 0)
			return symbol;

		if (addend < 0)
			return std::string("(").append(symbol).append(" - ").append(std::to_string(-addend)).append(")");

		return std::string("(").append(symbol).append(" + ").append(std::to_string(addend)).append(")");
	};

	auto mappingIt = mMappings.begin();
	int currentMappingType = mapping::Data;

	for (int i=0; i<mData.size();) {
		if (mappingIt != mMappings.end() && (mappingIt->offset <= i))
			currentMappingType = (mappingIt++)->type;

		switch (currentMappingType) {
		case mapping::Data:
			eventSection.set_code(i, lyn::event_code(
				lyn::event_code::CODE_BYTE,
				std::string("0x").append(toHexDigits(mData.byte_at(i), 2))
			));
			i++;
			break;

		case mapping::Thumb:
			eventSection.set_code(i, lyn::event_code(
				lyn::event_code::CODE_SHORT,
				std::string("0x").append(toHexDigits(mData.at<std::uint16_t>(i), 4))
			));
			i += 2;
			break;

		case mapping::ARM:
			eventSection.set_code(i, lyn::event_code(
				lyn::event_code::CODE_WORD,
				std::string("0x").append(toHexDigits(mData.at<std::uint32_t>(i), 8))
			));
			i += 4;
			break;
		}
	}

	for (auto& reloc : mRelocations) {
		switch (reloc.type) {

		case 0x02: // R_ARM_ABS32 (POIN Symbol + Addend)
			eventSection.set_code(reloc.offset, lyn::event_code(lyn::event_code::CODE_POIN, make_relocation_string(reloc.symbol, reloc.addend)));
			break;

		case 0x03: // R_ARM_REL32 (WORD Symbol + Addend - CURRENTOFFSET) // FIXME
			eventSection.set_code(reloc.offset, lyn::event_code(lyn::event_code::CODE_WORD, make_relocation_string(reloc.symbol, reloc.addend)));
			break;

		case 0x05: // R_ARM_ABS16 (SHORT Symbol + Addend)
			eventSection.set_code(reloc.offset, lyn::event_code(lyn::event_code::CODE_SHORT, make_relocation_string(reloc.symbol, reloc.addend)));
			break;

		case 0x08: // R_ARM_ABS8 (BYTE Symbol + Addend)
			eventSection.set_code(reloc.offset, lyn::event_code(lyn::event_code::CODE_BYTE, make_relocation_string(reloc.symbol, reloc.addend)));
			break;

		case 0x09: // R_ARM_SBREL32 (WORD Symbol + Addend) (POIN Symbol + Addend - 0x8000000) // FIXME
			eventSection.set_code(reloc.offset, lyn::event_code(lyn::event_code::CODE_WORD, make_relocation_string(reloc.symbol, reloc.addend)));
			break;

		case 0x0A: // R_ARM_THM_CALL (BL)
			eventSection.set_code(reloc.offset, lyn::event_code(lyn::event_code::MACRO_BL, make_relocation_string(reloc.symbol, reloc.addend)));
			break;

		}
	}

	eventSection.compress_codes();
	eventSection.optimize();

	output << "// section " << mName << std::endl << std::endl;

	if (!mSymbols.empty()) {
		output << "PUSH" << std::endl;
		int currentOffset = 0;

		for (auto& symbol : mSymbols) {
			output << "ORG (CURRENTOFFSET + 0x" << (symbol.offset - currentOffset) << "); "
				   << symbol.name << ":" << std::endl;
			currentOffset = symbol.offset;
		}

		output << "POP" << std::endl;
	}

	eventSection.write_to_stream(output);
}

int section_data::mapping_type_at(unsigned int offset) const {
	for (auto mapping : mMappings)
		if (mapping.offset <= offset)
			return mapping.type;

	return mapping::Data;
}

std::vector<section_data> section_data::make_from_elf(const elf_file& elfFile) {
	std::vector<section_data> result;
	result.resize(elfFile.sections().size());

	// Initializing written Sections from relevant elf sections

	for (int i=0; i<elfFile.sections().size(); ++i) {
		auto& elfSection = elfFile.sections()[i];

		if ((elfSection.sh_type == elf::SHT_PROGBITS)) {
			if ((elfSection.sh_flags & elf::SHF_ALLOC) && !(elfSection.sh_flags & elf::SHF_WRITE)) {
				auto& sectionData = result[i];

				sectionData.mName = elfFile.get_section_name(elfSection);
				sectionData.mOutType = section_data::OutROM;
				sectionData.mData.load_from_other(elfFile, elfSection.sh_offset, elfSection.sh_size);
			}
		}
	}

	// Initializing labels, mappings & relocations from other elf sections

	for (auto& elfSection : elfFile.sections()) {
		if (elfSection.sh_type == elf::SHT_SYMTAB) {
			int symbolCount = elfSection.sh_size / 0x10;

			for (int i=0; i<symbolCount; ++i) {
				auto sym = elfFile.symbol(elfSection, i);

				if (sym.st_shndx >= result.size())
					continue;

				auto& sectionData = result[sym.st_shndx];

				if (!sectionData.mOutType)
					continue;

				std::string symbolName = elfFile.string(elfFile.sections()[elfSection.sh_link], sym.st_name);

				switch (sym.type()) {
				case elf::STT_FUNC:
					// sym.st_value &= ~1; // remove eventual thumb bit
				case elf::STT_OBJECT: {
					if (sym.bind() == elf::STB_GLOBAL)
						sectionData.mSymbols.push_back({ symbolName, sym.st_value });

					break;
				}

				case elf::STT_NOTYPE: {
					if (sym.bind() != elf::STB_LOCAL)
						break;

					std::string subString = symbolName.substr(0, 3);

					if ((symbolName == "$t") || (subString == "$t."))
						sectionData.mMappings.push_back({ mapping::Thumb, sym.st_value });
					else if ((symbolName == "$a") || (subString == "$a."))
						sectionData.mMappings.push_back({ mapping::ARM, sym.st_value });
					else if ((symbolName == "$d") || (subString == "$d."))
						sectionData.mMappings.push_back({ mapping::Data, sym.st_value });

					break;
				}

				case elf::STT_FILE: {
					// TODO: output source filename, because why not
					break;
				}

				}
			}
		} else if (elfSection.sh_type == elf::SHT_REL) {
			int relCount = elfSection.sh_size / 0x08;

			auto& symbolSection = elfFile.sections()[elfSection.sh_link];
			auto& symbolNameSection = elfFile.sections()[symbolSection.sh_link];

			auto& sectionData = result[elfSection.sh_info];

			for (int i=0; i<relCount; ++i) {
				auto rel  = elfFile.rel(elfSection, i);
				auto name = elfFile.string(symbolNameSection, elfFile.symbol(symbolSection, rel.symId()).st_name);

				sectionData.mRelocations.push_back({ std::string(name), 0, rel.type(), rel.r_offset });
			}
		} else if (elfSection.sh_type == elf::SHT_RELA) {
			int relCount = elfSection.sh_size / 0x0C;

			auto& symbolSection = elfFile.sections()[elfSection.sh_link];
			auto& symbolNameSection = elfFile.sections()[symbolSection.sh_link];

			auto& sectionData = result[elfSection.sh_info];

			for (int i=0; i<relCount; ++i) {
				auto rela = elfFile.rela(elfSection, i);
				auto name = elfFile.string(symbolNameSection, elfFile.symbol(symbolSection, rela.symId()).st_name);

				sectionData.mRelocations.push_back({ std::string(name), rela.r_addend, rela.type(), rela.r_offset });
			}
		}
	}

	result.erase(std::remove_if(result.begin(), result.end(), [] (const section_data& sectionData) {
		return (!sectionData.mOutType);
	}), result.end());

	// Sorting mappings
	for (auto& sectionData : result) {
		std::sort(sectionData.mMappings.begin(), sectionData.mMappings.end(),
			[] (const mapping& left, const mapping& right) {
				return left.offset < right.offset;
			});
	}

	return result;
}

} // namespace lyn
