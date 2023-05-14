#include "event_object.h"

#include <fstream>

#include <algorithm>
#include <iomanip>

#include <fmt/core.h>

#include "natelf/arm32.hh"
#include "natelf/natelf.hh"

#include "core/data_file.h"
#include "ea/event_section.h"
#include "util/hex_write.h"

namespace lyn
{

void event_object::append_from_elf(const char * fileName)
{
    using namespace natelf;

    data_file file(fileName);

    std::span<std::uint8_t const> elf_view(file);

    if (!IsElf(elf_view))
        throw std::runtime_error("This file isn't an ELF.");

    if (!IsElf32(elf_view))
        throw std::runtime_error("This file is an ELF, but not 32bit.");

    if (!IsElfLittleEndian(elf_view))
        throw std::runtime_error("This file is a 32bit ELF, but not Little-Endian.");

    if (!IsElfArm32(elf_view))
        throw std::runtime_error("This file is a 32bit Little-Endian ELF, but not for ARM.");

    // note: this modifies the file in memory (it swaps bytes)
    if (!SanitizeElf(std::span(file)))
        throw std::runtime_error("Failed to sanitize ELF");

    auto readString = [&file](Elf32_Shdr const * section, unsigned offset) -> std::string
    { return file.cstr_at(section->sh_offset + offset); };

    Elf32_Ehdr const * ehdr = ElfPtr<Elf32_Ehdr>(elf_view);

    auto get_shdr = [&](std::size_t shn) -> Elf32_Shdr const *
    { return ElfPtr<Elf32_Shdr>(elf_view.subspan(ehdr->e_shoff + shn * ehdr->e_shentsize)); };

    Elf32_Shdr const * shstr_shdr = get_shdr(ehdr->e_shstrndx);

    if (shstr_shdr == nullptr)
        throw std::runtime_error("Failed to obtain section names.");

    std::vector<section_data> newSections(ehdr->e_shnum);
    std::vector<bool> outMap(ehdr->e_shnum, false);

    auto getLocalSymbolName = [this](int section, int index) -> std::string
    {
        return fmt::format("_L{0:08X}_{1:08X}", section, index);

        /*
        std::string result;
        result.reserve(0x10);

        result.append("_L");
        util::append_hex(result, mSections.size() + section);
        result.append("_");
        util::append_hex(result, index);

        return result;
        */
    };

    auto getGlobalSymbolName = [](const char * name) -> std::string
    {
        std::string result(name);
        std::size_t find = 0;

        while ((find = result.find('.')) != std::string::npos)
            result[find] = '_';

        return result;
    };

    for (unsigned int i = 0; i < ehdr->e_shnum; ++i)
    {
        auto shdr = ElfPtr<Elf32_Shdr>(elf_view.subspan(ehdr->e_shoff + i * ehdr->e_shentsize));

        auto flags = shdr->sh_flags;

        if ((flags & natelf::SHF_ALLOC) != 0 && (flags & natelf::SHF_WRITE) == 0)
        {
            auto & section = newSections.at(i);
            auto section_view = elf_view.subspan(shdr->sh_offset, shdr->sh_size);

            // TODO: put filename in name (whenever name will be useful)

            section.set_name(readString(shstr_shdr, shdr->sh_name));

            section.clear();
            section.reserve(section_view.size());

            std::copy(section_view.begin(), section_view.end(), std::back_inserter(section));

            outMap[i] = true;
        }
    }

    for (unsigned int si = 0; si < ehdr->e_shnum; ++si)
    {
        auto shdr = ElfPtr<Elf32_Shdr>(elf_view.subspan(ehdr->e_shoff + si * ehdr->e_shentsize));
        auto data = elf_view.subspan(shdr->sh_offset, shdr->sh_size);

        switch (shdr->sh_type)
        {
            case natelf::SHT_SYMTAB:
            {
                auto ent_count = shdr->sh_size / shdr->sh_entsize;
                auto name_shdr = get_shdr(shdr->sh_link);

                for (unsigned int i = 0; i < ent_count; ++i)
                {
                    Elf32_Sym const * sym = ElfPtr<Elf32_Sym>(data.subspan(i * shdr->sh_entsize, shdr->sh_entsize));

                    if (sym->st_shndx == natelf::SHN_ABS)
                    {
                        auto name = (NATELF_ELF32_ST_BIND(sym->st_info) == natelf::STB_LOCAL)
                            ? getLocalSymbolName(si, i)
                            : getGlobalSymbolName(readString(name_shdr, sym->st_name).c_str());

                        mAbsoluteSymbols.push_back(section_data::symbol { name, sym->st_value, false });
                    }
                    else
                    {
                        if (sym->st_shndx >= outMap.size() || !outMap.at(sym->st_shndx))
                            continue;

                        auto & section = newSections.at(sym->st_shndx);

                        std::string name = readString(name_shdr, sym->st_name);

                        if (NATELF_ELF32_ST_TYPE(sym->st_info) == natelf::STT_NOTYPE &&
                            NATELF_ELF32_ST_BIND(sym->st_info) == natelf::STB_LOCAL)
                        {
                            std::string subString = name.substr(0, 3);

                            if ((name == "$t") || (subString == "$t."))
                            {
                                section.set_mapping(sym->st_value, section_data::mapping::Thumb);
                                continue;
                            }

                            if ((name == "$a") || (subString == "$a."))
                            {
                                section.set_mapping(sym->st_value, section_data::mapping::ARM);
                                continue;
                            }

                            if ((name == "$d") || (subString == "$d."))
                            {
                                section.set_mapping(sym->st_value, section_data::mapping::Data);
                                continue;
                            }
                        }

                        if (NATELF_ELF32_ST_BIND(sym->st_info) == natelf::STB_LOCAL)
                            name = getLocalSymbolName(si, i);
                        else
                            name = getGlobalSymbolName(name.c_str());

                        section.symbols().push_back(section_data::symbol {
                            name, sym->st_value, (NATELF_ELF32_ST_BIND(sym->st_info) == natelf::STB_LOCAL) });
                    }
                }

                break;
            }

            case natelf::SHT_REL:
            {
                if (!outMap.at(shdr->sh_info))
                    break;

                auto ent_count = shdr->sh_size / shdr->sh_entsize;

                auto sym_shdr = get_shdr(shdr->sh_link);
                auto sym_name_shdr = get_shdr(sym_shdr->sh_link);

                auto sym_data = elf_view.subspan(sym_shdr->sh_offset, sym_shdr->sh_size);

                auto & section = newSections.at(shdr->sh_info);

                for (unsigned i = 0; i < ent_count; ++i)
                {
                    auto rel = ElfPtr<Elf32_Rel>(data.subspan(i * shdr->sh_entsize, shdr->sh_entsize));

                    auto sym_idx = NATELF_ELF32_R_SYM(rel->r_info);

                    auto sym =
                        ElfPtr<Elf32_Sym>(sym_data.subspan(sym_idx * sym_shdr->sh_entsize, sym_shdr->sh_entsize));

                    std::string name = (NATELF_ELF32_ST_BIND(sym->st_info) == natelf::STB_LOCAL)
                        ? getLocalSymbolName(shdr->sh_link, sym_idx)
                        : getGlobalSymbolName(readString(sym_name_shdr, sym->st_name).c_str());

                    section.relocations().push_back(
                        section_data::relocation { name, 0, NATELF_ELF32_R_TYPE(rel->r_info), rel->r_offset });
                }

                break;
            }

            case natelf::SHT_RELA:
            {
                if (!outMap.at(shdr->sh_info))
                    break;

                auto ent_count = shdr->sh_size / shdr->sh_entsize;

                auto sym_shdr = get_shdr(shdr->sh_link);
                auto sym_name_shdr = get_shdr(sym_shdr->sh_link);

                auto sym_data = elf_view.subspan(sym_shdr->sh_offset, sym_shdr->sh_size);

                auto & section = newSections.at(shdr->sh_info);

                for (unsigned i = 0; i < ent_count; ++i)
                {
                    auto rela = ElfPtr<Elf32_Rela>(data.subspan(i * shdr->sh_entsize, shdr->sh_entsize));

                    auto sym_idx = NATELF_ELF32_R_SYM(rela->r_info);

                    auto sym =
                        ElfPtr<Elf32_Sym>(sym_data.subspan(sym_idx * sym_shdr->sh_entsize, sym_shdr->sh_entsize));

                    std::string name = (NATELF_ELF32_ST_BIND(sym->st_info) == natelf::STB_LOCAL)
                        ? getLocalSymbolName(shdr->sh_link, sym_idx)
                        : getGlobalSymbolName(readString(sym_name_shdr, sym->st_name).c_str());

                    section.relocations().push_back(section_data::relocation {
                        name, rela->r_addend, NATELF_ELF32_R_TYPE(rela->r_info), rela->r_offset });
                }

                break;
            }
        }
    }

    // Remove empty sections

    newSections.erase(
        std::remove_if(
            newSections.begin(), newSections.end(), [](const section_data & section) { return (section.size() == 0); }),
        newSections.end());

    // Add to existing section list

    mSections.reserve(mSections.size() + newSections.size());

    std::copy(
        std::make_move_iterator(newSections.begin()), std::make_move_iterator(newSections.end()),
        std::back_inserter(mSections));
}

void event_object::try_transform_relatives()
{
    for (auto & section : mSections)
    {
        for (auto & relocation : section.relocations())
        {
            if (auto relocatelet = mRelocator.get_relocatelet(relocation.type))
            {
                if (!relocatelet->is_absolute() && relocatelet->can_make_trampoline())
                {
                    std::string renamed;

                    renamed.reserve(4 + relocation.symbolName.size());

                    renamed.append("_LP_"); // local proxy
                    renamed.append(relocation.symbolName);

                    bool exists = false;

                    for (auto & section : mSections)
                        for (auto & sym : section.symbols())
                            if (sym.name == renamed)
                                exists = true;

                    if (!exists)
                    {
                        section_data newData = relocatelet->make_trampoline(relocation.symbolName, relocation.addend);
                        newData.symbols().push_back(
                            { renamed, (newData.mapping_type_at(0) == section_data::mapping::Thumb), true });

                        mSections.push_back(std::move(newData));
                    }

                    relocation.symbolName = renamed;
                    relocation.addend = 0; // TODO: -4
                }
            }
        }
    }
}

void event_object::try_relocate_relatives()
{
    unsigned offset = 0;

    for (auto & section : mSections)
    {
        section.relocations().erase(
            std::remove_if(
                section.relocations().begin(), section.relocations().end(),
                [this, &section, offset](section_data::relocation & relocation) -> bool
                {
                    unsigned symOffset = 0;

                    auto relocatelet = mRelocator.get_relocatelet(relocation.type);

                    for (auto & symSection : mSections)
                    {
                        for (auto & symbol : symSection.symbols())
                        {
                            if (symbol.name != relocation.symbolName)
                                continue;

                            if (relocatelet && !relocatelet->is_absolute())
                            {
                                relocatelet->apply_relocation(
                                    section, relocation.offset, symOffset + symbol.offset, relocation.addend);

                                return true;
                            }

                            relocation.symbolName = "CURRENTOFFSET";
                            relocation.addend += (symOffset + symbol.offset) - (offset + relocation.offset);

                            return false;
                        }

                        symOffset += symSection.size();

                        if (unsigned misalign = (symOffset % 4))
                            symOffset += (4 - misalign);
                    }

                    return false;
                }),
            section.relocations().end());

        offset += section.size();

        if (unsigned misalign = (offset % 4))
            offset += (4 - misalign);
    }
}

void event_object::try_relocate_absolutes()
{
    for (auto & section : mSections)
    {
        section.relocations().erase(
            std::remove_if(
                section.relocations().begin(), section.relocations().end(),
                [this, &section](const section_data::relocation & relocation) -> bool
                {
                    for (auto & symbol : mAbsoluteSymbols)
                    {
                        if (symbol.name != relocation.symbolName)
                            continue;

                        if (auto relocatelet = mRelocator.get_relocatelet(relocation.type))
                        {
                            if (relocatelet->is_absolute())
                            {
                                relocatelet->apply_relocation(
                                    section, relocation.offset, symbol.offset, relocation.addend);

                                return true;
                            }
                        }

                        return false;
                    }

                    return false;
                }),
            section.relocations().end());
    }
}

void event_object::remove_unnecessary_symbols()
{
    for (auto & section : mSections)
    {
        section.symbols().erase(
            std::remove_if(
                section.symbols().begin(), section.symbols().end(),
                [this](const section_data::symbol & symbol) -> bool
                {
                    if (!symbol.isLocal)
                        return false; // symbol may be used outside of the scope of this object

                    for (auto & section : mSections)
                        for (auto & reloc : section.relocations())
                            if (reloc.symbolName == symbol.name)
                                return false; // a relocation is dependant on this symbol

                    return true; // symbol is local and unused, we can remove it safely (hopefully)
                }),
            section.symbols().end());
    }
}

void event_object::cleanup()
{
    for (auto & section : mSections)
    {
        std::sort(
            section.symbols().begin(), section.symbols().end(),
            [](const section_data::symbol & a, const section_data::symbol & b) -> bool { return a.offset < b.offset; });
    }
}

std::vector<event_object::hook> event_object::get_hooks() const
{
    std::vector<hook> result;

    for (auto & absSymbol : mAbsoluteSymbols)
    {
        if (!(absSymbol.offset & 0x8000000))
            continue; // Not in ROM

        for (auto & section : mSections)
        {
            for (auto & locSymbol : section.symbols())
            {
                if (absSymbol.name != locSymbol.name)
                    continue; // Not same symbol

                result.push_back({ (absSymbol.offset & (~0x8000000)), absSymbol.name });
            }
        }
    }

    return result;
}

void event_object::write_events(std::ostream & output) const
{
    unsigned offset = 0;

    for (auto & section : mSections)
    {
        event_section events; // = section.make_events();
        events.resize(section.size());

        output << "ALIGN 4" << std::endl;

        for (auto & relocation : section.relocations())
        {
            if (auto relocatelet = mRelocator.get_relocatelet(relocation.type))
            {
                // This is probably the worst hack I've ever written
                // lyn needs a rewrite

                // (I makes sure that relative relocations to known absolute values will reference the value and not the
                // name)

                auto it = std::find_if(
                    mAbsoluteSymbols.begin(), mAbsoluteSymbols.end(),
                    [&](const section_data::symbol & sym) { return sym.name == relocation.symbolName; });

                auto symName =
                    (it == mAbsoluteSymbols.end()) ? relocation.symbolName : util::make_hex_string("$", it->offset);

                events.map_code(
                    relocation.offset,
                    relocatelet->make_event_code(section, relocation.offset, symName, relocation.addend));
            }
            else if (relocation.type != R_ARM_V4BX) // dirty hack
                throw std::runtime_error(
                    std::string("unhandled relocation type #").append(std::to_string(relocation.type)));
        }

        events.compress_codes();
        events.optimize();

        if (std::any_of(
                section.symbols().begin(), section.symbols().end(),

                [](const section_data::symbol & sym) { return !sym.isLocal; }))
        {
            output << "PUSH" << std::endl;
            int currentOffset = 0;

            for (auto & symbol : section.symbols())
            {
                if (symbol.isLocal)
                    continue;

                output << "ORG CURRENTOFFSET+$" << std::hex << (symbol.offset - currentOffset) << ";" << symbol.name
                       << ":" << std::endl;
                currentOffset = symbol.offset;
            }

            output << "POP" << std::endl;
        }

        if (std::any_of(
                section.symbols().begin(), section.symbols().end(),

                [](const section_data::symbol & sym) { return sym.isLocal; }))
        {
            output << "{" << std::endl;
            output << "PUSH" << std::endl;

            int currentOffset = 0;

            for (auto & symbol : section.symbols())
            {
                if (!symbol.isLocal)
                    continue;

                output << "ORG CURRENTOFFSET+$" << std::hex << (symbol.offset - currentOffset) << ";" << symbol.name
                       << ":" << std::endl;
                currentOffset = symbol.offset;
            }

            output << "POP" << std::endl;

            events.write_to_stream(output, section);

            output << "}" << std::endl;
        }
        else
        {
            events.write_to_stream(output, section);
        }

        offset += section.size();

        if (unsigned misalign = (offset % 4))
            offset += (4 - misalign);
    }
}

} // namespace lyn
