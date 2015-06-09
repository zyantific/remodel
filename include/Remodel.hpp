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

/**     
 * @file
 * @brief Contains the core classes of the library.
 */

// NOTE: triple-slash doxygen comments here to allow C-style comments in code samples

/// @mainpage
///          
/// remodel is a lightweight C++ library that allows creating wrappers for proprietary data 
/// structures and classes (with possibly many unknown fields) of closed source applications or 
/// network traffic avoiding padding fields or messy casts.
/// 
/// @section samples Examples
/// @subsection basic_sample Basic usage
/// 
/// Imagine a scenario where you have instances of @c Dog in memory (let's say in your 
/// dog-simulator game that you intend to write mods for) that need be accessed.
///      
/// @code
///     class CustomString
///     {
///         char *data;
///         std::size_t length;
///     public:
///         const char* str() const { return data; }
///         std::size_t size() const { return length; }
///     };
///     
///     class Dog
///     {
///         CustomString name;
///         CustomString* race;
///         // ..
///         // possibly many other unknown fields here
///         // ..
///         uint8_t age;
///         bool hatesKittehz;
///     public:
///         virtual int calculateFluffiness() const {/* ... */}
///         virtual void giveGoodie(int amount) {/* ... */}
///         // .. more methods ..
///     };
/// @endcode
///         
/// Now the remodeled version:
/// @code
///     class CustomString : public AdvancedClassWrapper<8 /* struct size in bytes */>
///     {
///         REMODEL_ADV_WRAPPER(CustomString)
///         // Note we omit the privates here because we decided we only need the methods.
///     public:
///         MemberFunction<const char* (*)()> str{this, 0x12345678 /* function addr */};
///         MemberFunction<std::size_t (*)()> size{this, 0x87654321};
///     };
///     
///     // We don't create fields referring to `Dog`, so we don't have to know its
///     // size and can simply use `ClassWrapper` rather than `AdvancedClassWrapper`.
///     class Dog : public ClassWrapper
///     {
///         REMODEL_WRAPPER(Dog)
///         // We cheat and make the private fields public for our mod.
///     public:
///         Field<CustomString> name{this, 4 /* struct offset in bytes */};
///         Field<CustomString*> race{this, 12};
///         // Note that we can just omit the unknown fields here without breaking
///         // the integrity of the struct. No padding required.
///         Field<uint8_t> age{this, 124};
///         Field<bool> hatesKittehz{this, 125};
///     public:
///         VirtualFunction<int (*)()> calculateFluffiness{this, 0 /* vftable index */};
///         VirtualFunction<void (*)(int)> giveGoodie{this, 4};
///     };
/// @endcode
///
/// And that's it! You can now use these wrappers pretty similar to how you'd use the original 
/// class.
/// @code
///     auto dog = wrapper_cast<Dog>(dogInstanceLocation);
///     // Don't give the bad dog too much of the good stuff!
///     dog.giveGoodie(dog.hatesKittehz ? 2 : 7);
///     // Year is over, +1 to age.
///     ++dog.age;
///     // What was it's race again?
///     const char* race = dog.race->toStrong().str();
/// @endcode
/// If you read the above snippet carefully, you probably came up with the question why there is
/// a @c toStrong call where you didn't expect one. When fields are created for types that are
/// wrappers themselves, the library automatically rewrites those with @c TheWrapperType::Weak
/// which is required to allow pointer and array semantics to behave correctly. These "weak" 
/// wrappers can be evolved to "strong" (normal) ones with a simple @c toStrong() call and can 
/// then be used as expected.
/// 
/// @subsection create_wrap_inst Creating wrapper instances
/// Instances of any advanced wrapper (meaning wrappers that derive from @c AdvancedClassWrapper)
/// can be created using the @c Instantiable member-type which is derived from your wrapper itself
/// and extends it with actual memory that holds the data of the actual object. In case you were
/// wondering, the @c Instantiable member-type is declared by the @c REMODEL_ADV_WRAPPER macro.
/// @code
///     struct Flea
///     {
///        // ..
///     };
///     
///     // Wrappers need to be derived from `AdvancedClassWrapper` in order to 
///     // allow instantiation.
///     class Cat : public AdvancedClassWrapper<6>
///     {
///        REMODEL_ADV_WRAPPER(Cat)
///     public:
///        enum Gender : uint8_t
///        {
///            Male,
///            Female,
///        };
///     
///        Field<uint8_t> age   {this, 0};
///        Field<Gender>  gender{this, 1};
///        Field<Flea*>   fleas {this, 2};
///     };
///
///     int main()
///     { 
///         // You can simply create instances by appending `::Instantiable` to 
///         // your wrapper type and use them just like 'normal' classes.
///         Cat::Instantiable stackCat;
///         stackCat.age = 7;
/// 
///         // Need a heap-allocated cat? No problem.
///         auto heapCat = new Cat::Instantiable;
///         heapCat->gender = Cat::Male;
///
///         return 0;
///     }
/// @endcode
///
/// @subsection cust_cdtors Custom con and destructors
///
/// It is possible to define custom routines that serve as con and destructors when instantiating
/// wrappers. When no custom routines are specified, default ones are generated that just do
/// nothing.
///
/// Let's expand the example from the previous section with this technique:
/// @code
///     struct Flea
///     {
///        // ..
///     };
///     
///     class Cat : public AdvancedClassWrapper<6>
///     {
///        REMODEL_ADV_WRAPPER(Cat)
///     public:
///        enum Gender : uint8_t
///        {
///            Male,
///            Female,
///        };
///     
///        Field<uint8_t> age   {this, 0};
///        Field<Gender>  gender{this, 1};
///        Field<Flea*>   fleas {this, 2};
///     
///        void construct() // (1)
///        {
///            age    = 0;
///            gender = Male;
///            fleas  = nullptr;
///        }
///     
///        void construct(uint8_t age, Gender gender, Flea* fleas) // (2)
///        {
///            this->age    = age;
///            this->gender = gender;
///            this->fleas  = fleas;
///        }
///     
///        void destruct()
///        {
///            if (fleas)
///            {
///                delete [] fleas;
///            }
///        }
///     };
///
///     int main()
///     { 
///         Cat::Instantiable felix{3, Cat::Male, nullptr}; // construct (2) used
///         Cat::Instantiable unknownCat;                   // construct (1) used
///         return 0;
///     }
/// @endcode
/// It is also possible to use @c Function (or derivatives) wrappers as con and destruct routines, 
/// calling the original con an destructors in memory. In this case, however, you won't be able to
/// use overloads. To work around that, you can create multiple @c MemberFunction instances and
/// call the correct one in overloaded @c construct routines.

