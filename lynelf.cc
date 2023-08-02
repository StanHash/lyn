#include "lynelf.hh"

#include <stdexcept>

#include "natelf/arm32.hh"
#include "natelf/natelf.hh"

using namespace natelf;

LynElf::LynElf(std::string_view const & display_name, std::span<std::uint8_t> const & raw_elf_view)
    : display_name(std::move(display_name)), raw_elf_view(raw_elf_view)
{
    ref_ehdr = ElfPtr<Elf32_Ehdr>(raw_elf_view);

    secs.resize(ref_ehdr->e_shnum);

    // init section header refs

    auto raw_shdrs = raw_elf_view.subspan(ref_ehdr->e_shoff);

    for (unsigned int i = 0; i < ref_ehdr->e_shnum; i++)
    {
        auto shdr = ElfPtr<Elf32_Shdr>(raw_shdrs.subspan(i * ref_ehdr->e_shentsize, ref_ehdr->e_shentsize));
        secs[i].ref_shdr = shdr;
    }

    // init section name refs and data

    Elf32_Shdr const * shstr_shdr = secs[ref_ehdr->e_shstrndx].ref_shdr;
    auto shstr_data = raw_elf_view.subspan(shstr_shdr->sh_offset, shstr_shdr->sh_size);

    for (unsigned int i = 0; i < ref_ehdr->e_shnum; i++)
    {
        auto shdr = secs[i].ref_shdr;

        if (shdr->sh_name >= shstr_data.size())
            secs[i].ref_name = reinterpret_cast<char const *>(shstr_data.data());
        else
            secs[i].ref_name = reinterpret_cast<char const *>(shstr_data.data()) + shdr->sh_name;

        if (shdr->sh_type != SHT_NOBITS)
            secs[i].data = raw_elf_view.subspan(shdr->sh_offset, shdr->sh_size);
    }
}

bool LynElf::IsImplicitReference() const
{
    for (auto & sec : secs)
    {
        if (sec.ref_shdr->sh_size != 0)
        {
            switch (sec.ref_shdr->sh_type)
            {
                case SHT_PROGBITS:
                case SHT_NOBITS:
                    // implicit references cannot have allocated sections
                    if ((sec.ref_shdr->sh_flags & SHF_ALLOC) != 0)
                        return false;

                    break;

                case SHT_SYMTAB:
                    auto ent_count = sec.EntryCount();

                    for (unsigned int i = 0; i < ent_count; i++)
                    {
                        auto sym = ElfPtr<Elf32_Sym>(sec.Entry(i));

                        if (sym->GetBind() != STB_LOCAL)
                        {
                            // global symbols can only be absolute (or undefined)
                            if (sym->st_shndx != SHN_UNDEF && sym->st_shndx != SHN_ABS)
                                return false;
                        }
                    }

                    break;
            }
        }
    }

    return true;
}

std::unordered_map<std::string_view, std::uint32_t> LynElf::BuildReferenceAddresses() const
{
    std::unordered_map<std::string_view, std::uint32_t> result;

    bool is_rel_elf = ref_ehdr->e_type == ET_REL;

    for (auto & sec : secs)
    {
        if (sec.ref_shdr->sh_type == SHT_SYMTAB)
        {
            auto ent_count = sec.EntryCount();
            auto & name_sec = secs[sec.ref_shdr->sh_link];

            for (unsigned int i = 0; i < ent_count; i++)
            {
                auto sym = ElfPtr<Elf32_Sym>(sec.Entry(i));

                if (sym->GetBind() != STB_LOCAL)
                {
                    auto cstr = reinterpret_cast<char const *>(name_sec.data.data()) +
                        (sym->st_name < name_sec.data.size() ? sym->st_name : 0);

                    std::string_view name { cstr };

                    // for relocatable files, reference syms are those that are absolute
                    // we ignore every other symbol
                    if (is_rel_elf && sym->st_shndx != SHN_ABS)
                        continue;

                    result[name] = sym->st_value;
                }
            }
        }
    }

    return result;
}
