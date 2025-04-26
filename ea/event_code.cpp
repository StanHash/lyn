#include "event_code.h"

namespace lyn {

std::vector<event_code::event_code_type> event_code::msCodeTypeLibrary = {
//  { name,    name,     size, align },
	{ "BYTE",  "BYTE",   1,    1     }, // CODE_BYTE  = 0
	{ "SHORT", "SHORT2", 2,    2     }, // CODE_SHORT = 1
	{ "WORD",  "WORD2",  4,    4     }, // CODE_WORD  = 2
	{ "POIN",  "POIN2",  4,    4     }, // CODE_POIN  = 3
};

event_code::event_code(code_type_enum type, const std::string& argument)
	: mCodeType(type), mArguments({ argument }) {}

event_code::event_code(code_type_enum type, const std::initializer_list<std::string>& arguments)
	: mCodeType(type), mArguments(arguments) {}

void event_code::write_to_stream(std::ostream& output) const {
	auto& codeType = msCodeTypeLibrary[mCodeType];

	output << codeType.name;

	for (auto& arg : mArguments)
		output << " " << arg;
}

void event_code::write_to_stream_misaligned(std::ostream& output) const {
	auto& codeType = msCodeTypeLibrary[mCodeType];

	output << codeType.nameMisaligned;

	for (auto& arg : mArguments)
		output << " " << arg;
}

unsigned int event_code::code_size() const {
	return msCodeTypeLibrary[mCodeType].size * mArguments.size();
}

unsigned int event_code::code_align() const {
	return msCodeTypeLibrary[mCodeType].align;
}

} // namespace lyn
