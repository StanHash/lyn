#include "event_object.h"

#include <algorithm>
#include <cassert>
#include <format>
#include <ostream>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "elfcpp/elfcpp_file.h"
#include "elfcpp/arm.h"

#include "core/data_file.h"
#include "ea/event_section.h"

namespace elfcpp {

// for some reason this is required in order to make the whole thing work

template<int Size, bool BigEndian, typename File>
const int Elf_file<Size, BigEndian, File>::ehdr_size;

template<int Size, bool BigEndian, typename File>
const int Elf_file<Size, BigEndian, File>::phdr_size;

template<int Size, bool BigEndian, typename File>
const int Elf_file<Size, BigEndian, File>::shdr_size;

template<int Size, bool BigEndian, typename File>
const int Elf_file<Size, BigEndian, File>::sym_size;

template<int Size, bool BigEndian, typename File>
const int Elf_file<Size, BigEndian, File>::rel_size;

template<int Size, bool BigEndian, typename File>
const int Elf_file<Size, BigEndian, File>::rela_size;

} // namespace elfcpp

namespace lyn {

void event_object::append_from_elf(const char* fileName)
{
	data_file file(fileName);

	elfcpp::Elf_file<32, false, lyn::data_file> elfFile(&file);

	auto readString = [&file] (elfcpp::Shdr<32, false> section, unsigned offset) -> std::string
	{
		return file.cstr_at(section.get_sh_offset() + offset);
	};

	std::vector<section_data> newSections(elfFile.shnum());
	std::vector<bool> outMap(elfFile.shnum(), false);

	auto getLocalSymbolName = [this] (int section, int index) -> std::string
	{
		return std::format("_L{0:X}_{1:X}", mSections.size() + section, index);
	};

	auto getGlobalSymbolName = [] (const char* name) -> std::string
	{
		std::string result(name);
		std::size_t find = 0;

		while ((find = result.find('.')) != std::string::npos)
			result[find] = '_';

		return result;
	};

	for (unsigned i = 0; i < elfFile.shnum(); ++i)
	{
		auto flags = elfFile.section_flags(i);

		if ((flags & elfcpp::SHF_ALLOC) && !(flags & elfcpp::SHF_WRITE))
		{
			auto& section = newSections.at(i);
			auto  loc     = elfFile.section_contents(i);

			// TODO: put filename in name (whenever name will be useful)

			section.set_name(elfFile.section_name(i));

			section.resize(loc.data_size);

			std::copy(
				std::next(file.begin(), loc.file_offset),
				std::next(file.begin(), loc.file_offset+loc.data_size),
				section.begin());

			outMap[i] = true;
		}
	}

	for (unsigned si = 0; si < elfFile.shnum(); ++si)
	{
		elfcpp::Shdr<32, false> header(&file, elfFile.section_header(si));

		switch (header.get_sh_type())
		{

		case elfcpp::SHT_SYMTAB:
		{
			const unsigned count = header.get_sh_size() / header.get_sh_entsize();
			const elfcpp::Shdr<32, false> nameShdr(&file, elfFile.section_header(header.get_sh_link()));

			for (unsigned i = 0; i < count; ++i)
			{
				elfcpp::Sym<32, false> sym(&file, data_file::Location(
					header.get_sh_offset() + i * header.get_sh_entsize(),
					header.get_sh_entsize()
				));

				switch (sym.get_st_shndx())
				{

				case elfcpp::SHN_ABS:
				{
					auto name = (sym.get_st_bind() == elfcpp::STB_LOCAL)
						? getLocalSymbolName(si, i)
						: getGlobalSymbolName(readString(nameShdr, sym.get_st_name()).c_str());

					bool is_function = sym.get_st_type() == elfcpp::STT_FUNC;

					mAbsoluteSymbols.push_back(section_data::symbol {
						name,
						sym.get_st_value(),
						false,
						is_function,
					});

					break;
				} // case elfcpp::SHN_ABS

				default:
				{
					if (sym.get_st_shndx() >= outMap.size())
						break;

					if (!outMap.at(sym.get_st_shndx()))
						break;

					auto& section = newSections.at(sym.get_st_shndx());

					std::string name = readString(nameShdr, sym.get_st_name());

					if (sym.get_st_type() == elfcpp::STT_NOTYPE && sym.get_st_bind() == elfcpp::STB_LOCAL)
					{
						std::string subString = name.substr(0, 3);

						if ((name == "$t") || (subString == "$t."))
						{
							// section.set_mapping(sym.get_st_value(), section_data::mapping::Thumb);
							break;
						}

						if ((name == "$a") || (subString == "$a."))
						{
							// section.set_mapping(sym.get_st_value(), section_data::mapping::ARM);
							break;
						}

						if ((name == "$d") || (subString == "$d."))
						{
							// section.set_mapping(sym.get_st_value(), section_data::mapping::Data);
							break;
						}
					}

					if (sym.get_st_bind() == elfcpp::STB_LOCAL)
						name = getLocalSymbolName(si, i);
					else
						name = getGlobalSymbolName(name.c_str());

					bool is_local = sym.get_st_bind() == elfcpp::STB_LOCAL;
					bool is_function = sym.get_st_type() == elfcpp::STT_FUNC;

					section.symbols().push_back(section_data::symbol {
						name,
						sym.get_st_value(),
						is_local,
						is_function,
					});

					break;
				} // default

				} // switch (sym.get_st_shndx())
			}

			break;
		} // case elfcpp::SHT_SYMTAB

		case elfcpp::SHT_REL:
		{
			if (!outMap.at(header.get_sh_info()))
				break;

			const unsigned count = header.get_sh_size() / header.get_sh_entsize();

			const elfcpp::Shdr<32, false> symShdr(&file, elfFile.section_header(header.get_sh_link()));
			const elfcpp::Shdr<32, false> symNameShdr(&file, elfFile.section_header(symShdr.get_sh_link()));

			auto& section = newSections.at(header.get_sh_info());

			for (unsigned i = 0; i < count; ++i)
			{
				const elfcpp::Rel<32, false> rel(&file, data_file::Location(
					header.get_sh_offset() + i * header.get_sh_entsize(),
					header.get_sh_entsize()
				));

				const elfcpp::Sym<32, false> sym(&file, data_file::Location(
					symShdr.get_sh_offset() + elfcpp::elf_r_sym<32>(rel.get_r_info()) * symShdr.get_sh_entsize(),
					symShdr.get_sh_entsize()
				));

				const std::string name = (sym.get_st_bind() == elfcpp::STB_LOCAL)
					? getLocalSymbolName(header.get_sh_link(), elfcpp::elf_r_sym<32>(rel.get_r_info()))
					: getGlobalSymbolName(readString(symNameShdr, sym.get_st_name()).c_str());

				section.relocations().push_back(section_data::relocation {
					name,
					0,
					elfcpp::elf_r_type<32>(rel.get_r_info()),
					rel.get_r_offset()
				});
			}

			break;
		} // case elfcpp::SHT_REL

		case elfcpp::SHT_RELA:
		{
			if (!outMap.at(header.get_sh_info()))
				break;

			const unsigned count = header.get_sh_size() / header.get_sh_entsize();

			const elfcpp::Shdr<32, false> symShdr(&file, elfFile.section_header(header.get_sh_link()));
			const elfcpp::Shdr<32, false> symNameShdr(&file, elfFile.section_header(symShdr.get_sh_link()));

			auto& section = newSections.at(header.get_sh_info());

			for (unsigned i = 0; i < count; ++i)
			{
				const elfcpp::Rela<32, false> rela(&file, data_file::Location(
					header.get_sh_offset() + i * header.get_sh_entsize(),
					header.get_sh_entsize()
				));

				const elfcpp::Sym<32, false> sym(&file, data_file::Location(
					symShdr.get_sh_offset() + elfcpp::elf_r_sym<32>(rela.get_r_info()) * symShdr.get_sh_entsize(),
					symShdr.get_sh_entsize()
				));

				const std::string name = (sym.get_st_bind() == elfcpp::STB_LOCAL)
					? getLocalSymbolName(header.get_sh_link(), elfcpp::elf_r_sym<32>(rela.get_r_info()))
					: getGlobalSymbolName(readString(symNameShdr, sym.get_st_name()).c_str());

				section.relocations().push_back(section_data::relocation {
					name,
					rela.get_r_addend(),
					elfcpp::elf_r_type<32>(rela.get_r_info()),
					rela.get_r_offset()
				});
			}

			break;
		} // case elfcpp::SHT_RELA

		} // switch (header.get_sh_type())
	}

	// Remove empty sections

	newSections.erase(std::remove_if(newSections.begin(), newSections.end(),
		[] (const section_data& section)
		{
			return (section.size()==0);
		}
	), newSections.end());

	// Add to existing section list

	mSections.reserve(mSections.size() + newSections.size());

	std::copy(
		std::make_move_iterator(newSections.begin()),
		std::make_move_iterator(newSections.end()),
		std::back_inserter(mSections)
	);
}

void event_object::try_transform_relatives() {
	for (auto& section : mSections) {
		for (auto& relocation : section.relocations()) {
			if (auto relocatelet = mRelocator.get_relocatelet(relocation.type)) {
				if (!relocatelet->is_absolute() && relocatelet->can_make_trampoline()) {
					std::string renamed;

					renamed.reserve(4 + relocation.symbolName.size());

					renamed.append("_LP_"); // local proxy
					renamed.append(relocation.symbolName);

					bool exists = false;

					for (auto& section : mSections)
						for (auto& sym : section.symbols())
							if (sym.name == renamed)
								exists = true;

					if (!exists) {
						section_data newData = relocatelet->make_trampoline(relocation.symbolName, relocation.addend);
						newData.symbols().push_back({ renamed, (newData.mapping_type_at(0) == section_data::mapping::Thumb), true });

						mSections.push_back(std::move(newData));
					}

					relocation.symbolName = renamed;
					relocation.addend = 0; // TODO: -4
				}
			}
		}
	}
}

void event_object::try_relocate_relatives() {
	unsigned offset = 0;

	for (auto& section : mSections) {
		section.relocations().erase(
			std::remove_if(
				section.relocations().begin(),
				section.relocations().end(),
				[this, &section, offset] (section_data::relocation& relocation) -> bool {
					unsigned symOffset = 0;

					auto relocatelet = mRelocator.get_relocatelet(relocation.type);

					for (auto& symSection : mSections) {
						for (auto& symbol : symSection.symbols()) {
							if (symbol.name != relocation.symbolName)
								continue;

							if (relocatelet && !relocatelet->is_absolute()) {
								relocatelet->apply_relocation(
									section,
									relocation.offset,
									symOffset + symbol.offset,
									relocation.addend
								);

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
				}
			),
			section.relocations().end()
		);

		offset += section.size();

		if (unsigned misalign = (offset % 4))
			offset += (4 - misalign);
	}
}

void event_object::try_relocate_absolutes() {
	auto absolute_ids = make_absolute_symbol_map();

	for (auto& section : mSections) {
		section.relocations().erase(
			std::remove_if(
				section.relocations().begin(),
				section.relocations().end(),
				[this, &section, &absolute_ids] (const section_data::relocation& relocation) -> bool {
					auto it = absolute_ids.find(relocation.symbolName);

					if (it != absolute_ids.end()) {
						auto & symbol = mAbsoluteSymbols[it->second];

						if (auto relocatelet = mRelocator.get_relocatelet(relocation.type)) {
							if (relocatelet->is_absolute()) {
								relocatelet->apply_relocation(
									section,
									relocation.offset,
									symbol.offset,
									relocation.addend
								);

								return true;
							}
						}

						return false;
					}

					return false;
				}
			),
			section.relocations().end()
		);
	}
}

void event_object::remove_unnecessary_symbols() {
	for (auto& section : mSections) {
		section.symbols().erase(
			std::remove_if(
				section.symbols().begin(),
				section.symbols().end(),
				[this] (const section_data::symbol& symbol) -> bool {
					if (!symbol.is_local)
						return false; // symbol may be used outside of the scope of this object

					for (auto& section : mSections)
						for (auto& reloc : section.relocations())
							if (reloc.symbolName == symbol.name)
								return false; // a relocation is dependant on this symbol

					return true; // symbol is local and unused, we can remove it safely (hopefully)
				}
			),
			section.symbols().end()
		);
	}
}

void event_object::cleanup() {
	for (auto& section : mSections) {
		std::sort(
			section.symbols().begin(),
			section.symbols().end(),
			[] (const section_data::symbol& a, const section_data::symbol& b) -> bool {
				return a.offset < b.offset;
			}
		);
	}
}

std::vector<event_object::hook> event_object::get_hooks() const {
	std::vector<hook> result;

	std::unordered_set<std::string_view> symbol_names;

	for (auto& section : mSections) {
		for (auto& locSymbol : section.symbols()) {
			if (!locSymbol.is_local && !locSymbol.name.empty()) {
				std::string_view name(locSymbol.name);
				symbol_names.insert(name);
			}
		}
	}

	for (auto& absSymbol : mAbsoluteSymbols) {
		if (symbol_names.contains(std::string_view(absSymbol.name))) {
			if (absSymbol.offset < 0x08000000 || absSymbol.offset >= 0x0A000000) {
				std::string message(std::format("attempting to replace `{0}`, which is not in ROM (reference address: 0x{1:08X})",
					absSymbol.name, absSymbol.offset));

				throw std::runtime_error(message);
			}

			if (!absSymbol.is_function) {
				std::string message(std::format("attempting to replace `{0}`, which is not a function",
					absSymbol.name));

				throw std::runtime_error(message);
			}

			result.push_back({ (absSymbol.offset - 0x08000000), absSymbol.name });
		}
	}

	return result;
}

void event_object::write_events(std::ostream& output) const {
	unsigned offset = 0;

	/* we do this here but this should really be something that have already */
	auto abs_symbol_map = make_absolute_symbol_map();

	for (auto& section : mSections) {
		output << "ALIGN 4" << std::endl;

		if (std::any_of(
			section.symbols().begin(),
			section.symbols().end(),

			[] (const section_data::symbol& sym) {
				return !sym.is_local;
			}
		)) {
			output << "PUSH" << std::endl;
			int currentOffset = 0;

			for (auto& symbol : section.symbols()) {
				if (symbol.is_local)
					continue;

				output << "ORG CURRENTOFFSET+$" << std::hex << (symbol.offset - currentOffset) << ";"
					   << symbol.name << ":" << std::endl;
				currentOffset = symbol.offset;
			}

			output << "POP" << std::endl;
		}

		// TODO: this should never be true?
		if (std::any_of(
			section.symbols().begin(),
			section.symbols().end(),

			[] (const section_data::symbol& sym) {
				return sym.is_local;
			}
		)) {
			output << "{" << std::endl;
			output << "PUSH" << std::endl;

			int currentOffset = 0;

			for (auto& symbol : section.symbols()) {
				if (!symbol.is_local)
					continue;

				output << "ORG CURRENTOFFSET+$" << std::hex << (symbol.offset - currentOffset) << ";"
					   << symbol.name << ":" << std::endl;
				currentOffset = symbol.offset;
			}

			output << "POP" << std::endl;

			write_section_data_event(output, section, abs_symbol_map);

			output << "}" << std::endl;
		} else {
			write_section_data_event(output, section, abs_symbol_map);
		}

		offset += section.size();

		if (unsigned misalign = (offset % 4))
			offset += (4 - misalign);
	}
}

void event_object::write_section_data_event(
	std::ostream& output,
	const section_data& section,
	const std::unordered_map<std::string_view, size_t>& abs_symbol_map) const
{
	constexpr size_t ALIGNMENT_MASK = 0b111; // 4, 2, 1

	size_t prev_tail_offset = 0;

	for (auto& relocation : section.relocations())
	{
		// write any bytes we skipped over

		if (prev_tail_offset != relocation.offset)
		{
			assert(prev_tail_offset < relocation.offset);

			int alignment = prev_tail_offset & ALIGNMENT_MASK;

			std::span<const unsigned char> bytes(
				section.data() + prev_tail_offset,
				relocation.offset - prev_tail_offset);

			write_event_bytes(output, alignment, bytes);
		}

		// translate relocation into event

		if (auto relocatelet = mRelocator.get_relocatelet(relocation.type))
		{
			// This is probably the worst hack I've ever written
			// lyn needs a rewrite

			// (I makes sure that relative relocations to known absolute values will reference the value and not the name)

			auto it = abs_symbol_map.find(relocation.symbolName);

			auto symName = (it == abs_symbol_map.end())
				? relocation.symbolName
				: std::format("${0:X}", mAbsoluteSymbols[it->second].offset);

			event_code code(relocatelet->make_event_code(
				section,
				relocation.offset,
				symName,
				relocation.addend));

			int alignment = relocation.offset & ALIGNMENT_MASK;

			if ((alignment % code.code_align()) == 0)
				code.write_to_stream(output);
			else
				code.write_to_stream_misaligned(output);

			output << std::endl;

			prev_tail_offset = relocation.offset + code.code_size();
		}
		else if (relocation.type != elfcpp::R_ARM_V4BX) // another dirty hack
			throw std::runtime_error(std::format("unhandled relocation type #{0}", relocation.type));
	}

	// write any bytes left over

	if (prev_tail_offset != section.size())
	{
		assert(prev_tail_offset < section.size());

		int alignment = prev_tail_offset & ALIGNMENT_MASK;

		std::span<const unsigned char> bytes(
			section.data() + prev_tail_offset,
			section.size() - prev_tail_offset);

		write_event_bytes(output, alignment, bytes);
	}
}

std::unordered_map<std::string_view, size_t> event_object::make_absolute_symbol_map() const {
	std::unordered_map<std::string_view, size_t> result;

	for (size_t i = 0; i < mAbsoluteSymbols.size(); i++) {
		result.insert({ mAbsoluteSymbols[i].name, i });
	}

	return result;
}

} // namespace lyn
