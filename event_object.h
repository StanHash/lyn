#ifndef EVENT_OBJECT_H
#define EVENT_OBJECT_H

#include "elf/elf_file.h"

namespace lyn {

class event_object {
protected:
	struct mapping {
		enum {
			Data,
			Thumb,
			ARM
		} type;
		unsigned int offset;
	};

	struct label_data {
		std::string name;
		unsigned int offset;
	};

	struct relocation {
		std::string symbol;
		int addend;

		int type;
		unsigned int offset;
	};

	struct section_data {
		enum {
			None,
			ROMData
		} outType = None;

		std::string name;

		std::vector<mapping> mappings;
		std::vector<label_data> labels;
		std::vector<relocation> relocations;

		std::vector<unsigned char> data;
	};

public:
	void load_from_elf(const lyn::elf_file& elfFile);
	void write_events(std::ostream& output) const;

private:
	std::vector<section_data> mSectionDatas;
};

} // namespace lyn

#endif // EVENT_OBJECT_H
