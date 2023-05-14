#include "arm32.hh"

namespace natelf
{

bool IsElfArm32(std::span<std::uint8_t const> const & elf_view)
{
    Elf32_Ehdr const * ehdr = reinterpret_cast<Elf32_Ehdr const *>(elf_view.data());
    return ehdr->e_machine == EM_ARM;
}

} // namespace natelf
