#include <cinttypes>
#include <cstdio>
#include <vector>

#include "layout.hh"
#include "lynelf.hh"
#include "output.hh"
#include "relocation.hh"
#include "symtab.hh"

#include "fmt/format.h"

std::vector<std::vector<std::uint8_t>> LoadRawElves(std::span<std::string_view const> const & elf_paths)
{
    std::vector<std::vector<std::uint8_t>> result;

    for (auto & path : elf_paths)
    {
        std::string path_real(path);

        FILE * own_file = std::fopen(path_real.c_str(), "rb");

        if (own_file == nullptr)
        {
            throw std::runtime_error(fmt::format("error: failed to open '{0}' for read.", path));
        }

        std::vector<std::uint8_t> elf_data;

        std::fseek(own_file, 0, SEEK_END);
        elf_data.resize(ftell(own_file));

        std::fseek(own_file, 0, SEEK_SET);
        std::size_t read = fread(elf_data.data(), 1, elf_data.size(), own_file);

        std::fclose(own_file);

        if (read != elf_data.size())
        {
            throw std::runtime_error(fmt::format("error: IO error while attempting to read from '{0}'.", path));
        }

        result.push_back(std::move(elf_data));
    }

    return result;
}

int main(int argc, char const * argv[])
{
    std::vector<std::string_view> elf_paths;
    std::optional<std::string_view> reference_path;

    for (int i = 1; i < argc; i++)
    {
        std::string_view arg_view(argv[i]);

        if (arg_view[0] == '-')
        {
            // TODO: other parameters
        }
        else
        {
            elf_paths.push_back(arg_view);
        }
    }

    try
    {
        // new process order:
        // - load all elves
        // - prepare layout add any extra sections if necessary (hooks)
        // - relocate from reference
        // - build global symtab
        // - gc
        // - finish layout (precise offsets)
        // - populate symtab with addresses
        // - relocate
        // - output event

        std::optional<LynElf> reference_elf;

        // TODO: build elf_paths

        // Step 1. Load all elves

        auto raw_elves = LoadRawElves(elf_paths);

        std::vector<LynElf> elves;

        for (unsigned int i = 0; i < raw_elves.size(); i++)
        {
            auto & path = elf_paths[i];
            auto & raw_elf = raw_elves[i];

            LynElf lyn_elf(path, raw_elf);

            if (!reference_elf && lyn_elf.IsImplicitReference())
            {
                // TODO: warn about implicit references
                std::fprintf(stderr, "REF: %s\n", path.data());
                reference_elf = { std::move(lyn_elf) };
            }
            else
            {
                elves.push_back(std::move(lyn_elf));
            }
        }

        std::unordered_map<std::string_view, std::uint32_t> reference;

        if (reference_elf != std::nullopt)
            reference = reference_elf->BuildReferenceAddresses();

        // Step 1.5. mark auto hooks
        // TODO

        // Step 2. prepare layout

        auto layout = PrepareLayout(elves, reference);

        // Step 3. build symbol table

        auto symtab = BuildSymTable(elves);

        // Step 4. GC
        // TODO

        // Step 5. finish layout
        FinalizeLayout(layout, elves);
        LayoutSymbols(symtab, elves, layout);

        // Step 6. relocate
        RelocateSections(elves, layout, symtab);

        // Step 7. output
        Output(stdout, elves, symtab, layout);
    }
    catch (std::exception const & e)
    {
        auto message = fmt::format("lyn error: {0}", e.what());
        std::fprintf(stderr, "%s\n", message.c_str());

        return 1;
    }

    return 0;
}
