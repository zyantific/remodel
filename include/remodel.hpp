#ifndef _REMODEL_REMODEL_HPP_
#define _REMODEL_REMODEL_HPP_

#include <functional>

#include "utils.hpp"
#include "operators.hpp"
#include "config.hpp"
#include "platform.hpp"

namespace remodel
{

namespace internal
{
    class ProxyImplBase;
} // namespace internal

// ============================================================================================== //
// Base classes for wrapper classes                                                               //
// ============================================================================================== //

// ---------------------------------------------------------------------------------------------- //
// [ClassWrapper]                                                                                 //
// ---------------------------------------------------------------------------------------------- //

class ClassWrapper
{
    friend class internal::ProxyImplBase;
protected:
    void* m_raw = nullptr;

    explicit ClassWrapper(void* raw)
        : m_raw(raw)
    {
        
    }
public:
    virtual ~ClassWrapper() = default;

    ClassWrapper(const ClassWrapper& other)
        : m_raw(other.m_raw)
    {
        
    }

    ClassWrapper& operator = (const ClassWrapper& other)
    {
        m_raw = other.m_raw;
        return *this;
    }

    // Disable address-of operator to avoid confusion 
    // (use addressOfObj/addressOfWrapper instead)
    ClassWrapper& operator & () = delete;

    void* addressOfObj() { return m_raw; }
    const void* addressOfObj() const { return m_raw; }
};

// ---------------------------------------------------------------------------------------------- //
// [AdvancedClassWrapper]                                                                         //
// ---------------------------------------------------------------------------------------------- //

template<std::size_t objSizeT>
class AdvancedClassWrapper : public ClassWrapper
{
protected:
    template<typename WrapperT>
    friend WrapperT wrapper_cast(void* raw);

    explicit AdvancedClassWrapper(void* raw)
        : ClassWrapper(raw)
    {
        
    }
public:
    using ObjSize = std::size_t;
    enum : ObjSize { objSize = objSizeT };

    AdvancedClassWrapper(const AdvancedClassWrapper& other)
        : ClassWrapper(other)
    {
        
    }

    AdvancedClassWrapper& operator = (const AdvancedClassWrapper& other)
    { 
        this->ClassWrapper::operator = (other); 
        return *this; 
    }
};

// Macro forwarding constructors and implementing other wrapper logic required.
#define REMODEL_WRAPPER_IMPL(classname, base)                                                      \
    protected:                                                                                     \
        template<typename WrapperT>                                                                \
        friend WrapperT remodel::wrapper_cast(void *raw);                                          \
        explicit classname(void* raw)                                                              \
            : base(raw) {}                                                                         \
    public:                                                                                        \
        classname(const classname& other)                                                          \
            : base(other) {}                                                                       \
        classname& operator = (const classname& other)                                             \
            { this->base::operator = (other); return *this; }                                      \
        /* allow field access via -> (required to allow -> on wrapped struct fields) */            \
        classname* operator -> () { return this; }                                                 \
        classname* addressOfWrapper() { return this; }                                             \
        const classname* addressOfWrapper() const { return this; }                                 \
    private:

#define REMODEL_WRAPPER(classname) REMODEL_WRAPPER_IMPL(classname, ClassWrapper)
#define REMODEL_ADV_WRAPPER(classname) REMODEL_WRAPPER_IMPL(classname, AdvancedClassWrapper)

// ============================================================================================== //
// Casting function(s) and out-of-class "operators"                                               //
// ============================================================================================== //

// Naming convention violated here because, well, casts should look this way.
template<typename WrapperT>
inline WrapperT wrapper_cast(void *raw)
{
    WrapperT nrvo{raw};
    return nrvo;
}

template<typename WrapperT>
inline utils::CloneConstType<WrapperT, void>* addressOfObj(WrapperT& wrapper)
{
    static_assert(std::is_base_of<ClassWrapper, WrapperT>::value,
        "addressOfObj is only supported for class-wrappers");

    return wrapper.addressOfObj();
}

template<typename WrapperT>
inline WrapperT* addressOfWrapper(WrapperT& wrapper)
{
    static_assert(std::is_base_of<ClassWrapper, WrapperT>::value,
        "addressOfWrapper is only supported for class-wrappers and fields");

    return wrapper.addressOfWrapper();
}

// ============================================================================================== //
// Default PtrGetter implementations                                                              //
// ============================================================================================== //

// ---------------------------------------------------------------------------------------------- //
// [OffsGetter]                                                                                   //
// ---------------------------------------------------------------------------------------------- //

class OffsGetter
{
    ptrdiff_t m_offs;
public:
    // TODO: ptrdiff_t is not perfect here (consider using @c Global and /3GB on windows),
    //       find a better type
    explicit OffsGetter(ptrdiff_t offs)
        : m_offs(offs)
    {
        
    }

