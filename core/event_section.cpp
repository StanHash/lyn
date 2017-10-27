#include "event_section.h"

#include <stdexcept>

namespace lyn {

event_section::event_section() {}

void event_section::write_to_stream(std::ostream& output) const {
	for (int pos = 0; pos < mCodeMap.size();) {
		const event_code& code = mCodes[mCodeMap[pos]];

		output << code << std::endl;
		pos += code.code_size();
	}

	output << std::endl;
}

void event_section::resize(unsigned int size) {
	mCodeMap.resize(size);
}

void event_section::set_code(unsigned int offset, event_code&& code) {
	int size = code.code_size();

	if ((offset % code.code_align()) != 0)
		throw std::runtime_error("EA ERROR: tried to add misaligned code!");

	if (offset + size > mCodeMap.size())
		mCodeMap.resize(offset + size);

	int index = mCodes.size();
	mCodes.push_back(std::move(code));

	for (int i=0; i<size; ++i)
		mCodeMap[offset + i] = index;
}

void event_section::compressCodes() {
	// TODO: event_section::compressCodes
}

} // namespace lyn
