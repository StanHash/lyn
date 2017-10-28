#ifndef ARM_RELOCATOR_H
#define ARM_RELOCATOR_H

#include "section_data.h"
#include "core/event_code.h"

namespace lyn {

class arm_relocator {
public:
	event_code make_relocation_code(const section_data::relocation& relocation) const;

protected:
	std::string abs_reloc_string(const std::string& symbol, int addend) const;
	std::string rel_reloc_string(const std::string& symbol, int addend) const;
};

} // namespace lyn

#endif // ARM_RELOCATOR_H
