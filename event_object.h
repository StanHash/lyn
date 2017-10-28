#ifndef EVENT_OBJECT_H
#define EVENT_OBJECT_H

#include "elf/elf_file.h"
#include "arm_relocator.h"
#include "section_data.h"

namespace lyn {

class event_object {
public:
	void load_from_elf(const lyn::elf_file& elfFile);
	void write_events(std::ostream& output) const;

protected:
	static std::string make_relocation_string(const std::string& symbol, int addend);

private:
	arm_relocator mRelocator;
	std::vector<section_data> mSectionDatas;
};

} // namespace lyn

#endif // EVENT_OBJECT_H
