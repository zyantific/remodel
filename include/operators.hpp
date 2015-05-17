/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 athre0z
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _REMODEL_OPERATORS_HPP_
#define _REMODEL_OPERATORS_HPP_

#include "utils.hpp"

#include <type_traits>
#undef min // FUUUU MS
#undef max

namespace remodel
{
namespace operators
{

// ============================================================================================== //
// Constants                                                                                      //
// ============================================================================================== //

enum : Flags
{
    // Arithmetic operators
    ASSIGN                  = 1UL <<  0, ///< @see Assign
    ADD                     = 1UL <<  1, ///< @see Add
    SUBTRACT                = 1UL <<  2, ///< @see Subtract
    MULTIPLY                = 1UL <<  3, ///< @see Multiply
    DIVIDE                  = 1UL <<  4, ///< @see Divide
    MODULO                  = 1UL <<  5, ///< @see Modulo
    UNARY_PLUS              = 1UL <<  6, ///< @see UnaryPlus
    UNARY_MINUS             = 1UL <<  7, ///< @see UnaryMinus
    INCREMENT               = 1UL <<  8, ///< @see Increment
    DECREMENT               = 1UL <<  9, ///< @see Decrement

    /// All arithmetic operators.
    ARITHMETIC = ASSIGN | ADD | SUBTRACT | MULTIPLY | DIVIDE | MODULO 
        | UNARY_PLUS | UNARY_MINUS | INCREMENT | DECREMENT,

    // Bitwise operators
    BITWISE_OR              = 1UL << 10, ///< @see BitwiseOr
    BITWISE_AND             = 1UL << 11, ///< @see BitwiseAnd
    BITWISE_XOR             = 1UL << 12, ///< @see BitweiseXor
    BITWISE_NOT             = 1UL << 13, ///< @see BitwiseLeftShift
    BITWISE_LEFT_SHIFT      = 1UL << 14, ///< @see BitwiseRightShift
    BITWISE_RIGHT_SHIFT     = 1UL << 15, ///< @see BitwiseNot

    /// All bitwise operators.
    BITWISE = BITWISE_OR | BITWISE_AND | BITWISE_XOR | BITWISE_NOT
        | BITWISE_LEFT_SHIFT | BITWISE_RIGHT_SHIFT,

    // Comparision operators
    EQ_COMPARE              = 1UL << 16, ///< @see EqCompare
    NEQ_COMPARE             = 1UL << 17, ///< @see NeqCompare
    GT_COMPARE              = 1UL << 18, ///< @see GtCompare
    LT_COMPARE              = 1UL << 19, ///< @see LtCompare
    GTE_COMPARE             = 1UL << 20, ///< @see GteCompare
    LTE_COMPARE             = 1UL << 21, ///< @see LteCompare

    /// All comparision operators.
    COMPARE = EQ_COMPARE | NEQ_COMPARE | GT_COMPARE | LT_COMPARE
        | GTE_COMPARE | LTE_COMPARE,

    // Logical operators
    LOG_NOT                 = 1UL << 22, ///< @see LogAnd
    LOG_AND                 = 1UL << 23, ///< @see LogOr
    LOG_OR                  = 1UL << 24, ///< @see LogNot

    /// All logical operators.
    LOGICAL = LOG_NOT | LOG_AND | LOG_OR,
    
    // Member and pointer operators
    ARRAY_SUBSCRIPT         = 1UL << 25, ///< @see ArraySubscript
    INDIRECTION             = 1UL << 26, ///< @see Indirection
    ADDRESS_OF              = 1UL << 27, ///< @see AddressOf
    STRUCT_DEREFERENCE      = 1UL << 28, ///< @see StructDreference
    MEMBER_PTR_DEREFERENCE  = 1UL << 29, ///< @see MemberPtrDereference

    // Other operators
    CALL                    = 1UL << 30, ///< @see Call
    COMMA                   = 1UL << 31, ///< @see Comma