    void* operator () (void* raw)
    {
        return reinterpret_cast<void*>(
            reinterpret_cast<uintptr_t>(raw) + m_offs
            );
    }
};

// ---------------------------------------------------------------------------------------------- //
// [VFtableGetter]                                                                                //
// ---------------------------------------------------------------------------------------------- //

class VFTableGetter
{
    std::size_t m_vftableIdx;
public:
    explicit VFTableGetter(std::size_t vftableIdx)
        : m_vftableIdx(vftableIdx)
    {}

    void* operator () (void* raw)
    {
        return reinterpret_cast<void*>(
            *reinterpret_cast<uintptr_t*>(
                *reinterpret_cast<uintptr_t*>(raw) + m_vftableIdx * sizeof(uintptr_t)
                )
            );
    }
};

// ============================================================================================== //
// Helper class(es) helping to create wrappers from raw pointers                                  //
// ============================================================================================== //

// ---------------------------------------------------------------------------------------------- //
// [WrapperPtr]                                                                                   //
// ---------------------------------------------------------------------------------------------- //

// TODO: find a better name (WeakWrapper?)

namespace internal
{

template<typename WrapperT, typename=std::size_t>
class WrapperPtrImpl
{
    static_assert(utils::BlackBoxConsts<WrapperT>::false_,
        "WrapperPtrs can only be created for AdvancedWrappers");
};

#pragma pack(push, 1)
template<typename WrapperT>
class WrapperPtrImpl<WrapperT, typename WrapperT::ObjSize /* manual SFINAE */>
{
    uint8_t m_dummy[WrapperT::objSize];
protected:
    WrapperPtrImpl() = default;
public:
    void* raw()         { return this; }
    WrapperT toStrong() { return wrapper_cast<WrapperT>(this); }
};
#pragma pack(pop)
 
} // namespace internal


template<typename WrapperT>
struct WrapperPtr final : internal::WrapperPtrImpl<WrapperT> {};

// Verify assumptions about this class.
static_assert(std::is_trivial<WrapperPtr<AdvancedClassWrapper<sizeof(int)>>>::value, 
    "internal library error");
static_assert(sizeof(int) == sizeof(WrapperPtr<AdvancedClassWrapper<sizeof(int)>>), 
    "internal library error");

// ============================================================================================== //
// Abstract proxy object implementation                                                           //
// ============================================================================================== //

// ---------------------------------------------------------------------------------------------- //
// [ProxyImplBase]                                                                                //
// ---------------------------------------------------------------------------------------------- //

namespace internal
{ 

class ProxyImplBase
{
protected:
    using PtrGetter = std::function<void*(void* rawBasePtr)>;

    PtrGetter m_ptrGetter;
    ClassWrapper *m_parent;

    ProxyImplBase(ClassWrapper *parent, PtrGetter ptrGetter)
        : m_ptrGetter(ptrGetter)
        , m_parent(parent)
    {
        
    }

    ProxyImplBase(const ProxyImplBase&) = delete;
    ProxyImplBase& operator = (const ProxyImplBase&) = delete;

    void* rawPtr()              { return this->m_ptrGetter(this->m_parent->m_raw); }
    const void* crawPtr() const { return this->m_ptrGetter(this->m_parent->m_raw); }
public:
    virtual ~ProxyImplBase() = default;

