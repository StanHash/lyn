#include <iostream>
#include <fstream>
#include <iomanip>

#include "elf/elf.h"
#include "core/event_section.h"

struct SectionInfo {
	struct SectionMapping {
		enum {
			None,
			Thumb,
			ARM,
			Data
		} type;
		unsigned int offset;
	};

	struct LabelSymbol {
		std::string name;
		unsigned int offset;
	};

	struct Relocation {
		std::string symbolName;
		int type;
		int addend;
		unsigned int offset;
	};

	bool doOutputSection = false;
	std::vector<SectionMapping> mappings;
	std::vector<LabelSymbol> symbols;
	std::vector<Relocation> relocations;
	std::vector<unsigned char> data;
};

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

int main(int argc, char** argv) {
	if (argc < 2)
		return 1;

	std::string input(argv[1]);

	std::ifstream file;

	file.open(input, std::ios::in | std::ios::binary);

	if (!file.is_open())
		return 1;

	ELF elf;
	elf.load_from_stream(file);

	file.close();

	std::vector<SectionInfo> infos;
	infos.resize(elf.sections().size());

	std::cout << std::hex;

	// Step I: Go through all SHT_PROGBITS elf sections, and initialize EA sections from those

	for (int i=0; i<elf.sections().size(); ++i) {
		auto& elfSection = elf.sections()[i];

		if ((elfSection.sh_type == elf_section_header::SHT_PROGBITS) &&
			(elfSection.sh_flags & elf_section_header::SHF_ALLOC) &&
			!(elfSection.sh_flags & elf_section_header::SHF_WRITE)) {
			auto& sectionInfo = infos[i];

			sectionInfo.doOutputSection = true;
			sectionInfo.data.reserve(elfSection.sh_size);

			for (int i=0; i<elfSection.sh_size; ++i)
				sectionInfo.data.push_back(elf.file().byte_at(elfSection.sh_offset + i));
		}
	}

	// Step II: Go through all SHT_SYMTAB elf sections, map thumb/arm/data accordingly, and initialize EA labels

	for (auto& section : elf.sections()) {
		if (section.sh_type != elf_section_header::SHT_SYMTAB)
			continue;

		int symbolCount = section.sh_size / 0x10;
		for (int i=0; i<symbolCount; ++i) {
			auto sym = elf.symbol(section, i);

			if (sym.st_shndx >= infos.size() || !infos[sym.st_shndx].doOutputSection)
				continue;

			std::string symName = elf.get_string(elf.sections()[section.sh_link], sym.st_name);
			auto& info = infos[sym.st_shndx];

			switch (sym.type()) {
			case elf_symbol::STT_FUNC:
			case elf_symbol::STT_OBJECT:
				if (sym.bind() == elf_symbol::STB_GLOBAL)
					info.symbols.push_back({ symName, sym.st_value });

				break;

			case elf_symbol::STT_NOTYPE:
				if (sym.bind() != elf_symbol::STB_LOCAL)
					break;
				if ((symName == "$t") || (symName.substr(0, 3) == "$t."))
					info.mappings.push_back({ SectionInfo::SectionMapping::Thumb, sym.st_value });
				else if ((symName == "$a") || (symName.substr(0, 3) == "$a."))
					info.mappings.push_back({ SectionInfo::SectionMapping::ARM, sym.st_value });
				else if ((symName == "$d") || (symName.substr(0, 3) == "$d."))
					info.mappings.push_back({ SectionInfo::SectionMapping::Data, sym.st_value });

				break;

			case elf_symbol::STT_FILE:
				// TODO: output source filename, because why not
				break;
			}
		}
	}

	// Step III: Go through all SHT_REL & SHT_RELA elf sections, and replace relevant data with code referencing those values

	for (auto& section : elf.sections()) {
		if (section.sh_type == elf_section_header::SHT_REL) {
			int relCount = section.sh_size / 0x08;

			auto& symbolSection = elf.sections()[section.sh_link];
			auto& info = infos[section.sh_info];

			for (int i=0; i<relCount; ++i) {
				auto rel = elf.rel(section, i);
				auto name =  elf.get_string(elf.sections()[symbolSection.sh_link], elf.symbol(symbolSection, rel.symId()).st_name);

				info.relocations.push_back({ std::string(name), rel.type(), 0, rel.r_offset });
			}
		} else if (section.sh_type == elf_section_header::SHT_RELA) {
			int relCount = section.sh_size / 0x0C;

			auto& symbolSection = elf.sections()[section.sh_link];
			auto& info = infos[section.sh_info];

			for (int i=0; i<relCount; ++i) {
				auto rela = elf.rela(section, i);
				auto name = elf.get_string(elf.sections()[symbolSection.sh_link], elf.symbol(symbolSection, rela.symId()).st_name);

				info.relocations.push_back({ std::string(name), rela.type(), rela.r_addend, rela.r_offset });
			}
		}
	}

	// FINAL STEP: converting everything to EA code

	for (int i=0; i<infos.size(); ++i) {
		auto& info = infos[i];

		if (!info.doOutputSection)
			continue;

		lyn::event_section eventSection;
		eventSection.set_size(info.data.size());

		for (int i=0; i<info.data.size(); i += 2)
			eventSection.set_code(i, { lyn::event_section::event_code::SHORT, std::string("0x").append(toHexDigits(info.data[i] | (info.data[i+1] << 8), 4)) });

		for (auto& reloc : info.relocations) {
			switch (reloc.type) {
			case 0x0A: // R_ARM_THM_CALL (bl)
			{
				std::string blRange = "(pointer - CURRENTOFFSET - 4)>>1";
				std::string bl1 = "((((BLRange)>>11)&0x7ff)|0xf000)";
				std::string bl2 = "(((BLRange)&0x7ff)|0xf800)";

				std::string::size_type found = std::string::npos;

				while ((found = blRange.find("pointer")) != std::string::npos)
					blRange.replace(found, 7, reloc.symbolName);

				while ((found = bl1.find("BLRange")) != std::string::npos)
					bl1.replace(found, 7, blRange);

				while ((found = bl2.find("BLRange")) != std::string::npos)
					bl2.replace(found, 7, blRange);

				eventSection.set_code(reloc.offset + 0, { lyn::event_section::event_code::SHORT, bl1, lyn::event_section::event_code::ForceNewline });
				eventSection.set_code(reloc.offset + 2, { lyn::event_section::event_code::SHORT, bl2, lyn::event_section::event_code::ForceContinue });

				break;
			}
			}
		}
		//	std::cout << reloc.symbolName << ":" << reloc.type << ":" << reloc.offset << std::endl;

		std::cout << "// section " << elf.get_section_name(elf.sections()[i]) << std::endl << std::endl;

		if (!info.symbols.empty()) {
			std::cout << "PUSH" << std::endl;
			int currentOffset = 0;

			for (auto& symbol : info.symbols) {
				std::cout << "ORG (CURRENTOFFSET + $" << (symbol.offset - currentOffset) << "); "
						  << symbol.name << ":" << std::endl;
				currentOffset = symbol.offset;
			}

			std::cout << "POP" << std::endl;
		}

		eventSection.write_to_stream(std::cout);
	}

	return 0;
}