#ifndef _REMODEL_REMODEL_HPP_
#define _REMODEL_REMODEL_HPP_

#include <functional>
#include <stdint.h>
#include <cstddef>

#include "zycore/Operators.hpp"
#include "zycore/Utils.hpp"
#include "zycore/Optional.hpp"

#include "Platform.hpp"

namespace remodel
{

namespace internal
{
    class FieldBase;
    using namespace zycore;
} // namespace internal

// We require that data-pointers are equal in size to code-pointers.
static_assert(sizeof(void(*)()) == sizeof(void*), "unsupported platform");

// ============================================================================================== //
// Base classes for wrapper classes                                                               //
// ============================================================================================== //

// ---------------------------------------------------------------------------------------------- //
// [ClassWrapper]                                                                                 //
// ---------------------------------------------------------------------------------------------- //

/**
 * @brief   Base class for class wrappers.
 */
class ClassWrapper
{
    friend class internal::FieldBase;
protected:
    void* m_raw = nullptr;

    /**
     * @internal
     * @brief   Constructor.
     * @param   raw The raw pointer of the wrapped object.
     */
    explicit ClassWrapper(void* raw)
        : m_raw{raw}
    {}
public:
    /**
     * @brief   Destructor.
     */
    virtual ~ClassWrapper() = default;

    /**
     * @brief   Copy constructor.
     * @param   other   The instance to copy from.
     */
    ClassWrapper(const ClassWrapper& other)
        : m_raw{other.m_raw}
    {}

    /**
     * @brief   Assignment operator.
     * @param   other   The instance to assign from.
     * @return  @c *this.
     */
    ClassWrapper& operator = (const ClassWrapper& other)
    {
        m_raw = other.m_raw;
        return *this;
    }

    /**
     * @brief   Address-of operator disabled to avoid confusion 
     * @return  The result of the operation.
     * Use @c addressOfObj and @c addressOfWrapper instead.
     */
    ClassWrapper& operator & () = delete;

    /**
     * @brief   Obtains a raw pointer to the wrapped object.
     * @return  The desired pointer.
     */
    void* addressOfObj() { return m_raw; }

    /**
     * @brief   Obtains a const raw pointer to the wrapped object.
     * @copydetails addressOfObj
     */
    const void* addressOfObj() const { return m_raw; }

    // addressOfWrapper is implemented in the REMODEL_WRAPPER/REMODEL_ADV_WRAPPER macro.
};

// ---------------------------------------------------------------------------------------------- //
// [AdvancedClassWrapper] + helper classes                                                        //
// ---------------------------------------------------------------------------------------------- //

namespace internal
{

/**
 * @internal
 * @brief   TMP helper type implementation that does nothing when called.
 * @tparam  WrapperT        Type of the wrapper.
 * @tparam  useCustomCtorT  @c true to call the custom constructor, @c false to do nothing.
 * @tparam  ArgsT           Constructor argument types.
 */
template<typename WrapperT, bool useCustomCtorT, typename... ArgsT>
struct InstantiableWrapperCtorCaller
{
    static void Call(WrapperT* /*thiz*/, ArgsT&&... /*args*/)
    {
        // default construction, just do nothing.
    }
};

/**
 * @internal
 * @brief   TMP helper type implementation that calls the custom constructor.
 * @copydetails InstantiableWrapperCtorCaller
 */
template<typename WrapperT, typename... ArgsT>
struct InstantiableWrapperCtorCaller<WrapperT, true, ArgsT...>
{
    static void Call(WrapperT* thiz, ArgsT&&... args)
    {
        thiz->construct(std::forward<ArgsT>(args)...);
    }
};

/**
 * @brief   TMP helper type implementation that does nothing when called.
 * @tparam  WrapperT        Type of the wrapper.
 * @tparam  useCustomDtorT  @c true to call the custom constructor, @c false to do nothing.
 */
template<typename WrapperT, bool useCustomDtorT>
struct InstantiableWrapperDtorCaller
{
    static void Call(WrapperT* /*thiz*/)
    {
        // default destruction, just do nothing.
    }
};

/**
 * @brief   TMP helper type implementation that calls the custom destructor.
 * @copydetails InstantiableWrapperDtorCaller
 */
template<typename WrapperT>
struct InstantiableWrapperDtorCaller<WrapperT, true>
{
    static void Call(WrapperT* thiz)
    {
        thiz->destruct();
    }
};

/**
 * @brief   Template making wrapper types instantiable.
 * @tparam  WrapperT    Wrapepr type.
 */
#pragma pack(push, 1)
template<typename WrapperT>
class InstantiableWrapper 
    : public WrapperT
    , public NonCopyable
{
    uint8_t m_data[WrapperT::kObjSize];

    /**
     * @internal
     * @brief   Marker type.
     */
    struct Yep {};

    /**
     * @internal
     * @brief   Marker type.
     */
    struct Nope {};

    /**
     * @internal
     * @brief   TMP tool determining if we should use default-construction for the wrapper.
     */
    class HasCustomCtor
    {
        template<typename C> static Yep  test(decltype(&C::construct));
        template<typename C> static Nope test(...                    );
    public:
        static const bool Value = std::is_same<decltype(test<WrapperT>(nullptr)), Yep>::value;
    };

    /**
     * @internal
     * @brief   TMP tool determining if we should use default-destruction for the wrapper.
     */
    class HasCustomDtor
    {
        template<typename C> static Yep  test(decltype(&C::destruct));
        template<typename C> static Nope test(...                   );
    public:
        static const bool Value = std::is_same<decltype(test<WrapperT>(nullptr)), Yep>::value;
    };
public:
    /**
     * @brief   Constructor.
     * @tparam  ArgsT   Constructor argument types.
     * @param   args    Arguments passed to the @c construct routine.
     *                          
     * If no custom @c construct routine is defined in the wrapper, an empty argument list is 
     * expected. The default @c construct routine does nothing.
     */
    template<typename... ArgsT>
    explicit InstantiableWrapper(ArgsT&&... args)
        : WrapperT{&m_data}
    {
        InstantiableWrapperCtorCaller<
            WrapperT, HasCustomCtor::Value, ArgsT...
            >::Call(this, std::forward<ArgsT>(args)...);
    }

    /**
     * @brief   Destructor.
     *          
     * If no custom @c destruct routine is defined in the wrapper, a default @c destruct routine is 
     * generated that does nothing.
     */
    ~InstantiableWrapper()
    {
        InstantiableWrapperDtorCaller<WrapperT, HasCustomDtor::Value>::Call(this);
    }
};
#pragma pack(pop)

} // namespace internal

/**
 * @brief   Advanced version of the base class for wrappers.
 * @tparam  objSizeT    The size of the wrapped class, in bytes.
 */
template<std::size_t objSizeT>
class AdvancedClassWrapper : public ClassWrapper
{
protected:
    explicit AdvancedClassWrapper(void* raw)
        : ClassWrapper{raw}
    {}
public:
    using IsAdvWrapper = void;

