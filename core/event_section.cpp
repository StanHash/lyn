#include "event_section.h"

#include <stdexcept>

namespace lyn {

event_section::event_section() {}

void event_section::write_to_stream(std::ostream& output) const {
	event_code::code_type lastType = event_code::NONE;

	for (int pos = 0; pos < mCodeMap.size();) {
		const event_code& code = mCodes[mCodeMap[pos]];

		if ((lastType != code.type) || code.newline_policy == event_code::ForceNewline) {
			output << std::endl << code_identifier(code);
			lastType = code.type;
		}

		output << " " << code.argument;
		pos += code_size(code);
	}

	output << std::endl;
}

void event_section::set_size(unsigned int size) {
	mCodeMap.resize(size);
}

void event_section::set_code(unsigned int offset, event_code&& code) {
	int size = code_size(code);

	if ((offset % size) != 0)
		throw std::runtime_error("EA ERROR: tried to add misaligned code!");

	if (offset + size > mCodeMap.size())
		mCodeMap.resize(offset + size);

	int index = mCodes.size();
	mCodes.push_back(code);

	for (int i=0; i<size; ++i)
		mCodeMap[offset + i] = index;
}

int event_section::code_size(event_code::code_type type) {
	switch (type) {
	case event_code::BYTE:
		return 1;

	case event_code::SHORT:
		return 2;

	case event_code::WORD:
	case event_code::POIN:
		return 4;
	}

	return 0;
}

const char* event_section::code_identifier(event_code::code_type type) {
	static const char* identifiers[4] = {
		"BYTE",
		"SHORT",
		"WORD",
		"POIN"
	};

	return identifiers[(int) type];
}

} // namespace lyn
