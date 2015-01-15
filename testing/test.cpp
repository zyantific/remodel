#include "remodel.hpp"
#include "gtest/gtest.h"

#include <cstdint>

using namespace Remodel;

namespace 
{

// ============================================================================================== //
// Arithmetic operator testing                                                                    //
// ============================================================================================== //

class ArithmeticOperatorTest : public ::testing::Test
{
protected:
    struct A
    {
        int x;
    };

    class WrapA : public ClassWrapper
    {
        REMODEL_WRAPPER(WrapA)
    public:
        Field<int> x {thiz(), 0};
    };
protected:
    ArithmeticOperatorTest()
        : wrapA(wrapperCast<WrapA>(&a))
    {
        a.x = 1000;
    }
protected:
    A a;
    WrapA wrapA;
};

TEST_F(ArithmeticOperatorTest, BinaryWrapperWrapped)
{
    EXPECT_EQ(wrapA.x + 100, 1000 + 100);
    EXPECT_EQ(wrapA.x - 100, 1000 - 100);
    EXPECT_EQ(wrapA.x * 100, 1000 * 100);
    EXPECT_EQ(wrapA.x / 100, 1000 / 100);
    EXPECT_EQ(wrapA.x % 100, 1000 % 100);
}

TEST_F(ArithmeticOperatorTest, BinaryWrappedWrapped)
{
    EXPECT_EQ(wrapA.x + wrapA.x, 1000 + 1000);
    EXPECT_EQ(wrapA.x - wrapA.x, 1000 - 1000);
    EXPECT_EQ(wrapA.x * wrapA.x, 1000 * 1000);
    EXPECT_EQ(wrapA.x / wrapA.x, 1000 / 1000);
    EXPECT_EQ(wrapA.x % wrapA.x, 1000 % 1000);
}
                                                            
TEST_F(ArithmeticOperatorTest, BinaryWrappedWrapper)
{
    EXPECT_EQ(2000 + wrapA.x, 2000 + 1000);
    EXPECT_EQ(2000 - wrapA.x, 2000 - 1000);
    EXPECT_EQ(2000 * wrapA.x, 2000 * 1000);
    EXPECT_EQ(2000 / wrapA.x, 2000 / 1000);
    EXPECT_EQ(2000 % wrapA.x, 2000 % 1000);
}
                                                            
TEST_F(ArithmeticOperatorTest, BinaryCompoundWrapperWrapped)
{
    wrapA.x += 100;
    wrapA.x -= 100;
    wrapA.x *= 100;
    wrapA.x /= 100;
    wrapA.x %= 100;

    EXPECT_EQ(a.x, ((((1000 + 100) - 100) * 100) / 100) % 100);
}

TEST_F(ArithmeticOperatorTest, BinaryCompoundWrappedWrapped)
{
    wrapA.x += wrapA.x;
    wrapA.x -= wrapA.x;
    wrapA.x *= wrapA.x;
    wrapA.x /= wrapA.x + 1;
    wrapA.x %= wrapA.x + 1;

    decltype(a.x) ref = 1000;
    ref += ref;
    ref -= ref;
    ref *= ref;
    ref /= ref + 1;
    ref %= ref + 1;

    EXPECT_EQ(a.x, ref);
}

TEST_F(ArithmeticOperatorTest, BinaryCompoundWrappedWrapper)
{
    decltype(a.x) x = 100;

    x += wrapA.x;
    x -= wrapA.x;
    x *= wrapA.x;
    x /= wrapA.x;
    x %= wrapA.x;

    EXPECT_EQ(x, ((((100 + 1000) - 1000) * 1000) / 1000) % 1000);
}

TEST_F(ArithmeticOperatorTest, UnaryWrapped)
{
    EXPECT_EQ(+wrapA.x,     +a.x);
    EXPECT_EQ(-wrapA.x,     -a.x);

    EXPECT_EQ(wrapA.x++,    1000);
    EXPECT_EQ(a.x,          1001);
    EXPECT_EQ(++wrapA.x,    1002);
    EXPECT_EQ(a.x,          1002);

    EXPECT_EQ(wrapA.x--,    1002);
    EXPECT_EQ(a.x,          1001);
    EXPECT_EQ(--wrapA.x,    1000);
    EXPECT_EQ(a.x,          1000);
}

// ============================================================================================== //
// Bitwise operator testing                                                                       //
// ============================================================================================== //

class BitwiseOperatorTest : public ::testing::Test
{
public:
    struct A
    {
        uint32_t x;
    };

