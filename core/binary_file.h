#ifndef BINARY_FILE_H
#define BINARY_FILE_H

#include <iostream>
#include <cstdarg>

#include "data_chunk.h"

namespace lyn {

struct binary_file : public data_chunk {
	struct Location {
		Location(size_type off, size_type len)
			: file_offset(off), data_size(len) {}

		size_type file_offset, data_size;
	};

	struct View : public Location {
		View(const binary_file* file, size_type off, size_type len)
			: Location(off, len), pFile(file) {}

		const value_type* data() const {
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

	void load_from_stream(std::istream& input);

	void error(const char* format, ...) const;

	void ensure_aligned(unsigned align);
};

} // namespace lyn

#endif // BINARY_FILE_H
