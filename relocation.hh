#ifndef LYN_RELOCATION_HH
#define LYN_RELOCATION_HH

#include "layout.hh"
#include "lynelf.hh"
#include "symtab.hh"

#include <bit>
#include <span>
#include <vector>

struct RelocationPart
{
    unsigned int bit_offset;
    unsigned int bit_size;
    int truncate;

    // gets mask for this part
    template <typename T>
    constexpr T GetMask() const
        requires std::is_integral_v<T>
    {
        return ((static_cast<T>(1) << bit_size) - 1) << bit_offset;
    }

    constexpr int GetEffectiveShift() const
    {
        return bit_offset - truncate;
    }

    // extract part value from input
    template <typename T>
    constexpr T Extract(T from) const
        requires std::is_integral_v<T>
    {
        T const mask = GetMask<T>();
        int const shift = GetEffectiveShift();

        if (shift > 0)
            return (from & mask) >> shift;
        else if (shift < 0)
            return (from & mask) << (-shift);
        else
            return from & mask;
    }

    // inject part value into input (and return result)
    template <typename T>
    constexpr T Inject(T into, T value) const
        requires std::is_integral_v<T>
    {
        T const mask = GetMask<T>();
        int const shift = GetEffectiveShift();

        if (shift > 0)
            return ((((value) << shift) & mask) | (into & ~mask));

        if (shift < 0)
            return ((((value) >> (-shift)) & mask) | (into & ~mask));

        return (value & mask) | (into & ~mask);
    }
};

struct RelocationInfo
{
    bool is_relative;
    unsigned int part_size; // 1, 2, or 4 bytes
    unsigned int sign_bit;  // if 0, unsigned
    std::span<RelocationPart const> parts;

    // TODO: we could generate masks and such in the constructor so we don't need to compute them every time?
    // this may not be faster because it means more mem accesses

    bool CanEncode(std::int64_t value) const;

    std::int64_t Extract(std::span<std::uint8_t const> const & bytes) const;
    void Inject(std::span<std::uint8_t> const & bytes, std::int64_t value) const;
};

RelocationInfo const & GetArm32RelocationInfo(unsigned int rel_type);

void RelocateSections(
    std::vector<LynElf> & elves, std::vector<LynSec> const & layout, std::vector<LynSym> const & symtab);

#endif // LYN_RELOCATION_HH
