#include "remodel.hpp"
#include "gtest/gtest.h"

#include <cstdint>
#include <numeric>

using namespace remodel;

namespace 
{

// ============================================================================================== //
// [utils::Optional] testing                                                                      //
// ============================================================================================== //

class UtilsOptionalTest : public testing::Test {};

// Yeah, I know, this is far from being an exhaustive test.
// TODO: make it exhaustive.
TEST_F(UtilsOptionalTest, OptionalTest)
{
    using utils::Optional;

    {
        Optional<int> emptyOpt{utils::kEmpty};
        EXPECT_FALSE(emptyOpt.hasValue());
        EXPECT_FALSE(emptyOpt);
        auto otherOpt = emptyOpt;
        EXPECT_FALSE(otherOpt.hasValue());
    }
    
    {
        Optional<int> intOpt{123};
        EXPECT_TRUE(intOpt.hasValue());
        EXPECT_TRUE(intOpt);
        EXPECT_EQ(intOpt.value(), 123);

        auto otherOpt = intOpt;
        EXPECT_TRUE(intOpt.hasValue());
        EXPECT_TRUE(otherOpt.hasValue());
        EXPECT_EQ(intOpt.value(), 123);
        EXPECT_EQ(otherOpt.value(), 123);

        auto val = intOpt.release();
        EXPECT_FALSE(intOpt.hasValue());
        EXPECT_EQ(val, 123);

        auto movedOpt = std::move(otherOpt);
        EXPECT_FALSE(otherOpt.hasValue());
        EXPECT_TRUE(movedOpt.hasValue());
        EXPECT_EQ(movedOpt.value(), 123);
    }
}

// ============================================================================================== //
// Field arithmetic operator testing                                                              //
// ============================================================================================== //

class ArithmeticOperatorTest : public testing::Test
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
        Field<int> x{this, offsetof(A, x)};
    };
protected:
    ArithmeticOperatorTest()
        : wrapA{wrapper_cast<WrapA>(&a)}
    {
        a.x = 1000;
    }
protected:
    A     a;
    WrapA wrapA;
};

TEST_F(ArithmeticOperatorTest, BinaryWrapperWrapped)
{
    EXPECT_EQ(wrapA.x + 100, 1000 + 100);
    EXPECT_EQ(wrapA.x - 100, 1000 - 100);
    EXPECT_EQ(wrapA.x * 100, 1000 * 100);
    EXPECT_EQ(wrapA.x / 100, 1000 / 100);
    EXPECT_EQ(wrapA.x % 100, 1000 % 100);

    EXPECT_EQ(wrapA.x = 200, 200);
    EXPECT_EQ(a.x,           200);
}

TEST_F(ArithmeticOperatorTest, BinaryWrappedWrapped)
{
    EXPECT_EQ(wrapA.x + wrapA.x, 1000 + 1000);
    EXPECT_EQ(wrapA.x - wrapA.x, 1000 - 1000);
    EXPECT_EQ(wrapA.x * wrapA.x, 1000 * 1000);
    EXPECT_EQ(wrapA.x / wrapA.x, 1000 / 1000);
    EXPECT_EQ(wrapA.x % wrapA.x, 1000 % 1000);

    EXPECT_EQ(wrapA.x = wrapA.x, 1000);
    EXPECT_EQ(wrapA.x,           1000);
}
                                                            
TEST_F(ArithmeticOperatorTest, BinaryWrappedWrapper)
{
    EXPECT_EQ(2000 + wrapA.x, 2000 + 1000);
    EXPECT_EQ(2000 - wrapA.x, 2000 - 1000);
    EXPECT_EQ(2000 * wrapA.x, 2000 * 1000);
    EXPECT_EQ(2000 / wrapA.x, 2000 / 1000);
    EXPECT_EQ(2000 % wrapA.x, 2000 % 1000);

    decltype(a.x) test;
    EXPECT_EQ(test = wrapA.x, 1000);
    EXPECT_EQ(test,           1000);
}
                                                            