    static const std::size_t kObjSize = objSizeT;

    AdvancedClassWrapper(const AdvancedClassWrapper& other)
        : ClassWrapper{other}
    {}

    AdvancedClassWrapper& operator = (const AdvancedClassWrapper& other)
    { 
        this->ClassWrapper::operator = (other); 
        return *this; 
    }
};

/**
 * @internal
 * @brief   Macro forwarding constructors and implementing other wrapper logic required.
 * @param   classname   The class name of the wrapper.
 * @param   base        The wrapper base class that is derived from.
 */
#define REMODEL_WRAPPER_IMPL(classname, base)                                                      \
    protected:                                                                                     \
        template<typename WrapperT>                                                                \
        friend WrapperT remodel::wrapper_cast(void *raw);                                          \
        explicit classname(void* raw)                                                              \
            : base(raw) /* MSVC12 requires parentheses here */ {}                                  \
    public:                                                                                        \
        classname(const classname& other)                                                          \
            : base(other) /* MSVC12 requires parentheses here */ {}                                \
        classname& operator = (const classname& other)                                             \
            { this->base::operator = (other); return *this; }                                      \
        /* allow field access via -> (required to allow -> on wrapped struct fields) */            \
        classname* operator -> () { return this; }                                                 \
        classname* addressOfWrapper() { return this; }                                             \
        const classname* addressOfWrapper() const { return this; }                                 \
    private:

/**
 * @brief   Macro forwarding constructors and implementing other wrapper logic required.
 * @param   classname   The class name of the wrapper.
 * @see     ClassWrapper
 */
#define REMODEL_WRAPPER(classname)                                                                 \
    REMODEL_WRAPPER_IMPL(classname, ClassWrapper)

/**
 * @brief   Macro forwarding constructors and implementing other wrapper logic required.
 * @param   classname   The class name of the wrapper.
 * @see     AdvancedClassWrapper
 */
#define REMODEL_ADV_WRAPPER(classname)                                                             \
    REMODEL_WRAPPER_IMPL(classname, AdvancedClassWrapper)                                          \
    public:                                                                                        \
        using Instantiable = internal::InstantiableWrapper<classname>;                             \
        using Weak = WeakWrapper<classname>;                                                       \
    private:

// ============================================================================================== //
// Casting function(s) and out-of-class "operators"                                               //
// ============================================================================================== //

/**
 * @brief   Creates a wrapper from a "raw" void pointer.
 * @tparam  WrapperT    The wrapper type to create.
 * @param   raw         The raw pointer to create the wrapper from.
 * @note    The naming convention violated here because, well, casts should look this way.
 * @return  The resulting wrapper.
 */
template<typename WrapperT>
inline WrapperT wrapper_cast(void *raw)
{
    WrapperT nrvo{raw};
    return nrvo;
}

/**
 * @brief   Creates a wrapper from a "raw" pointer in @c uintptr_t representation.
 * @copydetails wrapper_cast(void*)
 */
template<typename WrapperT>
inline WrapperT wrapper_cast(uintptr_t raw)
{
    WrapperT nrvo{wrapper_cast<WrapperT>(reinterpret_cast<void*>(raw))};
    return nrvo;
}

/**
 * @brief   Obtains the address of an object wrapped by this field/wrapper.
 * @tparam  WrapperT    Type of the wrapper.
 * @param   wrapper     The wrapper.
 * @return  The address of the wrapped object.
 */
template<typename WrapperT>
inline zycore::CloneConst<WrapperT, void>* addressOfObj(WrapperT& wrapper)
{
    static_assert(std::is_base_of<ClassWrapper, WrapperT>::value
        || std::is_base_of<internal::FieldBase, WrapperT>::value,
        "addressOfObj is only supported for class-wrappers");

    return wrapper.addressOfObj();
}

/**
 * @brief   Obtains the address of a field/wrapper (NOT of the wrapped object).
 * @tparam  WrapperT    Type of the wrapper.
 * @param   wrapper     The wrapper.
 * @return  The address of the wrapper.
 */
template<typename WrapperT>
inline WrapperT* addressOfWrapper(WrapperT& wrapper)
{
    static_assert(std::is_base_of<ClassWrapper, WrapperT>::value
        || std::is_base_of<internal::FieldBase, WrapperT>::value,
        "addressOfWrapper is only supported for class-wrappers and fields");

    return wrapper.addressOfWrapper();
}

// ============================================================================================== //
// Default PtrGetter implementations                                                              //
// ============================================================================================== //

// ---------------------------------------------------------------------------------------------- //
// [OffsGetter]                                                                                   //
// ---------------------------------------------------------------------------------------------- //

/**
 * @brief   @c PtrGetter functor adding an offset to the passed raw address.
 */
class OffsGetter
{
    std::ptrdiff_t m_offs;
public:
    /**
     * @brief   Constructor.
     * @param   offs    The offset to apply to the raw pointer.
     * @todo    std::ptrdiff_t is not perfect here (consider envs using @c Global and /3GB on 
     *          windows), find a better type.
     */
    explicit OffsGetter(std::ptrdiff_t offs)
        : m_offs{offs}
    {}

    void* operator () (void* raw)
    {
        return reinterpret_cast<void*>(
            reinterpret_cast<uintptr_t>(raw) + m_offs
            );
    }
};

// ---------------------------------------------------------------------------------------------- //
// [AbsGetter]                                                                                    //
// ---------------------------------------------------------------------------------------------- //

/**
 * @brief   @c PtrGetter functor ignoring the passed raw address, always returning a fixed address.
 */
class AbsGetter
{
    void* m_ptr;
public:
    /**
     * @brief   Constructor.
     * @param   ptr The pointer to return on calls (ignoring the raw pointer).
     */
    explicit AbsGetter(void* ptr)
        : m_ptr{ptr}
    {}

