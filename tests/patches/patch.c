int MyFunc(int arg_0) __attribute__((short_call));

__attribute__((section("__lyn_at_0x0800BEEF"))) int AtTest(int arg_0)
{
    return MyFunc(arg_0) + 1;
}

void OtherFunc(void)
{
    MyFunc(MyFunc(1));
}
