#include "binary_file.h"

#include <iterator>

namespace lyn {

void binary_file::load_from_stream(std::istream& input) {
	input.seekg(0, std::ios::end);
	resize(input.tellg());
	input.seekg(0);

	for (auto it = begin(); it != end(); ++it)
		*it = input.get();
}

void binary_file::error(const char* format, ...) const {
	std::va_list args;

	va_start(args, format);

	unsigned stringSize; {
		std::va_list argsCpy;
		va_copy(argsCpy, args);

		stringSize = std::vsnprintf(nullptr, 0, format, argsCpy);
		va_end(argsCpy);
	}

	std::vector<char> buf(stringSize + 1);
	std::vsnprintf(buf.data(), buf.size(), format, args);

	va_end(args);

	throw std::runtime_error(std::string(buf.begin(), buf.end())); // TODO: better error
}

void binary_file::ensure_aligned(unsigned align) {
	if (unsigned off = (size() % align))
		resize(size() + (align - off));
}

} // namespace lyn