    /**
     * @brief   Constructor.
     * @param   ptr The pointer to return on calls (ignoring the raw pointer) in
     *              @c uint representation.
     */
    explicit AbsGetter(uintptr_t ptr)
        : AbsGetter{reinterpret_cast<void*>(ptr)}
    {}

    void* operator () (void*)
    {
        return m_ptr;
    }
};

// ---------------------------------------------------------------------------------------------- //
// [VfTableGetter]                                                                                //
// ---------------------------------------------------------------------------------------------- //

/**
 * @brief   @c PtrGetter functor obtaining a function address using the virtual function table.
 */
class VfTableGetter
{
    std::size_t m_vftableIdx;
    std::size_t m_vftableOffset;
public:
    /**
     * @brief   Constructor.
     * @param   vftableIdx      Index of the function inside the table.
     * @param   vftableOffset   Offset of the vftable-pointer in the class.
     */
    explicit VfTableGetter(std::size_t vftableIdx, std::size_t vftableOffset = 0)
        : m_vftableIdx   {vftableIdx}
        , m_vftableOffset{vftableOffset}
    {}

    void* operator () (void* raw)
    {
        return reinterpret_cast<void*>(
            *reinterpret_cast<uintptr_t*>(
                *reinterpret_cast<uintptr_t*>(
                    reinterpret_cast<uintptr_t>(raw) + m_vftableOffset
                ) 
                + m_vftableIdx * sizeof(uintptr_t)
            )
        );
    }
};

// ============================================================================================== //
// Helper class(es) to create wrappers from raw pointers                                          //
// ============================================================================================== //

// ---------------------------------------------------------------------------------------------- //
// [WrapperPtr]                                                                                   //
// ---------------------------------------------------------------------------------------------- //

namespace internal
{

/**
 * @internal
 * @brief   Weak wrapper fall-through capturing invalid instantiations.
 * @tparam  WrapperT    Wrapper type.
 */
template<typename WrapperT, typename = void>
class WeakWrapperImpl
{
    static_assert(BlackBoxConsts<WrapperT>::kFalse,
        "WeakWrapper can only be created for AdvancedWrappers");
};

#pragma pack(push, 1)
/**
 * @internal
 * @brief   Weak wrapper implementation capturing correct instantiations.
 * @tparam  WrapperT    Wrapper type derived from @c AdvancedClassWrapper.
 */
template<typename WrapperT>
class WeakWrapperImpl<WrapperT, typename WrapperT::IsAdvWrapper /* manual SFINAE */>
{
    uint8_t m_dummy[WrapperT::kObjSize];
protected:
    /**
     * @brief   Default constructor.
     */
    WeakWrapperImpl() = default;
public:
    /**
     * @brief   Gets the raw pointer 
     * @return  The raw pointer.
     */
    void* raw() { return this; }

    /**
     * @brief   Converts this weak wrapper to a strong ("normal") one.
     * @return  A strong wrapper.
     */
    WrapperT toStrong() { return wrapper_cast<WrapperT>(this); }
};
#pragma pack(pop)
 
} // namespace internal

/**
 * @brief   Weak wrapper helper type.
 * @tparam  WrapperT    Type of the strong wrapper to create a weak wrapper for.
 *                      
 * Other than with normal wrappers, the @c this pointer of this class points to the actual object
 * which allows creation of raw pointers to weak wrappers which is useful when defining functions
 * that take pointers to wrapped types. Weak wrappers can then be evolved to strong wrappers
 * with a simple @c toStrong invocation.
 */
template<typename WrapperT>
struct WeakWrapper final : internal::WeakWrapperImpl<WrapperT> {};

// Verify assumptions about this class.
static_assert(std::is_trivial<WeakWrapper<AdvancedClassWrapper<sizeof(int)>>>::value, 
    "internal library error");
static_assert(sizeof(int) == sizeof(WeakWrapper<AdvancedClassWrapper<sizeof(int)>>), 
    "internal library error");

// ============================================================================================== //
// Abstract field object implementation                                                           //
// ============================================================================================== //

// ---------------------------------------------------------------------------------------------- //
// [FieldBase]                                                                                    //
// ---------------------------------------------------------------------------------------------- //

namespace internal
{ 

/**
 * @internal
 * @brief   Base class for all kinds of wrapper-fields.
 */
class FieldBase
{
protected:
    /**
     * @brief   Definition of the prototype for pointer-getters.
     */
    using PtrGetter = std::function<void*(void* rawBasePtr)>;

    /**
     * @brief   Constructor.
     * @param   parent      If non-null, the parent.
     * @param   ptrGetter   A @c PtrGetter calculating the actual offset of the proxied object.
     */
    FieldBase(ClassWrapper* parent, PtrGetter ptrGetter)
        : m_ptrGetter{ptrGetter}
        , m_parent{parent}
    {}

    /**
     * @brief   Deleted copy constructor.
     */
    FieldBase(const FieldBase&) = delete;

    /**
     * @brief   Deleted assignment operator.
     * @note    This is just here as an assertation so internal code cannot copy fields by 
     *          accident. The concrete implementation hides this operator in favor of an operator
     *          that copies the data wrapped by one proxy object to another (emulating normal
     *          assignment semantics).
     */
    FieldBase& operator = (const FieldBase&) = delete;

    /**
     * @brief   Gets a pointer to the parent of this field.
     * @return  The parent.
     */
    ClassWrapper* parent() { return m_parent; }

    /**
     * @brief   Gets a constant pointer to the parent of this field.
     * @return  The parent.
     */
    const ClassWrapper* parent() const { return m_parent; }

    /**
     * @brief   Gets the @c PtrGetter used for address calculation.
     * @return  The used @c PtrGetter.
     */
    const PtrGetter& ptrGetter() const { return m_ptrGetter; }

    /**
     * @brief   Obtains a pointer to the raw object using the @c PtrGetter.
     * @return  The requested pointer.
     */
    void* rawPtr()
    {
        return this->m_ptrGetter(this->m_parent ? this->m_parent->m_raw : nullptr);
    }