    /// All operators.
    ALL                     = 0xFFFFFFFFUL,
};

// ============================================================================================== //
// [Proxy]                                                                                        //
// ============================================================================================== //

// TODO: find a better name for this class
template<typename WrapperT, typename WrappedT>
class Proxy
{
public:
    virtual WrappedT& valueRef() = 0;
    virtual const WrappedT& valueCRef() const = 0;
    virtual ~Proxy() = default;
};

// ============================================================================================== //
// Operator forwarders                                                                            //
// ============================================================================================== //

#define REMODEL_FORWARD_BINARY_RVALUE_OP(op)                                                       \
     template<typename rhsT>                                                                       \
     auto operator op (const rhsT& rhs) const                                                      \
         -> decltype(std::declval<Proxy<WrapperT, WrappedT>>().valueCRef() op rhs)                 \
     {                                                                                             \
         return this->valueCRef() op rhs;                                                          \
     }

#define REMODEL_FORWARD_BINARY_COMPOUND_ASSIGNMENT_OP(op)                                          \
     template<typename rhsT>                                                                       \
     auto operator op##= (const rhsT& rhs)                                                         \
         -> decltype(std::declval<Proxy<WrapperT, WrappedT>>()                                     \
            .valueRef() op##= rhs)                                                                 \
     {                                                                                             \
         return this->valueRef() op##= rhs;                                                        \
     }

#define REMODEL_FORWARD_UNARY_RVALUE_OP(op)                                                        \
     auto operator op ()                                                                           \
         -> decltype(op std::declval<Proxy<WrapperT, WrappedT>>()                                  \
            .valueRef())                                                                           \
     {                                                                                             \
         return op this->valueRef();                                                               \
     }

#define REMODEL_DEF_BINARY_BITARITH_OP_FORWARDER(name, op)                                         \
    template<typename WrapperT, typename WrappedT>                                                 \
    struct name : virtual Proxy<WrapperT, WrappedT>                                                \
    {                                                                                              \
        REMODEL_FORWARD_BINARY_RVALUE_OP(op)                                                       \
        REMODEL_FORWARD_BINARY_COMPOUND_ASSIGNMENT_OP(op)                                          \
    };

#define REMODEL_DEF_BINARY_OP_FORWARDER(name, op)                                                  \
    template<typename WrapperT, typename WrappedT>                                                 \
    struct name : virtual Proxy<WrapperT, WrappedT>                                                \
    {                                                                                              \
        REMODEL_FORWARD_BINARY_RVALUE_OP(op)                                                       \
    };

#define REMODEL_DEF_UNARY_OP_FORWARDER(name, op)                                                   \
    template<typename WrapperT, typename WrappedT>                                                 \
    struct name : virtual Proxy<WrapperT, WrappedT>                                                \
    {                                                                                              \
        REMODEL_FORWARD_UNARY_RVALUE_OP(op)                                                        \
    };

// ---------------------------------------------------------------------------------------------- //
// Arithmetic operators                                                                           //
// ---------------------------------------------------------------------------------------------- //

/**
 * @brief   Addition operator (+) forwarder.
 */
REMODEL_DEF_BINARY_BITARITH_OP_FORWARDER(Add,           + )
/**
 * @brief   Subtraction operator (-) forwarder.
 */
REMODEL_DEF_BINARY_BITARITH_OP_FORWARDER(Subtract,      - )
/**
 * @brief   Multiplication operator (*) forwarder.
 */
REMODEL_DEF_BINARY_BITARITH_OP_FORWARDER(Multiply,      * )
/**
 * @brief   Division operator (/) forwarder.
 */
REMODEL_DEF_BINARY_BITARITH_OP_FORWARDER(Divide,        / )
/**
 * @brief   Modulo operator (%) forwarder.
 */
REMODEL_DEF_BINARY_BITARITH_OP_FORWARDER(Modulo,        % )
/**
 * @brief   Unary plus operator (+x) forwarder.
 */
REMODEL_DEF_UNARY_OP_FORWARDER          (UnaryPlus,     + )
/**
 * @brief   Unary minus operator (-x) forwarder.
 */
REMODEL_DEF_UNARY_OP_FORWARDER          (UnaryMinus,    - )

// Implementing a generic assignment operator mix-in is impossible because templated 
// assignment operators do not suppress generation of the default copy assignment operator.
// The operator is implemented here anyways for completeness.

/**
 * @brief   Assignment operator (=) forwarder.
 */
template<typename WrapperT, typename WrappedT>
struct Assign : virtual Proxy<WrapperT, WrappedT>
{
    template<typename rhsT>
    auto operator = (const rhsT& rhs)
        -> decltype(std::declval<Proxy<WrapperT, WrappedT>>()
            .valueRef() = rhs)
    {
        return this->valueRef() = rhs;
    }
};

/**
 * @brief   Incrementation operator (++x, x++) forwarder.
 */
template<typename WrapperT, typename WrappedT>
struct Increment : virtual Proxy<WrapperT, WrappedT>
{
    auto operator ++ ()
        -> decltype(++std::declval<Proxy<WrapperT, WrappedT>>()
            .valueRef())
    {
        return ++this->valueRef();
    }

    auto operator ++ (int)
        -> decltype(std::declval<Proxy<WrapperT, WrappedT>>()
            .valueRef()++)
    {
        return this->valueRef()++;
    }
};

/**
 * @brief   Decrementation operator (--x, x--) forwarder.
 */
template<typename WrapperT, typename WrappedT>
struct Decrement : virtual Proxy<WrapperT, WrappedT>
{
    auto operator -- ()
        -> decltype(--std::declval<Proxy<WrapperT, WrappedT>>()
            .valueRef())
    {
        return --this->valueRef();
    }

    auto operator -- (int)
        -> decltype(std::declval<Proxy<WrapperT, WrappedT>>()
            .valueRef()--)
    {
        return this->valueRef()--;
    }
};

// ---------------------------------------------------------------------------------------------- //
// Bitwise                                                                                        //
// ---------------------------------------------------------------------------------------------- //

/**
 * @brief   Bitwise OR operator (|) forwarder.
 */
REMODEL_DEF_BINARY_BITARITH_OP_FORWARDER(BitwiseOr,         | )
/**
 * @brief   Bitwise AND operator (&) forwarder.
 */
REMODEL_DEF_BINARY_BITARITH_OP_FORWARDER(BitwiseAnd,        & )
/**
 * @brief   Bitwise XOR operator (^) forwarder.
 */
REMODEL_DEF_BINARY_BITARITH_OP_FORWARDER(BitweiseXor,       ^ )
/**
 * @brief   Bitwise left shift operator (<<) forwarder.
 */
REMODEL_DEF_BINARY_BITARITH_OP_FORWARDER(BitwiseLeftShift,  <<)
/**
 * @brief   Bitwise right shift operator (>>) forwarder.
 */
REMODEL_DEF_BINARY_BITARITH_OP_FORWARDER(BitwiseRightShift, >>)
/**
 * @brief   Bitwise NOT operator (~) forwarder.
 */
REMODEL_DEF_UNARY_OP_FORWARDER          (BitwiseNot,        ~ )

// ---------------------------------------------------------------------------------------------- //
// Comparision                                                                                    //
// ---------------------------------------------------------------------------------------------- //

/**
 * @brief  Equality comparison operator (==) forwarder.
 */
REMODEL_DEF_BINARY_OP_FORWARDER(EqCompare,  ==)
/**
 * @brief  Inequality comparison operator (!=) forwarder.
 */
REMODEL_DEF_BINARY_OP_FORWARDER(NeqCompare, !=)
/**
 * @brief  Greater than comparison operator (>) forwarder.
 */
REMODEL_DEF_BINARY_OP_FORWARDER(GtCompare,  > )
/**
 * @brief  Less than comparison operator (<) forwarder.
 */
REMODEL_DEF_BINARY_OP_FORWARDER(LtCompare,  < )
/**
 * @brief Greater than equal comparison operator (>=) forwarder.
 */
REMODEL_DEF_BINARY_OP_FORWARDER(GteCompare, >=)
/**
 * @brief Less than equal comparison operator (<=) forwarder.
 */
REMODEL_DEF_BINARY_OP_FORWARDER(LteCompare, <=)

// ---------------------------------------------------------------------------------------------- //
// Logical operators                                                                              //
// ---------------------------------------------------------------------------------------------- //

/**
 * @brief Logical AND operator (&&) forwarder.
 */
REMODEL_DEF_BINARY_OP_FORWARDER(LogAnd, &&)
/**
 * @brief Logical OR operator (||) forwarder.
 */
REMODEL_DEF_BINARY_OP_FORWARDER(LogOr,  ||)
/**
 * @brief Logical NOT operator (!) forwarder.
 */
REMODEL_DEF_UNARY_OP_FORWARDER (LogNot, ! )

// ---------------------------------------------------------------------------------------------- //
// Member and pointer operators                                                                   //
// ---------------------------------------------------------------------------------------------- //

/**
 * @brief Indirection operator (*pointer) forwarder.
 */
REMODEL_DEF_UNARY_OP_FORWARDER (Indirection,    * ) // TODO: test case
/**
 * @brief Address-of operator (&lvalue) forwarder.
 */
REMODEL_DEF_UNARY_OP_FORWARDER (AddressOf,      & ) // TODO: test case

/**
 * @brief Struct dereference operator (->) forwarder.
 */
// TODO: test case
template<typename WrapperT, typename WrappedT>
struct StructDreference : virtual Proxy<WrapperT, WrappedT>
{
    auto operator -> ()
        -> std::decay_t<
            decltype(std::declval<Proxy<WrapperT, WrappedT>>().valueRef())
        >
    {
        return this->valueRef();
    }
};

/**
 * @brief Member pointer dereference operator (->*) forwarder.
 */
// TODO: test case
template<typename WrapperT, typename WrappedT>
struct MemberPtrDereference : virtual Proxy<WrapperT, WrappedT>
{
    template<typename rhsT>
    auto operator ->* (rhsT& ptr) 
        -> decltype(std::declval<Proxy<WrapperT, WrappedT>>()
            .valueRef().operator ->* (ptr))
    {
        return this->valueRef().operator ->* (ptr);
    }
};

/**
 * @brief Array subscript operator ([]) forwarder.
 */
template<typename WrapperT, typename WrappedT>
struct ArraySubscript : virtual Proxy<WrapperT, WrappedT>
{
    template<typename rhsT>
    auto operator [] (const rhsT& rhs)
        -> decltype(std::declval<Proxy<WrapperT, WrappedT>>()
            .valueRef()[rhs])
    {
        return this->valueRef()[rhs];
    }
};

// ---------------------------------------------------------------------------------------------- //
// Other operators                                                                                //
// ---------------------------------------------------------------------------------------------- //

/**
 * @brief Call operator (obj()) forwarder.
 */
// TODO: test case
template<typename WrapperT, typename WrappedT>
struct Call : virtual Proxy<WrapperT, WrappedT>
{
    template<typename... argsT>
    auto operator () (argsT... args)
        -> decltype(std::declval<Proxy<WrapperT, WrappedT>>()
            .valueRef()(args...))
    {
        return this->valueRef()(args...);
    }
};

/**
 * @brief Comma operator (,) forwarder.
 */
// TODO: test case
template<typename WrapperT, typename WrappedT>
struct Comma : virtual Proxy<WrapperT, WrappedT>
{
    template<typename rhsT>
    auto operator , (rhsT& rhs)
        -> decltype(std::declval<Proxy<WrapperT, WrappedT>>()
            .valueRef() , rhs)
    {
        return this->valueRef() , rhs;
    }  
};

// ============================================================================================== //
// Convenience class(es)                                                                          //
// ============================================================================================== //

// ---------------------------------------------------------------------------------------------- //
// [ForwardByFlags]                                                                               //
// ---------------------------------------------------------------------------------------------- //

using utils::InheritIfFlags;

/**
 * @brief   Forwards operators depending selected by flags.
 * @tparam  WrapperT    Type of the wrapper.
 * @tparam  WrappedT    Type of the wrapped object.
 * @tparam  flagsT      Flags describing the operators to forward.
 */
template<typename WrapperT, typename WrappedT, Flags flagsT>
struct ForwardByFlags
    : InheritIfFlags<flagsT, ASSIGN,                   Assign               <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, ADD,                      Add                  <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, SUBTRACT,                 Subtract             <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, MULTIPLY,                 Multiply             <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, DIVIDE,                   Divide               <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, MODULO,                   Modulo               <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, UNARY_PLUS,               UnaryPlus            <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, UNARY_MINUS,              UnaryMinus           <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, INCREMENT,                Increment            <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, DECREMENT,                Decrement            <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, BITWISE_OR,               BitwiseOr            <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, BITWISE_AND,              BitwiseAnd           <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, BITWISE_XOR,              BitweiseXor          <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, BITWISE_NOT,              BitwiseNot           <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, BITWISE_LEFT_SHIFT,       BitwiseLeftShift     <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, BITWISE_RIGHT_SHIFT,      BitwiseRightShift    <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, EQ_COMPARE,               EqCompare            <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, NEQ_COMPARE,              NeqCompare           <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, GT_COMPARE,               GtCompare            <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, LT_COMPARE,               LtCompare            <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, GTE_COMPARE,              GteCompare           <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, LTE_COMPARE,              LteCompare           <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, LOG_NOT,                  LogNot               <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, LOG_AND,                  LogAnd               <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, LOG_OR,                   LogOr                <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, ARRAY_SUBSCRIPT,          ArraySubscript       <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, INDIRECTION,              Indirection          <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, ADDRESS_OF,               AddressOf            <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, STRUCT_DEREFERENCE,       StructDreference     <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, MEMBER_PTR_DEREFERENCE,   MemberPtrDereference <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, CALL,                     Call                 <WrapperT, WrappedT>>
    , InheritIfFlags<flagsT, COMMA,                    Comma                <WrapperT, WrappedT>>
{
    
};

// ============================================================================================== //

#undef REMODEL_DEF_UNARY_OP_FORWARDER
#undef REMODEL_DEF_BINARY_OP_FORWARDER
#undef REMODEL_DEF_BINARY_BITARITH_OP_FORWARDER
#undef REMODEL_FORWARD_UNARY_RVALUE_OP
#undef REMODEL_FORWARD_BINARY_COMPOUND_ASSIGNMENT_OP
#undef REMODEL_FORWARD_BINARY_RVALUE_OP

} // namespace operators
} // namespace remodel

#endif // _REMODEL_OPERATORS_HPP_
