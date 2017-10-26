#ifndef BINARY_FILE_H
#define BINARY_FILE_H

#include <iostream>
#include <vector>
#include <cstdint>

namespace lyn {

class binary_file {
public:
	using byte_t = unsigned char;
	using size_t = std::size_t;

public:
	void load_from_stream(std::istream& input);

	size_t size() const { return mData.size(); }

	std::vector<byte_t>&       data()       { return mData; }
	const std::vector<byte_t>& data() const { return mData; }

	template<typename T, int amount = sizeof(T)>
	T get(int& pos) const;

	template<typename T, int amount = sizeof(T)>
	T at(int pos) const { return get<T, amount>(pos); }

	byte_t get_byte(int& pos) const;
	byte_t byte_at(int pos) const { return get_byte(pos); }

private:
	std::vector<byte_t> mData;
};

template<typename T, int byte_count = sizeof(T)>
T binary_file::get(int& pos) const {
	T result = 0;

	for (int i=0; i<byte_count; ++i)
		result |= (get_byte(pos) << (i*8));

	return result;
}

} // namespace lyn

#endif // BINARY_FILE_H
