#ifndef ARM_RELOCATOR_H
#define ARM_RELOCATOR_H

#include "section_data.h"
#include "core/event_section.h"

namespace lyn {

class arm_relocator {
public:
	arm_relocator();

	event_section make_thumb_veneer(const std::string& targetSymbol) const;
	event_code make_relocation_code(const section_data::relocation& relocation) const;

protected:
	std::string abs_reloc_string(const std::string& symbol, int addend) const;
	std::string rel_reloc_string(const std::string& symbol, int addend) const;

	std::string pcrel_reloc_string(const std::string& symbol, int addend) const;
	std::string bl_op1_string(const std::string& valueString) const;
	std::string bl_op2_string(const std::string& valueString) const;

	lyn::event_code bl_code(const std::string& symbol, int addend) const;

private:
	event_section mThumbVeneerTemplate;
};

} // namespace lyn

#endif // ARM_RELOCATOR_H
