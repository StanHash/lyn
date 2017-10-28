#include "binary_file.h"

#include <iterator>

namespace lyn {

void binary_file::load_from_stream(std::istream& input) {
	input.seekg(0, std::ios::end);
	mData.resize(input.tellg());
	input.seekg(0);

	for (int i=0; i<mData.size(); ++i)
		mData[i] = input.get();
}

void binary_file::load_from_other(const binary_file &other, unsigned int start, int size) {
	if (size < 0)
		size = other.size() - start;

	if (start > other.size())
		return; // TODO: throw exception

	if (start+size > other.size())
		return; // TODO: throw exception

	mData.clear();
	mData.reserve(size - start);

	std::copy(
		std::next(other.mData.begin(), start),
		std::next(other.mData.begin(), start+size),
		std::back_inserter(mData)
	);
}

bool binary_file::is_cstr_at(int pos) const {
	do {
		if (mData[pos] == 0)
			return true;
	} while ((++pos) < size());

	return false;
}

const char* binary_file::cstr_at(int pos) const {
	return reinterpret_cast<const char*>(&mData[pos]);
}

binary_file::byte_t binary_file::read_byte(int& pos) const {
	if (pos >= mData.size())
		throw std::out_of_range("aa"); // TODO

	return mData[pos++];
}

} // namespace lyn
