#ifndef NATELF_NATELF_HH
#define NATELF_NATELF_HH

#include <cinttypes>
#include <optional>
#include <span>

namespace natelf
{

// Data Types
// https://www.sco.com/developers/gabi/latest/ch4.intro.html

typedef uint32_t Elf32_Addr; // Unsigned program address
typedef uint16_t Elf32_Half; // Unsigned file offset
typedef uint32_t Elf32_Off;  // Unsigned medium integer
typedef int32_t Elf32_Sword; // Unsigned integer
typedef uint32_t Elf32_Word; // Signed integer

typedef uint64_t Elf64_Addr;  // Unsigned program address
typedef uint64_t Elf64_Off;   // Unsigned file offset
typedef uint16_t Elf64_Half;  // Unsigned medium integer
typedef uint32_t Elf64_Word;  // Unsigned integer
typedef int32_t Elf64_Sword;  // Signed integer
typedef uint64_t Elf64_Xword; // Unsigned long integer
typedef int64_t Elf64_Sxword; // Signed long integer

// ELF Header
// https://www.sco.com/developers/gabi/latest/ch4.eheader.html

constexpr std::size_t EI_MAG0 = 0;       // File identification
constexpr std::size_t EI_MAG1 = 1;       // File identification
constexpr std::size_t EI_MAG2 = 2;       // File identification
constexpr std::size_t EI_MAG3 = 3;       // File identification
constexpr std::size_t EI_CLASS = 4;      // File class
constexpr std::size_t EI_DATA = 5;       // Data encoding
constexpr std::size_t EI_VERSION = 6;    // File version
constexpr std::size_t EI_OSABI = 7;      // Operating system/ABI identification
constexpr std::size_t EI_ABIVERSION = 8; // ABI version
constexpr std::size_t EI_PAD = 9;        // Start of padding bytes
constexpr std::size_t EI_NIDENT = 16;    // Size of e_ident[]

constexpr std::uint8_t ELFMAG0 = 0x7f; // e_ident[EI_MAG0]
constexpr std::uint8_t ELFMAG1 = 'E';  // e_ident[EI_MAG1]
constexpr std::uint8_t ELFMAG2 = 'L';  // e_ident[EI_MAG2]
constexpr std::uint8_t ELFMAG3 = 'F';  // e_ident[EI_MAG3]

constexpr std::uint8_t ELFCLASSNONE = 0; // Invalid class
constexpr std::uint8_t ELFCLASS32 = 1;   // 32-bit objects
constexpr std::uint8_t ELFCLASS64 = 2;   // 64-bit objects

constexpr std::uint8_t ELFDATANONE = 0; // Invalid data encoding
constexpr std::uint8_t ELFDATA2LSB = 1; // Little-Endian
constexpr std::uint8_t ELFDATA2MSB = 2; // Big-Endian

struct Elf32_Ehdr
{
    unsigned char e_ident[EI_NIDENT];
    Elf32_Half e_type;
    Elf32_Half e_machine;
    Elf32_Word e_version;
    Elf32_Addr e_entry;
    Elf32_Off e_phoff;
    Elf32_Off e_shoff;
    Elf32_Word e_flags;
    Elf32_Half e_ehsize;
    Elf32_Half e_phentsize;
    Elf32_Half e_phnum;
    Elf32_Half e_shentsize;
    Elf32_Half e_shnum;
    Elf32_Half e_shstrndx;
};

struct Elf64_Ehdr
{
    unsigned char e_ident[EI_NIDENT];
    Elf64_Half e_type;
    Elf64_Half e_machine;
    Elf64_Word e_version;
    Elf64_Addr e_entry;
    Elf64_Off e_phoff;
    Elf64_Off e_shoff;
    Elf64_Word e_flags;
    Elf64_Half e_ehsize;
    Elf64_Half e_phentsize;
    Elf64_Half e_phnum;
    Elf64_Half e_shentsize;
    Elf64_Half e_shnum;
    Elf64_Half e_shstrndx;
};

// e_type
constexpr Elf32_Half ET_NONE = 0;        // No file type
constexpr Elf32_Half ET_REL = 1;         // Relocatable file
constexpr Elf32_Half ET_EXEC = 2;        // Executable file
constexpr Elf32_Half ET_DYN = 3;         // Shared object file
constexpr Elf32_Half ET_CORE = 4;        // Core file
constexpr Elf32_Half ET_LOOS = 0xFE00;   // Operating system-specific
constexpr Elf32_Half ET_HIOS = 0xFEFF;   // Operating system-specific
constexpr Elf32_Half ET_LOPROC = 0xFF00; // Processor-specific
constexpr Elf32_Half ET_HIPROC = 0xFFFF; // Processor-specific

// e_machine
constexpr Elf32_Half EM_NONE = 0; // No machine
// I don't really care about defining them all here

// e_version
constexpr Elf32_Word EV_NONE = 0;    // Invalid version
constexpr Elf32_Word EV_CURRENT = 1; // Current version

// Sections
// https://www.sco.com/developers/gabi/latest/ch4.sheader.html

constexpr Elf32_Half SHN_UNDEF = 0;
constexpr Elf32_Half SHN_LORESERVE = 0xFF00;
constexpr Elf32_Half SHN_LOPROC = 0xFF00;
constexpr Elf32_Half SHN_HIPROC = 0xFF1F;
constexpr Elf32_Half SHN_LOOS = 0xFF20;
constexpr Elf32_Half SHN_HIOS = 0xFF3F;
constexpr Elf32_Half SHN_ABS = 0xFFF1;
constexpr Elf32_Half SHN_COMMON = 0xFFF2;
constexpr Elf32_Half SHN_XINDEX = 0xFFFF;
constexpr Elf32_Half SHN_HIRESERVE = 0xFFFF;

struct Elf32_Shdr
{
    Elf32_Word sh_name;
    Elf32_Word sh_type;
    Elf32_Word sh_flags;
    Elf32_Addr sh_addr;
    Elf32_Off sh_offset;
    Elf32_Word sh_size;
    Elf32_Word sh_link;
    Elf32_Word sh_info;
    Elf32_Word sh_addralign;
    Elf32_Word sh_entsize;
};

struct Elf64_Shdr
{
    Elf64_Word sh_name;
    Elf64_Word sh_type;
    Elf64_Xword sh_flags;
    Elf64_Addr sh_addr;
    Elf64_Off sh_offset;
    Elf64_Xword sh_size;
    Elf64_Word sh_link;
    Elf64_Word sh_info;
    Elf64_Xword sh_addralign;
    Elf64_Xword sh_entsize;
};

constexpr Elf32_Word SHT_NULL = 0;
constexpr Elf32_Word SHT_PROGBITS = 1;
constexpr Elf32_Word SHT_SYMTAB = 2;
constexpr Elf32_Word SHT_STRTAB = 3;
constexpr Elf32_Word SHT_RELA = 4;
constexpr Elf32_Word SHT_HASH = 5;
constexpr Elf32_Word SHT_DYNAMIC = 6;
constexpr Elf32_Word SHT_NOTE = 7;
constexpr Elf32_Word SHT_NOBITS = 8;
constexpr Elf32_Word SHT_REL = 9;
constexpr Elf32_Word SHT_SHLIB = 10;
constexpr Elf32_Word SHT_DYNSYM = 11;
constexpr Elf32_Word SHT_INIT_ARRAY = 14;
constexpr Elf32_Word SHT_FINI_ARRAY = 15;
constexpr Elf32_Word SHT_PREINIT_ARRAY = 16;
constexpr Elf32_Word SHT_GROUP = 17;
constexpr Elf32_Word SHT_SYMTAB_SHNDX = 18;
constexpr Elf32_Word SHT_LOOS = 0x60000000;
constexpr Elf32_Word SHT_HIOS = 0x6FFFFFFF;
constexpr Elf32_Word SHT_LOPROC = 0x70000000;
constexpr Elf32_Word SHT_HIPROC = 0x7FFFFFFF;
constexpr Elf32_Word SHT_LOUSER = 0x80000000;
constexpr Elf32_Word SHT_HIUSER = 0xFFFFFFFF;

constexpr Elf32_Word SHF_WRITE = 0x1;
constexpr Elf32_Word SHF_ALLOC = 0x2;
constexpr Elf32_Word SHF_EXECINSTR = 0x4;
constexpr Elf32_Word SHF_MERGE = 0x10;
constexpr Elf32_Word SHF_STRINGS = 0x20;
constexpr Elf32_Word SHF_INFO_LINK = 0x40;
constexpr Elf32_Word SHF_LINK_ORDER = 0x80;
constexpr Elf32_Word SHF_OS_NONCONFORMING = 0x100;
constexpr Elf32_Word SHF_GROUP = 0x200;
constexpr Elf32_Word SHF_TLS = 0x400;
constexpr Elf32_Word SHF_COMPRESSED = 0x800;
constexpr Elf32_Word SHF_MASKOS = 0x0FF00000;
constexpr Elf32_Word SHF_MASKPROC = 0xF0000000;

// Symbol Table
// https://www.sco.com/developers/gabi/latest/ch4.symtab.html

#define NATELF_ELF32_ST_BIND(i) ((i) >> 4)
#define NATELF_ELF32_ST_TYPE(i) (0xF & (i))
#define NATELF_ELF32_ST_INFO(b, t) (((b) << 4) + (0xF & (t)))

#define NATELF_ELF64_ST_BIND(i) ((i) >> 4)
#define NATELF_ELF64_ST_TYPE(i) (0xF & (i))
#define NATELF_ELF64_ST_INFO(b, t) (((b) << 4) + (0xF & (t)))

#define NATELF_ELF32_ST_VISIBILITY(o) (0x3 & (o))
#define NATELF_ELF64_ST_VISIBILITY(o) (0x3 & (o))

struct Elf32_Sym
{
    Elf32_Word st_name;
    Elf32_Addr st_value;
    Elf32_Word st_size;
    unsigned char st_info;
    unsigned char st_other;
    Elf32_Half st_shndx;

