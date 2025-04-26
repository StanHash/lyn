#ifndef EVENT_CODE_H
#define EVENT_CODE_H

#include <vector>
#include <string>
#include <ostream>

namespace lyn {

struct event_code {
	enum code_type_enum {
		CODE_BYTE,
		CODE_SHORT,
		CODE_WORD,
		CODE_POIN,
	};

public:
	event_code(code_type_enum type, const std::string& argument);
	event_code(code_type_enum type, const std::initializer_list<std::string>& arguments);

	void write_to_stream_misaligned(std::ostream& output) const;
	void write_to_stream(std::ostream& output) const;

	unsigned int code_size() const;
	unsigned int code_align() const;

private:
	code_type_enum mCodeType;
	std::vector<std::string> mArguments;

private:
	struct event_code_type {
		std::string name;
		std::string nameMisaligned;

		unsigned int size;
		unsigned int align;
	};

	static std::vector<event_code_type> msCodeTypeLibrary;
};

} // namespace lyn

#endif // EVENT_CODE_H