TEST_F(ArithmeticOperatorTest, BinaryCompoundWrapperWrapped)
{
    wrapA.x += 100;
    wrapA.x -= 100;
    wrapA.x *= 100;
    wrapA.x /= 100;
    wrapA.x %= 100;

    EXPECT_EQ(a.x, (1000 + 100 - 100) * 100 / 100 % 100);
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

    EXPECT_EQ(x, (100 + 1000 - 1000) * 1000 / 1000 % 1000);
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
// Field bitwise operator testing                                                                 //
// ============================================================================================== //

class BitwiseOperatorTest : public testing::Test
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
        Field<uint32_t> x{this, offsetof(A, x)};
    };
protected:
    BitwiseOperatorTest()
        : wrapA{wrapper_cast<WrapA>(&a)}
    {
        a.x = 0xCAFEBABE;
    }
protected:
    A     a;
    WrapA wrapA;
};

TEST_F(BitwiseOperatorTest, BinaryWrapperWrapped)
{
    EXPECT_EQ(wrapA.x |  100, 0xCAFEBABE |  100);
    EXPECT_EQ(wrapA.x &  100, 0xCAFEBABE &  100);
    EXPECT_EQ(wrapA.x ^  100, 0xCAFEBABE ^  100);
    EXPECT_EQ(wrapA.x << 12,  0xCAFEBABE << 12);
    EXPECT_EQ(wrapA.x >> 12,  0xCAFEBABE >> 12);
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

    EXPECT_EQ(a.x, ((0xCAFEBABE | 0x100) & 0x100 ^ 0x100) << 1 >> 4);
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

    EXPECT_EQ(x, ((0xCAFEBABE | 0x100) & 0x100 ^ 0x100) << 1 >> 4);
}

TEST_F(BitwiseOperatorTest, UnaryWrapped)
{
    EXPECT_EQ(~wrapA.x, ~a.x);
}

// ============================================================================================== //
// Field comparision operator testing                                                             //
// ============================================================================================== //

class ComparisionOperatorTest : public testing::Test
{
public:
    struct A
    {
        int32_t x;
        float y;
    };

    class WrapA : public ClassWrapper
    {
        REMODEL_WRAPPER(WrapA)
    public:
        Field<int32_t> x{this, offsetof(A, x)};
        Field<float>   y{this, offsetof(A, y)};
    };
protected:
    ComparisionOperatorTest()
        : wrapA{wrapper_cast<WrapA>(&a)}
    {
        a.x = 1234;
        a.y = 567.89f;
    }
protected:
    A     a;
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
// Field logical operator testing                                                                 //
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
// Array field testing                                                                            //
// ============================================================================================== //

class ArrayFieldTest : public testing::Test
{
public:
    struct A
    {
        float         x;
        int           y;
        unsigned char z;
    };

    struct B
    {
        A x[12];
    };

    class WrapA : public AdvancedClassWrapper<sizeof(A)>
    {
        REMODEL_ADV_WRAPPER(WrapA)
    public:
        Field<float>         x{this, offsetof(A, x)};
        Field<int>           y{this, offsetof(A, y)};
        Field<unsigned char> z{this, offsetof(A, z)};
    };

    class WrapB : public ClassWrapper
    {
        REMODEL_WRAPPER(WrapB)
    public:
        Field<A[12]>     x    {this, offsetof(B, x)};
        Field<WrapA[12]> wrapX{this, offsetof(B, x)};
    }; 
protected:
    ArrayFieldTest()
        : wrapB{wrapper_cast<WrapB>(&b)}
    {
        for (std::size_t i = 0; i < sizeof(b.x) / sizeof(*b.x); ++i)
        {
            b.x[i] = {1.0f, static_cast<int>(2 * i), static_cast<unsigned char>(i & 0xFF)};
        }
    }
protected:
    A     a;
    B     b;
    WrapB wrapB;
};

TEST_F(ArrayFieldTest, PlainArrayFields)
{
    // Array subscript access
    for (std::size_t i = 0; i < sizeof(b.x) / sizeof(*b.x); ++i)
    {
        EXPECT_EQ(wrapB.x[i].z--, i & 0xFF               );
        EXPECT_EQ(wrapB.x[i].z,   ((i & 0xFF) - 1) & 0xFF);
    }

    // Indirection access
    EXPECT_EQ((*wrapB.x).z, (*b.x).z);

    // Integer addition, subtraction
    EXPECT_EQ(wrapB.x + 10, b.x + 10);
    EXPECT_EQ(wrapB.x - 10, b.x - 10);

    // Array subtraction
    EXPECT_EQ(wrapB.x - wrapB.x, 0);
}

TEST_F(ArrayFieldTest, WrappedArrayFields)
{
    // Array subscript access
    for (std::size_t i = 0; i < sizeof(b.x) / sizeof(*b.x); ++i)
    {
        EXPECT_EQ(wrapB.wrapX[i].toStrong().z--, i & 0xFF               );
        EXPECT_EQ(wrapB.wrapX[i].toStrong().z,   ((i & 0xFF) - 1) & 0xFF);
    }

    // Indirection access
    EXPECT_EQ((*wrapB.wrapX).toStrong().z, (*b.x).z);

    // Integer addition, subtraction
    EXPECT_EQ(wrapB.wrapX + 10, static_cast<void*>(b.x + 10));
    EXPECT_EQ(wrapB.wrapX - 10, static_cast<void*>(b.x - 10));

    // Array subtraction
    EXPECT_EQ(wrapB.wrapX - wrapB.wrapX, 0);
}

// ============================================================================================== //
// Struct field testing                                                                           //
// ============================================================================================== //

class StructFieldTest : public testing::Test
{
public:
    struct A
    {
        uint32_t x;
    };