    class WrapA : public ClassWrapper
    {
        REMODEL_WRAPPER(WrapA)
    public:
        Field<uint32_t> x {thiz(), 0};
    };
protected:
    BitwiseOperatorTest()
        : wrapA(wrapperCast<WrapA>(&a))
    {
        a.x = 0xCAFEBABE;
    }
protected:
    A a;
    WrapA wrapA;
};

TEST_F(BitwiseOperatorTest, BinaryWrapperWrapped)
{
    EXPECT_EQ(wrapA.x |  100, 0xCAFEBABE |  100);
    EXPECT_EQ(wrapA.x &  100, 0xCAFEBABE &  100);
    EXPECT_EQ(wrapA.x ^  100, 0xCAFEBABE ^  100);
    EXPECT_EQ(wrapA.x << 12, 0xCAFEBABE << 12);
    EXPECT_EQ(wrapA.x >> 12, 0xCAFEBABE >> 12);
}

TEST_F(BitwiseOperatorTest, BinaryWrappedWrapped)
{
    EXPECT_EQ(wrapA.x |  wrapA.x, 0xCAFEBABE |  0xCAFEBABE);
    EXPECT_EQ(wrapA.x &  wrapA.x, 0xCAFEBABE &  0xCAFEBABE);
    EXPECT_EQ(wrapA.x ^  wrapA.x, 0xCAFEBABE ^  0xCAFEBABE);
    //EXPECT_EQ(wrapA.x << wrapA.x, 0xCAFEBABE << 0xCAFEBABE);
    //EXPECT_EQ(wrapA.x >> wrapA.x, 0xCAFEBABE >> 0xCAFEBABE);
}
                                                            
TEST_F(BitwiseOperatorTest, BinaryWrappedWrapper)
{
    EXPECT_EQ(0x1234 |  wrapA.x, 0x1234 | 0xCAFEBABE);
    EXPECT_EQ(0x1234 &  wrapA.x, 0x1234 & 0xCAFEBABE);
    EXPECT_EQ(0x1234 ^  wrapA.x, 0x1234 ^ 0xCAFEBABE);

    a.x = 3;
    EXPECT_EQ(0x1234 << wrapA.x, 0x1234 << 3);
    EXPECT_EQ(0x1234 >> wrapA.x, 0x1234 >> 3);
}
                                                            
TEST_F(BitwiseOperatorTest, BinaryCompoundWrapperWrapped)
{
    wrapA.x |=  100;
    wrapA.x &=  100;
    wrapA.x ^=  100;
    wrapA.x <<= 1;
    wrapA.x >>= 4;

    EXPECT_EQ(a.x, ((((0xCAFEBABE | 0x100) & 0x100) ^ 0x100) << 1) >> 4);
}

TEST_F(BitwiseOperatorTest, BinaryCompoundWrappedWrapped)
{
    wrapA.x |=  100;
    wrapA.x &=  100;
    wrapA.x ^=  100;
    wrapA.x <<= 1;
    wrapA.x >>= 4;

    decltype(a.x) ref = 0xCAFEBABE;
    ref |=  100;
    ref &=  100;
    ref ^=  100;
    ref <<= 1;
    ref >>= 4;

    EXPECT_EQ(a.x, ref);
}

TEST_F(BitwiseOperatorTest, BinaryCompoundWrappedWrapper)
{
    decltype(a.x) x = 100;

    x |=  wrapA.x;
    x &=  wrapA.x;
    x ^=  wrapA.x;
    x <<= wrapA.x;
    x >>= wrapA.x;

    EXPECT_EQ(x, ((((0xCAFEBABE | 0x100) & 0x100) ^ 0x100) << 1) >> 4);
}

TEST_F(BitwiseOperatorTest, UnaryWrapped)
{
    EXPECT_EQ(~wrapA.x, ~a.x);
}

// ============================================================================================== //
// Comparision operator testing                                                                   //
// ============================================================================================== //

class ComparisionOperatorTest : public ::testing::Test
{
public:
    struct A
    {
        uint32_t x;
        float y;
    };

