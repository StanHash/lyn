#include "elf.h"

const std::string ELF::symbolTableSectionName = ".symtab";
const std::string ELF::symbolNamesSectionName = ".strtab";

void ELF::load_from_stream(std::istream& input) {
	mFile.load_from_stream(input);

	load_header();
	load_sections();
}

const std::vector<elf_section_header>& ELF::sections() const {
	return mSectionHeaders;
}

const lyn::binary_file& ELF::file() const {
	return mFile;
}

elf_symbol ELF::symbol(const elf_section_header& section, int index) const {
	if (section.sh_type != elf_section_header::SHT_SYMTAB)
		throw std::runtime_error("ELF ERROR: tried to read symbol off non-symtab section!");

	return symbol_at(section.sh_offset + index * 0x10);
}

elf_rel ELF::rel(const elf_section_header& section, int index) const {
	if (section.sh_type != elf_section_header::SHT_REL)
		throw std::runtime_error("ELF ERROR: tried to read REL entry off non-rel section!");

	return rel_at(section.sh_offset + index * 0x08);
}

elf_rela ELF::rela(const elf_section_header &section, int index) const {
	if (section.sh_type != elf_section_header::SHT_RELA)
		throw std::runtime_error("ELF ERROR: tried to read RELA entry off non-rela section!");

	return rela_at(section.sh_offset + index * 0x0C);
}

/*
const elf_section_header& ELF::get_section(int index) const {
	return mSectionHeaders[index];
} //*/

const elf_section_header& ELF::get_shstrtab_section() const {
	return mSectionHeaders[mRawHeader.e_shstrndx];
}

/*
const elf_section_header& ELF::find_symbol_table() const {
	for (const elf_section_header& sectionHeader : mSectionHeaders)
		if (sectionHeader.sh_type == elf_section_header::SHT_SYMTAB)
			if (symbolTableSectionName == get_section_name(sectionHeader))
				return sectionHeader;
}

const elf_section_header& ELF::find_symname_table() const {
	for (const elf_section_header& sectionHeader : mSectionHeaders)
		if (sectionHeader.sh_type == elf_section_header::SHT_STRTAB)
			if (symbolNamesSectionName == get_section_name(sectionHeader))
				return sectionHeader;
} //*/

const char* ELF::get_string(const elf_section_header& header, int pos) const {
	if (header.sh_type != elf_section_header::SHT_STRTAB)
		throw std::runtime_error("ELF ERROR: tried to read string from non-strtab section!");

	return (const char*) &(mFile.data()[header.sh_offset + pos]);
}

const char* ELF::get_section_name(const elf_section_header& header) const {
	return get_string(get_shstrtab_section(), header.sh_name);
}

/*
const char* ELF::get_symbol_name(const elf_symbol& symbol) const {
	return (const char*) &(mFile.data()[find_symname_table().sh_offset + symbol.st_name]);
} //*/

void ELF::load_header() {
	int pos = 0;

	for (int i=0; i<elf_header::EI_SIZE; ++i)
		mRawHeader.e_ident[i] = mFile.get<std::uint8_t>(pos);

	pos = 0x10;

	mRawHeader.e_type      = mFile.get<std::uint16_t>(pos);
	mRawHeader.e_machine   = mFile.get<std::uint16_t>(pos);
	mRawHeader.e_version   = mFile.get<std::uint32_t>(pos);
	mRawHeader.e_entry     = mFile.get<std::uint32_t>(pos);
	mRawHeader.e_phoff     = mFile.get<std::uint32_t>(pos);
	mRawHeader.e_shoff     = mFile.get<std::uint32_t>(pos);
	mRawHeader.e_flags     = mFile.get<std::uint32_t>(pos);
	mRawHeader.e_ehsize    = mFile.get<std::uint16_t>(pos);
	mRawHeader.e_phentsize = mFile.get<std::uint16_t>(pos);
	mRawHeader.e_phnum     = mFile.get<std::uint16_t>(pos);
	mRawHeader.e_shentsize = mFile.get<std::uint16_t>(pos);
	mRawHeader.e_shnum     = mFile.get<std::uint16_t>(pos);
	mRawHeader.e_shstrndx  = mFile.get<std::uint16_t>(pos);
}

void ELF::load_sections() {
	mSectionHeaders.resize(mRawHeader.e_shnum);

	for (int i=0; i<mRawHeader.e_shnum; ++i) {
		int pos = mRawHeader.e_shoff + i * mRawHeader.e_shentsize;

		elf_section_header& sectionHeader = mSectionHeaders[i];

		sectionHeader.sh_name      = mFile.get<std::uint32_t>(pos);
		sectionHeader.sh_type      = mFile.get<std::uint32_t>(pos);
		sectionHeader.sh_flags     = mFile.get<std::uint32_t>(pos);
		sectionHeader.sh_addr      = mFile.get<std::uint32_t>(pos);
		sectionHeader.sh_offset    = mFile.get<std::uint32_t>(pos);
		sectionHeader.sh_size      = mFile.get<std::uint32_t>(pos);
		sectionHeader.sh_link      = mFile.get<std::uint32_t>(pos);
		sectionHeader.sh_info      = mFile.get<std::uint32_t>(pos);
		sectionHeader.sh_addralign = mFile.get<std::uint32_t>(pos);
		sectionHeader.sh_entsize   = mFile.get<std::uint32_t>(pos);
	}
}

elf_symbol ELF::read_symbol(int& pos) const {
	elf_symbol result;

	result.st_name  = mFile.get<std::uint32_t>(pos);
	result.st_value = mFile.get<std::uint32_t>(pos);
	result.st_size  = mFile.get<std::uint32_t>(pos);
	result.st_info  = mFile.get<std::uint8_t>(pos);
	result.st_other = mFile.get<std::uint8_t>(pos);
	result.st_shndx = mFile.get<std::uint16_t>(pos);

	return result;
}

elf_rel ELF::read_rel(int& pos) const {
	elf_rel result;

	result.r_offset = mFile.get<std::uint32_t>(pos);
	result.r_info   = mFile.get<std::uint32_t>(pos);

	return result;
}

elf_rela ELF::read_rela(int& pos) const {
	elf_rela result;

	result.r_offset = mFile.get<std::uint32_t>(pos);
	result.r_info   = mFile.get<std::uint32_t>(pos);
	result.r_addend = mFile.get<std::int32_t>(pos);

	return result;
}
