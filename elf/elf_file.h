#ifndef ELF_FILE_H
#define ELF_FILE_H

#include "raw_elf.h"
#include "../core/binary_file.h"

#include <vector>
#include <string>
#include <iostream>

namespace lyn {

class elf_file : public binary_file {
public:
	void load_from_stream(std::istream& input);

	const std::vector<elf::section_header>& sections() const;

	elf::symbol symbol(const elf::section_header& section, int index) const;
	elf::rel rel(const elf::section_header& section, int index) const;
	elf::rela rela(const elf::section_header& section, int index) const;

	const char* string(const elf::section_header& header, int pos) const;

	const elf::section_header& get_shstrtab_section() const;
	const char* get_section_name(const elf::section_header& header) const;

protected:
	void load_header();
	void load_sections();

	elf::symbol symbol_at(int pos) const { return read_symbol(pos); }
	elf::symbol read_symbol(int& pos) const;

	elf::rel rel_at(int pos) const { return read_rel(pos); }
	elf::rel read_rel(int& pos) const;

	elf::rela rela_at(int pos) const { return read_rela(pos); }
	elf::rela read_rela(int& pos) const;

private:
	elf::header mRawHeader;
	std::vector<elf::section_header> mSectionHeaders;
};

} // namespace lyn

#endif // ELF_FILE_H
