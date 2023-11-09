#ifndef LYN_LAYOUT_HH
#define LYN_LAYOUT_HH

#include <cinttypes>
#include <vector>

#include "address.hh"
#include "lynelf.hh"
#include "symtab.hh"

struct LynSec
{
    LynAddress address;
    std::uint32_t elf_idx;
    std::uint32_t sec_idx;

    std::string GetDisplayName(std::vector<LynElf> const & elves) const;
};

// prepare a the layed out LynSec list. Floating sections aren't layed out yet.
// this also populates lyn_sec in each elf sec.
std::vector<LynSec>
PrepareLayout(std::vector<LynElf> & elves, std::unordered_map<std::string_view, std::uint32_t> const & reference);

// generates offsets for floating sections
void FinalizeLayout(std::vector<LynSec> & layout, std::vector<LynElf> const & elves);

void LayoutSymbols(std::vector<LynSym> & symtab, std::vector<LynElf> const & elves, std::vector<LynSec> const & layout);

#endif // LYN_LAYOUT_HH
