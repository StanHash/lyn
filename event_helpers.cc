#include "event_helpers.hh"

#include <cassert>

#include <array>

#include "fmt/format.h"

#include "byte_utils.hh"

EventCode::EventCode(EventCode::Kind kind, std::string const & argument, bool currentoffset_anchor)
    : m_currentoffset_anchor(currentoffset_anchor), m_kind(kind), m_args({ argument })
{
}

EventCode::EventCode(EventCode::Kind kind, std::vector<std::string> && arguments, bool currentoffset_anchor)
    : m_currentoffset_anchor(currentoffset_anchor), m_kind(kind), m_args(std::move(arguments))
{
}

char const * EventCode::GetRawName(bool aligned) const
{
    static char const * const s_raw_names[4] = { "BYTE", "SHORT", "WORD", "POIN" };
    static char const * const s_raw_names_non_aligned[4] = { "BYTE", "SHORT2", "WORD2", "POIN2" };

    if (!aligned)
        return s_raw_names_non_aligned[static_cast<unsigned int>(m_kind)];

    return s_raw_names[static_cast<unsigned int>(m_kind)];
}

unsigned int EventCode::GetRawAlignment() const
{
    switch (m_kind)
    {
        case Kind::BYTE:
            return 1;

        case Kind::SHORT:
            return 2;

        case Kind::WORD:
        case Kind::POIN:
            return 4;
    }

    return 4;
}

std::string EventCode::Format(bool aligned) const
{
    return fmt::format("{0} {1}", GetRawName(aligned), fmt::join(m_args.begin(), m_args.end(), " "));
}

void EventCode::WriteToStream(std::FILE * ref_output, bool aligned) const
{
    fmt::println(ref_output, "{0} {1}", GetRawName(aligned), fmt::join(m_args.begin(), m_args.end(), " "));
}

unsigned int EventCode::CodeSize() const
{
    // NOTE: raw alignment is size
    return m_args.size() * GetRawAlignment();
}

bool EventCode::CanCombineWith(const EventCode & other) const
{
    return m_kind == other.m_kind && !other.m_currentoffset_anchor;
}

void EventCode::CombineWith(EventCode && other)
{
    assert(CanCombineWith(other));

    m_args.reserve(m_args.size() + other.m_args.size());
    std::move(other.m_args.begin(), other.m_args.end(), std::back_inserter(m_args));
}

EventBlock::EventBlock(unsigned int size) : m_code_map(size, -1)
{
}

void EventBlock::WriteToStream(std::FILE * ref_output, std::span<std::uint8_t const> const & base) const
{
    for (unsigned pos = 0; pos < m_code_map.size();)
    {
        if (m_code_map[pos] < 0)
        {
            auto here = std::next(m_code_map.begin(), pos);
            auto it = std::find_if(here, m_code_map.end(), [](int value) { return value >= 0; });

            unsigned int distance = std::distance(here, it);

            if ((distance >= 4) && ((pos % 4) == 0))
            {
                std::fprintf(ref_output, "WORD");

                while (distance >= 4)
                {
                    auto val = ReadIntegerBytes32<std::endian::little, std::uint32_t>(base.subspan(pos));

                    // TODO: better (don't repeat code)
                    if (val < 0x10)
                        std::fprintf(ref_output, " %" PRIu32, val);
                    else
                        std::fprintf(ref_output, " $%" PRIX32, val);

                    pos += 4;
                    distance -= 4;
                }

                std::fprintf(ref_output, "\n");
            }
            else if ((distance >= 2) && ((pos % 2) == 0))
            {
                auto val = ReadIntegerBytes16<std::endian::little, std::uint32_t>(base.subspan(pos));

                // TODO: better (don't repeat code)
                if (val < 0x10)
                    std::fprintf(ref_output, "SHORT %" PRIu32 "\n", val);
                else
                    std::fprintf(ref_output, "SHORT $%" PRIX32 "\n", val);

                pos += 2;
                distance -= 2;
            }
            else if (distance >= 1)
            {
                std::uint32_t const val = base[pos];

                // TODO: better (don't repeat code)
                if (val < 0x10)
                    std::fprintf(ref_output, "BYTE %" PRIu32 "\n", val);
                else
                    std::fprintf(ref_output, "BYTE $%" PRIX32 "\n", val);

                pos++;
                distance--;
            }
        }
        else
        {
            EventCode const & code = m_code[m_code_map[pos]];

            code.WriteToStream(ref_output, (pos % code.GetRawAlignment()) == 0);
            pos += code.CodeSize();

            // NOTE: ideally, if this is a WORD, we should continue by adding base data here
        }
    }
}

void EventBlock::MapCode(unsigned int offset, EventCode const & code)
{
    unsigned int size = code.CodeSize();

    int index = m_code.size();
    m_code.push_back(code);

    for (unsigned i = 0; i < size; ++i)
        m_code_map[offset + i] = index;
}

void EventBlock::MapCode(unsigned int offset, EventCode && code)
{
    unsigned int size = code.CodeSize();

    int index = m_code.size();
    m_code.push_back(std::move(code));

    for (unsigned i = 0; i < size; ++i)
        m_code_map[offset + i] = index;
}

void EventBlock::PackCodes()
{
    for (unsigned int offset = 0; offset < m_code_map.size();)
    {
        int index = m_code_map[offset];

        if (index < 0)
        {
            offset++;
            continue;
        }

        auto & code = m_code[index];

        if (offset + code.CodeSize() >= m_code_map.size())
            break;

        auto & next = m_code[m_code_map[offset + code.CodeSize()]];

        if (code.CanCombineWith(next))
        {
            code.CombineWith(std::move(next));

            for (unsigned int i = 0; i < code.CodeSize(); ++i)
                m_code_map[offset + i] = index;
        }
        else
        {
            offset += code.CodeSize();
        }
    }
}

void EventBlock::Optimize()
{
    auto old_code(std::move(m_code));
    m_code.clear();

    for (unsigned int offset = 0; offset < m_code_map.size();)
    {
        int const old_code_idx = m_code_map[offset];

        if (old_code_idx < 0)
        {
            m_code_map[offset] = -1;
            offset++;
        }
        else
        {
            unsigned int size = old_code[old_code_idx].CodeSize();

            MapCode(offset, std::move(old_code[old_code_idx]));
            offset += size;
        }
    }
}