    /**
     * @brief   Obtains a constant pointer to the raw object using the @c PtrGetter.
     * @return  The requested pointer.
     */
    const void* crawPtr() const
    {
        return this->m_ptrGetter(this->m_parent ? this->m_parent->m_raw : nullptr);
    }
public:
    /**   
     * @brief   Destructor.
     */
    virtual ~FieldBase() = default;
protected:
    PtrGetter     m_ptrGetter;
    ClassWrapper* m_parent;
};

// ============================================================================================== //
// Concrete field object implementation                                                           //
// ============================================================================================== //

/*
 * +-----------------------------+-------------------------------------------------+-----------+
 * | type                        | example(s)                                      | supported |
 * +-----------------------------+-------------------------------------------------+-----------+
 * | arithmetic types            | int, float, bool, size_t                        | yes       |
 * | trivial structs/classes     | struct A { int b; };                            | yes       |
 * | non-trivial structs/classes | struct A { virtual void b() {} };               | no        |
 * | enums                       | enum A { B = 0 };                               | yes       |
 * | enum classes                | enum class A { B = 0 };                         | yes       |
 * | wrapped classes             | struct A : ClassWrapper { REMODEL_WRAPPER(A) }; | yes       |
 * | functions                   | void a();                                       | yes       |
 * | virtual functions           | virtual void a();                               | yes       |
 * +-----------------------------+-------------------------------------------------+-----------+
 *
 * +-----------+-----------+--------------------------------------+
 * | qualifier | supported | notes                                |
 * + ----------+-----------+--------------------------------------+
 * | *         | yes       |                                      |
 * | &         | yes       | expects implementation of ref as ptr |
 * | &&        | no        | doesn't make any sense to wrap       |
 * | []        | no        | not permitted by C++ standard        |
 * | [N]       | yes       |                                      |
 * +-----------+-----------+--------------------------------------+
 */

#define REMODEL_FIELDIMPL_FORWARD_CTORS                                                            \
    public:                                                                                        \
        FieldImpl(ClassWrapper *parent, PtrGetter ptrGetter)                                       \
            : FieldBase{parent, ptrGetter}                                                         \
        {}                                                                                         \
                                                                                                   \
        explicit FieldImpl(const FieldImpl& other)                                                 \
            : FieldBase{other}                                                                     \
        {}                                                                                         \
    private:

// ---------------------------------------------------------------------------------------------- //
// [FieldImpl] fall-through implementation                                                        //
// ---------------------------------------------------------------------------------------------- //

/**
 * @internal
 * @brief   Fall-through field implementation capturing unsupported types.
 * @tparam  T   The wrapped type.
 */
template<typename T, typename=void>
class FieldImpl
{
    static_assert(BlackBoxConsts<T>::kFalse, "this types is not supported for wrapping");
};

// ---------------------------------------------------------------------------------------------- //
// [FieldImpl] for arithmetic types + enums                                                       //
// ---------------------------------------------------------------------------------------------- //

/**
 * @internal
 * @brief   Field implementation capturing arithmetic types and enums.
 * @tparam  T   The wrapped type.
 */
template<typename T>
class FieldImpl<T, std::enable_if_t<
        std::is_arithmetic<T>::value 
        // Enum classes do not implicitly convert to int, enums do. We use that for filtering.
        || (std::is_enum<T>::value && std::is_convertible<T, int>::value)
    >>
    : public FieldBase
    , public operators::ForwardByFlags<
        FieldImpl<T>, 
        T, 
        (operators::ARITHMETIC | operators::BITWISE | operators::COMMA | operators::COMPARE) 
            & ~(std::is_floating_point<T>::value ? operators::BITWISE_NOT : 0)
            & ~(std::is_unsigned<T>::value ? operators::UNARY_MINUS : 0)
            & ~(std::is_enum<T>::value ? operators::INCREMENT | operators::DECREMENT : 0)
            & ~(std::is_same<T, bool>::value ? 
                operators::INCREMENT | operators::DECREMENT | operators::BITWISE_NOT : 0
                )
    >
{
    REMODEL_FIELDIMPL_FORWARD_CTORS
};

// ---------------------------------------------------------------------------------------------- //
// [FieldImpl] for unknown-size arrays                                                            //
// ---------------------------------------------------------------------------------------------- //

/**
 * @internal
 * @brief   Field implementation capturing unknown-size array types (disallowed).
 * @tparam  T   The wrapped type.
 * @warning Unknown-size array type wrapping is not supported, this class simply rejects 
 *          compilation using a @c static_assert on instantiation.
 */
template<typename T>
class FieldImpl<T[]>
{
    static_assert(BlackBoxConsts<T>::kFalse, 
        "unknown size array struct fields are not permitted by the standard");
};

// ---------------------------------------------------------------------------------------------- //
// [FieldImpl] for known-size arrays                                                              //
// ---------------------------------------------------------------------------------------------- //

/**
 * @internal
 * @brief   Field implementation capturing arithmetic types.
 * @tparam  T   The wrapped type.
 */
template<typename T, std::size_t N>
class FieldImpl<T[N]>
    : public FieldBase
    , public operators::ForwardByFlags<
        FieldImpl<T[N]>,
        T[N],
        operators::ARRAY_SUBSCRIPT 
            | operators::INDIRECTION 
            | operators::SUBTRACT
            | operators::ADD
            | operators::COMMA
            | (std::is_class<T>::value ? operators::STRUCT_DEREFERENCE : 0)
    >
{
    REMODEL_FIELDIMPL_FORWARD_CTORS
    static_assert(!std::is_base_of<ClassWrapper, T>::value, "internal library error");
};

// ---------------------------------------------------------------------------------------------- //
// [FieldImpl] for structs/classes                                                                //
// ---------------------------------------------------------------------------------------------- //

/**
 * @internal
 * @brief   Field implementation capturing @c class and @c struct types.
 * @tparam  T   The wrapped type.
 */
template<typename T>
class FieldImpl<T, std::enable_if_t<std::is_class<T>::value>>
    : public FieldBase
    , public operators::Comma<FieldImpl<T>, T>
{
    REMODEL_FIELDIMPL_FORWARD_CTORS
    static_assert(std::is_trivial<T>::value, "wrapping is only supported for trivial types");
    static_assert(!std::is_base_of<ClassWrapper, T>::value, "internal library error");
public:
    // C++ does not allow overloading the dot operator (yet), so we provide an -> operator behaving
    // like the dot operator instead plus a more verbose syntax using the get() function.
    T& get()                            { return this->valueRef(); }
    const T& get() const                { return this->valueCRef(); }
    T* operator -> ()                   { return &get(); }
    const T* operator -> () const       { return &get(); }
};

// ---------------------------------------------------------------------------------------------- //
// [FieldImpl] for enums/enum classes                                                             //
// ---------------------------------------------------------------------------------------------- //

/**
 * @internal
 * @brief   Field implementation capturing @c enum\ class types.
 * @tparam  T   The wrapped type.
 */
template<typename T>
class FieldImpl<T, std::enable_if_t<
        // Enum classes do not implicitly convert to int, enums do. We use that for filtering.
        std::is_enum<T>::value && !std::is_convertible<T, int>::value
    >>
    : public FieldBase
    , public operators::Comma<FieldImpl<T>, T>
{
    REMODEL_FIELDIMPL_FORWARD_CTORS
public:
    // Nothing to do here!
};

// ---------------------------------------------------------------------------------------------- //
// [FieldImpl] for pointers                                                                       //
// ---------------------------------------------------------------------------------------------- //

/**
 * @internal
 * @brief   Field implementation capturing pointers.
 * @tparam  T   The wrapped type.
 */
template<typename T>
// We capture pointers with enable_if to maintain the CV-qualifiers on the pointer itself.
class FieldImpl<T, std::enable_if_t<std::is_pointer<T>::value>>
    : public FieldBase
    , public operators::ForwardByFlags<
        FieldImpl<T>,
        T,
        operators::ARRAY_SUBSCRIPT 
            | operators::INDIRECTION 
            | operators::SUBTRACT
            | operators::ADD
            | operators::COMMA
            | (std::is_class<std::remove_pointer_t<T>>::value ? operators::STRUCT_DEREFERENCE : 0)
    >
{
    REMODEL_FIELDIMPL_FORWARD_CTORS
};

// ---------------------------------------------------------------------------------------------- //
// [FieldImpl] for rvalue-references                                                              //
// ---------------------------------------------------------------------------------------------- //

/**
 * @internal
 * @brief   Field implementation capturing rvalue-references.
 * @tparam  T   The wrapped type.
 * @warning Wrapping rvalue-references is not supported. If you shoud ever find any real-world
 *          use case for wrapping rvalue-references, feel free to contact me.
 */
template<typename T>
class FieldImpl<T&&>
{
    static_assert(BlackBoxConsts<T>::kFalse, "rvalue-reference-fields are not supported");
};

#undef REMODEL_FIELDIMPL_FORWARD_CTORS

// ---------------------------------------------------------------------------------------------- //
// [RewriteWrappers] + helper types                                                               //
// ---------------------------------------------------------------------------------------------- //

// Step 3: Implementation capturing simple wrapper types.
template<typename BaseTypeT, typename QualifierStackT, typename = void>
struct RewriteWrappersStep3
{
    static_assert(BlackBoxConsts<QualifierStackT>::kFalse,
        "using wrapped types in fields requires usage of AdvancedClassWrapper as base");
};

// Step 3: Implementation capturing advanced wrapper types (manual SFINAE).
template<typename BaseTypeT, typename QualifierStackT>
struct RewriteWrappersStep3<BaseTypeT, QualifierStackT, typename BaseTypeT::IsAdvWrapper>
{
    // Rewrite wrapper type with WrapperPtr.
    using Type = ApplyQualifierStack<
        WeakWrapper<BaseTypeT>,
        BaseTypeT, 
        QualifierStackT
        >;
};

// Step 2: Implementation capturing non-wrapper types.
template<typename BaseTypeT, typename QualifierStackT, typename=void>
struct RewriteWrappersStep2
{
    // Nothing to do, just reassemble type.
    using Type = ApplyQualifierStack<BaseTypeT, BaseTypeT, QualifierStackT>;
};

// Step 2: Implementation capturing wrapper types.
template<typename BaseTypeT, typename QualifierStackT>
struct RewriteWrappersStep2<
    BaseTypeT, 
    QualifierStackT, 
    std::enable_if_t<std::is_base_of<ClassWrapper, BaseTypeT>::value>
> : RewriteWrappersStep3<BaseTypeT, QualifierStackT> {};

/**
 * @internal
 * @brief   Rewrites wrapper types to the corresponding @c WeakWrapper type.
 */
// Step 1: Dissect type into base-type and qualifier-stack.
template<typename T>
using RewriteWrappers = typename RewriteWrappersStep2<
    typename AnalyzeQualifiers<T>::BaseType,
    typename AnalyzeQualifiers<T>::QualifierStack
>::Type;

} // namespace internal

// ============================================================================================== //
// Remodeling classes                                                                             //
// ============================================================================================== //

// ---------------------------------------------------------------------------------------------- //
// [Field]                                                                                        //
// ---------------------------------------------------------------------------------------------- //

/**
 * @brief   Class representing a field (attribute, member variable) of a wrapper class.
 * @tparam  T   The type of the field represent.
 */
template<typename T>
class Field : public internal::FieldImpl<internal::RewriteWrappers<std::remove_reference_t<T>>>
{
    using RewrittenT = internal::RewriteWrappers<std::remove_reference_t<T>>;
    using CompleteProxy = internal::FieldImpl<RewrittenT>;
    static const bool kDoExtraDref = std::is_reference<T>::value;
protected: // Implementation of AbstractOperatorForwarder
    /**
     * @brief   Obtains a reference to the wrapped object.
     * @return  The reference to the wrapped object.
     */
    RewrittenT& valueRef() override
    { 
        return *static_cast<RewrittenT*>(
            kDoExtraDref ? *reinterpret_cast<RewrittenT**>(this->rawPtr()) : this->rawPtr()
            );
    }