    constexpr unsigned char GetBind() const
    {
        return NATELF_ELF32_ST_BIND(st_info);
    }

    constexpr unsigned char GetType() const
    {
        return NATELF_ELF32_ST_TYPE(st_info);
    }

    // helper
    constexpr bool IsDefined() const
    {
        return st_shndx != SHN_UNDEF;
    }
};

struct Elf64_Sym
{
    Elf64_Word st_name;
    unsigned char st_info;
    unsigned char st_other;
    Elf64_Half st_shndx;
    Elf64_Addr st_value;
    Elf64_Xword st_size;

    constexpr unsigned char GetBind() const
    {
        return NATELF_ELF64_ST_BIND(st_info);
    }

    constexpr unsigned char GetType() const
    {
        return NATELF_ELF64_ST_TYPE(st_info);
    }

    // helper
    constexpr bool IsDefined() const
    {
        return st_shndx != SHN_UNDEF;
    }
};

constexpr unsigned char STB_LOCAL = 0;
constexpr unsigned char STB_GLOBAL = 1;
constexpr unsigned char STB_WEAK = 2;
constexpr unsigned char STB_LOOS = 10;
constexpr unsigned char STB_HIOS = 12;
constexpr unsigned char STB_LOPROC = 13;
constexpr unsigned char STB_HIPROC = 15;

constexpr unsigned char STT_NOTYPE = 0;
constexpr unsigned char STT_OBJECT = 1;
constexpr unsigned char STT_FUNC = 2;
constexpr unsigned char STT_SECTION = 3;
constexpr unsigned char STT_FILE = 4;
constexpr unsigned char STT_COMMON = 5;
constexpr unsigned char STT_TLS = 6;
constexpr unsigned char STT_LOOS = 10;
constexpr unsigned char STT_HIOS = 12;
constexpr unsigned char STT_LOPROC = 13;
constexpr unsigned char STT_HIPROC = 15;

constexpr unsigned char STV_DEFAULT = 0;
constexpr unsigned char STV_INTERNAL = 1;
constexpr unsigned char STV_HIDDEN = 2;
constexpr unsigned char STV_PROTECTED = 3;

// Relocation
// https://www.sco.com/developers/gabi/latest/ch4.reloc.html

#define NATELF_ELF32_R_SYM(i) ((i) >> 8)
#define NATELF_ELF32_R_TYPE(i) (0xFF & (i))
#define NATELF_ELF32_R_INFO(s, t) (((s) << 8) + (0xFF & (t)))

#define NATELF_ELF64_R_SYM(i) ((i) >> 32)
#define NATELF_ELF64_R_TYPE(i) (0xFFFFFFFFL & (i))
#define NATELF_ELF64_R_INFO(s, t) (((s) << 32) + (0xFFFFFFFFL & (t)))

struct Elf32_Rel
{
    Elf32_Addr r_offset;
    Elf32_Word r_info;

