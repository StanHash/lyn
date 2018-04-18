#ifndef BINARY_FILE_H
#define BINARY_FILE_H

#include <iostream>
#include <vector>
#include <cstdint>
#include <cstdarg>

namespace lyn {

struct binary_file {
	using byte_t = unsigned char;
	using size_t = std::size_t;

	struct Location {
		Location(size_t off, size_t len)
			: file_offset(off), data_size(len) {}

		size_t file_offset, data_size;
	};

	struct View : public Location {
		View(const binary_file* file, size_t off, size_t len)
			: Location(off, len), pFile(file) {}

		const byte_t* data() const {
			return pFile->data().data() + file_offset;
		}

	private:
		const binary_file* pFile;
	};

	View view(size_t offset, size_t size) const {
		return View(this, offset, size);
	}

	View view(const Location& location) const {
		return View(this, location.file_offset, location.data_size);
	}

	void error(const char* format, ...) {
		std::va_list args;

		va_start(args, format);

		int strSize; {
			std::va_list argsCpy;
			va_copy(argsCpy, args);

			strSize = std::vsnprintf(nullptr, 0, format, argsCpy);
			va_end(argsCpy);
		}

		std::vector<char> buf(strSize + 1);
		std::vsnprintf(buf.data(), buf.size(), format, args);

		va_end(args);

		throw std::runtime_error(std::string(buf.begin(), buf.end())); // TODO: better error
	}

	void load_from_stream(std::istream& input);
	void load_from_other(const binary_file& other, unsigned int start = 0, int size = -1);

	void resize(unsigned int newSize) { mData.resize(newSize); }
	size_t size() const { return mData.size(); }

	void ensure_aligned(int align);

	bool is_cstr_at(int pos) const;
	const char* cstr_at(int pos) const;

	std::vector<byte_t>&       data()       { return mData; }
	const std::vector<byte_t>& data() const { return mData; }

	void combine_with(const binary_file& other);

	template<typename T, int byte_count = sizeof(T)>
	T read(int& pos) const;

	template<typename T, int byte_count = sizeof(T)>
	T at(int pos) const { return read<T, byte_count>(pos); }

	byte_t read_byte(int& pos) const;
	byte_t byte_at(int pos) const { return read_byte(pos); }

	template<typename T, int byte_count = sizeof(T)>
	void write(int pos, T value);

	void write_byte(int pos, byte_t value);

private:
	std::vector<byte_t> mData;
};

template<typename T, int byte_count>
T binary_file::read(int& pos) const {
	T result = 0;

	for (int i=0; i<byte_count; ++i)
		result |= (read_byte(pos) << (i*8));

	return result;
}

template<typename T, int byte_count>
void binary_file::write(int pos, T value) {
	for (int i=0; i<byte_count; ++i)
		write_byte(pos + i, (value >> (i*8)) & 0xFF);
}

} // namespace lyn

#endif // BINARY_FILE_H
