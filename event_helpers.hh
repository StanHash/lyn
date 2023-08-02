#ifndef LYN_EVENT_BLOCK_HH
#define LYN_EVENT_BLOCK_HH

#include <cinttypes>
#include <cstdio>

#include <span>
#include <string>
#include <vector>

struct EventCode
{
    enum struct Kind
    {
        BYTE,
        SHORT,
        WORD,
        POIN,
    };

    EventCode(Kind kind, std::string const & argument, bool currentoffset_anchor = false);
    EventCode(Kind kind, std::vector<std::string> && arguments, bool currentoffset_anchor = false);

    char const * GetRawName(bool aligned = true) const;
    unsigned int GetRawAlignment() const;

    std::string Format(bool aligned = true) const;
    void WriteToStream(std::FILE * ref_output, bool aligned = true) const;

    unsigned int CodeSize() const;

    bool CanCombineWith(EventCode const & other) const;
    void CombineWith(EventCode && other);

private:
    bool m_currentoffset_anchor : 1;
    Kind m_kind;
    std::vector<std::string> m_args;
};

// TODO: this could also encode symbols?
// or maybe have another wrapper for that?
struct EventBlock
{
    EventBlock(unsigned int size);

    void WriteToStream(std::FILE * ref_output, std::span<std::uint8_t const> const & base) const;

    void MapCode(unsigned int offset, EventCode const & code);
    void MapCode(unsigned int offset, EventCode && code);

    void PackCodes();
    void Optimize();

private:
    std::vector<int> m_code_map;
    std::vector<EventCode> m_code;
};

#endif // LYN_EVENT_BLOCK_HH