    struct B
    {
        A x;
    };
    
    class WrapA : public AdvancedClassWrapper<sizeof(A)>
    {
        REMODEL_ADV_WRAPPER(WrapA)
    public:
        Field<uint32_t> x{this, offsetof(A, x)};
    };

    class WrapB : public ClassWrapper
    {
        REMODEL_WRAPPER(WrapB)
    public:
        Field<A>     x    {this, offsetof(A, x)};
        Field<WrapA> wrapX{this, offsetof(A, x)};
    };
protected:
    StructFieldTest()
        : wrapB{wrapper_cast<WrapB>(&b)}
    {
        b.x.x = 123;
    }
protected:
    B b;
    WrapB wrapB;
};

TEST_F(StructFieldTest, NonWrappedStructField)
{
    EXPECT_EQ(123, wrapB.x->x++     );
    EXPECT_EQ(124, wrapB.x.get().x++);
    EXPECT_EQ(125, b.x.x            );
}
    
TEST_F(StructFieldTest, WrappedStructField)
{
    EXPECT_EQ(123, wrapB.wrapX->toStrong().x++                );
    EXPECT_EQ(124, wrapB.wrapX.get().toStrong().x++           );
    EXPECT_EQ(125, wrapper_cast<WrapA>(wrapB.wrapX->raw()).x++);
    EXPECT_EQ(126, b.x.x                                      );
}

// ============================================================================================== //
// Pointer field testing                                                                          //
// ============================================================================================== //

class PointerFieldTest : public testing::Test
{
public:
    struct A
    {
        uint32_t* x;
    };

    struct B
    {
        A *a;
    };
    
    class WrapA : public AdvancedClassWrapper<sizeof(A)>
    {
        REMODEL_ADV_WRAPPER(WrapA)
    public:
        Field<uint32_t*> x{this, offsetof(A, x)};
    };

    class WrapB : public ClassWrapper
    {
        REMODEL_WRAPPER(WrapB)
    public:
        Field<A*>     a    {this, offsetof(B, a)};
        Field<WrapA*> wrapA{this, offsetof(B, a)};
    };
protected:
    PointerFieldTest()
        : c{6358095}
        , wrapB{wrapper_cast<WrapB>(&b)}
    {
        a.x = &c;
        b.a = &a;
    }
protected:
    A        a;
    B        b;
    uint32_t c;
    WrapB    wrapB;
};

TEST_F(PointerFieldTest, PlainPointerFieldTest)
{
    EXPECT_EQ(&c,      wrapB.a->x     );
    EXPECT_EQ(6358095, (*wrapB.a->x)++);
    EXPECT_EQ(6358096, *wrapB.a->x    );
    EXPECT_EQ(6358096, c              );
}

TEST_F(PointerFieldTest, WrapperPointerFieldTest)
{
    EXPECT_EQ(&c,      wrapB.wrapA->toStrong().x     );
    EXPECT_EQ(6358095, (*wrapB.wrapA->toStrong().x)++);
    EXPECT_EQ(6358096, *wrapB.wrapA->toStrong().x    );
    EXPECT_EQ(6358096, c                             );
}

// ============================================================================================== //
// lvalue-reference-field testing                                                                 //
// ============================================================================================== //

class LvalueReferenceFieldTest : public testing::Test
{
public:
    struct A : utils::NonCopyable
    {
        uint32_t& x;

