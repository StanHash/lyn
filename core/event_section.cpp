#include "event_section.h"

#include <stdexcept>

namespace lyn {

event_section::event_section() {}

void event_section::write_to_stream(std::ostream& output) const {
	for (int pos = 0; pos < mCodeMap.size();) {
		const event_code& code = mCodes[mCodeMap[pos]];

		output << code << std::endl; // TODO: add support for non-endl code separator (';' specifically)
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

void event_section::compress_codes() {
	int currentOffset = 0;

	while (currentOffset < mCodeMap.size()) {
		int index = mCodeMap[currentOffset];
		auto& code = mCodes[index];

		if (currentOffset + code.code_size() >= mCodeMap.size())
			break;

		auto& nextCode = mCodes[mCodeMap[currentOffset + code.code_size()]];

		if (code.can_combine_with(nextCode)) {
			code.combine_with(nextCode);

			for (int i=0; i<code.code_size(); ++i)
				mCodeMap[currentOffset + i] = index;
		} else {
			currentOffset += code.code_size();
		}
	}
}

void event_section::optimize() {
	event_section other;

	other.resize(mCodeMap.size());

	int currentOffset = 0;

	while (currentOffset < mCodeMap.size()) {
		int size = mCodes[mCodeMap[currentOffset]].code_size();
		other.set_code(currentOffset, std::move(mCodes[mCodeMap[currentOffset]]));
		currentOffset += size;
	}

	mCodeMap = std::move(other.mCodeMap);
	mCodes   = std::move(other.mCodes);
}

} // namespace lyn
