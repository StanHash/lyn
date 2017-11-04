#include "section_data.h"

#include "core/event_section.h"

#include <algorithm>

namespace lyn {

std::string::value_type toHexDigit(std::uint32_t value) {
	value = (value & 0xF);

	if (value < 10)
		return '0' + value;
	return 'A' + (value - 10);
}

std::string toHexDigits(std::uint32_t value, int digits) {
	std::string result;
	result.resize(digits);

	for (int i=0; i<digits; ++i)
		result[digits-i-1] = toHexDigit(value >> (4*i));

	return result;
}

event_section section_data::make_events() const {
	event_section result;
	result.resize(size());

	auto mappingIt = mMappings.begin();
	int currentMappingType = mapping::Data;

	int pos = 0;

	while (pos < size()) {
		if (mappingIt != mMappings.end() && (mappingIt->offset <= pos))
			currentMappingType = (mappingIt++)->type;

		switch (currentMappingType) {
		case mapping::Data:
			result.set_code(pos, lyn::event_code(
				lyn::event_code::CODE_BYTE,
				std::string("$").append(toHexDigits(byte_at(pos), 2))
			));
			pos++;
			break;

		case mapping::Thumb:
			result.set_code(pos, lyn::event_code(
				lyn::event_code::CODE_SHORT,
				std::string("$").append(toHexDigits(at<std::uint16_t>(pos), 4))
			));
			pos += 2;
			break;

		case mapping::ARM:
			result.set_code(pos, lyn::event_code(
				lyn::event_code::CODE_WORD,
				std::string("$").append(toHexDigits(at<std::uint32_t>(pos), 8))
			));
			pos += 4;
			break;
		}
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

	data().reserve(size() + other.size());

	std::copy(
		other.data().begin(),
		other.data().end(),
		std::back_inserter(data())
	);
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

	data().reserve(size() + other.size());

	std::copy(
		other.data().begin(),
		other.data().end(),
		std::back_inserter(data())
	);
}

} // namespace lyn
