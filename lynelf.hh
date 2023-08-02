#ifndef LYN_LYNELF_HH
#define LYN_LYNELF_HH

#include <cinttypes>
#include <span>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "natelf/natelf.hh"

struct LynElf
{
    struct Reloc
    {
        std::uint32_t offset;
        std::uint32_t rel_type;
        std::uint32_t lyn_sym;

        Reloc(std::uint32_t offset, std::uint32_t rel_type, std::uint32_t lyn_sym)
            : offset(offset), rel_type(rel_type), lyn_sym(lyn_sym)
        {
        }

        constexpr bool operator<(Reloc const & other) const
        {
            return offset < other.offset;
        }
    };

    struct SecRef
    {
        using RelocVariant = std::variant<
            std::monostate, std::vector<natelf::Elf32_Rel const *>, std::vector<natelf::Elf32_Rela const *>>;

        natelf::Elf32_Shdr const * ref_shdr;
        char const * ref_name;

        std::span<std::uint8_t> data;

        // this links ot the layed out LynSec
        // this is only populated after the initial layout step
        std::optional<unsigned int> lyn_sec;

        // this lists relocations that couldn't be resolved
        // this is only populated after the relocation step
        std::vector<Reloc> pending_relocs;

        unsigned int EntryCount(void) const
        {
            return ref_shdr->sh_size / ref_shdr->sh_entsize;
        }

        std::span<std::uint8_t const> Entry(unsigned int idx) const
        {
            return data.subspan(ref_shdr->sh_entsize * idx, ref_shdr->sh_entsize);
        }
    };

    // NOTE: raw_elf_view must view into a sanitized elf!!
    LynElf(std::string_view const & display_name, std::span<std::uint8_t> const & raw_elf_view);

    // this is used for compatibility with old lyn versions, which doesn't distinguish patches from references
    // new uses should pass references explicitly
    bool IsImplicitReference() const;

    // builds a reference address table from the elf
    std::unordered_map<std::string_view, std::uint32_t> BuildReferenceAddresses() const;

    // for error/info messages
    std::string_view display_name;

    std::span<std::uint8_t const> raw_elf_view;

    natelf::Elf32_Ehdr const * ref_ehdr;

    // indexed the same way as raw elves
    std::vector<SecRef> secs;

    // lyn sym id = (*sym_indirection[symtab shn])[elf sym id]
    // this is only built after the lyn sym table was built
    std::vector<std::optional<std::vector<unsigned int>>> sym_indirection;
};

#endif // LYN_LYNELF_HH
