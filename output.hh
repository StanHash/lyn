#ifndef LYN_OUTPUT_HH
#define LYN_OUTPUT_HH

#include <cstdio>

#include "layout.hh"
#include "lynelf.hh"
#include "symtab.hh"

void Output(
    std::FILE * ref_file, std::vector<LynElf> const & elves, std::vector<LynSym> const & symtab,
    std::vector<LynSec> const & layout);

#endif // LYN_OUTPUT_HH
