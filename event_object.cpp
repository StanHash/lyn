#include "event_object.h"

#include <iomanip>
#include <algorithm>

#include "core/event_section.h"

namespace lyn {

void event_object::load_from_elf(const elf_file& elfFile) {
	auto newList = section_data::make_from_elf(elfFile);

	std::copy(
		std::make_move_iterator(newList.begin()),
		std::make_move_iterator(newList.end()),
		std::back_inserter(mSectionDatas)
	);
}

void event_object::write_events(std::ostream& output) const {
	for (auto& sectionData : mSectionDatas) {
		event_section events = sectionData.make_events();

		for (auto& relocation : sectionData.relocations())
			events.set_code(relocation.offset, mRelocator.make_relocation_code(relocation));

		events.compress_codes();
		events.optimize();

		// output << "// section " << sectionData.name() << std::endl << std::endl;

		if (!sectionData.symbols().empty()) {
			output << "PUSH" << std::endl;
			int currentOffset = 0;

			for (auto& symbol : sectionData.symbols()) {
				output << "ORG (CURRENTOFFSET + 0x" << std::hex << (symbol.offset - currentOffset) << "); "
					   << symbol.name << ":" << std::endl;
				currentOffset = symbol.offset;
			}

			output << "POP" << std::endl << std::endl;
		}

		events.write_to_stream(output);
	}
}

} // namespace lyn