        explicit A(uint32_t& x)
            : x{x}
        {}
    };

    struct B : utils::NonCopyable
    {
        A& a;

        explicit B(A& a)
            : a{a}
        {}
    };
    
    class WrapA : public AdvancedClassWrapper<sizeof(A)>
    {
        REMODEL_ADV_WRAPPER(WrapA)
    public:
        Field<uint32_t&> x {this, 0}; // hardcore value, offsetof on reference is illegal
    };

    class WrapB : public ClassWrapper
    {
        REMODEL_WRAPPER(WrapB)
    public:
        Field<A&>     a    {this, 0};
        Field<WrapA&> wrapA{this, 0};
    };
protected:
    LvalueReferenceFieldTest()
        : c{6358095}
        , a{c}
        , b{a}
        , wrapB{wrapper_cast<WrapB>(&b)}
    {}
protected:
    uint32_t c;
    A        a;
    B        b;
    WrapB    wrapB;
};

TEST_F(LvalueReferenceFieldTest, PlainRefFieldTest)
{
    EXPECT_EQ(&c,      &wrapB.a->x );
    EXPECT_EQ(6358095, wrapB.a->x++);
    EXPECT_EQ(6358096, wrapB.a->x  );
    EXPECT_EQ(6358096, c           );
}

TEST_F(LvalueReferenceFieldTest, WrapperRefFieldTest)
{
    EXPECT_EQ(&c,      wrapB.wrapA->toStrong().x.addressOfObj());
    EXPECT_EQ(6358095, wrapB.wrapA->toStrong().x++             );
    EXPECT_EQ(6358096, wrapB.wrapA->toStrong().x               );
    EXPECT_EQ(6358096, c                                       );
}

// ============================================================================================== //
// [Global] testing                                                                               //
// ============================================================================================== //

class GlobalTest : public testing::Test {};

TEST_F(GlobalTest, GlobalTest)
{
    auto global    = Global::instance();
    int myStackVar = 854693;
    Field<int> myStackVarField{global, reinterpret_cast<ptrdiff_t>(&myStackVar)};

    EXPECT_EQ(myStackVar, myStackVarField);

    ++myStackVarField;
    EXPECT_EQ(myStackVarField, 854693 + 1);
    EXPECT_EQ(myStackVar,      854693 + 1);
}

// ============================================================================================== //
// [Module] testing                                                                               //
// ============================================================================================== //

class ModuleTest : public testing::Test {};

TEST_F(ModuleTest, ModuleTest)
{
    static int myStaticVar = 854693;
    auto mainModulePtr = platform::obtainModuleHandle(nullptr);
    auto mainModule = Module::getModule(nullptr);

    EXPECT_TRUE(mainModule);
    EXPECT_TRUE(mainModulePtr != nullptr);

    Field<int> myStaticVarField{
        addressOfWrapper(mainModule.value()), 
        static_cast<ptrdiff_t>(reinterpret_cast<uintptr_t>(&myStaticVar)
            - reinterpret_cast<uintptr_t>(mainModulePtr))
    };

    ++myStaticVarField;
    EXPECT_EQ(myStaticVarField, 854693 + 1);
    EXPECT_EQ(myStaticVar,      854693 + 1);
}

// ============================================================================================== //
// [Function] testing                                                                             //
// ============================================================================================== //

class FunctionTest : public testing::Test
{
protected:
    static int add(int a, int b) { return a + b; }
    Function<int(*)(int, int)> wrapAdd{&add};
public:
    FunctionTest() = default;
};

TEST_F(FunctionTest, FunctionTest)
{
    EXPECT_EQ(add(1423,  6879), wrapAdd(1423, 6879 ));
    EXPECT_EQ(add(-1423, 6879), wrapAdd(-1423, 6879));
}

// ============================================================================================== //
// [MemberFunction] testing                                                                       //
// ============================================================================================== //

// We rely on the fact that the compiler handles member-function-pointers as regular function
// pointers on the hood here (which is not guaranteed by the standard), so let's just perform
// this test with MSVC for now (where we know that the stuff is implemented this way).
#ifdef REMODEL_MSVC

class MemberFunctionTest : public testing::Test
{
protected:
    struct A
    {
        int c = 42;
        int add(int a, int b) { return a + b + c; }
    };

