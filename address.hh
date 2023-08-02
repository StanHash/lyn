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
    std::uint32_t offset;
};

#endif // LYN_ADDRESS_HH