    /**
     * @copydoc valueRef
     */
    const RewrittenT& valueCRef() const override
    { 
        return *static_cast<const RewrittenT*>(
            kDoExtraDref 
                ? *reinterpret_cast<const RewrittenT**>(const_cast<void*>(this->crawPtr()))
                : this->crawPtr()
            );
    }
public:
    /**
     * @brief   Constructs a field from a parent and a @c PtrGetter.
     * @param   parent      The class wrapper that is the parent of this object.
     * @param   ptrGetter   The function used to calculate the final address of the wrapped field.
     * @see     Global
     * @see     Module
     */
    Field(ClassWrapper* parent, typename CompleteProxy::PtrGetter ptrGetter)
        : CompleteProxy{parent, ptrGetter}
    {}

    /**
     * @brief   Convenience constructs defaulting to an @c OffsGetter as @c ptrGetter.
     * @param   parent      The class wrapper that is the parent of this object.
     * @param   ptrGetter   The function used to calculate the final address of the wrapped field.
     * @see     Global
     * @see     Module
     */
    Field(ClassWrapper* parent, std::ptrdiff_t offset)
        : CompleteProxy{parent, OffsGetter{offset}}
    {}

    /**
     * @brief   Implicit cast to a reference to the wrapped field.
     * @return  The desired reference.
     */
    operator RewrittenT& () { return this->valueRef(); }
    