    class WrapA : public ClassWrapper
    {
        REMODEL_WRAPPER(WrapA)
    public:
        Field<uint32_t> x {thiz(), 0                };
        Field<float>    y {thiz(), sizeof(uint32_t) };
    };
protected:
    ComparisionOperatorTest()
        : wrapA(wrapperCast<WrapA>(&a))
    {
        a.x = 1234;
        a.y = 567.89f;
    }
protected:
    A a;
    WrapA wrapA;
};

TEST_F(ComparisionOperatorTest, BinaryWrapperWrapped)
{
    EXPECT_EQ(wrapA.x == 1234,      true );
    EXPECT_EQ(wrapA.y == 567.89f,   true );

    EXPECT_EQ(wrapA.x != 1234,      false);
    EXPECT_EQ(wrapA.y != 567.89f,   false);

    EXPECT_EQ(wrapA.x >  1233,      true );
    EXPECT_EQ(wrapA.x >  1234,      false);
    EXPECT_EQ(wrapA.y >  567.88f,   true );
    EXPECT_EQ(wrapA.y >  567.90f,   false);

    EXPECT_EQ(wrapA.x <  1233,      false);
    EXPECT_EQ(wrapA.x <  1234,      false);
    EXPECT_EQ(wrapA.y <  567.88f,   false);
    EXPECT_EQ(wrapA.y <  567.90f,   true );

    EXPECT_EQ(wrapA.x >= 1233,      true );
    EXPECT_EQ(wrapA.x >= 1234,      true );
    EXPECT_EQ(wrapA.y >= 567.88f,   true );
    EXPECT_EQ(wrapA.y >= 567.90f,   false);

    EXPECT_EQ(wrapA.x <= 1233,      false);
    EXPECT_EQ(wrapA.x <= 1234,      true );
    EXPECT_EQ(wrapA.y <= 567.88f,   false);
    EXPECT_EQ(wrapA.y <= 567.90f,   true );
}

TEST_F(ComparisionOperatorTest, BinaryWrappedWrapped)
{
    EXPECT_EQ(wrapA.x == wrapA.x, true);
    EXPECT_EQ(wrapA.y == wrapA.y, true);

    EXPECT_EQ(wrapA.x != wrapA.x, false);
    EXPECT_EQ(wrapA.y != wrapA.y, false);

    EXPECT_EQ(wrapA.x >  wrapA.x, false);
    EXPECT_EQ(wrapA.x >  wrapA.x, false);
    EXPECT_EQ(wrapA.y >  wrapA.y, false);
    EXPECT_EQ(wrapA.y >  wrapA.y, false);

    EXPECT_EQ(wrapA.x <  wrapA.x, false);
    EXPECT_EQ(wrapA.x <  wrapA.x, false);
    EXPECT_EQ(wrapA.y <  wrapA.y, false);
    EXPECT_EQ(wrapA.y <  wrapA.y, false);

    EXPECT_EQ(wrapA.x >= wrapA.x, true);
    EXPECT_EQ(wrapA.x >= wrapA.x, true);
    EXPECT_EQ(wrapA.y >= wrapA.y, true);
    EXPECT_EQ(wrapA.y >= wrapA.y, true);

    EXPECT_EQ(wrapA.x <= wrapA.x, true);
    EXPECT_EQ(wrapA.x <= wrapA.x, true);
    EXPECT_EQ(wrapA.y <= wrapA.y, true);
    EXPECT_EQ(wrapA.y <= wrapA.y, true);
}
                                                            
TEST_F(ComparisionOperatorTest, BinaryWrappedWrapper)
{
    EXPECT_EQ(1234    == wrapA.x, true );
    EXPECT_EQ(567.89f == wrapA.y, true );
                        
    EXPECT_EQ(1234    != wrapA.x, false);
    EXPECT_EQ(567.89f != wrapA.y, false);
                        
    EXPECT_EQ(1233    >  wrapA.x, false);
    EXPECT_EQ(1234    >  wrapA.x, false);
    EXPECT_EQ(567.88f >  wrapA.y, false);
    EXPECT_EQ(567.90f >  wrapA.y, true );
                        
    EXPECT_EQ(1233    <  wrapA.x, true );
    EXPECT_EQ(1234    <  wrapA.x, false);
    EXPECT_EQ(567.88f <  wrapA.y, true );
    EXPECT_EQ(567.90f <  wrapA.y, false);
                        
    EXPECT_EQ(1233    >= wrapA.x, false);
    EXPECT_EQ(1234    >= wrapA.x, true );
    EXPECT_EQ(567.88f >= wrapA.y, false);
    EXPECT_EQ(567.90f >= wrapA.y, true );
                        
    EXPECT_EQ(1233    <= wrapA.x, true );
    EXPECT_EQ(1234    <= wrapA.x, true );
    EXPECT_EQ(567.88f <= wrapA.y, true );
    EXPECT_EQ(567.90f <= wrapA.y, false);
}

// ============================================================================================== //
// Logical operator testing                                                                       //
// ============================================================================================== //

// We reuse the arithmetic operator test here.
class LogicalOperatorTest : public ArithmeticOperatorTest {};

TEST_F(LogicalOperatorTest, BinaryWrapperWrapped)
{
    EXPECT_EQ(wrapA.x && 432,   true );
    EXPECT_EQ(wrapA.x && 0,     false);
    EXPECT_EQ(wrapA.x || 432,   true );
    EXPECT_EQ(wrapA.x || 0,     true );
}

TEST_F(LogicalOperatorTest, BinaryWrappedWrapped)
{
    EXPECT_EQ(wrapA.x && wrapA.x, true);
    EXPECT_EQ(wrapA.x || wrapA.x, true);
}
                                                            
TEST_F(LogicalOperatorTest, BinaryWrappedWrapper)
{
    EXPECT_EQ(432 && wrapA.x, true );
    EXPECT_EQ(0   && wrapA.x, false);
    EXPECT_EQ(432 || wrapA.x, true );
    EXPECT_EQ(0   || wrapA.x, true );
}

TEST_F(LogicalOperatorTest, UnaryWrapped)
{
    EXPECT_EQ(!wrapA.x,     false);
    EXPECT_EQ(!!wrapA.x,    true);
}

// ============================================================================================== //
// Member and pointer operator testing                                                            //
// ============================================================================================== //

class PointerOperatorTest : public ::testing::Test
{
protected:
    struct A
    {
        int x;
    };

    class WrapA : public ClassWrapper
    {
        REMODEL_WRAPPER(WrapA)
    public:
        Field<int> x {thiz(), 0};
    };
protected:
    PointerOperatorTest()
        : wrapA(wrapperCast<WrapA>(&a))
    {
        a.x = 1000;
    }
protected:
    A a;
    WrapA wrapA;
};

// ============================================================================================== //

} // anon namespace

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    std::cin.get();
    return ret;
}