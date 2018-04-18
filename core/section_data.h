#ifndef SECTION_DATA_H
#define SECTION_DATA_H

#include "binary_file.h"

#include "ea/event_section.h"

namespace lyn {

class section_data : public binary_file {
public:
	struct mapping {
		enum type_enum {
			Data,
			Thumb,
			ARM
		};

		type_enum type;
		unsigned int offset;
	};

	struct symbol {
		std::string name;
		unsigned int offset;
		bool isLocal;
	};

	struct relocation {
		std::string symbolName;
		int addend;

		unsigned type;
		unsigned offset;
	};

	enum output_type {
		NoOut,
		OutROM,
	};

public:
	void set_name(const std::string& name) { mName = name; }
	const std::string& name() const { return mName; }

	const std::vector<relocation>& relocations() const { return mRelocations; }
	std::vector<relocation>& relocations() { return mRelocations; }

	const std::vector<symbol>& symbols() const { return mSymbols; }
	std::vector<symbol>& symbols() { return mSymbols; }

	event_section make_events() const;

	int mapping_type_at(unsigned int offset) const;
	void set_mapping(unsigned int offset, mapping::type_enum type);

	void combine_with(const section_data& other);
	void combine_with(section_data&& other);

private:
	std::string mName;

	std::vector<relocation> mRelocations;
	std::vector<symbol> mSymbols;
	std::vector<mapping> mMappings;
};

} // namespace lyn

#endif // SECTION_DATA_H
