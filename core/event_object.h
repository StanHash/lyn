#ifndef EVENT_OBJECT_H
#define EVENT_OBJECT_H

#include "arm_relocator.h"
#include "section_data.h"
#include <unordered_map>

namespace lyn {

class event_object {
public:
	struct hook {
		unsigned int originalOffset;
		std::string name;
	};

public:
	void append_from_elf(const char* fName);

	void try_transform_relatives();

	void try_relocate_relatives();
	void try_relocate_absolutes();

	void remove_unnecessary_symbols();

	void cleanup();

	std::vector<hook> get_hooks() const;

	void add_section(section_data&& section) {
		mSections.push_back(std::move(section));
	}

	void write_events(std::ostream& output) const;

	const std::vector<section_data::symbol>& absolute_symbols() const { return mAbsoluteSymbols; }

private:
	void write_section_data_event(
		std::ostream& output,
		const section_data& section,
		const std::unordered_map<std::string_view, size_t>& abs_symbol_map) const;

	std::unordered_map<std::string_view, size_t> make_absolute_symbol_map() const;

	arm_relocator mRelocator;

	std::vector<section_data> mSections;
	std::vector<section_data::symbol> mAbsoluteSymbols;
};

} // namespace lyn

#endif // EVENT_OBJECT_H
