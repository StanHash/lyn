#include "event_section.h"

#include <cassert>
#include <format>
#include <cstdint>

namespace lyn {

template<typename IntType, unsigned int ByteCount = sizeof(IntType)>
static IntType read_le(std::span<const unsigned char> bytes) {
	IntType result { 0 };

	assert(bytes.size() >= ByteCount);

	for (unsigned int i = 0; i < ByteCount; ++i) {
		result |= static_cast<IntType>(bytes[i]) << (i * 8);
	}

	return result;
}

void write_event_bytes(std::ostream& output, int alignment, std::span<const unsigned char> bytes) {
	/* This could probably be made to produce denser results in weird cases
	 * for example, 2 -> 6 ranges would print SHORT a; SHORT b, where it can be SHORT a b
	 * but like who cares */

	// using this to allow format_to magic
	std::ostreambuf_iterator<char> output_it(output);

	size_t offset = 0;

	while (offset < bytes.size()) {
		size_t length_left = bytes.size() - offset;

		if ((length_left >= 4) && ((alignment % 4) == 0)) {
			std::format_to(output_it, "WORD");

			while (length_left >= 4) {
				uint32_t value { read_le<uint32_t>(bytes.subspan(offset, 4)) };
				std::format_to(output_it, " ${0:X}", value);

				offset += 4;
				length_left -= 4;
			}

			*output_it++ = '\n';
		} else if ((length_left >= 2) && ((alignment % 2) == 0)) {
			uint32_t value { read_le<uint16_t>(bytes.subspan(offset, 2)) };
			std::format_to(output_it, "SHORT ${0:X}\n", value);

			alignment += 2;
			offset += 2;
		} else {
			uint32_t value { bytes[offset] };
			std::format_to(output_it, "BYTE ${0:X}\n", value);

			alignment++;
			offset++;
		}
	}
}

} // namespace lyn
