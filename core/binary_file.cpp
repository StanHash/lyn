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
	mData.reserve(size);

	std::copy(
		std::next(other.mData.begin(), start),
		std::next(other.mData.begin(), start+size),
		std::back_inserter(mData)
	);
}

void binary_file::ensure_aligned(int align) {
	if (int off = (size() % align))
		resize(size() + (align - off));
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

void binary_file::combine_with(const binary_file& other) {
	// move combination here wouldn't be that useful, since we're dealing with standard raw data
	// there's no allocation overhead in copying over moving

	data().reserve(size() + other.size());

	std::copy(
		other.data().begin(),
		other.data().end(),
		std::back_inserter(data())
	);
}

binary_file::byte_t binary_file::read_byte(int& pos) const {
	if (pos >= mData.size())
		throw std::out_of_range("tried to read byte out of range of data");

	return mData[pos++];
}

void binary_file::write_byte(int pos, byte_t value) {
	if (pos >= mData.size())
		throw std::out_of_range("tried to write byte out of range of data");

	mData[pos] = value;
}

} // namespace lyn
