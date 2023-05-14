#include "natelf.hh"

#include <bit>

namespace natelf
{

template <typename T>
static T le2h(T const & le)
    requires(std::is_integral_v<T>)
{
    if constexpr (std::endian::native != std::endian::little)
    {
        std::array bytes = std::bit_cast<std::array<std::uint8_t, sizeof(T)>>(le);

        T result = 0;

        for (std::size_t i = 0u; i < sizeof(T); i++)
            result |= static_cast<T>(bytes[i]) << (i * 8u);

        return result;
    }

    return le;
}

template <typename T>
static T be2h(T const & be)
    requires(std::is_integral_v<T>)
{
    if constexpr (std::endian::native != std::endian::big)
    {
        std::array bytes = std::bit_cast<std::array<std::uint8_t, sizeof(T)>>(be);

        T result = 0;

        for (std::size_t i = 0u; i < sizeof(T); i++)
            result = (result << 8u) | static_cast<T>(bytes[i]);

        return result;
    }

    return be;
}

template <std::endian E, typename T>
static T e2h(T const & value)
    requires(std::is_integral_v<T>)
{
    /*

    // C++23

    if constexpr (E != std::endian::native)
        return std::byteswap(value);

    */

    if constexpr (E == std::endian::little)
        return le2h(value);

    if constexpr (E == std::endian::big)
        return be2h(value);

    return value;
}

template <std::endian E, typename T>
static void e2h(T * field)
    requires(std::is_integral_v<T>)
{
    *field = e2h<E, T>(*field);
}

template <std::endian E> void SanitizeEhdr32(Elf32_Ehdr * ehdr)
{
    e2h<E>(&ehdr->e_type);
    e2h<E>(&ehdr->e_machine);
    e2h<E>(&ehdr->e_version);
    e2h<E>(&ehdr->e_entry);
    e2h<E>(&ehdr->e_phoff);
    e2h<E>(&ehdr->e_shoff);
    e2h<E>(&ehdr->e_flags);
    e2h<E>(&ehdr->e_ehsize);
    e2h<E>(&ehdr->e_phentsize);
    e2h<E>(&ehdr->e_phnum);
    e2h<E>(&ehdr->e_shentsize);
    e2h<E>(&ehdr->e_shnum);
    e2h<E>(&ehdr->e_shstrndx);
}

template <std::endian E> static void SanitizeShdr32(Elf32_Shdr * shdr)
{
    e2h<E>(&shdr->sh_name);
    e2h<E>(&shdr->sh_type);
    e2h<E>(&shdr->sh_flags);
    e2h<E>(&shdr->sh_addr);
    e2h<E>(&shdr->sh_offset);
    e2h<E>(&shdr->sh_size);
    e2h<E>(&shdr->sh_link);
    e2h<E>(&shdr->sh_info);
    e2h<E>(&shdr->sh_addralign);
    e2h<E>(&shdr->sh_entsize);
}

template <std::endian E> static void SanitizeSym32(Elf32_Sym * sym)
{
    e2h<E>(&sym->st_name);
    e2h<E>(&sym->st_value);
    e2h<E>(&sym->st_size);
    e2h<E>(&sym->st_shndx);
}

template <std::endian E> static void SanitizeRel32(Elf32_Rel * rel)
{
    e2h<E>(&rel->r_offset);
    e2h<E>(&rel->r_info);
}

template <std::endian E> static void SanitizeRela32(Elf32_Rela * rela)
{
    e2h<E>(&rela->r_offset);
    e2h<E>(&rela->r_info);
    e2h<E>(&rela->r_addend);
}

template <typename T>
static bool SanitizeSectionEntries(std::span<std::uint8_t> const & sec_data, std::size_t entsize, void (*sanitize)(T *))
{
    std::size_t entcount = sec_data.size() / entsize;

    for (std::size_t i = 0; i < entcount; i++)
    {
        auto ent = ElfPtr<T>(sec_data.subspan(i * entsize, entsize));

        if (ent == nullptr)
            return false;

        sanitize(ent);
    }

    return true;
}

template <std::endian E> static bool SanitizeElf32(std::span<std::uint8_t> const & elf_view)
{
    Elf32_Ehdr * ehdr = ElfPtr<Elf32_Ehdr>(elf_view);

    SanitizeEhdr32<E>(ehdr);

    for (std::size_t i = 0; i < ehdr->e_shnum; i++)
    {
        auto shdr = ElfPtr<Elf32_Shdr>(elf_view.subspan(ehdr->e_shoff + i * ehdr->e_shentsize));

        if (shdr == nullptr)
            return false;

        SanitizeShdr32<E>(shdr);

        auto sec_data = elf_view.subspan(shdr->sh_offset, shdr->sh_size);

        switch (shdr->sh_type)
        {
            case SHT_SYMTAB:
                if (!SanitizeSectionEntries(sec_data, shdr->sh_entsize, SanitizeSym32<E>))
                    return false;

                break;

            case SHT_REL:
                if (!SanitizeSectionEntries(sec_data, shdr->sh_entsize, SanitizeRel32<E>))
                    return false;

                break;

            case SHT_RELA:
                if (!SanitizeSectionEntries(sec_data, shdr->sh_entsize, SanitizeRela32<E>))
                    return false;

                break;
        }
    }

    return true;
}

bool IsElf(std::span<std::uint8_t const> const & elf_view)
{
    Elf32_Ehdr const * ehdr = ElfPtr<Elf32_Ehdr>(elf_view);

    if (ehdr == nullptr)
    {
        // cannot be an ELF file (too small)
        return false;
    }

    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 || ehdr->e_ident[EI_MAG1] != ELFMAG1 || ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
        ehdr->e_ident[EI_MAG3] != ELFMAG3)
    {
        // cannot be an ELF file (bad magic)
        return false;
    }

    return true;
}

bool IsElf32(std::span<std::uint8_t const> const & elf_view)
{
    Elf32_Ehdr const * ehdr = ElfPtr<Elf32_Ehdr>(elf_view);
    return ehdr != nullptr && ehdr->e_ident[EI_CLASS] == ELFCLASS32;
}

bool IsElfLittleEndian(std::span<std::uint8_t const> const & elf_view)
{
    Elf32_Ehdr const * ehdr = ElfPtr<Elf32_Ehdr>(elf_view);
    return ehdr != nullptr && ehdr->e_ident[EI_DATA] == ELFDATA2LSB;
}

bool SanitizeElf(std::span<std::uint8_t> const & elf_view)
{
    Elf32_Ehdr const * ehdr = ElfPtr<Elf32_Ehdr>(elf_view);

    if (ehdr == nullptr)
        return false;

    switch (ehdr->e_ident[EI_CLASS])
    {
        case ELFCLASS32:
            switch (ehdr->e_ident[EI_DATA])
            {
                case ELFDATA2LSB:
                    return SanitizeElf32<std::endian::little>(elf_view);

                case ELFDATA2MSB:
                    return SanitizeElf32<std::endian::big>(elf_view);

                default:
                    return false;
            }

        case ELFCLASS64:
            // TODO: handle this
            return false;

        default:
            return false;
    }
}

} // namespace natelf
