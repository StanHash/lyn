#include "relocation.hh"

#include <stdexcept>

#include "natelf/arm32.hh"
#include "natelf/natelf.hh"

#include "fmt/format.h"

#include "byte_utils.hh"

using namespace natelf;

bool RelocationInfo::CanEncode(std::int64_t value) const
{
    std::int64_t mask = 0;
    std::int64_t sign_extend_mask = ~mask;

    for (auto & part : parts)
    {
        std::int64_t const top_bit = static_cast<std::int64_t>(1) << part.bit_size;

        mask |= (top_bit - 1) << part.truncate;
        sign_extend_mask &= ~(((top_bit << part.truncate) - 1) >> 1);
    }

    // positive numbers

    if ((value & mask) == value)
        return true;

    // negative numbers

    if ((value & sign_extend_mask) == sign_extend_mask && (value & (mask | sign_extend_mask)) == value)
        return true;

    return false;
}

template <unsigned int part_size>
static std::int64_t
ExtractConst(std::span<std::uint8_t const> const & bytes, std::span<RelocationPart const> const & parts)
{
    std::int64_t result = 0;

    for (unsigned int i = 0; i < parts.size(); i++)
    {
        auto & part = parts[i];
        auto part_bytes = bytes.subspan(i * part_size, part_size);

        std::int64_t part_from = ReadIntegerBytes<std::endian::little, std::int64_t, part_size>(part_bytes);
        result |= part.Extract(part_from);
    }

    return result;
}

template <unsigned int part_size>
static void
InjectConst(std::span<std::uint8_t> const & bytes, std::span<RelocationPart const> const & parts, std::int64_t value)
{
    for (unsigned int i = 0; i < parts.size(); i++)
    {
        auto & part = parts[i];
        auto part_bytes = bytes.subspan(i * part_size, part_size);

        std::int64_t part_into = ReadIntegerBytes<std::endian::little, std::int64_t, part_size>(part_bytes);
        WriteIntegerBytes<std::endian::little, std::int64_t, part_size>(part_bytes, part.Inject(part_into, value));
    }
}

std::int64_t RelocationInfo::Extract(std::span<std::uint8_t const> const & bytes) const
{
    std::int64_t result = 0;

    switch (part_size)
    {
        case 1:
            result = ExtractConst<1>(bytes, parts);
            break;

        case 2:
            result = ExtractConst<2>(bytes, parts);
            break;

        case 4:
            result = ExtractConst<4>(bytes, parts);
            break;

        default:
            // TODO: assert no other values
            return std::numeric_limits<std::int64_t>::min();
    }

    if (sign_bit != 0)
    {
        // sign extending bit magic
        std::int64_t const mask = 1 << sign_bit;
        result = (result ^ mask) - mask;
    }

    return result;
}

void RelocationInfo::Inject(std::span<std::uint8_t> const & bytes, std::int64_t value) const
{
    switch (part_size)
    {
        case 1:
            InjectConst<1>(bytes, parts, value);
            break;

        case 2:
            InjectConst<2>(bytes, parts, value);
            break;

        case 4:
            InjectConst<4>(bytes, parts, value);
            break;

            // TODO: assert no other values
    }
}

RelocationInfo const & GetArm32RelocationInfo(unsigned int rel_type)
{
    static std::array<RelocationPart, 0> const no_part { {} };
    static std::array<RelocationPart, 1> const one_4byte { { { 0u, 32u, 0 } } };
    static std::array<RelocationPart, 1> const one_2byte { { { 0u, 16u, 0 } } };
    static std::array<RelocationPart, 1> const one_1byte { { { 0u, 8u, 0 } } };
    static std::array<RelocationPart, 1> const thm_jump11_parts { { { 0u, 11u, 1 } } };
    static std::array<RelocationPart, 1> const thm_jump8_parts { { { 0u, 8u, 1 } } };
    static std::array<RelocationPart, 1> const jump24_parts { { { 0u, 24u, 2 } } };

    static std::array<RelocationPart, 2> const thm_bl_parts { {
        { 0u, 11u, 12 }, // LR = PC + ((value) << 12)
        { 0u, 11u, 1 },  // PC = LR + ((value) << 1)
    } };

    static RelocationInfo const abs32 { false, 4u, 0u, { one_4byte } };
    static RelocationInfo const rel32 { true, 4u, 31u, { one_4byte } };
    static RelocationInfo const abs16 { false, 2u, 0u, { one_2byte } };
    static RelocationInfo const abs8 { false, 1u, 0u, { one_1byte } };
    static RelocationInfo const thm_call { true, 2u, 22u, { thm_bl_parts } };
    static RelocationInfo const thm_jump11 { true, 2u, 11u, { thm_jump11_parts } };
    static RelocationInfo const thm_jump8 { true, 2u, 8u, { thm_jump8_parts } };
    static RelocationInfo const jump24 { true, 4u, 25u, { jump24_parts } };
    static RelocationInfo const v4bx { false, 2u, 0u, { no_part } };

    switch (rel_type)
    {
        case R_ARM_ABS32:
            return abs32;

        case R_ARM_REL32:
            return rel32;

        case R_ARM_ABS16:
            return abs16;

        case R_ARM_THM_CALL:
            return thm_call;

        case R_ARM_THM_JUMP11:
            return thm_jump11;

        case R_ARM_THM_JUMP8:
            return thm_jump8;

        case R_ARM_CALL:
        case R_ARM_JUMP24:
            // is there a difference between the two?
            return jump24;

        case R_ARM_V4BX:
            return v4bx;

        case R_ARM_NONE:
        default:
            throw std::runtime_error(fmt::format("Unhandled ARM32 relocation type: {0}", rel_type));
    }
}

