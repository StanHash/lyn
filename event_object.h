#ifndef EVENT_OBJECT_H
#define EVENT_OBJECT_H

#include "elf/elf_file.h"
#include "arm_relocator.h"
#include "section_data.h"

namespace lyn {

class event_object : public section_data {
public:
	struct hook {
		unsigned int originalOffset;
		std::string name;
	};

public:
	void append_from_elf(const lyn::elf_file& elfFile);

	void make_trampolines();

	void link_locals();
	void link_absolutes();

	std::vector<hook> get_hooks() const;

	void write_events(std::ostream& output) const;

private:
	arm_relocator mRelocator;

	std::vector<section_data::symbol> mAbsoluteSymbols;
};

} // namespace lyn

#endif // EVENT_OBJECT_H
