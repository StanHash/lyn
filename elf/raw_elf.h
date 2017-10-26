#ifndef RAW_ELF_H
#define RAW_ELF_H

#include <cstdint>

struct elf_header {
	enum {
		EI_MAG0, // 0x7F
		EI_MAG1, // 'E'
		EI_MAG2, // 'L'
		EI_MAG3, // 'F'

		EI_CLASS,
		EI_DATA,
		EI_VERSION,
		EI_OSABI,
		EI_ABIVERSION,

		EI_PAD,

		EI_SIZE
	};

	char e_ident[EI_SIZE];

	std::uint16_t e_type;
	std::uint16_t e_machine;
	std::uint32_t e_version;
	std::uint32_t e_entry;
	std::uint32_t e_phoff;
	std::uint32_t e_shoff;
	std::uint32_t e_flags;
	std::uint16_t e_ehsize;    // size of this header.
	std::uint16_t e_phentsize; // size of a program header table entry.
	std::uint16_t e_phnum;     // number of entries in the program header table.
	std::uint16_t e_shentsize; // size of a section header table entry.
	std::uint16_t e_shnum;     // entries in the section header table.
	std::uint16_t e_shstrndx;  // index of the section header table entry that contains the section names.
};

struct elf_program_header {
	enum {
		PT_NULL,
		PT_LOAD,
		PT_DYNAMIC,
		PT_INTERP,
		PT_NOTE,
		PT_SHLIB,
		PT_PHDR,

		PT_LOOS   = 0x60000000,
		PT_HIOS   = 0x6FFFFFFF,
		PT_LOPROC = 0x70000000,
		PT_HIPROC = 0x7FFFFFFF,
	};

	std::uint32_t p_type;
	std::uint32_t p_offset;
	std::uint32_t p_vaddr;
	std::uint32_t p_paddr;
	std::uint32_t p_filesz;
	std::uint32_t p_memsz;
	std::uint32_t p_flags;
	std::uint32_t p_align;
};

struct elf_section_header {
	enum {
		SHT_NULL          = 0x00,
		SHT_PROGBITS      = 0x01,
		SHT_SYMTAB        = 0x02,
		SHT_STRTAB        = 0x03,
		SHT_RELA          = 0x04,
		SHT_HASH          = 0x05,
		SHT_DYNAMIC       = 0x06,
		SHT_NOTE          = 0x07,
		SHT_NOBITS        = 0x08,
		SHT_REL           = 0x09,
		SHT_SHLIB         = 0x0A,
		SHT_DYNSYM        = 0x0B,
		SHT_INIT_ARRAY    = 0x0E,
		SHT_FINI_ARRAY    = 0x0F,
		SHT_PREINIT_ARRAY = 0x10,
		SHT_GROUP         = 0x11,
		SHT_SYMTAB_SHNDX  = 0x12,
		SHT_NUM           = 0x13,
		SHT_LOOS          = 0x60000000
	};

	enum {
		SHF_WRITE            = 0x01,
		SHF_ALLOC            = 0x02,
		SHF_EXECINSTR        = 0x04,
		SHF_MERGE            = 0x10,
		SHF_STRINGS          = 0x20,
		SHF_INFO_LINK        = 0x40,
		SHF_LINK_ORDER       = 0x80,
		SHF_OS_NONCONFORMING = 0x100,
		SHF_GROUP            = 0x200,
		SHF_TLS              = 0x400,
		SHF_MASKOS           = 0x0FF00000,
		SHF_MASKPROC         = 0xF0000000
	};

	enum {
		SHN_UNDEF  = 0,
		SHN_ABS    = 0xFFF1,
		SHN_COMMON = 0xFFF2
	};

	std::uint32_t sh_name;
	std::uint32_t sh_type;
	std::uint32_t sh_flags;
	std::uint32_t sh_addr;
	std::uint32_t sh_offset;
	std::uint32_t sh_size;
	std::uint32_t sh_link;
	std::uint32_t sh_info;
	std::uint32_t sh_addralign;
	std::uint32_t sh_entsize;
};

struct elf_symbol {
	enum {
		STB_LOCAL  = 0,
		STB_GLOBAL = 1,
		STB_WEAK   = 2
	};

	enum {
		STT_NOTYPE  = 0,
		STT_OBJECT  = 1,
		STT_FUNC    = 2,
		STT_SECTION = 3,
		STT_FILE    = 4
	};

	std::uint32_t st_name;
	std::uint32_t st_value;
	std::uint32_t st_size;
	std::uint8_t  st_info;
	std::uint8_t  st_other;
	std::uint16_t st_shndx;

	constexpr int type() const { return st_info & 0xF; }
	constexpr int bind() const { return st_info >> 4; }
};

struct elf_rel {
	std::uint32_t r_offset;
	std::uint32_t r_info;

	constexpr int type() const  { return r_info & 0xFF; }
	constexpr int symId() const { return r_info >> 8; }
};

struct elf_rela : public elf_rel {
	std::int32_t  r_addend;
};

#endif // RAW_ELF_H