    /**
     * @brief   Implicit cast to a constant reference to the wrapped field.
     * @return  The desired reference.
     */
    operator const RewrittenT& () const { return this->valueCRef(); }    

    /**
     * @brief   Assignment operator simulating normal copy semantics for fields.
     * @param   rhs The right hand side.
     * @return  @c *this.
     */
    RewrittenT& operator = (const Field& rhs)
    {
        return this->valueRef() = rhs.valueCRef();
    }

    /**
     * @brief   Assignment operator simulating normal copy semantics for fields.
     * @param   rhs The right hand side.
     * @return  @c *this.
     */
    RewrittenT& operator = (const RewrittenT& rhs)
    {
        return this->valueRef() = rhs;
    }

    /**
     * @brief   Obtains a raw pointer to the wrapped object.
     * @return  The desired pointer.
     */
    void* addressOfObj()                     { return &this->valueRef(); }

    /**
     * @brief   Obtains a constant raw pointer to the wrapped object.
     * @return  The desired pointer.
     */
    const void* addressOfObj() const         { return &this->valueCRef(); }

    /**
     * @brief   Obtains a pointer to the wrapper object.
     * @return  @c this.
     */
    Field<T>* addressOfWrapper()             { return this; }

    /**
     * @brief   Obtains a constant pointer to the wrapper object.
     * @return  @c this.
     */
    const Field<T>* addressOfWrapper() const { return this; }
};

// ---------------------------------------------------------------------------------------------- //
// [Function]                                                                                     //
// ---------------------------------------------------------------------------------------------- //

namespace internal
{

/**
 * @internal
 * @brief   Fall-through implementation for non-fptr types.
 * @tparam  T   Invalid template argument.
 */
template<typename T> 
class FunctionImpl
{
    static_assert(BlackBoxConsts<T>::kFalse,
        "function wrappers expect a function pointer definition as template parameter");
};

/**
 * @internal
 * @brief   A macro that defines a function implementation for a given calling convention.
 * @param   callingConv The calling convention.
 */
#define REMODEL_DEF_FUNCTION(callingConv)                                                          \
    template<typename RetT, typename... ArgsT>                                                     \
    class FunctionImpl<RetT (callingConv*)(ArgsT...)>                                              \
        : public internal::FieldBase                                                               \
    {                                                                                              \
    protected:                                                                                     \
        using FunctionPtr = RetT(callingConv*)(ArgsT...);                                          \
    public:                                                                                        \
        explicit FunctionImpl(PtrGetter ptrGetter)                                                 \
            : FieldBase{nullptr, ptrGetter}                                                        \
        {}                                                                                         \
                                                                                                   \
        FunctionPtr get()                                                                          \
        {                                                                                          \
            return (FunctionPtr)this->rawPtr();                                                    \
        }                                                                                          \
                                                                                                   \
        RetT operator () (ArgsT&&... args)                                                         \
        {                                                                                          \
            return get()(std::forward<ArgsT>(args)...);                                            \
        }                                                                                          \
    }

#ifdef ZYCORE_MSVC
    REMODEL_DEF_FUNCTION(__cdecl);
    REMODEL_DEF_FUNCTION(__stdcall);
    REMODEL_DEF_FUNCTION(__thiscall);
    REMODEL_DEF_FUNCTION(__fastcall);
    REMODEL_DEF_FUNCTION(__vectorcall);
#elif defined(ZYCORE_GNUC)
    REMODEL_DEF_FUNCTION(__attribute__((cdecl)));
    //REMODEL_DEF_FUNCTION(__attribute__((stdcall)));
    //REMODEL_DEF_FUNCTION(__attribute__((fastcall)));
    //REMODEL_DEF_FUNCTION(__attribute__((thiscall)));
#endif

#undef REMODEL_DEF_FUNCTION

} // namespace internal

/**
 * @brief   Function wrapper template.
 * @tparam  T   A function pointer definition equal to the prototype of the wrapped function.
 */
template<typename T>
struct Function : internal::FunctionImpl<T>
{
    /**
     * @brief   Constructs an instance with a custom @c PtrGetter.
     * @param   ptrGetter   The @c PtrGetter to use for address calculation.
     */
    explicit Function(typename Function<T>::PtrGetter ptrGetter)
        : internal::FunctionImpl<T>(ptrGetter) // MSVC12 requires parentheses here
    {}

    /**
     * @brief   Constructs an instance from a pointer to the function in @c uint representation.
     * @param   ptrGetter   The absolute address of the function in @c uint representation.
     */
    explicit Function(uintptr_t absAddress)
        : Function{AbsGetter{absAddress}}
    {}

