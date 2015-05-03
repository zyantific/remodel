#ifndef _REMODEL_OPERATORS_HPP_
#define _REMODEL_OPERATORS_HPP_

#include <cstdint>
#include <type_traits>
#undef min // FUUUU MS
#undef max

namespace remodel
{
namespace operators
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

    ALL                     = 0xFFFFFFFFUL,
};

// ============================================================================================== //
// [AbstractOperatorForwarder]                                                                    //
// ============================================================================================== //

// TODO: find a better name for this class
template<typename WrapperT, typename WrappedT>
class AbstractOperatorForwarder
{
public:
    virtual WrappedT& valueRef() = 0;
    virtual const WrappedT& valueCRef() const = 0;
    virtual ~AbstractOperatorForwarder() = default;
};

// ============================================================================================== //
// Operator forwarders                                                                            //
// ============================================================================================== //

#define REMODEL_FORWARD_BINARY_RVALUE_OP(op)                                                       \
     template<typename rhsT>                                                                       \
     auto operator op (const rhsT& rhs) const                                                      \
         -> decltype(std::declval<AbstractOperatorForwarder<WrapperT, WrappedT>>()                 \
            .valueCRef() op rhs)                                                                   \
     {                                                                                             \
         return this->valueCRef() op rhs;                                                          \
     }

