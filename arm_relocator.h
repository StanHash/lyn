#ifndef ARM_RELOCATOR_H
#define ARM_RELOCATOR_H

#include "section_data.h"
#include "core/event_section.h"

#include <memory>
#include <map>

namespace lyn {

class arm_relocator {
public:
	struct relocatelet {
		virtual ~relocatelet() {}

		virtual event_code make_event_code(const std::string& sym, int addend) const = 0;
		virtual void apply_relocation(section_data& data, unsigned int offset, unsigned int value, int addend) const = 0;

		virtual bool is_absolute() const { return true; }
		virtual bool can_make_trampoline() const { return false; }
		virtual section_data make_trampoline(const std::string& symbol, int addend) const { return section_data(); }
	};

public:
	arm_relocator();

	const relocatelet* get_relocatelet(int relocationIndex) const;

public:
	static std::string abs_reloc_string(const std::string& symbol, int addend);
	static std::string rel_reloc_string(const std::string& symbol, int addend);

	static std::string pcrel_reloc_string(const std::string& symbol, int addend);
	static std::string bl_op1_string(const std::string& valueString);
	static std::string bl_op2_string(const std::string& valueString);

	static lyn::event_code bl_code(const std::string& symbol, int addend);
	static section_data make_thumb_veneer(const std::string& symbol, int addend);

private:
	section_data mThumbVeneerTemplate;

	std::map<int, std::unique_ptr<relocatelet>> mRelocatelets;
};

} // namespace lyn

#endif // ARM_RELOCATOR_H
