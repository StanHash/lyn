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

void binary_file::load_from_other(const binary_file &other, unsigned int start, int size) {
	if (size < 0)
		size = other.size() - start;

	if (start > other.size())
		return; // TODO: throw exception

	if (start+size > other.size())
		return; // TODO: throw exception

	clear();
	reserve(size);

	std::copy(
		std::next(other.begin(), start),
		std::next(other.begin(), start+size),
		std::back_inserter(*this)
	);
}

void binary_file::ensure_aligned(int align) {
	if (unsigned off = (size() % align))
		resize(size() + (align - off));
}

} // namespace lyn
