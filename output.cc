#include "output.hh"

#include <cstdio>

#include <stdexcept>

#include "fmt/format.h"

#include "byte_utils.hh"
#include "event_helpers.hh"
#include "layout.hh"
#include "lynelf.hh"
#include "relocation.hh"
#include "symtab.hh"

static EventCode::Kind EventCodeKindFromReloc(RelocationInfo const & info)
{
    switch (info.part_size)
    {
        case 1:
            return EventCode::Kind::BYTE;

        case 2:
            return EventCode::Kind::SHORT;

        case 4:
            return info.is_relative ? EventCode::Kind::WORD : EventCode::Kind::POIN;

        default:
            throw std::runtime_error("(internal error) bad relocation info");
    }
}

void Output(
    std::FILE * ref_file, std::vector<LynElf> const & elves, std::vector<LynSym> const & symtab,
    std::vector<LynSec> const & layout)
{
    std::uint32_t rom_float_offset = 0;

    // these should be inputs:
    std::optional<std::string> float_rom_anchor_name;
    bool print_info_comments = true;

    if (layout.size() == 0)
    {
        // TODO: warning, empty output
        return;
    }

    std::fprintf(ref_file, "ALIGN 4\n");

    if (float_rom_anchor_name)
    {
        fmt::println(ref_file, "{0}:", *float_rom_anchor_name);
    }

    for (auto & lyn_sec : layout)
    {
        auto & elf = elves[lyn_sec.elf_idx];
        auto & sec = elf.secs[lyn_sec.sec_idx];
        auto & data = sec.data;

        EventBlock event_block(data.size());

        for (auto & reloc : sec.pending_relocs)
        {
            auto & reloc_info = GetArm32RelocationInfo(reloc.rel_type);

            if (reloc_info.parts.size() != 0)
            {
                EventCode::Kind code_kind = EventCodeKindFromReloc(reloc_info);
                bool currentoffset_anchor = false;

                std::string target_expr;
                std::int64_t addend = reloc_info.Extract(data.subspan(reloc.offset));

                auto & lyn_sym = symtab[reloc.lyn_sym];

                if (lyn_sym.name_ref != nullptr && lyn_sym.name_ref[0] != '\0')
                {
                    // this makes it easy and fast
                    target_expr = lyn_sym.name_ref;
                }
                else
                {
                    // TODO: we should probably give symbols anchored values to make this easier

                    auto & elf = elves[lyn_sym.elf_idx];
                    auto & sec = elf.secs[lyn_sym.sec_idx];
                    auto elf_sym = natelf::ElfPtr<natelf::Elf32_Sym>(sec.Entry(lyn_sym.sym_idx));

                    if (elf_sym->IsDefined() && elf_sym->st_shndx < elf.secs.size())
                    {
                        if (!elf.secs[elf_sym->st_shndx].lyn_sec)
                        {
                            throw std::runtime_error("(lyn internal error) relocating to symbol in discarded section.");
                        }

                        auto & target_lyn_sec = layout[*elf.secs[elf_sym->st_shndx].lyn_sec];

                        auto anchor = target_lyn_sec.address.anchor;
                        auto offset = target_lyn_sec.address.offset + elf_sym->st_value;

                        if (anchor == lyn_sec.address.anchor)
                        {
                            target_expr = "CURRENTOFFSET";
                            currentoffset_anchor = true;
                            addend += offset;
                            addend -= rom_float_offset;
                        }
                        else
                        {
                            throw std::runtime_error("UNIMPLEMENTED");
                        }
                    }
                    else
                    {
                        throw std::runtime_error("(malformed elf) undefined symbol with no name");
                    }
                }

                bool is_complex_expr = false;

                if (reloc_info.is_relative)
                {
                    target_expr = fmt::format("{0}-CURRENTOFFSET", std::move(target_expr));
                    currentoffset_anchor = true;
                    is_complex_expr = true;
                }

                if (addend > 0)
                {
                    target_expr = fmt::format("{0}+{1}", std::move(target_expr), +addend);
                    is_complex_expr = true;
                }
                else if (addend < 0)
                {
                    target_expr = fmt::format("{0}-{1}", std::move(target_expr), -addend);
                    is_complex_expr = true;
                }

                // a relocation part is always in the form of:
                // ((TARGET<<SHIFT)&MASK)|BASE

                std::vector<std::string> exprs;

                for (unsigned int i = 0; i < reloc_info.parts.size(); i++)
                {
                    auto & part = reloc_info.parts[i];

                    std::string part_expr = target_expr;
                    bool is_part_complex_expr = is_complex_expr;

                    int shift = part.GetEffectiveShift();

                    if (shift != 0)
                    {
                        // add parenthesises
                        if (is_part_complex_expr)
                            part_expr = fmt::format("({0})", std::move(part_expr));

                        is_part_complex_expr = true;

                        if (shift > 0)
                        {
                            part_expr = fmt::format("{0}<<{1}", std::move(part_expr), +shift);
                        }
                        else
                        {
                            part_expr = fmt::format("{0}>>{1}", std::move(part_expr), -shift);
                        }
                    }

                    if (part.bit_offset != 0 || part.bit_size != 8 * reloc_info.part_size)
                    {
                        std::uint32_t mask = part.GetMask<std::uint32_t>();

                        if (is_part_complex_expr)
                        {
                            part_expr = fmt::format("({0})&{1}", std::move(part_expr), mask);
                        }
                        else
                        {
                            part_expr = fmt::format("{0}&{1}", std::move(part_expr), mask);
                        }

                        is_part_complex_expr = true;

                        std::uint32_t base = 0;
                        std::uint32_t part_offset = reloc.offset + i * reloc_info.part_size;

                        switch (reloc_info.part_size)
                        {
                            case 1:
                                base = data[reloc.offset];
                                break;

                            case 2:
                                base =
                                    ReadIntegerBytes16<std::endian::little, std::uint32_t>(data.subspan(part_offset));
                                break;

                            case 4:
                                base =
                                    ReadIntegerBytes32<std::endian::little, std::uint32_t>(data.subspan(part_offset));
                                break;
                        }

                        base = base & ~mask;

                        if (base != 0)
                        {
                            if (is_part_complex_expr)
                            {
                                part_expr = fmt::format("({0})|{1}", std::move(part_expr), base);
                            }
                            else
                            {
                                part_expr = fmt::format("{0}|{1}", std::move(part_expr), base);
                            }
                        }
                    }

                    exprs.push_back(std::move(part_expr));
                }

                event_block.MapCode(reloc.offset, EventCode(code_kind, std::move(exprs), currentoffset_anchor));
            }
        }

        event_block.PackCodes();
        event_block.Optimize();

        /*

        if (std::any_of(
                section.symbols().begin(), section.symbols().end(),
                [](const section_data::symbol & sym) { return !sym.isLocal; }))
        {
            output << "PUSH" << std::endl;
            int currentOffset = 0;

            for (auto & symbol : section.symbols())
            {
                if (symbol.isLocal)
                    continue;

                output << "ORG CURRENTOFFSET+$" << std::hex << (symbol.offset - currentOffset) << ";" << symbol.name
                       << ":" << std::endl;
                currentOffset = symbol.offset;
            }

            output << "POP" << std::endl;
        }

        if (std::any_of(
                section.symbols().begin(), section.symbols().end(),

                [](const section_data::symbol & sym) { return sym.isLocal; }))
        {
            output << "{" << std::endl;
            output << "PUSH" << std::endl;

            int currentOffset = 0;

            for (auto & symbol : section.symbols())
            {
                if (!symbol.isLocal)
                    continue;

                output << "ORG CURRENTOFFSET+$" << std::hex << (symbol.offset - currentOffset) << ";" << symbol.name
                       << ":" << std::endl;
                currentOffset = symbol.offset;
            }

            output << "POP" << std::endl;

            events.write_to_stream(output, section);

            output << "}" << std::endl;
        }
        else
        {
            events.write_to_stream(output, section);
        }

        */

        std::string display_sec_name = fmt::format("{0}({1})", sec.ref_name, elf.display_name);

        if (lyn_sec.address.anchor == LynAnchor::FLOAT_ROM)
        {
            if (print_info_comments)
                fprintf(ref_file, "// %s\n", display_sec_name.c_str());

            event_block.WriteToStream(ref_file, data);

            rom_float_offset += data.size();

            // TODO: could we align less if we need align less?

            std::uint32_t misalign = (rom_float_offset % 4);

            if (misalign != 0)
            {
                fprintf(ref_file, "ALIGN 4\n");
                rom_float_offset += (4 - misalign);
            }
        }
        else if (lyn_sec.address.anchor == LynAnchor::ABSOLUTE)
        {
            std::uint32_t addr = lyn_sec.address.offset;

            if (addr >= 0x08000000 && addr <= 0x09FFFFFF)
            {
                // ROM ADDR

                if (print_info_comments)
                    std::fprintf(ref_file, "// %s\n", display_sec_name.c_str());

                std::fprintf(ref_file, "PUSH\nORG 0x%06X\n", (addr & 0x1FFFFFF));
                // TODO: write symbols here
                event_block.WriteToStream(ref_file, data);
                std::fprintf(ref_file, "POP\n");
            }
        }
    }
}
