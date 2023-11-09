#ifndef LYN_ADDRESS_HH
#define LYN_ADDRESS_HH

#include <cinttypes>

enum struct LynAnchor
{
    ABSOLUTE,
    FLOAT_ROM,
    // FLOAT_RAM, // TODO: output floating RAM
    // DISCARD, // TODO: GC
};

struct LynAddress
{
    LynAnchor anchor;
    std::int32_t offset;

    constexpr LynAddress() : anchor(LynAnchor::ABSOLUTE), offset(0)
    {
    }

    constexpr LynAddress(LynAnchor anchor, std::int32_t offset) : anchor(anchor), offset(offset)
    {
    }

    constexpr LynAddress Offset(std::int32_t by) const
    {
        return LynAddress(anchor, offset + by);
    }
};

#endif // LYN_ADDRESS_HH