    constexpr Elf32_Word GetSym() const
    {
        return NATELF_ELF32_R_SYM(r_info);
    }

    constexpr Elf32_Word GetType() const
    {
        return NATELF_ELF32_R_TYPE(r_info);
    }

    constexpr Elf32_Sword GetAddend() const
    {
        return 0;
    }
};

struct Elf32_Rela
{
    Elf32_Addr r_offset;
    Elf32_Word r_info;
    Elf32_Sword r_addend;

    constexpr Elf32_Word GetSym() const
    {
        return NATELF_ELF32_R_SYM(r_info);
    }

    constexpr Elf32_Word GetType() const
    {
        return NATELF_ELF32_R_TYPE(r_info);
    }

    constexpr Elf32_Sword GetAddend() const
    {
        return r_addend;
    }
};

struct Elf64_Rel
{
    Elf64_Addr r_offset;
    Elf64_Xword r_info;

    constexpr Elf64_Xword GetSym() const
    {
        return NATELF_ELF64_R_SYM(r_info);
    }

    constexpr Elf64_Xword GetType() const
    {
        return NATELF_ELF64_R_TYPE(r_info);
    }

    constexpr Elf64_Sxword GetAddend() const
    {
        return 0;
    }
};

struct Elf64_Rela
{
    Elf64_Addr r_offset;
    Elf64_Xword r_info;
    Elf64_Sxword r_addend;

    constexpr Elf64_Xword GetSym() const
    {
        return NATELF_ELF64_R_SYM(r_info);
    }

    constexpr Elf64_Xword GetType() const
    {
        return NATELF_ELF64_R_TYPE(r_info);
    }

    constexpr Elf64_Sxword GetAddend() const
    {
        return r_addend;
    }
};

// TODO: executable ELF definitions

// natelf functions

bool IsElf(std::span<std::uint8_t const> const & elf_view);
bool IsElf32(std::span<std::uint8_t const> const & elf_view);
bool IsElfLittleEndian(std::span<std::uint8_t const> const & elf_view);

bool SanitizeElf(std::span<std::uint8_t> const & elf_view);

template <typename T> inline T const * ElfPtr(std::span<std::uint8_t const> const & elf_view)
{
    return elf_view.size() >= sizeof(T) ? reinterpret_cast<T const *>(elf_view.data()) : nullptr;
}

template <typename T> inline T * ElfPtr(std::span<std::uint8_t> const & elf_view)
{
    return elf_view.size() >= sizeof(T) ? reinterpret_cast<T *>(elf_view.data()) : nullptr;
}

} // namespace natelf

#endif // NATELF_NATELF_HH
