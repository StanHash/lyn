#include "event_object.h"

#include <iomanip>
#include <algorithm>

#include "core/event_section.h"

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

void event_object::load_from_elf(const elf_file& elfFile) {
	mSectionDatas.resize(elfFile.sections().size());

	// Initializing written Sections from relevant elf sections

	for (int i=0; i<elfFile.sections().size(); ++i) {
		auto& elfSection = elfFile.sections()[i];

		if ((elfSection.sh_type == elf::SHT_PROGBITS)) {
			if ((elfSection.sh_flags & elf::SHF_ALLOC) && !(elfSection.sh_flags & elf::SHF_WRITE)) {
				auto& sectionData = mSectionDatas[i];

				sectionData.name = elfFile.get_section_name(elfSection);

				sectionData.outType = section_data::ROMData;
				sectionData.data.resize(elfSection.sh_size);

				for (int i=0; i<elfSection.sh_size; ++i)
					sectionData.data[i] = elfFile.byte_at(elfSection.sh_offset + i);
			}
		}
	}

	// Initializing labels, mappings & relocations from other elf sections

	for (auto& elfSection : elfFile.sections()) {
		if (elfSection.sh_type == elf::SHT_SYMTAB) {
			int symbolCount = elfSection.sh_size / 0x10;

			for (int i=0; i<symbolCount; ++i) {
				auto sym = elfFile.symbol(elfSection, i);

				if (sym.st_shndx >= mSectionDatas.size())
					continue;

				auto& sectionData = mSectionDatas[sym.st_shndx];

				if (!sectionData.outType)
					continue;

				std::string symbolName = elfFile.string(elfFile.sections()[elfSection.sh_link], sym.st_name);

				switch (sym.type()) {
				case elf::STT_FUNC:
					sym.st_value &= ~1; // remove eventual thumb bit
				case elf::STT_OBJECT: {
					if (sym.bind() == elf::STB_GLOBAL)
						sectionData.labels.push_back({ symbolName, sym.st_value });

					break;
				}

				case elf::STT_NOTYPE: {
					if (sym.bind() != elf::STB_LOCAL)
						break;

					std::string subString = symbolName.substr(0, 3);

					if ((symbolName == "$t") || (subString == "$t."))
						sectionData.mappings.push_back({ mapping::Thumb, sym.st_value });
					else if ((symbolName == "$a") || (subString == "$a."))
						sectionData.mappings.push_back({ mapping::ARM, sym.st_value });
					else if ((symbolName == "$d") || (subString == "$d."))
						sectionData.mappings.push_back({ mapping::Data, sym.st_value });

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

			auto& sectionData = mSectionDatas[elfSection.sh_info];

			for (int i=0; i<relCount; ++i) {
				auto rel  = elfFile.rel(elfSection, i);
				auto name = elfFile.string(symbolNameSection, elfFile.symbol(symbolSection, rel.symId()).st_name);

				std::cout << name << " " << elfFile.symbol(symbolSection, rel.symId()).type() << std::endl;

				sectionData.relocations.push_back({ std::string(name), 0, rel.type(), rel.r_offset });
			}
		} else if (elfSection.sh_type == elf::SHT_RELA) {
			int relCount = elfSection.sh_size / 0x0C;

			auto& symbolSection = elfFile.sections()[elfSection.sh_link];
			auto& symbolNameSection = elfFile.sections()[symbolSection.sh_link];

			auto& sectionData = mSectionDatas[elfSection.sh_info];

			for (int i=0; i<relCount; ++i) {
				auto rela = elfFile.rela(elfSection, i);
				auto name = elfFile.string(symbolNameSection, elfFile.symbol(symbolSection, rela.symId()).st_name);

				sectionData.relocations.push_back({ std::string(name), rela.r_addend, rela.type(), rela.r_offset });
			}
		}
	}

	// Sorting mappings
	for (auto& sectionData : mSectionDatas)
		std::sort(sectionData.mappings.begin(), sectionData.mappings.end(), [] (const mapping& left, const mapping& right) {
			return left.offset < right.offset;
		});
}

void event_object::write_events(std::ostream& output) const {
	for (int i=0; i<mSectionDatas.size(); ++i) {
		auto& sectionData = mSectionDatas[i];

		if (!sectionData.outType)
			continue;

		lyn::event_section eventSection;
		eventSection.resize(sectionData.data.size());

		for (int i=0; i<sectionData.data.size(); i += 2)
			eventSection.set_code(i, lyn::event_code(
				lyn::event_code::CODE_SHORT,
				std::string("0x").append(toHexDigits(sectionData.data[i] | (sectionData.data[i+1] << 8), 4))
			));

		for (auto& reloc : sectionData.relocations) {
			switch (reloc.type) {

			case 0x02: // R_ARM_ABS32 (POIN Symbol + Addend)
				eventSection.set_code(reloc.offset, lyn::event_code(lyn::event_code::CODE_POIN, reloc.symbol));
				break;

			case 0x03: // R_ARM_REL32 (WORD Symbol + Addend - CURRENTOFFSET)
				eventSection.set_code(reloc.offset, lyn::event_code(lyn::event_code::CODE_WORD, reloc.symbol));
				break;

			case 0x05: // R_ARM_ABS16 (SHORT Symbol + Addend - CURRENTOFFSET)
				eventSection.set_code(reloc.offset, lyn::event_code(lyn::event_code::CODE_SHORT, reloc.symbol));
				break;

			case 0x08: // R_ARM_ABS8 (BYTE Symbol + Addend - CURRENTOFFSET)
				eventSection.set_code(reloc.offset, lyn::event_code(lyn::event_code::CODE_BYTE, reloc.symbol));
				break;

			case 0x09: // R_ARM_SBREL32 (WORD Symbol + Addend) (POIN Symbol + Addend - 0x8000000)
				eventSection.set_code(reloc.offset, lyn::event_code(lyn::event_code::CODE_WORD, reloc.symbol));
				break;

			case 0x0A: // R_ARM_THM_CALL (BL)
				eventSection.set_code(reloc.offset, lyn::event_code(lyn::event_code::MACRO_BL, reloc.symbol));
				break;

			}
		}

		eventSection.compress_codes();
		eventSection.optimize();

		output << "// section " << sectionData.name << std::endl << std::endl;

		if (!sectionData.labels.empty()) {
			output << "PUSH" << std::endl;
			int currentOffset = 0;

			for (auto& label : sectionData.labels) {
				output << "ORG (CURRENTOFFSET + 0x" << (label.offset - currentOffset) << "); "
					   << label.name << ":" << std::endl;
				currentOffset = label.offset;
			}

			output << "POP" << std::endl;
		}

		eventSection.write_to_stream(output);
	}
}

} // namespace lyn
