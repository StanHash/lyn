#ifndef LYN_SYMTAB_HH
#define LYN_SYMTAB_HH

#include <cstddef>

#include <optional>
#include <vector>

#include "address.hh"
#include "lynelf.hh"

enum struct SymScope
{
    LOCAL,
    GLOBAL,
    UNDEFINED,
};

struct LynSym
{
    std::uint32_t elf_idx;
    std::uint32_t sec_idx;
    std::uint32_t sym_idx;
    char const * name_ref;
    SymScope scope;
    std::optional<LynAddress> address;
};

// builds a shared sym table that lists each local sym, merging where necessary.
// this also populates sym_indirection arrays in each elf.
std::vector<LynSym> BuildSymTable(std::vector<LynElf> & elves);

#endif // LYN_SYMTAB_HH