    // Disable address-of operator to avoid confusion.
    // TODO: add functions to get address of fields
    ProxyImplBase* operator & () = delete;
};

// ============================================================================================== //
// Concrete proxy object implementation                                                           //
// ============================================================================================== //

/*
 * +-----------+--------+--------------+--------------+
 * | qualifier | has CV | POD sup.     | wrapper sup. |
 * + ----------+--------+--------------+--------------+
 * | <none>    | yes    | yes          | yes          |
 * | *         | yes    | yes          | yes          |
 * | &         | no     | not yet      | not yet      |
 * | &&        | no     | no           | no           |
 * | []        | no     | no           | no           |
 * | [N]       | no     | yes          | yes          |
 * +-----------+--------+--------------+--------------+
 * 
 * has CV       = can additionally be qualified with CV
 * POD sup.     = supported for wrapping on plain (POD) types
 * wrapper sup. = supported for wrapping on wrapper types (e.g. Field<MyWrapper*>)
 */

#define REMODEL_PROXY_FORWARD_CTORS                                                                \
    public:                                                                                        \
        Proxy(ClassWrapper *parent, PtrGetter ptrGetter)                                           \
            : ProxyImplBase(parent, ptrGetter)                                                     \
        {}                                                                                         \
                                                                                                   \
        explicit Proxy(const Proxy& other)                                                         \
            : ProxyImplBase(other)                                                                 \
        {}                                                                                         \
    private:

// ---------------------------------------------------------------------------------------------- //
// [Proxy] fall-through implementation                                                            //
// ---------------------------------------------------------------------------------------------- //

template<typename T, typename=void>
class Proxy
{
    static_assert(utils::BlackBoxConsts<T>::false_, "this types is not supported for wrapping");
};

// ---------------------------------------------------------------------------------------------- //
// [Proxy] for arithmetic types                                                                   //
// ---------------------------------------------------------------------------------------------- //

template<typename T>
class Proxy<T, std::enable_if_t<std::is_arithmetic<T>::value>>
    : public ProxyImplBase
    , public operators::ForwardByFlags<
        Proxy<T>, 
        T, 
        (operators::ARITHMETIC | operators::BITWISE) 
            & ~(std::is_floating_point<T>::value ? operators::BITWISE_NOT : 0)
            & ~(std::is_unsigned<T>::value ? operators::UNARY_MINUS : 0)
    >
{
    REMODEL_PROXY_FORWARD_CTORS
protected: // Implementation of AbstractOperatorForwarder
    T& valueRef() override              { return *static_cast<T*>(this->rawPtr()); }
    const T& valueCRef() const override { return *static_cast<const T*>(this->crawPtr()); }
};

// ---------------------------------------------------------------------------------------------- //
// [Proxy] for unknown-size arrays                                                                //
// ---------------------------------------------------------------------------------------------- //

template<typename T>
class Proxy<T[]>
{
    static_assert(utils::BlackBoxConsts<T>::false_, 
        "unknown size array struct fields are not permitted by the standard");
};

// ---------------------------------------------------------------------------------------------- //
// [Proxy] for known-size arrays                                                                  //
// ---------------------------------------------------------------------------------------------- //

template<typename T, std::size_t N>
class Proxy<T[N]>
    : public ProxyImplBase
    , public operators::ForwardByFlags<
        Proxy<T[N]>,
        T[N],
        operators::ARRAY_SUBSCRIPT 
            | operators::INDIRECTION 
            | operators::SUBTRACT
            | operators::ADD
            | (std::is_class<T>::value ? operators::STRUCT_DEREFERENCE : 0)
    >
{
    REMODEL_PROXY_FORWARD_CTORS
    static_assert(!std::is_base_of<ClassWrapper, T>::value, "internal library error");
protected: // Implementation of AbstractOperatorForwarder
    using TN = T[N];
    TN& valueRef() override              { return *static_cast<TN*>(this->rawPtr()); }
    const TN& valueCRef() const override { return *static_cast<const TN*>(this->crawPtr()); }
};

// ---------------------------------------------------------------------------------------------- //
// [Proxy] for structs/classes                                                                    //
// ---------------------------------------------------------------------------------------------- //

template<typename T>
class Proxy<T, std::enable_if_t<std::is_class<T>::value>>
    : public ProxyImplBase
    , public operators::AbstractOperatorForwarder<Proxy<T>, T>
{
    REMODEL_PROXY_FORWARD_CTORS
    static_assert(!std::is_base_of<ClassWrapper, T>::value, "internal library error");
protected: // Implementation of AbstractOperatorForwarder
    T& valueRef() override              { return *static_cast<T*>(this->rawPtr()); }
    const T& valueCRef() const override { return *static_cast<const T*>(this->crawPtr()); }
public:
    // C++ does not allow overloading the dot operator, so we provide an -> operator behaving
    // like the dot operator instead plus a more verbose syntax using the get() function.
    T& get()                            { return this->valueRef(); }
    const T& get() const                { return this->valueCRef(); }
    T* operator -> ()                   { return &get(); }
    const T* operator -> () const       { return &get(); }
};

// ---------------------------------------------------------------------------------------------- //
// [Proxy] for pointers                                                                           //
// ---------------------------------------------------------------------------------------------- //

// We capture pointers with enable_if rather than T* to maintain the CV-qualifiers on the pointer
// itself.
template<typename T>
class Proxy<T, std::enable_if_t<std::is_pointer<T>::value>>
    : public ProxyImplBase
    , public operators::ForwardByFlags<
        Proxy<T>,
        T,
        operators::ARRAY_SUBSCRIPT 
            | operators::INDIRECTION 
            | operators::SUBTRACT
            | operators::ADD
            | (std::is_class<std::remove_pointer_t<T>>::value ? operators::STRUCT_DEREFERENCE : 0)
    >
{
    REMODEL_PROXY_FORWARD_CTORS
protected: // Implementation of AbstractOperatorForwarder
    T& valueRef() override              { return *static_cast<T*>(this->rawPtr()); }
    const T& valueCRef() const override { return *static_cast<const T*>(this->crawPtr()); }
};

// ---------------------------------------------------------------------------------------------- //
// [Proxy] for lvalue-references                                                                  //
// ---------------------------------------------------------------------------------------------- //

template<typename T>
class Proxy<T&>
{
    static_assert(utils::BlackBoxConsts<T>::false_, "reference-fields are not supported");
};

// ---------------------------------------------------------------------------------------------- //
// [Proxy] for rvalue-references                                                                  //
// ---------------------------------------------------------------------------------------------- //

template<typename T>
class Proxy<T&&>
{
    static_assert(utils::BlackBoxConsts<T>::false_, "rvalue-reference-fields are not supported");
};

#undef REMODEL_PROXY_FORWARD_CTORS

// ---------------------------------------------------------------------------------------------- //
// [RewriteWrappers]                                                                              //
// ---------------------------------------------------------------------------------------------- //

// Step 2: Implementation capturing simple wrapper types.
template<typename BaseTypeT, typename QualifierStackT, typename=std::size_t>
struct RewriteWrappersStep2
{
    static_assert(utils::BlackBoxConsts<QualifierStackT>::false_,
        "using wrapped types in fields requires usage of AdvancedClassWrapper as base");
};

// Step 2: Implementation capturing advanced wrapper types (manual SFINAE).
template<typename BaseTypeT, typename QualifierStackT>
struct RewriteWrappersStep2<BaseTypeT, QualifierStackT, typename BaseTypeT::ObjSize>
{
    // Rewrite wrapper type with WrapperPtr.
    using Type = utils::ApplyQualifierStackType<
        WrapperPtr<BaseTypeT>,
        BaseTypeT, 
        QualifierStackT
    >;
};

// Step 1: Implementation capturing non-wrapper types.
template<typename BaseTypeT, typename QualifierStackT, typename=void>
struct RewriteWrappersStep1
{
    // Nothing to do, just reassemble type.
    using Type = utils::ApplyQualifierStackType<BaseTypeT, BaseTypeT, QualifierStackT>;
};

// Step 1: Implementation capturing wrapper types.
template<typename BaseTypeT, typename QualifierStackT>
struct RewriteWrappersStep1<
    BaseTypeT, 
    QualifierStackT, 
    std::enable_if_t<std::is_base_of<ClassWrapper, BaseTypeT>::value>
> : RewriteWrappersStep2<BaseTypeT, QualifierStackT> {};

template<typename T>
struct RewriteWrappers : RewriteWrappersStep1<
    typename utils::AnalyzeQualifiers<T>::BaseType,
    typename utils::AnalyzeQualifiers<T>::QualifierStack
> {};

template<typename T>
using RewriteWrappersType = typename RewriteWrappers<T>::Type;

} // namespace internal

// ============================================================================================== //
// Remodeling classes                                                                             //
// ============================================================================================== //

// ---------------------------------------------------------------------------------------------- //
// [Field]                                                                                        //
// ---------------------------------------------------------------------------------------------- //

template<typename T>
class Field : public internal::Proxy<internal::RewriteWrappersType<T>>
{
    using RewrittenT = internal::RewriteWrappersType<T>;
    using CompleteProxy = internal::Proxy<RewrittenT>;
public:
    Field(ClassWrapper *parent, typename CompleteProxy::PtrGetter ptrGetter)
        : CompleteProxy(parent, ptrGetter)
    {}

