#ifndef LYN_SYMTAB_HH
#define LYN_SYMTAB_HH

#include <cstddef>
#include <vector>

#include "lynelf.hh"

enum struct SymScope
{
    LOCAL,
    GLOBAL,
    UNDEFINED,
    REFERENCE,
};

struct LynSym
{
    std::uint32_t elf_idx;
    std::uint32_t sec_idx;
    std::uint32_t sym_idx;
    char const * name_ref;
    SymScope scope;
};

// builds a shared sym table that lists each local sym, merging where necessary.
// this also populates sym_indirection arrays in each elf.
std::vector<LynSym> BuildSymTable(std::vector<LynElf> & elves);

#endif // LYN_SYMTAB_HH
