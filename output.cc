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

void OutputSymbols(std::FILE * ref_file, std::vector<LynSym> const & symtab, bool print_info_comments)
{
    std::vector<std::reference_wrapper<LynSym const>> float_rom_syms;
    std::vector<std::reference_wrapper<LynSym const>> absolute_syms;

    for (auto & lyn_sym : symtab)
    {
        if (!lyn_sym.address || lyn_sym.scope != SymScope::GLOBAL || lyn_sym.name_ref == nullptr ||
            lyn_sym.name_ref[0] == '\0')
            continue;

        switch (lyn_sym.address->anchor)
        {
            case LynAnchor::FLOAT_ROM:
                float_rom_syms.push_back(std::ref(lyn_sym));
                break;

            case LynAnchor::ABSOLUTE:
                absolute_syms.push_back(std::ref(lyn_sym));
                break;
        }
    }

    std::sort(
        float_rom_syms.begin(), float_rom_syms.end(),
        [](auto & a, auto & b) { return a.get().address->offset < b.get().address->offset; });

    std::int32_t rom_float_offset = 0;
    bool pushed_for_symbols = false;

    for (auto lyn_sym_ref : float_rom_syms)
    {
        auto & lyn_sym = lyn_sym_ref.get();

        if (!pushed_for_symbols)
        {
            std::fprintf(ref_file, "PUSH\n");
            pushed_for_symbols = true;

            if (print_info_comments)
                std::fprintf(ref_file, "// Symbols\n");
        }

        auto & lyn_addr = *lyn_sym.address;

        int diff = lyn_addr.offset - rom_float_offset;

        if (diff > 0)
            fmt::print("ORG CURRENTOFFSET+{0} ; ", diff);

        rom_float_offset = lyn_addr.offset;

        fmt::println("{0}:", lyn_sym.name_ref);
    }

    for (auto lyn_sym_ref : absolute_syms)
    {
        auto & lyn_sym = lyn_sym_ref.get();

        if (!pushed_for_symbols)
        {
            std::fprintf(ref_file, "PUSH\n");
            pushed_for_symbols = true;

            if (print_info_comments)
                std::fprintf(ref_file, "// Symbols\n");
        }

        auto & lyn_addr = *lyn_sym.address;

        if (lyn_addr.offset < 0x08000000 || lyn_addr.offset > 0x09FFFFFF)
        {
            // output #define for absolute symbols we not in ROM
            fmt::println("#define {0} 0x{1:08X}", lyn_sym.name_ref, (std::uint32_t)lyn_addr.offset);
        }
        else
        {
            fmt::println("ORG 0x{0:06X} ; {1}:", lyn_addr.offset - 0x08000000, lyn_sym.name_ref);
        }
    }

    if (pushed_for_symbols)
    {
        std::fprintf(ref_file, "POP\n");
        pushed_for_symbols = false;
    }
}

void OutputSymbols2(std::FILE * ref_file, std::vector<LynSym> const & symtab, bool print_info_comments)
{
    int rom_float_offset = 0;
    bool pushed_for_symbols = false;

    // output float rom symbols

    for (auto & lyn_sym : symtab)
    {
        if (!lyn_sym.address || lyn_sym.address->anchor != LynAnchor::FLOAT_ROM)
            continue;

        if (lyn_sym.scope == SymScope::GLOBAL && lyn_sym.name_ref != nullptr && lyn_sym.name_ref[0] != '\0')
        {
            if (!pushed_for_symbols)
            {
                std::fprintf(ref_file, "PUSH\n");
                pushed_for_symbols = true;

                if (print_info_comments)
                    std::fprintf(ref_file, "// Symbols\n");
            }

            auto & lyn_addr = *lyn_sym.address;

            // stan from 3 months ago you moron use signed int on things you do math on please thank
            int diff = ((int)lyn_addr.offset) - rom_float_offset;

            if (diff > 0)
            {
                fmt::print("ORG CURRENTOFFSET+{0} ; ", diff);
            }
            else
            {
                fmt::print("ORG CURRENTOFFSET-{0} ; ", -diff);
            }

            rom_float_offset = lyn_addr.offset;

            fmt::println("{0}:", lyn_sym.name_ref);
        }
    }

    // output abs symbols

    for (auto & lyn_sym : symtab)
    {
        if (!lyn_sym.address || lyn_sym.address->anchor != LynAnchor::ABSOLUTE)
            continue;

        if (lyn_sym.scope == SymScope::GLOBAL && lyn_sym.name_ref != nullptr && lyn_sym.name_ref[0] != '\0')
        {
            if (!pushed_for_symbols)
            {
                std::fprintf(ref_file, "PUSH\n");
                pushed_for_symbols = true;

                if (print_info_comments)
                    std::fprintf(ref_file, "// Symbols\n");
            }

            auto & lyn_addr = *lyn_sym.address;

            if (lyn_addr.offset < 0x08000000 || lyn_addr.offset >= 0x0A000000)
            {
                // output #define for absolute symbols we don't have anything else for
                fmt::println("#define {0} 0x{1:08X}", lyn_sym.name_ref, (std::uint32_t)lyn_addr.offset);
            }
            else
            {
                fmt::println("ORG 0x{0:06X} ; {1}:", lyn_addr.offset - 0x08000000, lyn_sym.name_ref);
            }
        }
    }

    if (pushed_for_symbols)
    {
        std::fprintf(ref_file, "POP\n");
        pushed_for_symbols = false;
    }
}

