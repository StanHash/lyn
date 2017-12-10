#include "util.h"

namespace stan {

static std::string::value_type to_hex_digit(std::uint32_t value) {
	value = (value & 0xF);

	if (value < 10)
		return '0' + value;
	return 'A' + (value - 10);
}

std::string to_hex_digits(std::uint32_t value, int digits) {
	std::string result;
	result.resize(digits);

	for (int i=0; i<digits; ++i)
		result[digits-i-1] = to_hex_digit(value >> (4*i));

	return result;
}

} // namespace stan
