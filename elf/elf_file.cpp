#include "elf_file.h"

namespace lyn {

void elf_file::load_from_stream(std::istream& input) {
	binary_file::load_from_stream(input);

	load_header();
	load_sections();
}

const std::vector<elf::section_header>& elf_file::sections() const {
	return mSectionHeaders;
}

elf::symbol elf_file::symbol(const elf::section_header& section, int index) const {
	if (section.sh_type != elf::SHT_SYMTAB)
		throw std::runtime_error("ELF ERROR: tried to read symbol off non-symtab section!");

	return symbol_at(section.sh_offset + index * 0x10);
}

elf::rel elf_file::rel(const elf::section_header& section, int index) const {
	if (section.sh_type != elf::SHT_REL)
		throw std::runtime_error("ELF ERROR: tried to read REL entry off non-rel section!");

	return rel_at(section.sh_offset + index * 0x08);
}

elf::rela elf_file::rela(const elf::section_header &section, int index) const {
	if (section.sh_type != elf::SHT_RELA)
		throw std::runtime_error("ELF ERROR: tried to read RELA entry off non-rela section!");

	return rela_at(section.sh_offset + index * 0x0C);
}

const char* elf_file::string(const elf::section_header& header, int pos) const {
	if (header.sh_type != elf::SHT_STRTAB)
		throw std::runtime_error("ELF ERROR: tried to read string from non-strtab section!");

	if (!is_cstr_at(header.sh_offset + pos))
		throw std::runtime_error("ELF ERROR: strtab section contains non null-terminated string!");

	return cstr_at(header.sh_offset + pos);
}

const elf::section_header& elf_file::get_shstrtab_section() const {
	return mSectionHeaders[mRawHeader.e_shstrndx];
}

const char* elf_file::get_section_name(const elf::section_header& header) const {
	return string(get_shstrtab_section(), header.sh_name);
}

void elf_file::load_header() {
	int pos = 0;

	for (int i=0; i<elf::EI_SIZE; ++i)
		mRawHeader.e_ident[i] = read<std::uint8_t>(pos);

	pos = 0x10;

	mRawHeader.e_type      = read<std::uint16_t>(pos);
	mRawHeader.e_machine   = read<std::uint16_t>(pos);
	mRawHeader.e_version   = read<std::uint32_t>(pos);
	mRawHeader.e_entry     = read<std::uint32_t>(pos);
	mRawHeader.e_phoff     = read<std::uint32_t>(pos);
	mRawHeader.e_shoff     = read<std::uint32_t>(pos);
	mRawHeader.e_flags     = read<std::uint32_t>(pos);
	mRawHeader.e_ehsize    = read<std::uint16_t>(pos);
	mRawHeader.e_phentsize = read<std::uint16_t>(pos);
	mRawHeader.e_phnum     = read<std::uint16_t>(pos);
	mRawHeader.e_shentsize = read<std::uint16_t>(pos);
	mRawHeader.e_shnum     = read<std::uint16_t>(pos);
	mRawHeader.e_shstrndx  = read<std::uint16_t>(pos);
}

void elf_file::load_sections() {
	mSectionHeaders.resize(mRawHeader.e_shnum);

	for (int i=0; i<mRawHeader.e_shnum; ++i) {
		int pos = mRawHeader.e_shoff + i * mRawHeader.e_shentsize;

		elf::section_header& sectionHeader = mSectionHeaders[i];

		sectionHeader.sh_name      = read<std::uint32_t>(pos);
		sectionHeader.sh_type      = read<std::uint32_t>(pos);
		sectionHeader.sh_flags     = read<std::uint32_t>(pos);
		sectionHeader.sh_addr      = read<std::uint32_t>(pos);
		sectionHeader.sh_offset    = read<std::uint32_t>(pos);
		sectionHeader.sh_size      = read<std::uint32_t>(pos);
		sectionHeader.sh_link      = read<std::uint32_t>(pos);
		sectionHeader.sh_info      = read<std::uint32_t>(pos);
		sectionHeader.sh_addralign = read<std::uint32_t>(pos);
		sectionHeader.sh_entsize   = read<std::uint32_t>(pos);
	}
}

elf::symbol elf_file::read_symbol(int& pos) const {
	elf::symbol result;

	result.st_name  = read<std::uint32_t>(pos);
	result.st_value = read<std::uint32_t>(pos);
	result.st_size  = read<std::uint32_t>(pos);
	result.st_info  = read<std::uint8_t>(pos);
	result.st_other = read<std::uint8_t>(pos);
	result.st_shndx = read<std::uint16_t>(pos);

	return result;
}

elf::rel elf_file::read_rel(int& pos) const {
	elf::rel result;

	result.r_offset = read<std::uint32_t>(pos);
	result.r_info   = read<std::uint32_t>(pos);

	return result;
}

elf::rela elf_file::read_rela(int& pos) const {
	elf::rela result;

	result.r_offset = read<std::uint32_t>(pos);
	result.r_info   = read<std::uint32_t>(pos);
	result.r_addend = read<std::int32_t>(pos);

	return result;
}

} // namespace lyn