void Output(
    std::FILE * ref_file, std::vector<LynElf> const & elves, std::vector<LynSym> const & symtab,
    std::vector<LynSec> const & layout)
{
    // these should be inputs:
    std::optional<std::string> float_rom_anchor_name;
    bool print_info_comments = true;

    std::fprintf(ref_file, "ALIGN 4\n");

    if (float_rom_anchor_name)
    {
        fmt::println(ref_file, "{0}:", *float_rom_anchor_name);
    }

    OutputSymbols(ref_file, symtab, print_info_comments);

    std::uint32_t rom_float_offset = 0;

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

                // TODO: for some reason this includes locals
                if (lyn_sym.scope != SymScope::LOCAL && lyn_sym.name_ref != nullptr && lyn_sym.name_ref[0] != '\0')
                {
                    // this makes it easy and fast
                    target_expr = lyn_sym.name_ref;
                }
                else
                {
                    if (lyn_sym.address == std::nullopt)
                    {
                        throw std::runtime_error("undefined symbol with no name");
                    }

                    auto & sym_addr = *lyn_sym.address;
                    addend += sym_addr.offset;

                    if (sym_addr.anchor == lyn_sec.address.anchor)
                    {
                        target_expr = "CURRENTOFFSET";
                        currentoffset_anchor = true;
                        addend -= rom_float_offset + reloc.offset;
                    }
                    else
                    {
                        switch (sym_addr.anchor)
                        {
                            case LynAnchor::ABSOLUTE:
                                target_expr = "";
                                break;

                            default:
                                throw std::runtime_error("UNIMPLEMENTED");
                        }
                    }
                }

                bool is_complex_expr = false;

                // add addend expr

                if (target_expr.empty())
                {
                    if (addend < 0x10)
                        target_expr = fmt::format("{0}", addend);
                    else
                        target_expr = fmt::format("${0:X}", addend);
                }
                else if (addend > 0)
                {
                    target_expr = fmt::format("{0}+{1}", std::move(target_expr), +addend);
                    is_complex_expr = true;
                }
                else if (addend < 0)
                {
                    target_expr = fmt::format("{0}-{1}", std::move(target_expr), -addend);
                    is_complex_expr = true;
                }

                // add relative expr

                if (reloc_info.is_relative)
                {
                    target_expr = fmt::format("{0}-CURRENTOFFSET", std::move(target_expr));
                    currentoffset_anchor = true;
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

        std::string display_sec_name = lyn_sec.GetDisplayName(elves);

        if (lyn_sec.address.anchor == LynAnchor::FLOAT_ROM)
        {
            constexpr std::uint32_t align = 4;

            std::uint32_t misalign = (rom_float_offset % align);

            if (misalign != 0)
            {
                fmt::println(ref_file, "ALIGN {0}", align);
                rom_float_offset += (align - misalign);
            }

            if (print_info_comments)
                fmt::println(ref_file, "// Section: {0}", display_sec_name.c_str());

            event_block.WriteToStream(ref_file, data);

            rom_float_offset += data.size();
        }
        else if (lyn_sec.address.anchor == LynAnchor::ABSOLUTE)
        {
            std::uint32_t addr = lyn_sec.address.offset;

            if (addr >= 0x08000000 && addr <= 0x09FFFFFF)
            {
                // ROM ADDR

                if (print_info_comments)
                    fmt::println(ref_file, "// {0}", display_sec_name.c_str());

                std::fprintf(ref_file, "PUSH\nORG 0x%06X\n", (addr & 0x1FFFFFF));
                event_block.WriteToStream(ref_file, data);
                std::fprintf(ref_file, "POP\n");
            }
        }
    }
}