    struct WrapA : ClassWrapper
    {
        REMODEL_WRAPPER(WrapA)
    public:
        int(A::*pAdd)(int, int){&A::add};
        MemberFunction<int (__thiscall*)(int, int)> add{this, reinterpret_cast<uintptr_t&>(pAdd)};
    };
public:
    MemberFunctionTest() = default;
protected:
    A a;
    WrapA wrapA{wrapper_cast<WrapA>(&a)};
};

TEST_F(MemberFunctionTest, FunctionTest)
{
    EXPECT_EQ(a.add(1423,  6879), wrapA.add(1423, 6879 ));
    EXPECT_EQ(a.add(-1423, 6879), wrapA.add(-1423, 6879));
}

#endif // ifdef REMODEL_MSVC

// ============================================================================================== //
// [VirtualFunction] testing                                                                      //
// ============================================================================================== //

// Vftable layout is highly compiler-dependant, just perform this test with MSVC for now.
#ifdef REMODEL_MSVC

class VirtualFunctionTest : public testing::Test
{
protected:
    struct A
    {
        int c = 42;
        virtual int add(int a, int b) { return a + b + c; }
    };

    struct WrapA : ClassWrapper
    {
        REMODEL_WRAPPER(WrapA)
    public:
        VirtualFunction<int (__thiscall*)(int, int)> add{this, 0};
    };
public:
    VirtualFunctionTest() = default;
protected:
    A a;
    WrapA wrapA{wrapper_cast<WrapA>(&a)};
};

TEST_F(VirtualFunctionTest, FunctionTest)
{
    EXPECT_EQ(a.add(1423,  6879), wrapA.add(1423, 6879 ));
    EXPECT_EQ(a.add(-1423, 6879), wrapA.add(-1423, 6879));
}

#endif // ifdef REMODEL_MSVC

// ============================================================================================== //
// [MyWrapperType::Instantiable] testing                                                          //
// ============================================================================================== //

class InstantiableTest : public testing::Test
{
protected:
    struct A
    {
        int    a;
        float  b;
        double c;
    };

    struct WrapA
        : AdvancedClassWrapper<sizeof(A)>
    {
        REMODEL_ADV_WRAPPER(WrapA)
    public:
        Field<int>    a{this, offsetof(A, a)};
        Field<float>  b{this, offsetof(A, b)};
        Field<double> c{this, offsetof(A, c)};
    };

    struct WrapACustomCtor
        : AdvancedClassWrapper<sizeof(A)>
    {
        REMODEL_ADV_WRAPPER(WrapACustomCtor)
    public:
        Field<int>    a{this, offsetof(A, a)};
        Field<float>  b{this, offsetof(A, b)};
        Field<double> c{this, offsetof(A, c)};
    
        void construct(int a_, float b_, double c_)
        {
            a = a_;
            b = b_;
            c = c_;
        }
    };

    struct WrapACustomDtor
        : AdvancedClassWrapper<sizeof(A)>
    {
        REMODEL_ADV_WRAPPER(WrapACustomDtor)
    public:
        Field<int>    a{this, offsetof(A, a)};
        Field<float>  b{this, offsetof(A, b)};
        Field<double> c{this, offsetof(A, c)};

        void destruct()
        {
            throw int{123};
        }
    };
protected:
    InstantiableTest() = default;
};

TEST_F(InstantiableTest, InstantiableTest)
{
    WrapA::Instantiable simple;
    // Nothing more to check with the simple version, just create an instance here to force 
    // compilation and to detect any possible compiler-time errors.
    (void)simple;

    WrapACustomCtor::Instantiable customCtor{42, 43.f, 44.};
    EXPECT_EQ       (customCtor->a, 42  );
    EXPECT_FLOAT_EQ (customCtor->b, 43.f);
    EXPECT_DOUBLE_EQ(customCtor->c, 44. );

    // We throw an exception in destruct to see if it was actually called correctly.
    EXPECT_THROW({WrapACustomDtor::Instantiable customDtor;}, int);
}

// ============================================================================================== //

} // anon namespace

#include <setjmp.h>

int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
