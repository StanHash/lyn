#include "section_data.h"

#include "../ea/event_section.h"
#include "util/hex_write.h"

#include <algorithm>

namespace lyn {

event_section section_data::make_events() const {
	event_section result;
	result.resize(size());

	/*
	auto mappingIt = mMappings.begin();
	int currentMappingType = mapping::Data;
	// */

	unsigned pos = 0;

	while (pos < size()) {
		unsigned left = size() - pos;

		if (left >= 4) {
			result.set_code(pos, lyn::event_code(
				lyn::event_code::CODE_WORD,
				util::make_hex_string("$", at<std::uint32_t>(pos))
			));

			pos += 4;
		} else if (left >= 2) {
			result.set_code(pos, lyn::event_code(
				lyn::event_code::CODE_SHORT,
				util::make_hex_string("$", at<std::uint16_t>(pos))
			));

			pos += 2;
		} else {
			result.set_code(pos, lyn::event_code(
				lyn::event_code::CODE_BYTE,
				util::make_hex_string("$", byte_at(pos))
			));

			pos++;
		}

		/*

		if (mappingIt != mMappings.end() && (mappingIt->offset <= pos))
			currentMappingType = (mappingIt++)->type;


		switch (currentMappingType) {
		case mapping::Data:
			result.set_code(pos, lyn::event_code(
				lyn::event_code::CODE_BYTE,
				util::make_hex_string("$", byte_at(pos))
			));
			pos++;
			break;

		case mapping::Thumb:
			result.set_code(pos, lyn::event_code(
				lyn::event_code::CODE_SHORT,
				util::make_hex_string("$", at<std::uint16_t>(pos))
			));
			pos += 2;
			break;

		case mapping::ARM:
			result.set_code(pos, lyn::event_code(
				lyn::event_code::CODE_WORD,
				util::make_hex_string("$", at<std::uint32_t>(pos))
			));
			pos += 4;
			break;
		}

		// */
	}

	return result;
}

int section_data::mapping_type_at(unsigned int offset) const {
	for (auto mapping : mMappings)
		if (mapping.offset <= offset)
			return mapping.type;

	return mapping::Data;
}

void section_data::set_mapping(unsigned int offset, mapping::type_enum type) {
	for (auto& mapping : mMappings)
		if (mapping.offset == offset) {
			mapping.type = type;
			return;
		}
	mMappings.push_back({ type, offset });

	std::sort(mMappings.begin(), mMappings.end(), [] (const mapping& left, const mapping& right) {
		return left.offset < right.offset;
	});
}

void section_data::combine_with(const section_data& other) {
	mMappings.reserve(mMappings.size() + other.mMappings.size());
	mSymbols.reserve(mSymbols.size() + other.mSymbols.size());
	mRelocations.reserve(mRelocations.size() + other.mRelocations.size());

	for (auto mapping : other.mMappings) {
		mapping.offset += size();
		mMappings.push_back(std::move(mapping));
	}

	for (auto symbol : other.mSymbols) {
		symbol.offset += size();
		mSymbols.push_back(symbol);
	}

	for (auto relocation : other.mRelocations) {
		relocation.offset += size();
		mRelocations.push_back(relocation);
	}

	binary_file::combine_with(other);
}

void section_data::combine_with(section_data&& other) {
	mMappings.reserve(mMappings.size() + other.mMappings.size());
	mSymbols.reserve(mSymbols.size() + other.mSymbols.size());
	mRelocations.reserve(mRelocations.size() + other.mRelocations.size());

	for (auto& mapping : other.mMappings) {
		mapping.offset += size();
		mMappings.push_back(std::move(mapping));
	}

	for (auto& symbol : other.mSymbols) {
		symbol.offset += size();
		mSymbols.push_back(std::move(symbol));
	}

	for (auto& relocation : other.mRelocations) {
		relocation.offset += size();
		mRelocations.push_back(std::move(relocation));
	}

	binary_file::combine_with(other);
}

} // namespace lyn