#define REMODEL_FORWARD_BINARY_COMPOUND_ASSIGNMENT_OP(op)                                          \
     template<typename rhsT>                                                                       \
     auto operator op##= (const rhsT& rhs)                                                         \
         -> decltype(std::declval<AbstractOperatorForwarder<WrapperT, WrappedT>>()                 \
            .valueRef() op##= rhs)                                                                 \
     {                                                                                             \
         return this->valueRef() op##= rhs;                                                        \
     }

#define REMODEL_FORWARD_UNARY_RVALUE_OP(op)                                                        \
     auto operator op ()                                                                           \
         -> decltype(op std::declval<AbstractOperatorForwarder<WrapperT, WrappedT>>()              \
            .valueRef())                                                                           \
     {                                                                                             \
         return op this->valueRef();                                                               \
     }

#define REMODEL_DEF_BINARY_BITARITH_OP_FORWARDER(name, op)                                         \
    template<typename WrapperT, typename WrappedT>                                                 \
    struct name : virtual AbstractOperatorForwarder<WrapperT, WrappedT>                            \
    {                                                                                              \
        REMODEL_FORWARD_BINARY_RVALUE_OP(op)                                                       \
        REMODEL_FORWARD_BINARY_COMPOUND_ASSIGNMENT_OP(op)                                          \
    };

#define REMODEL_DEF_BINARY_OP_FORWARDER(name, op)                                                  \
    template<typename WrapperT, typename WrappedT>                                                 \
    struct name : virtual AbstractOperatorForwarder<WrapperT, WrappedT>                            \
    {                                                                                              \
        REMODEL_FORWARD_BINARY_RVALUE_OP(op)                                                       \
    };

#define REMODEL_DEF_UNARY_OP_FORWARDER(name, op)                                                   \
    template<typename WrapperT, typename WrappedT>                                                 \
    struct name : virtual AbstractOperatorForwarder<WrapperT, WrappedT>                            \
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
template<typename WrapperT, typename WrappedT>
struct Assign : virtual AbstractOperatorForwarder<WrapperT, WrappedT>
{
    WrappedT& operator = (const WrappedT& rhs)
    {
        return this->valueRef() = rhs;
    }

    WrappedT& operator = (const WrapperT& rhs)
    {
        return this->valueRef() = rhs.valueRef();
    }

    //template<typename rhsT>
    //auto operator = (const rhsT& rhs)
    //    -> decltype(std::declval<AbstractOperatorForwarder<WrapperT, WrappedT>>()
    //        .valueRef() = rhs)
    //{
    //    return this->valueRef() = rhs;
    //}
};

template<typename WrapperT, typename WrappedT>
struct Increment : virtual AbstractOperatorForwarder<WrapperT, WrappedT>
{
    auto operator ++ ()
        -> decltype(++std::declval<AbstractOperatorForwarder<WrapperT, WrappedT>>()
            .valueRef())
    {
        return ++this->valueRef();
    }

    auto operator ++ (int)
        -> decltype(std::declval<AbstractOperatorForwarder<WrapperT, WrappedT>>()
            .valueRef()++)
    {
        return this->valueRef()++;
    }
};

template<typename WrapperT, typename WrappedT>
struct Decrement : virtual AbstractOperatorForwarder<WrapperT, WrappedT>
{
    auto operator -- ()
        -> decltype(--std::declval<AbstractOperatorForwarder<WrapperT, WrappedT>>()
            .valueRef())
    {
        return --this->valueRef();
    }

    auto operator -- (int)
        -> decltype(std::declval<AbstractOperatorForwarder<WrapperT, WrappedT>>()
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
template<typename WrapperT, typename WrappedT>
struct StructDreference : virtual AbstractOperatorForwarder<WrapperT, WrappedT>
{
    auto operator -> ()
        ->  decltype(std::declval<AbstractOperatorForwarder<WrapperT, WrappedT>>()
            .valueRef().operator -> ())
    {
        return this->valueRef().operator -> ();
    }
};

// TODO: test
template<typename WrapperT, typename WrappedT>
struct MemberPtrDereference : virtual AbstractOperatorForwarder<WrapperT, WrappedT>
{
    template<typename rhsT>
    auto operator ->* (rhsT& ptr) 
        -> decltype(std::declval<AbstractOperatorForwarder<WrapperT, WrappedT>>()
            .valueRef().operator ->* (ptr))
    {
        return this->valueRef().operator ->* (ptr);
    }
};

// TODO: test
template<typename WrapperT, typename WrappedT>
struct ArraySubscript : virtual AbstractOperatorForwarder<WrapperT, WrappedT>
{
    template<typename rhsT>
    auto operator [] (const rhsT& rhs)
        -> decltype(std::declval<AbstractOperatorForwarder<WrapperT, WrappedT>>()
            .valueRef()[rhs])
    {
        return this->valueRef()[rhs];
    }
};

//
// Other operators
//
    
// TODO: test
template<typename WrapperT, typename WrappedT>
struct Call : virtual AbstractOperatorForwarder<WrapperT, WrappedT>
{
    template<typename... argsT>
    auto operator () (argsT... args)
        -> decltype(std::declval<AbstractOperatorForwarder<WrapperT, WrappedT>>()
            .valueRef()(args...))
    {
        return this->valueRef()(args...);
    }
};

// TODO: test
template<typename WrapperT, typename WrappedT>
struct Comma : virtual AbstractOperatorForwarder<WrapperT, WrappedT>
{
    template<typename rhsT>
    auto operator , (rhsT& rhs)
        -> decltype(std::declval<AbstractOperatorForwarder<WrapperT, WrappedT>>()
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

template<typename WrapperT, typename WrappedT, Flags flagsT>
struct ForwardByFlags
    : InheritIf<flagsT, ASSIGN,                   Assign               <WrapperT, WrappedT>>
    , InheritIf<flagsT, ADD,                      Add                  <WrapperT, WrappedT>>
    , InheritIf<flagsT, SUBTRACT,                 Subtract             <WrapperT, WrappedT>>
    , InheritIf<flagsT, MULTIPLY,                 Multiply             <WrapperT, WrappedT>>
    , InheritIf<flagsT, DIVIDE,                   Divide               <WrapperT, WrappedT>>
    , InheritIf<flagsT, MODULO,                   Modulo               <WrapperT, WrappedT>>
    , InheritIf<flagsT, UNARY_PLUS,               UnaryPlus            <WrapperT, WrappedT>>
    , InheritIf<flagsT, UNARY_MINUS,              UnaryMinus           <WrapperT, WrappedT>>
    , InheritIf<flagsT, INCREMENT,                Increment            <WrapperT, WrappedT>>
    , InheritIf<flagsT, DECREMENT,                Decrement            <WrapperT, WrappedT>>
    , InheritIf<flagsT, BITWISE_OR,               BitwiseOr            <WrapperT, WrappedT>>
    , InheritIf<flagsT, BITWISE_AND,              BitwiseAnd           <WrapperT, WrappedT>>
    , InheritIf<flagsT, BITWISE_XOR,              BitweiseXor          <WrapperT, WrappedT>>
    , InheritIf<flagsT, BITWISE_NOT,              BitwiseNot           <WrapperT, WrappedT>>
    , InheritIf<flagsT, BITWISE_LEFT_SHIFT,       BitwiseLeftShift     <WrapperT, WrappedT>>
    , InheritIf<flagsT, BITWISE_RIGHT_SHIFT,      BitwiseRightShift    <WrapperT, WrappedT>>
    , InheritIf<flagsT, EQ_COMPARE,               EqCompare            <WrapperT, WrappedT>>
    , InheritIf<flagsT, NEQ_COMPARE,              NeqCompare           <WrapperT, WrappedT>>
    , InheritIf<flagsT, GT_COMPARE,               GtCompare            <WrapperT, WrappedT>>
    , InheritIf<flagsT, LT_COMPARE,               LtCompare            <WrapperT, WrappedT>>
    , InheritIf<flagsT, GTE_COMPARE,              GteCompare           <WrapperT, WrappedT>>
    , InheritIf<flagsT, LTE_COMPARE,              LteCompare           <WrapperT, WrappedT>>
    , InheritIf<flagsT, LOG_NOT,                  LogNot               <WrapperT, WrappedT>>
    , InheritIf<flagsT, LOG_AND,                  LogAnd               <WrapperT, WrappedT>>
    , InheritIf<flagsT, LOG_OR,                   LogOr                <WrapperT, WrappedT>>
    , InheritIf<flagsT, ARRAY_SUBSCRIPT,          ArraySubscript       <WrapperT, WrappedT>>
    , InheritIf<flagsT, INDIRECTION,              Indirection          <WrapperT, WrappedT>>
    , InheritIf<flagsT, ADDRESS_OF,               AddressOf            <WrapperT, WrappedT>>
    , InheritIf<flagsT, STRUCT_DEREFERENCE,       StructDreference     <WrapperT, WrappedT>>
    , InheritIf<flagsT, MEMBER_PTR_DEREFERENCE,   MemberPtrDereference <WrapperT, WrappedT>>
    , InheritIf<flagsT, CALL,                     Call                 <WrapperT, WrappedT>>
    , InheritIf<flagsT, COMMA,                    Comma                <WrapperT, WrappedT>>
{
    
};

// ============================================================================================== //

// TODO: undefs

} // namespace Operators
} // namespace Remodel

#endif // _REMODEL_OPERATORS_HPP_