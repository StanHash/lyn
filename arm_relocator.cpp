#include "arm_relocator.h"

namespace lyn {

event_code arm_relocator::make_relocation_code(const section_data::relocation& relocation) const {
	switch (relocation.type) {
	case 0x02: // R_ARM_ABS32 (POIN Symbol + Addend)
		return lyn::event_code(lyn::event_code::CODE_POIN, abs_reloc_string(relocation.symbol, relocation.addend));

	case 0x03: // R_ARM_REL32 (WORD Symbol + Addend - CURRENTOFFSET)
		return lyn::event_code(lyn::event_code::CODE_WORD, rel_reloc_string(relocation.symbol, relocation.addend));

	case 0x05: // R_ARM_ABS16 (SHORT Symbol + Addend)
		return lyn::event_code(lyn::event_code::CODE_SHORT, abs_reloc_string(relocation.symbol, relocation.addend));

	case 0x08: // R_ARM_ABS8 (BYTE Symbol + Addend)
		return lyn::event_code(lyn::event_code::CODE_BYTE, abs_reloc_string(relocation.symbol, relocation.addend));

	case 0x09: // R_ARM_SBREL32 (WORD Symbol + Addend)
		return lyn::event_code(lyn::event_code::CODE_WORD, abs_reloc_string(relocation.symbol, relocation.addend));

	case 0x0A: // R_ARM_THM_CALL (BL)
		return lyn::event_code(lyn::event_code::MACRO_BL, abs_reloc_string(relocation.symbol, relocation.addend));

	default:
		throw std::runtime_error(std::string("ARM RELOC ERROR: Unhandled relocation type ").append(std::to_string(relocation.type)));
	}
}

std::string arm_relocator::abs_reloc_string(const std::string& symbol, int addend) const {
	if (addend == 0)
		return symbol;

	std::string result;
	result.reserve(5 + 8 + symbol.size()); // 5 ("( - )") + 8 (addend literal int) + symbol string

	result.append("(").append(symbol);

	if (addend < 0)
		result.append(" - ").append(std::to_string(-addend));
	else
		result.append(" + ").append(std::to_string(addend));

	result.append(")");
	return result;
}

std::string arm_relocator::rel_reloc_string(const std::string& symbol, int addend) const {
	if (addend == 0)
		return symbol;

	std::string result;
	result.reserve(21 + 8 + symbol.size()); // 21 ("( -  - CURRENTOFFSET)") + 8 (addend literal int) + symbol string

	result.append("(").append(symbol);

	if (addend < 0)
		result.append(" - ").append(std::to_string(-addend));
	else
		result.append(" + ").append(std::to_string(addend));

	result.append(" - CURRENTOFFSET)");
	return result;
}

} // namespace lyn