template <typename RT = Elf32_Rel>
static void ApplyRelocations(
    LynElf & elf, LynElf::SecRef & rel_sec, std::vector<LynElf> const & elves, std::vector<LynSec> const & layout,
    std::vector<LynSym> const & symtab)
{
    auto symtab_sec_idx = rel_sec.ref_shdr->sh_link;

    if (elf.sym_indirection[symtab_sec_idx] == std::nullopt)
        throw std::runtime_error(fmt::format(
            "(internal error) sym_indirection wasn't built properly for {0}({1})", elf.secs[symtab_sec_idx].ref_name,
            elf.display_name));

    auto & sym_indirection = *elf.sym_indirection[symtab_sec_idx];
    auto & sec = elf.secs[rel_sec.ref_shdr->sh_info];

    // do not relocate sections that aren't being output
    if (sec.lyn_sec == std::nullopt)
        return;

    auto & lyn_sec = layout[*sec.lyn_sec];
    LynAddress sec_addr = lyn_sec.address;

    auto ent_count = rel_sec.EntryCount();

    for (unsigned int i = 0; i < ent_count; i++)
    {
        auto elf_rel = ElfPtr<RT>(rel_sec.Entry(i));
        auto lyn_sym_idx = sym_indirection[elf_rel->GetSym()];
        auto & sym = symtab[lyn_sym_idx];

        auto elf_sym = ElfPtr<Elf32_Sym>(elves[sym.elf_idx].secs[sym.sec_idx].Entry(sym.sym_idx));

        auto rel_type = elf_rel->GetType();
        auto & rel_info = GetArm32RelocationInfo(rel_type);

        auto data = sec.data.subspan(elf_rel->r_offset);

        std::int32_t const addend = elf_rel->GetAddend() + rel_info.Extract(data);

        LynAddress target_address = { LynAnchor::ABSOLUTE, 0u };

        switch (elf_sym->st_shndx)
        {
            case SHN_ABS:
                target_address = { LynAnchor::ABSOLUTE, elf_sym->st_value };
                break;

            case SHN_UNDEF:
                // we cannot relocate to undefined things

                // encode addend and add reloc to pending
                rel_info.Inject(data, addend);
                sec.pending_relocs.emplace_back(elf_rel->r_offset, rel_type, lyn_sym_idx);

                continue;

            case SHN_COMMON:
                // we cannot relocate to COMMON
                // TODO: handle COMMON
                throw std::runtime_error("Relocation to COMMON symbol");

            default:
                auto & target_elf = elves[sym.elf_idx];

                if (elf_sym->st_shndx >= target_elf.secs.size())
                {
                    throw std::runtime_error(fmt::format(
                        "Malformed ELF ({0}): symbol defined for SHDR out of bounds", target_elf.display_name));
                }

                auto & target_sec = target_elf.secs[elf_sym->st_shndx];

                if (target_sec.lyn_sec == std::nullopt)
                    throw std::runtime_error("(internal error) relocating to discarded symbol");

                auto & target_lyn_sec_addr = layout[*target_sec.lyn_sec].address;
                target_address.anchor = target_lyn_sec_addr.anchor;
                target_address.offset = target_lyn_sec_addr.offset + elf_sym->st_value;

                break;
        }

        if (!rel_info.is_relative && target_address.anchor == LynAnchor::ABSOLUTE)
        {
            // absolute reloc with absolute target

            std::int32_t const value = target_address.offset + addend;
            rel_info.Inject(data, value);
        }
        else if (rel_info.is_relative && target_address.anchor == sec_addr.anchor)
        {
            // relative relocs with target of same anchor, we know the distance

            std::int32_t const rel_offset = sec_addr.offset + elf_rel->r_offset;
            std::int32_t const value = target_address.offset + addend - rel_offset;
            rel_info.Inject(data, value);
        }
        else
        {
            // unknown distance or absolute value

            // encode addend and add reloc to pending
            rel_info.Inject(data, addend);
            sec.pending_relocs.emplace_back(elf_rel->r_offset, rel_type, lyn_sym_idx);
        }
    }

    std::sort(sec.pending_relocs.begin(), sec.pending_relocs.end());
}

void RelocateSections(
    std::vector<LynElf> & elves, std::vector<LynSec> const & layout, std::vector<LynSym> const & symtab)
{
    for (unsigned int elf_idx = 0; elf_idx < elves.size(); elf_idx++)
    {
        auto & elf = elves[elf_idx];

        for (unsigned int rel_sec_idx = 0; rel_sec_idx < elf.secs.size(); rel_sec_idx++)
        {
            auto & rel_sec = elf.secs[rel_sec_idx];

            switch (rel_sec.ref_shdr->sh_type)
            {
                case SHT_REL:
                    ApplyRelocations<Elf32_Rel>(elf, rel_sec, elves, layout, symtab);
                    break;

                case SHT_RELA:
                    ApplyRelocations<Elf32_Rela>(elf, rel_sec, elves, layout, symtab);
                    break;
            }
        }
    }
}
