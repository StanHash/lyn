#ifndef EVENT_SECTION_H
#define EVENT_SECTION_H

#include "event_code.h"

#include <iostream>

namespace lyn {

class event_section {
public:
	event_section();

	void write_to_stream(std::ostream& output) const;

	void resize(unsigned int size);

	void set_code(unsigned int offset, event_code&& code);

	void compressCodes();

private:
	std::vector<int> mCodeMap;
	std::vector<event_code> mCodes;
};

} // namespace lyn

#endif // EVENT_SECTION_H