    Field(ClassWrapper *parent, ptrdiff_t offset)
        : CompleteProxy(parent, OffsGetter(offset))
    {}

    operator const RewrittenT& () const { return this->valueCRef(); }
    operator RewrittenT& () { return this->valueRef(); }

    RewrittenT& operator = (const Field& rhs)
    {
        return this->valueRef() = rhs.valueCRef();
    }

    RewrittenT& operator = (const RewrittenT& rhs)
    {
        return this->valueRef() = rhs;
    }
};

// ---------------------------------------------------------------------------------------------- //
// [Function]                                                                                     //
// ---------------------------------------------------------------------------------------------- //

template<typename> class Function;
#define REMODEL_DEF_FUNCTION(callingConv)                                                          \
    template<typename RetT, typename... ArgsT>                                                     \
    class Function<RetT (callingConv*)(ArgsT...)>                                                  \
        : public internal::ProxyImplBase                                                           \
    {                                                                                              \
    protected:                                                                                     \
        using FunctionPtr = RetT(callingConv*)(ArgsT...);                                          \
    public:                                                                                        \
        Function(ClassWrapper* parent, PtrGetter ptrGetter)                                        \
            : ProxyImplBase(parent, ptrGetter)                                                     \
        {}                                                                                         \
                                                                                                   \
        FunctionPtr get()                                                                          \
        {                                                                                          \
            return (FunctionPtr)this->rawPtr();                                                    \
        }                                                                                          \
                                                                                                   \
        RetT operator () (ArgsT... args)                                                           \
        {                                                                                          \
            return get()(args...);                                                                 \
        }                                                                                          \
    }

#ifdef REMODEL_MSVC
    REMODEL_DEF_FUNCTION(__cdecl);
    REMODEL_DEF_FUNCTION(__stdcall);
    REMODEL_DEF_FUNCTION(__thiscall);
    REMODEL_DEF_FUNCTION(__fastcall);
    REMODEL_DEF_FUNCTION(__vectorcall);
#elif defined(REMODEL_GNUC)
    //REMODEL_DEF_FUNCTION(__attribute__(cdecl));
    //REMODEL_DEF_FUNCTION(__attribute__(stdcall));
    //REMODEL_DEF_FUNCTION(__attribute__(fastcall));
    //REMODEL_DEF_FUNCTION(__attribute__(thiscall));
#endif

#undef REMODEL_DEF_FUNCTION

// ---------------------------------------------------------------------------------------------- //
// [MemberFunction]                                                                               //
// ---------------------------------------------------------------------------------------------- //

template<typename> class MemberFunction;
#define REMODEL_DEF_MEMBER_FUNCTION(callingConv)                                                   \
    template<typename RetT, typename... ArgsT>                                                     \
    class MemberFunction<RetT (callingConv*)(ArgsT...)>                                            \
        : public internal::ProxyImplBase                                                           \
    {                                                                                              \
    protected:                                                                                     \
        using FunctionPtr = RetT(callingConv*)(void *thiz, ArgsT... args);                         \
    public:                                                                                        \
        MemberFunction(ClassWrapper* parent, PtrGetter ptrGetter)                                  \
            : ProxyImplBase(parent, ptrGetter)                                                     \
        {}                                                                                         \
                                                                                                   \
        FunctionPtr get()                                                                          \
        {                                                                                          \
            return (FunctionPtr)this->rawPtr();                                                    \
        }                                                                                          \
                                                                                                   \
        RetT operator () (ArgsT... args)                                                           \
        {                                                                                          \
            return get()(this->rawPtr(), args...);                                                 \
        }                                                                                          \
    }

#ifdef REMODEL_MSVC
    REMODEL_DEF_MEMBER_FUNCTION(__cdecl);
    REMODEL_DEF_MEMBER_FUNCTION(__stdcall);
    REMODEL_DEF_MEMBER_FUNCTION(__thiscall);
    REMODEL_DEF_MEMBER_FUNCTION(__fastcall);
    REMODEL_DEF_MEMBER_FUNCTION(__vectorcall);
#elif defined(REMODEL_GNUC)
    //REMODEL_DEF_MEMBER_FUNCTION(__attribute__(cdecl));
    //REMODEL_DEF_MEMBER_FUNCTION(__attribute__(stdcall));
    //REMODEL_DEF_MEMBER_FUNCTION(__attribute__(fastcall));
    //REMODEL_DEF_MEMBER_FUNCTION(__attribute__(thiscall));
#endif

#undef REMODEL_DEF_MEMBER_FUNCTION

// ---------------------------------------------------------------------------------------------- //
// [VirtualFunction]                                                                              //
// ---------------------------------------------------------------------------------------------- //

template<typename> struct VirtualFunction;
#define REMODEL_DEF_VIRT_FUNCTION(callingConv)                                                     \
    template<typename RetT, typename... ArgsT>                                                     \
    struct VirtualFunction<RetT (callingConv*)(ArgsT...)>                                          \
        : public MemberFunction<RetT (callingConv*)(ArgsT...)>                                     \
    {                                                                                              \
        VirtualFunction(ClassWrapper* parent, std::size_t vftableIdx)                              \
            : MemberFunction(parent, VFTableGetter(vftableIdx))                                    \
        {}                                                                                         \
    }

#ifdef REMODEL_MSVC
    REMODEL_DEF_VIRT_FUNCTION(__cdecl);
    REMODEL_DEF_VIRT_FUNCTION(__stdcall);
    REMODEL_DEF_VIRT_FUNCTION(__thiscall);
    REMODEL_DEF_VIRT_FUNCTION(__fastcall);
    REMODEL_DEF_VIRT_FUNCTION(__vectorcall);
#elif defined(REMODEL_GNUC)
    //REMODEL_DEF_VIRT_FUNCTION(__attribute__(cdecl));
    //REMODEL_DEF_VIRT_FUNCTION(__attribute__(stdcall));
    //REMODEL_DEF_VIRT_FUNCTION(__attribute__(fastcall));
    //REMODEL_DEF_VIRT_FUNCTION(__attribute__(thiscall));
#endif

#undef REMODEL_DEF_VIRT_FUNCTION

// ============================================================================================== //
// Classes that may be used to place objects in a global or module level space                    //
// ============================================================================================== //

// ---------------------------------------------------------------------------------------------- //
// [Global]                                                                                       //
// ---------------------------------------------------------------------------------------------- //

class Global
    : public ClassWrapper
    , public utils::NonCopyable
{
    REMODEL_WRAPPER(Global)

    Global() : ClassWrapper(nullptr) {}
public:
    static Global* instance()
    {
        static Global thiz;
        return thiz.addressOfWrapper();
    }
};

// ---------------------------------------------------------------------------------------------- //
// [Module]                                                                                       //
// ---------------------------------------------------------------------------------------------- //

class Module : public ClassWrapper
{
    REMODEL_WRAPPER(Module)
public:
    static utils::Optional<Module> getModule(const char* moduleName)
    {
        auto modulePtr = platform::obtainModuleHandle(moduleName);
        if (!modulePtr) return utils::Empty;
        return {utils::InPlace, wrapper_cast<Module>(modulePtr)};
    }
};

// ============================================================================================== //

} // namespace remodel

#endif //_REMODEL_REMODEL_HPP_
