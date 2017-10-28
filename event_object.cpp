#include "event_object.h"

#include <iomanip>
#include <algorithm>

#include "core/event_section.h"

namespace lyn {

void event_object::load_from_elf(const elf_file& elfFile) {
	mSectionDatas = section_data::make_from_elf(elfFile);
}

void event_object::write_events(std::ostream& output) const {
	for (auto& sectionData : mSectionDatas)
		sectionData.write_events(output);
}

} // namespace lyn
