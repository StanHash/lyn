#ifndef ELF_H
#define ELF_H

#include "raw_elf.h"
#include "../core/binary_file.h"

#include <vector>
#include <string>
#include <iostream>

class ELF {
public:
	void load_from_stream(std::istream& input);

	const std::vector<elf_section_header>& sections() const;
	const lyn::binary_file& file() const;

	elf_symbol symbol(const elf_section_header& section, int index) const;
	elf_rel rel(const elf_section_header& section, int index) const;
	elf_rela rela(const elf_section_header& section, int index) const;

	const char* get_string(const elf_section_header& header, int pos) const;

	const elf_section_header& get_shstrtab_section() const;
	const char* get_section_name(const elf_section_header& header) const;

protected:
	void load_header();
	void load_sections();

	elf_symbol symbol_at(int pos) const { return read_symbol(pos); }
	elf_symbol read_symbol(int& pos) const;

	elf_rel rel_at(int pos) const { return read_rel(pos); }
	elf_rel read_rel(int& pos) const;

	elf_rela rela_at(int pos) const { return read_rela(pos); }
	elf_rela read_rela(int& pos) const;

public:
	static const std::string symbolTableSectionName;
	static const std::string symbolNamesSectionName;

private:
	elf_header mRawHeader;
	std::vector<elf_section_header> mSectionHeaders;

	lyn::binary_file mFile;
};

#endif // ELF_H
