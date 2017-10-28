#ifndef EVENT_CODE_H
#define EVENT_CODE_H

#include <vector>
#include <string>

namespace lyn {

class event_code {
public:
	enum code_type_enum {
		CODE_BYTE,
		CODE_SHORT,
		CODE_WORD,
		CODE_POIN,
		MACRO_BL
	};

public:
	event_code(code_type_enum type, const std::string& argument);
	event_code(code_type_enum type, const std::initializer_list<std::string>& arguments);

	std::string get_code_string() const;
	void write_to_stream(std::ostream& output) const;

	const std::string& code_name() const;

	unsigned int code_size() const;
	unsigned int code_align() const;

	bool can_combine_with(const event_code& other) const;
	void combine_with(event_code&& other);

private:
	int mCodeType;
	std::vector<std::string> mArguments;

private:
	struct event_code_type {
		std::string name;

		enum {
			Raw,
			Macro
		} type;

		unsigned int size;
		unsigned int align;
	};

	static std::vector<event_code_type> msCodeTypeLibrary;
};

} // namespace lyn

std::ostream& operator << (std::ostream& stream, const lyn::event_code& eventCode);

#endif // EVENT_CODE_H
