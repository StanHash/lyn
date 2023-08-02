#ifndef LYN_BYTE_UTILS_HH
#define LYN_BYTE_UTILS_HH

#include <cinttypes>

#include <bit>
#include <span>
#include <type_traits>

// NOTE: this is maybe a bit too much
// we could just as well have Read8, Read16 and Read32 functions
// also L being a template param is awkward

template <std::endian E, typename T, unsigned int L = sizeof(T)>
inline T ReadIntegerBytes(std::span<std::uint8_t const> const & bytes)
    requires std::is_integral_v<T>
{
    if constexpr (E == std::endian::little)
    {
        T result = 0;

        for (unsigned int i = 0; i < L; i++)
            result += static_cast<T>(bytes[i]) << (i * 8);

        return result;
    }

    if constexpr (E == std::endian::big)
    {
        T result = 0;

        for (unsigned int i = 0; i < L; i++)
            result += static_cast<T>(bytes[i]) << ((L - (i + 1)) * 8);

        return result;
    }
}

template <std::endian E, typename T = std::uint32_t>
inline T ReadIntegerBytes32(std::span<std::uint8_t const> const & bytes)
    requires std::is_integral_v<T>
{
    return ReadIntegerBytes<E, T, 4u>(bytes);
}

template <std::endian E, typename T = std::uint16_t>
inline T ReadIntegerBytes16(std::span<std::uint8_t const> const & bytes)
    requires std::is_integral_v<T>
{
    return ReadIntegerBytes<E, T, 2u>(bytes);
}

template <std::endian E, typename T, unsigned int L = sizeof(T)>
inline void WriteIntegerBytes(std::span<std::uint8_t> const & bytes, T value)
    requires std::is_integral_v<T>
{
    if constexpr (E == std::endian::little)
    {
        for (unsigned int i = 0; i < L; i++)
            bytes[i] = static_cast<std::uint8_t>(value >> (i * 8));
    }

    if constexpr (E == std::endian::big)
    {
        for (unsigned int i = 0; i < L; i++)
            bytes[i] = static_cast<std::uint8_t>(value >> ((L - (i + 1)) * 8));
    }
}

#endif // LYN_BYTE_UTILS_HH
