#ifndef EVENT_OBJECT_H
#define EVENT_OBJECT_H

#include "elf/elf_file.h"
#include "arm_relocator.h"
#include "section_data.h"

namespace lyn {

class event_object : public section_data {
public:
	void append_from_elf(const lyn::elf_file& elfFile);

	void make_trampolines();

	void link_locals();
	void link_absolutes();

	void write_events(std::ostream& output) const;

	bool try_relocate(const section_data::relocation& relocation);

private:
	arm_relocator mRelocator;

	std::vector<section_data::symbol> mAbsoluteSymbols;
};

} // namespace lyn

#endif // EVENT_OBJECT_H
