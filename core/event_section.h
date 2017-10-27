#ifndef EVENT_SECTION_H
#define EVENT_SECTION_H

#include <string>
#include <vector>
#include <iostream>

namespace lyn {

class event_section {
public:
	struct event_code {
		enum code_type {
			BYTE,
			SHORT,
			WORD,
			POIN,

			NONE = -1
		};

		code_type   type;
		std::string argument;

		enum {
			Whatever,
			ForceNewline,
			ForceContinue
		} newline_policy = Whatever;
	};

public:
	event_section();

	void write_to_stream(std::ostream& output) const;

	void resize(unsigned int size);

	void set_code(unsigned int offset, event_code&& code);

public:
	static int code_size(const event_code& code) { return code_size(code.type); }
	static int code_size(event_code::code_type type);

	static const char* code_identifier(const event_code& code) { return code_identifier(code.type); }
	static const char* code_identifier(event_code::code_type type);

private:
	std::vector<int> mCodeMap;
	std::vector<event_code> mCodes;
};

} // namespace lyn

#endif // EVENT_SECTION_H
