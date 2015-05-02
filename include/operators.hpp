#ifndef _REMODEL_OPERATORS_HPP_
#define _REMODEL_OPERATORS_HPP_

#include <cstdint>
#include <type_traits>
#undef min // FUUUU MS
#undef max

namespace remodel
{
namespace Operators
{

// ============================================================================================== //
// Constants and types                                                                            //
// ============================================================================================== //

using Flags = uint32_t;

enum : Flags
{
    // Arithmetic
    ASSIGN                  = 1UL <<  0,
    ADD                     = 1UL <<  1,
    SUBTRACT                = 1UL <<  2,
    MULTIPLY                = 1UL <<  3,
    DIVIDE                  = 1UL <<  4,
    MODULO                  = 1UL <<  5,
    UNARY_PLUS              = 1UL <<  6,
    UNARY_MINUS             = 1UL <<  7,
    INCREMENT               = 1UL <<  8,
    DECREMENT               = 1UL <<  9,

    ARITHMETIC = ASSIGN | ADD | SUBTRACT | MULTIPLY | DIVIDE | MODULO 
        | UNARY_PLUS | UNARY_MINUS | INCREMENT | DECREMENT,

    // Bitwise
    BITWISE_OR              = 1UL << 10,
    BITWISE_AND             = 1UL << 11,
    BITWISE_XOR             = 1UL << 12,
    BITWISE_NOT             = 1UL << 13,
    BITWISE_LEFT_SHIFT      = 1UL << 14,
    BITWISE_RIGHT_SHIFT     = 1UL << 15,

    BITWISE = BITWISE_OR | BITWISE_AND | BITWISE_XOR | BITWISE_NOT
        | BITWISE_LEFT_SHIFT | BITWISE_RIGHT_SHIFT,

    // Comparision
    EQ_COMPARE              = 1UL << 16,
    NEQ_COMPARE             = 1UL << 17,
    GT_COMPARE              = 1UL << 18,
    LT_COMPARE              = 1UL << 19,
    GTE_COMPARE             = 1UL << 20,
    LTE_COMPARE             = 1UL << 21,

    COMPARE = EQ_COMPARE | NEQ_COMPARE | GT_COMPARE | LT_COMPARE
        | GTE_COMPARE | LTE_COMPARE,

    // Logical operators
    LOG_NOT                 = 1UL << 22,
    LOG_AND                 = 1UL << 23,
    LOG_OR                  = 1UL << 24,

    LOGICAL = LOG_NOT | LOG_AND | LOG_OR,
    
    // Member and pointer operators
    ARRAY_SUBSCRIPT         = 1UL << 25,
    INDIRECTION             = 1UL << 26,
    ADDRESS_OF              = 1UL << 27,
    STRUCT_DEREFERENCE      = 1UL << 28,
    MEMBER_PTR_DEREFERENCE  = 1UL << 29,

    // Other operators
    CALL                    = 1UL << 30,
    COMMA                   = 1UL << 31,

    ALL = 0xFFFFFFFF
};

// ============================================================================================== //
// [AbstractOperatorForwarder]                                                                    //
// ============================================================================================== //

// TODO: find a better name for this class
template<typename wrapperT, typename wrappedT>
class AbstractOperatorForwarder
{
public:
    virtual wrappedT& valueRef() = 0;
    virtual const wrappedT& valueCRef() const = 0;
    virtual ~AbstractOperatorForwarder() = default;
protected:
    using AbstractOperatorForwarder_t = AbstractOperatorForwarder<wrapperT, wrappedT>;

    static AbstractOperatorForwarder_t& base(wrapperT &ref)
    {
        return static_cast<AbstractOperatorForwarder_t&>(ref);
    }

