#ifndef EVENT_SECTION_H
#define EVENT_SECTION_H

#include <ostream>
#include <span>

namespace lyn {

/* TODO: rename and/or move this
 * (this used to define a class but simplifications made it redundant) */

void write_event_bytes(std::ostream& output, int alignment, std::span<const unsigned char> bytes);

} // namespace lyn

#endif // EVENT_SECTION_H