    /**
     * @brief   Constructs an instance from a raw pointer to the function.
     * @tparam  RetT    The return type of the wrapped function.
     * @tparam  ArgsT   The argument types of the wrapped function.
     * @param   ptr     The function pointer of the function to wrap.
     */
    template<typename RetT, typename... ArgsT>
    explicit Function(RetT(*ptr)(ArgsT...))
    // This cast magic is required because the C++ standard does not permit casting code pointers
    // into data pointers as it doesn't require those to be the same size. remodel, however, makes
    // that assumption (which is validated by a static_cast to reject unsupported platforms), so
    // we can safely bypass the restriction using an extra level of pointers.
        : Function{AbsGetter{*reinterpret_cast<void**>(&ptr)}}
    {}
};

// ---------------------------------------------------------------------------------------------- //
// [MemberFunction]                                                                               //
// ---------------------------------------------------------------------------------------------- //

namespace internal
{

/**
 * @internal
 * @brief   Fall-through implementation for non-fptr types.
 * @tparam  T   Invalid template argument.
 */
template<typename T> 
class MemberFunctionImpl
{
    static_assert(BlackBoxConsts<T>::kFalse,
        "function wrappers expect a function pointer definition as template parameter");
};

/**
 * @internal
 * @brief   A macro that defines a member-function implementation for a given calling convention.
 * @param   callingConv The calling convention.
 */
#define REMODEL_DEF_MEMBER_FUNCTION(callingConv)                                                   \
    template<typename RetT, typename... ArgsT>                                                     \
    class MemberFunctionImpl<RetT (callingConv*)(ArgsT...)>                                        \
        : public internal::FieldBase                                                               \
    {                                                                                              \
    protected:                                                                                     \
        using FunctionPtr = RetT(callingConv*)(void* thiz, ArgsT... args);                         \
    public:                                                                                        \
        MemberFunctionImpl(ClassWrapper* parent, PtrGetter ptrGetter)                              \
            : FieldBase{parent, ptrGetter}                                                         \
        {}                                                                                         \
                                                                                                   \
        FunctionPtr get()                                                                          \
        {                                                                                          \
            return (FunctionPtr)this->rawPtr();                                                    \
        }                                                                                          \
                                                                                                   \
        RetT operator () (ArgsT&&... args)                                                         \
        {                                                                                          \
            return get()(addressOfObj(*this->m_parent), std::forward<ArgsT>(args)...);             \
        }                                                                                          \
    }

#ifdef ZYCORE_MSVC
    REMODEL_DEF_MEMBER_FUNCTION(__cdecl);
    REMODEL_DEF_MEMBER_FUNCTION(__stdcall);
    REMODEL_DEF_MEMBER_FUNCTION(__thiscall);
    REMODEL_DEF_MEMBER_FUNCTION(__fastcall);
    REMODEL_DEF_MEMBER_FUNCTION(__vectorcall);
#elif defined(ZYCORE_GNUC)
    REMODEL_DEF_MEMBER_FUNCTION(__attribute__((cdecl)));
    //REMODEL_DEF_MEMBER_FUNCTION(__attribute__((stdcall)));
    //REMODEL_DEF_MEMBER_FUNCTION(__attribute__((fastcall)));
    //REMODEL_DEF_MEMBER_FUNCTION(__attribute__((thiscall)));
#endif

#undef REMODEL_DEF_MEMBER_FUNCTION

} // namespace internal

/**
 * @brief   Member function wrapper template.
 * @tparam  T   A function pointer definition equal to the prototype of the wrapped function.
 */
template<typename T>
struct MemberFunction : internal::MemberFunctionImpl<T>
{
    /**
     * @brief   Constructs an instance with a custom @c PtrGetter.
     * @param   parent      The class wrapper instance this member-function belongs to.
     * @param   ptrGetter   The @c PtrGetter to use for address calculation.
     */
    explicit MemberFunction(ClassWrapper* parent, typename MemberFunction::PtrGetter ptrGetter)
        : internal::MemberFunctionImpl<T>(parent, ptrGetter) // MSVC12 requires parentheses here
    {}

    /**
     * @brief   Constructs an instance from a pointer to the member-function in @c uint 
     *          representation.
     * @param   parent      The class wrapper instance this member-function belongs to.
     * @param   ptrGetter   A pointer to the member-function to wrap in @c uint representation.
     */
    explicit MemberFunction(ClassWrapper* parent, uintptr_t absAddress)
        : MemberFunction{parent, AbsGetter{absAddress}}
    {}

    /**
     * @brief   Constructs an instance from a raw pointer to the member-function.
     * @param   parent      The class wrapper instance this member-function belongs to.
     * @param   ptrGetter   A raw pointer to the member-function.
     */
    explicit MemberFunction(ClassWrapper* parent, void* absAddress)
        : MemberFunction{parent, AbsGetter{absAddress}}
    {}
};

// ---------------------------------------------------------------------------------------------- //
// [VirtualFunction]                                                                              //
// ---------------------------------------------------------------------------------------------- //

/**
 * @brief   Convenience wrapper around @c MemberFunction constructing from a vftable index.
 * @tparam  T   A function pointer definition equal to the prototype of the wrapped function.
 */
template<typename T>
struct VirtualFunction : MemberFunction<T>
{
    /**
     * @brief   Constructs an instance from a vftable index.
     * @param   parent          The class wrapper instance this member-function belongs to.
     * @param   vftableIdx      Index of the function inside the table.
     * @param   vftableOffset   Offset of the vftable-pointer in the class.
     */
    explicit VirtualFunction(
            ClassWrapper* parent, std::size_t vftableIdx, std::size_t vftableOffset = 0)
        : MemberFunction<T>(parent, VfTableGetter{vftableIdx, vftableOffset})
        // MSVC12 requires parentheses here
    {}
};

// ============================================================================================== //
// Classes that may be used to place objects in a global or module level space                    //
// ============================================================================================== //

// ---------------------------------------------------------------------------------------------- //
// [Global]                                                                                       //
// ---------------------------------------------------------------------------------------------- //

/**
 * @brief   Allows declaration of global variables as fields using absolute addresses.
 */
class Global
    : public ClassWrapper
    , public zycore::NonCopyable
{
    REMODEL_WRAPPER(Global)

    /**
     * @brief   Default constructor.
     */
    Global() : ClassWrapper{nullptr} {}
public:
    /**
     * @brief   Gets the instance of the singleton.
     * @return  The instance.
     */
    static Global* instance()
    {
        static Global thiz;
        return thiz.addressOfWrapper();
    }
};

// ---------------------------------------------------------------------------------------------- //
// [Module]                                                                                       //
// ---------------------------------------------------------------------------------------------- //

/**
 * @brief   Allows declaration of global variables as fields using module relative addresses.
 */
class Module : public ClassWrapper
{
    REMODEL_WRAPPER(Module)
public:
    /**
     * @brief   Gets a module by it's name (e.g. @c ntdll.dll).
     * @param   moduleName  The name of the desired module.
     * @return  If found, the module, else an empty optional.
     */
    static zycore::Optional<Module> getModule(const char* moduleName)
    {
        auto modulePtr = platform::obtainModuleHandle(moduleName);
        if (!modulePtr) return zycore::kEmpty;
        return {zycore::kInPlace, wrapper_cast<Module>(modulePtr)};
    }
};

// ============================================================================================== //

} // namespace remodel

#endif //_REMODEL_REMODEL_HPP_
