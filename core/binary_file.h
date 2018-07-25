#ifndef BINARY_FILE_H
#define BINARY_FILE_H

#include <iostream>
#include <cstdarg>

#include "data_chunk.h"

namespace lyn {

struct binary_file : public data_chunk {
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
			return pFile->data() + file_offset;
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

	void ensure_aligned(int align);
};

} // namespace lyn

#endif // BINARY_FILE_H
