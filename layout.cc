#include "layout.hh"

#include <stdexcept>

#include "fmt/format.h"

#include "natelf/natelf.hh"

using namespace natelf;

std::vector<LynSec>
PrepareLayout(std::vector<LynElf> & elves, std::unordered_map<std::string_view, std::uint32_t> const & reference)
{
    std::vector<LynSec> result;

    for (unsigned int elf_idx = 0; elf_idx < elves.size(); elf_idx++)
    {
        auto & elf = elves[elf_idx];

        for (unsigned int sec_idx = 0; sec_idx < elf.secs.size(); sec_idx++)
        {
            auto & sec = elf.secs[sec_idx];

            // ignore empty sections (should we?)
            if (sec.ref_shdr->sh_size == 0)
                continue;

            auto flags = sec.ref_shdr->sh_flags;

            if ((flags & SHF_ALLOC) != 0)
            {
                auto anchor = LynAnchor::FLOAT_ROM;
                auto offset = 0u;

                if ((flags & SHF_WRITE) != 0)
                {
                    throw std::runtime_error(fmt::format(
                        "Error: lyn can't layout writable (RAM) section {0}({1}).", sec.ref_name, elf.display_name));

                    // anchor = LynAnchor::FLOAT_RAM;
                }

                // TODO: __lyn__at, __lyn__replace, ... sections

                unsigned int lynsec_id = result.size();
                auto & lynsec = result.emplace_back();

                lynsec.address = { anchor, offset };
                lynsec.elf_idx = elf_idx;
                lynsec.sec_idx = sec_idx;

                sec.lyn_sec = { lynsec_id };
            }
        }
    }

    return result;
}

void FinalizeLayout(std::vector<LynSec> & layout, std::vector<LynElf> const & elves)
{
    std::uint32_t float_rom_offset = 0u;

    for (auto & lyn_sec : layout)
    {
        auto & elf = elves[lyn_sec.elf_idx];
        auto & sec = elf.secs[lyn_sec.sec_idx];

        switch (lyn_sec.address.anchor)
        {
            case LynAnchor::FLOAT_ROM:
                // every FLOAT_ROM address is aligned 4, for ease of processing
                // if we wanted to be more correct, we would align to sec.ref_shdr->sh_addralign
                lyn_sec.address.offset = (float_rom_offset + 3) & ~3;
                float_rom_offset = lyn_sec.address.offset + sec.ref_shdr->sh_size;
                break;

            case LynAnchor::ABSOLUTE:
                // ignore!
                break;
        }
    }
}

void LayoutSymbols(std::vector<LynSym> & symtab, std::vector<LynElf> const & elves, std::vector<LynSec> const & layout)
{
    for (unsigned int i = 0; i < symtab.size(); i++)
    {
        auto & sym = symtab[i];
        auto & elf = elves[sym.elf_idx];
        auto & sec = elf.secs[sym.sec_idx];

        auto elf_sym = ElfPtr<Elf32_Sym>(sec.Entry(sym.sym_idx));

        switch (elf_sym->st_shndx)
        {
            case SHN_UNDEF:
                continue;

            case SHN_COMMON:
                continue;

            case SHN_ABS:
                sym.address = { LynAddress(LynAnchor::ABSOLUTE, elf_sym->st_value) };
                break;

            default:
                if (elf_sym->st_shndx < elf.secs.size())
                {
                    auto & data_sec = elf.secs[elf_sym->st_shndx];

                    if (data_sec.lyn_sec)
                    {
                        auto & sec_addr = layout[*data_sec.lyn_sec].address;
                        sym.address = { sec_addr.Offset(elf_sym->st_value) };
                    }
                }

                break;
        }
    }
}
