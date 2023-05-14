#ifndef NATELF_ARM32_HH
#define NATELF_ARM32_HH

#include "natelf.hh"

namespace natelf
{

// ehdr e_machine (mandatory)
#define EM_ARM 0x28

// I don't care for these but let's define them anyway
#define EF_ARM_ABIMASK 0xFF000000
#define EF_ARM_BE8 0x00800000
#define EF_ARM_GCCMASK 0x00400FFF
#define EF_ARM_ABI_FLOAT_HARD 0x00000400
#define EF_ARM_ABI_FLOAT_SOFT 0x00000200

#define SHT_ARM_ATTRIBUTES 0x70000003 // Object file compatibility attributes

#define R_ARM_NONE 0
#define R_ARM_ABS32 2
#define R_ARM_REL32 3
#define R_ARM_ABS16 5
#define R_ARM_ABS8 8
#define R_ARM_THM_CALL 10
#define R_ARM_CALL 28
#define R_ARM_JUMP24 29
#define R_ARM_V4BX 40
#define R_ARM_THM_JUMP11 102
#define R_ARM_THM_JUMP8 103

bool IsElfArm32(std::span<std::uint8_t const> const & elf_view);

} // namespace natelf

#endif // NATELF_ARM32_HH
