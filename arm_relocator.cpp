#include "arm_relocator.h"

namespace lyn {

arm_relocator::arm_relocator() {
	mThumbVeneerTemplate.resize(0x10);

	mThumbVeneerTemplate.set_code(0, lyn::event_code(lyn::event_code::CODE_SHORT, "0x4778")); // bx pc
	mThumbVeneerTemplate.set_code(2, lyn::event_code(lyn::event_code::CODE_SHORT, "0x46C0")); // nop
	mThumbVeneerTemplate.set_code(4, lyn::event_code(lyn::event_code::CODE_WORD, "0xE59FC000")); // ldr ip, =target
	mThumbVeneerTemplate.set_code(8, lyn::event_code(lyn::event_code::CODE_WORD, "0xE12FFF1C")); // bx ip
}

event_section arm_relocator::make_thumb_veneer(const std::string targetSymbol) const {
	event_section result(mThumbVeneerTemplate);

	result.set_code(0x0C, lyn::event_code(lyn::event_code::CODE_POIN, targetSymbol));

	return result;
}

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

	case 0x0A: // R_ARM_THM_CALL
		return bl_code(relocation.symbol, relocation.addend);

	default:
		throw std::runtime_error(std::string("ARM RELOC ERROR: Unhandled relocation type ").append(std::to_string(relocation.type)));

	}
}

std::string arm_relocator::abs_reloc_string(const std::string& symbol, int addend) const {
	if (addend == 0)
		return symbol;

	std::string result;
	result.reserve(3 + 8 + symbol.size()); // 5 ("(-)") + 8 (addend literal int) + symbol string

	result.append("(").append(symbol);

	if (addend < 0)
		result.append("-").append(std::to_string(-addend));
	else
		result.append("+").append(std::to_string(addend));

	result.append(")");
	return result;
}

std::string arm_relocator::rel_reloc_string(const std::string& symbol, int addend) const {
	if (addend == 0)
		return symbol;

	std::string result;
	result.reserve(17 + 8 + symbol.size()); // 21 ("(--CURRENTOFFSET)") + 8 (addend literal int) + symbol string

	result.append("(").append(symbol);

	if (addend < 0)
		result.append("-").append(std::to_string(-addend));
	else
		result.append("+").append(std::to_string(addend));

	result.append("-CURRENTOFFSET)");
	return result;
}

std::string arm_relocator::bl_value_string(const std::string& symbol, int addend) const {
	return rel_reloc_string(symbol, addend-4);
}

std::string arm_relocator::bl_op1_string(const std::string &valueString) const {
	std::string result;
	result.reserve(3 + 20 + valueString.size());

	result.append("(((");
	result.append(valueString);
	result.append(">>12)&0x7FF)|0xF000)");

	return result;
}

std::string arm_relocator::bl_op2_string(const std::string& valueString) const {
	std::string result;
	result.reserve(3 + 19 + valueString.size());

	result.append("(((");
	result.append(valueString);
	result.append(">>1)&0x7FF)|0xF800)");

	return result;
}

lyn::event_code arm_relocator::bl_code(const std::string& symbol, int addend) const {
	std::string blValue = bl_value_string(symbol, addend);

	return lyn::event_code(lyn::event_code::CODE_SHORT, {
		bl_op1_string(blValue),
		bl_op2_string(blValue)
	}, lyn::event_code::ALLOW_NONE);
}

} // namespace lyn