    static const AbstractOperatorForwarder_t& base(const wrapperT &ref)
    { 
        return static_cast<const AbstractOperatorForwarder_t&>(ref);
    }
};

// ============================================================================================== //
// Operator forwarders                                                                            //
// ============================================================================================== //

#define REMODEL_FORWARD_BINARY_RVALUE_OP(op)                                                       \
     template<typename rhsT>                                                                       \
     auto operator op (const rhsT& rhs) const                                                      \
         -> decltype(std::declval<AbstractOperatorForwarder<wrapperT, wrappedT>>()                 \
            .valueCRef() op rhs)                                                                   \
     {                                                                                             \
         return this->valueCRef() op rhs;                                                          \
     }

#define REMODEL_FORWARD_BINARY_COMPOUND_ASSIGNMENT_OP(op)                                          \
     template<typename rhsT>                                                                       \
     auto operator op##= (const rhsT& rhs)                                                         \
         -> decltype(std::declval<AbstractOperatorForwarder<wrapperT, wrappedT>>()                 \
            .valueRef() op##= rhs)                                                                 \
     {                                                                                             \
         return this->valueRef() op##= rhs;                                                        \
     }

#define REMODEL_FORWARD_UNARY_RVALUE_OP(op)                                                        \
     auto operator op ()                                                                           \
         -> decltype(op std::declval<AbstractOperatorForwarder<wrapperT, wrappedT>>()              \
            .valueRef())                                                                           \
     {                                                                                             \
         return op this->valueRef();                                                               \
     }

#define REMODEL_DEF_BINARY_BITARITH_OP_FORWARDER(name, op)                                         \
    template<typename wrapperT, typename wrappedT>                                                 \
    struct name : virtual AbstractOperatorForwarder<wrapperT, wrappedT>                            \
    {                                                                                              \
        REMODEL_FORWARD_BINARY_RVALUE_OP(op)                                                       \
        REMODEL_FORWARD_BINARY_COMPOUND_ASSIGNMENT_OP(op)                                          \
    };

#define REMODEL_DEF_BINARY_OP_FORWARDER(name, op)                                                  \
    template<typename wrapperT, typename wrappedT>                                                 \
    struct name : virtual AbstractOperatorForwarder<wrapperT, wrappedT>                            \
    {                                                                                              \
        REMODEL_FORWARD_BINARY_RVALUE_OP(op)                                                       \
    };

#define REMODEL_DEF_UNARY_OP_FORWARDER(name, op)                                                   \
    template<typename wrapperT, typename wrappedT>                                                 \
    struct name : virtual AbstractOperatorForwarder<wrapperT, wrappedT>                            \
    {                                                                                              \
        REMODEL_FORWARD_UNARY_RVALUE_OP(op)                                                        \
    };

//
// Arithmetic
//

REMODEL_DEF_BINARY_BITARITH_OP_FORWARDER(Add,           + )
REMODEL_DEF_BINARY_BITARITH_OP_FORWARDER(Subtract,      - )
REMODEL_DEF_BINARY_BITARITH_OP_FORWARDER(Multiply,      * )
REMODEL_DEF_BINARY_BITARITH_OP_FORWARDER(Divide,        / )
REMODEL_DEF_BINARY_BITARITH_OP_FORWARDER(Modulo,        % )
REMODEL_DEF_UNARY_OP_FORWARDER          (UnaryPlus,     + )
REMODEL_DEF_UNARY_OP_FORWARDER          (UnaryMinus,    - )

// WARNING: does *not* perform perfect forwarding
template<typename wrapperT, typename wrappedT>
struct Assign : virtual AbstractOperatorForwarder<wrapperT, wrappedT>
{
    wrappedT& operator = (const wrappedT& rhs)
    {
        return this->valueRef() = rhs;
    }

    wrappedT& operator = (const wrapperT& rhs)
    {
        return this->valueRef() = rhs.valueRef();
    }

    //template<typename rhsT>
    //auto operator = (const rhsT& rhs)
    //    -> decltype(std::declval<AbstractOperatorForwarder<wrapperT, wrappedT>>()
    //        .valueRef() = rhs)
    //{
    //    return this->valueRef() = rhs;
    //}
};

template<typename wrapperT, typename wrappedT>
struct Increment : virtual AbstractOperatorForwarder<wrapperT, wrappedT>
{
    auto operator ++ ()
        -> decltype(++std::declval<AbstractOperatorForwarder<wrapperT, wrappedT>>()
            .valueRef())
    {
        return ++this->valueRef();
    }

    auto operator ++ (int)
        -> decltype(std::declval<AbstractOperatorForwarder<wrapperT, wrappedT>>()
            .valueRef()++)
    {
        return this->valueRef()++;
    }
};

template<typename wrapperT, typename wrappedT>
struct Decrement : virtual AbstractOperatorForwarder<wrapperT, wrappedT>
{
    auto operator -- ()
        -> decltype(--std::declval<AbstractOperatorForwarder<wrapperT, wrappedT>>()
            .valueRef())
    {
        return --this->valueRef();
    }

    auto operator -- (int)
        -> decltype(std::declval<AbstractOperatorForwarder<wrapperT, wrappedT>>()
            .valueRef()--)
    {
        return this->valueRef()--;
    }
};

//
// Bitwise
// 

REMODEL_DEF_BINARY_BITARITH_OP_FORWARDER(BitwiseOr,         | )
REMODEL_DEF_BINARY_BITARITH_OP_FORWARDER(BitwiseAnd,        & )
REMODEL_DEF_BINARY_BITARITH_OP_FORWARDER(BitweiseXor,       ^ )
REMODEL_DEF_BINARY_BITARITH_OP_FORWARDER(BitwiseLeftShift,  <<)
REMODEL_DEF_BINARY_BITARITH_OP_FORWARDER(BitwiseRightShift, >>)
REMODEL_DEF_UNARY_OP_FORWARDER          (BitwiseNot,        ~ )

//
// Comparision
// 

REMODEL_DEF_BINARY_OP_FORWARDER(EqCompare,  ==)
REMODEL_DEF_BINARY_OP_FORWARDER(NeqCompare, !=)
REMODEL_DEF_BINARY_OP_FORWARDER(GtCompare,  > )
REMODEL_DEF_BINARY_OP_FORWARDER(LtCompare,  < )
REMODEL_DEF_BINARY_OP_FORWARDER(GteCompare, >=)
REMODEL_DEF_BINARY_OP_FORWARDER(LteCompare, <=)

//
// Logical operators
//

REMODEL_DEF_BINARY_OP_FORWARDER(LogAnd, &&)
REMODEL_DEF_BINARY_OP_FORWARDER(LogOr,  ||)
REMODEL_DEF_UNARY_OP_FORWARDER (LogNot, ! )

//
// Member and pointer operators
// 

REMODEL_DEF_UNARY_OP_FORWARDER (Indirection,    * ) // TODO: test
REMODEL_DEF_UNARY_OP_FORWARDER (AddressOf,      & ) // TODO: test

// TODO: test
template<typename wrapperT, typename wrappedT>
struct StructDreference : virtual AbstractOperatorForwarder<wrapperT, wrappedT>
{
    auto operator -> ()
        ->  decltype(std::declval<AbstractOperatorForwarder<wrapperT, wrappedT>>()
            .valueRef().operator -> ())
    {
        return this->valueRef().operator -> ();
    }
};

// TODO: test
template<typename wrapperT, typename wrappedT>
struct MemberPtrDereference : virtual AbstractOperatorForwarder<wrapperT, wrappedT>
{
    template<typename rhsT>
    auto operator ->* (rhsT& ptr) 
        -> decltype(std::declval<AbstractOperatorForwarder<wrapperT, wrappedT>>()
            .valueRef().operator ->* (ptr))
    {
        return this->valueRef().operator ->* (ptr);
    }
};

// TODO: test
template<typename wrapperT, typename wrappedT>
struct ArraySubscript : virtual AbstractOperatorForwarder<wrapperT, wrappedT>
{
    template<typename rhsT>
    auto operator [] (const rhsT& rhs)
        -> decltype(std::declval<AbstractOperatorForwarder<wrapperT, wrappedT>>()
            .valueRef()[rhs])
    {
        return this->valueRef()[rhs];
    }
};

//
// Other operators
//
    
// TODO: test
template<typename wrapperT, typename wrappedT>
struct Call : virtual AbstractOperatorForwarder<wrapperT, wrappedT>
{
    template<typename... argsT>
    auto operator () (argsT... args)
        -> decltype(std::declval<AbstractOperatorForwarder<wrapperT, wrappedT>>()
            .valueRef()(args...))
    {
        return this->valueRef()(args...);
    }
};

// TODO: test
template<typename wrapperT, typename wrappedT>
struct Comma : virtual AbstractOperatorForwarder<wrapperT, wrappedT>
{
    template<typename rhsT>
    auto operator , (rhsT& rhs)
        -> decltype(std::declval<AbstractOperatorForwarder<wrapperT, wrappedT>>()
            .valueRef().operator , (rhs))
    {
        return this->valueRef().operator , (rhs);
    }  
};

// ============================================================================================== //

namespace Internal
{
    template<typename T, bool doInheritT>
    struct InheritIfHelper {};

    template<typename T>
    struct InheritIfHelper<T, true> : T {};
} // namespace Internal

template<Flags flagsT, Flags inheritFlagsT, typename T>
struct InheritIf : Internal::InheritIfHelper<T, (flagsT  & inheritFlagsT) != 0> {};

template<typename wrapperT, typename wrappedT, Flags flagsT>
struct ForwardByFlags
    : InheritIf<flagsT, ASSIGN,                   Assign               <wrapperT, wrappedT>>
    , InheritIf<flagsT, ADD,                      Add                  <wrapperT, wrappedT>>
    , InheritIf<flagsT, SUBTRACT,                 Subtract             <wrapperT, wrappedT>>
    , InheritIf<flagsT, MULTIPLY,                 Multiply             <wrapperT, wrappedT>>
    , InheritIf<flagsT, DIVIDE,                   Divide               <wrapperT, wrappedT>>
    , InheritIf<flagsT, MODULO,                   Modulo               <wrapperT, wrappedT>>
    , InheritIf<flagsT, UNARY_PLUS,               UnaryPlus            <wrapperT, wrappedT>>
    , InheritIf<flagsT, UNARY_MINUS,              UnaryMinus           <wrapperT, wrappedT>>
    , InheritIf<flagsT, INCREMENT,                Increment            <wrapperT, wrappedT>>
    , InheritIf<flagsT, DECREMENT,                Decrement            <wrapperT, wrappedT>>
    , InheritIf<flagsT, BITWISE_OR,               BitwiseOr            <wrapperT, wrappedT>>
    , InheritIf<flagsT, BITWISE_AND,              BitwiseAnd           <wrapperT, wrappedT>>
    , InheritIf<flagsT, BITWISE_XOR,              BitweiseXor          <wrapperT, wrappedT>>
    , InheritIf<flagsT, BITWISE_NOT,              BitwiseNot           <wrapperT, wrappedT>>
    , InheritIf<flagsT, BITWISE_LEFT_SHIFT,       BitwiseLeftShift     <wrapperT, wrappedT>>
    , InheritIf<flagsT, BITWISE_RIGHT_SHIFT,      BitwiseRightShift    <wrapperT, wrappedT>>
    , InheritIf<flagsT, EQ_COMPARE,               EqCompare            <wrapperT, wrappedT>>
    , InheritIf<flagsT, NEQ_COMPARE,              NeqCompare           <wrapperT, wrappedT>>
    , InheritIf<flagsT, GT_COMPARE,               GtCompare            <wrapperT, wrappedT>>
    , InheritIf<flagsT, LT_COMPARE,               LtCompare            <wrapperT, wrappedT>>
    , InheritIf<flagsT, GTE_COMPARE,              GteCompare           <wrapperT, wrappedT>>
    , InheritIf<flagsT, LTE_COMPARE,              LteCompare           <wrapperT, wrappedT>>
    , InheritIf<flagsT, LOG_NOT,                  LogNot               <wrapperT, wrappedT>>
    , InheritIf<flagsT, LOG_AND,                  LogAnd               <wrapperT, wrappedT>>
    , InheritIf<flagsT, LOG_OR,                   LogOr                <wrapperT, wrappedT>>
    , InheritIf<flagsT, ARRAY_SUBSCRIPT,          ArraySubscript       <wrapperT, wrappedT>>
    , InheritIf<flagsT, INDIRECTION,              Indirection          <wrapperT, wrappedT>>
    , InheritIf<flagsT, ADDRESS_OF,               AddressOf            <wrapperT, wrappedT>>
    , InheritIf<flagsT, STRUCT_DEREFERENCE,       StructDreference     <wrapperT, wrappedT>>
    , InheritIf<flagsT, MEMBER_PTR_DEREFERENCE,   MemberPtrDereference <wrapperT, wrappedT>>
    , InheritIf<flagsT, CALL,                     Call                 <wrapperT, wrappedT>>
    , InheritIf<flagsT, COMMA,                    Comma                <wrapperT, wrappedT>>
{
    
};

// ============================================================================================== //

// TODO: undefs

} // namespace Operators
} // namespace Remodel

#endif // _REMODEL_OPERATORS_HPP_