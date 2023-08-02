#include "symtab.hh"

#include <stdexcept>
#include <string_view>
#include <unordered_map>

#include "fmt/format.h"

#include "natelf/natelf.hh"

using namespace natelf;

std::vector<LynSym> BuildSymTable(std::vector<LynElf> & elves)
{
    std::vector<LynSym> symtab;

    // this is temporary and only used here
    // once the symtab and indirection tables are built, this shouldn't be necessary
    std::unordered_map<std::string_view, unsigned int> named_sym_map;

    // for each elf
    for (unsigned int elf_idx = 0; elf_idx < elves.size(); elf_idx++)
    {
        auto & elf = elves[elf_idx];

        elf.sym_indirection.resize(elf.secs.size());

        // for each sec
        for (unsigned int sec_idx = 0; sec_idx < elf.secs.size(); sec_idx++)
        {
            auto & symtab_sec = elf.secs[sec_idx];

            if (symtab_sec.ref_shdr->sh_type != SHT_SYMTAB)
                continue;

            // sym tab

            // we need to do two things here:
            // add to the lyn symtab the references to elf syms
            // build the elf sym indirection table

            auto & name_sec = elf.secs[symtab_sec.ref_shdr->sh_link];

            auto ent_count = symtab_sec.EntryCount();

            std::vector<unsigned int> sym_indirection(ent_count, 0);
            symtab.reserve(symtab.size() + ent_count);

            for (unsigned int sym_idx = 0; sym_idx < ent_count; sym_idx++)
            {
                auto elf_sym = ElfPtr<Elf32_Sym>(symtab_sec.Entry(sym_idx));

                auto name = reinterpret_cast<char const *>(name_sec.data.data()) +
                    (elf_sym->st_name < name_sec.data.size() ? elf_sym->st_name : 0);

                if (name[0] != '\0' && elf_sym->GetBind() != STB_LOCAL)
                {
                    std::string_view name_view { name };
                    auto it = named_sym_map.find(name_view);

                    if (it != named_sym_map.end())
                    {
                        auto lyn_sym_idx = it->second;
                        auto & ent = symtab[lyn_sym_idx];

                        auto existing_elf_sym_span = elves[ent.elf_idx].secs[ent.sec_idx].Entry(ent.sym_idx);
                        auto existing_elf_sym = ElfPtr<Elf32_Sym>(existing_elf_sym_span);

                        // if this sym is undefined, redirect to existing definition regardless

                        if (elf_sym->IsDefined())
                        {
                            if (!existing_elf_sym->IsDefined() || existing_elf_sym->GetBind() == STB_WEAK)
                            {
                                // existing sym ent is yet undefined (or weakly defined)
                                // replace with this

                                ent.elf_idx = elf_idx;
                                ent.sec_idx = sec_idx;
                                ent.sym_idx = sym_idx;
                                ent.name_ref = name;
                                ent.scope = SymScope::GLOBAL;
                            }
                            else if (elf_sym->GetBind() != STB_WEAK)
                            {
                                // this sym name is defined twice

                                // BUG: multiple weak symbol definitions aren't diagnosed

                                // TODO: include elf names here
                                throw std::runtime_error(fmt::format("Multiple definitions of symbol `{0}`", name));
                            }
                        }

                        sym_indirection[sym_idx] = lyn_sym_idx;
                        continue;
                    }
                    else
                    {
                        named_sym_map[name_view] = symtab.size();
                    }
                }

                // add new lyn sym ent

                auto lyn_sym_idx = symtab.size();
                auto & ent = symtab.emplace_back();

                ent.elf_idx = elf_idx;
                ent.sec_idx = sec_idx;
                ent.sym_idx = sym_idx;
                ent.name_ref = name;

                ent.scope = SymScope::GLOBAL;

                if (elf_sym->st_shndx == SHN_UNDEF)
                    ent.scope = SymScope::UNDEFINED;

                if (elf_sym->GetBind() == STB_LOCAL)
                    ent.scope = SymScope::LOCAL;

                sym_indirection[sym_idx] = lyn_sym_idx;
            }

            // set the indirection table for this symtab section
            elf.sym_indirection[sec_idx] = { std::move(sym_indirection) };
        }
    }

    return symtab;
}
