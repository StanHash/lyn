#include "binary_file.h"

namespace lyn {

void binary_file::load_from_stream(std::istream& input) {
	input.seekg(0, std::ios::end);
	mData.resize(input.tellg());
	input.seekg(0);

	for (int i=0; i<mData.size(); ++i)
		mData[i] = input.get();
}

binary_file::byte_t binary_file::get_byte(int& pos) const {
	if (pos >= mData.size())
		throw std::out_of_range("aa"); // TODO

	return mData[pos++];
}

} // namespace lyn
