#include <cassert>

#include "natelf/arm32.hh"

#include "relocation.hh"

// TODO: better testing framework
// TODO: test CanEncode

static void TestAbs32(void)
{
    static RelocationInfo const abs32 = GetArm32RelocationInfo(R_ARM_ABS32);

    assert(abs32.parts[0].GetMask<std::uint32_t>() == 0xFFFFFFFF);
    assert(abs32.parts[0].GetMask<std::int64_t>() == 0xFFFFFFFF);

    std::array<std::uint8_t, 4u> bytes { 0xFF, 0xFF, 0xFF, 0xFF };

    assert(abs32.Extract(bytes) == 0xFFFFFFFF);

    abs32.Inject(bytes, 0xAAAAAAAA);
    assert(abs32.Extract(bytes) == 0xAAAAAAAA);

    abs32.Inject(bytes, 0xDEADBEEF);
    assert(abs32.Extract(bytes) == 0xDEADBEEF);

    abs32.Inject(bytes, 1);
    assert(abs32.Extract(bytes) == 1);

    abs32.Inject(bytes, -1);
    assert(abs32.Extract(bytes) == 0xFFFFFFFF);
}

static void TestThmCall(void)
{
    static RelocationInfo const thm_call = GetArm32RelocationInfo(R_ARM_THM_CALL);

    assert(thm_call.parts[0].GetMask<std::uint32_t>() == 0x7FF);
    assert(thm_call.parts[0].GetMask<std::int64_t>() == 0x7FF);
    assert(thm_call.parts[1].GetMask<std::uint32_t>() == 0x7FF);
    assert(thm_call.parts[1].GetMask<std::int64_t>() == 0x7FF);

    std::array<std::uint8_t, 4u> bytes { 0xFF, 0xFF, 0xFF, 0xFF };

    assert(thm_call.Extract(bytes) == -2);

    thm_call.Inject(bytes, 0xAAAAAAAA);
    assert(thm_call.Extract(bytes) == 0x2AAAAA);

    thm_call.Inject(bytes, 2);
    assert(thm_call.Extract(bytes) == 2);

    thm_call.Inject(bytes, -2);
    assert(thm_call.Extract(bytes) == -2);
}

int main(void)
{
    TestAbs32();
    TestThmCall();
}
