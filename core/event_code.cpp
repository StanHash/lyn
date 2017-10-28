#include "event_code.h"

#include <sstream>

namespace lyn {

std::vector<event_code::event_code_type> event_code::msCodeTypeLibrary = {
//  { name,    type,                               size, align },
	{ "BYTE",  event_code::event_code_type::Raw,   1,    1     }, // CODE_BYTE  = 0
	{ "SHORT", event_code::event_code_type::Raw,   2,    2     }, // CODE_SHORT = 1
	{ "WORD",  event_code::event_code_type::Raw,   4,    4     }, // CODE_WORD  = 2
	{ "POIN",  event_code::event_code_type::Raw,   4,    4     }, // CODE_POIN  = 3
	{ "BL",    event_code::event_code_type::Macro, 4,    2     }, // MACRO_BL   = 4
};

event_code::event_code(code_type_enum type, const std::string& argument)
	: mCodeType(type), mArguments({ argument }) {}

event_code::event_code(code_type_enum type, const std::initializer_list<std::string>& arguments)
	: mCodeType(type), mArguments(arguments) {}

std::string event_code::get_code_string() const {
	int reserve = code_name().size() + 2;

	for (auto& arg : mArguments)
		reserve += arg.size() + 2;

	std::string result;
	result.reserve(reserve);

	std::ostringstream sstream(result);
	write_to_stream(sstream);

	return result;
}

void event_code::write_to_stream(std::ostream& output) const {
	auto& codeType = msCodeTypeLibrary[mCodeType];

	output << codeType.name;

	if (codeType.type == event_code_type::Raw) {
		for (auto& arg : mArguments)
			output << " " << arg;
	} else { // codeType.type == event_code_type::Macro
		auto it = mArguments.begin();

		if (it == mArguments.end())
			return;

		output << "(" << *it;

		while ((++it) != mArguments.end())
			output << ", " << *it;

		output << ")";
	}
}

const std::string& event_code::code_name() const {
	return msCodeTypeLibrary[mCodeType].name;
}

unsigned int event_code::code_size() const {
	auto& codeType = msCodeTypeLibrary[mCodeType];

	// Macros have fixed size, but raw codes take one chunk per argument
	return ((codeType.type == event_code_type::Raw) ? (codeType.size * mArguments.size()) : codeType.size);
}

unsigned int event_code::code_align() const {
	return msCodeTypeLibrary[mCodeType].align;
}

bool event_code::can_combine_with(const event_code& other) const {
	if (msCodeTypeLibrary[mCodeType].type == event_code_type::Macro)
		return false;

	return (mCodeType == other.mCodeType);
}

void event_code::combine_with(event_code&& other) {
	if (!can_combine_with(other))
		throw std::runtime_error(std::string("EVENT CODE: tried combining the following codes:\n\t")
								 .append(get_code_string()).append("\n\t")
								 .append(other.get_code_string()).append("\n"));

	mArguments.reserve(mArguments.size() + other.mArguments.size());

	std::copy(
		std::make_move_iterator(other.mArguments.begin()),
		std::make_move_iterator(other.mArguments.end()),
		std::back_inserter(mArguments)
	);
}

} // namespace lyn

std::ostream& operator << (std::ostream& stream, const lyn::event_code& eventCode) {
	eventCode.write_to_stream(stream);
	return stream;
}
