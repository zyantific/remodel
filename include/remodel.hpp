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
// [ClassWrapper]                                                                                 //
// ============================================================================================== //

class ClassWrapper
{
    friend class internal::ProxyImplBase;
protected:
    void* m_raw = nullptr;

    explicit ClassWrapper(void* raw)
        : m_raw(raw)
    {}
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

// Macro forwarding constructors and implementing other wrapper logic required.
#define REMODEL_WRAPPER(classname)                                                                 \
    protected:                                                                                     \
        template<typename WrapperT>                                                                \
        friend WrapperT remodel::wrapper_cast(void *raw);                                          \
        explicit classname(void* raw)                                                              \
            : ClassWrapper(raw) {}                                                                 \
    public:                                                                                        \
        classname(const classname& other)                                                          \
            : ClassWrapper(other) {}                                                               \
        classname& operator = (const classname& other)                                             \
            { this->ClassWrapper::operator = (other); return *this; }                              \
        /* allow field access via -> (required to allow -> on wrapped struct fields) */            \
        classname* operator -> () { return this; }                                                 \
        classname* addressOfWrapper() { return this; }                                             \
        const classname* addressOfWrapper() const { return this; }                                 \
    private:

// ============================================================================================== //
// Casting function(s) and out-of-class "operators"                                               //
// ============================================================================================== //

// Naming convention violated here because, well, casts should look that way.
template<typename WrapperT>
inline WrapperT wrapper_cast(void *raw)
{
    WrapperT nrvo{raw};
    return nrvo;
}

template<typename WrapperT>
inline utils::CloneConst<WrapperT, void>* addressOfObj(WrapperT& wrapper)
{
    static_assert(std::is_base_of<ClassWrapper, WrapperT>::value,
        "addressOfObj is only supported for class-wrappers");

    return wrapper.addressOfObj();
}

template<typename WrapperT>
inline utils::CloneConst<WrapperT, void>* addressOfWrapper(WrapperT& wrapper)
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
    explicit OffsGetter(ptrdiff_t offs)
        : m_offs(offs)
    {}

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
    unsigned m_vftableIdx;
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
// [Proxy] for known-size arrays of non-wrapped types                                             //
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
            | operators::ADD // addition with integer constants
    >
{
    static_assert(std::is_trivial<T>::value, "arary fields may only be created of trivial types");
    REMODEL_PROXY_FORWARD_CTORS
protected: // Implementation of AbstractOperatorForwarder
    using TN = T[N];
    TN& valueRef() override              { return *static_cast<TN*>(this->rawPtr()); }
    const TN& valueCRef() const override { return *static_cast<const TN*>(this->crawPtr()); }
};

// ---------------------------------------------------------------------------------------------- //
// [Proxy] for non-wrapped trivial structs/classes                                                //
// ---------------------------------------------------------------------------------------------- //

template<typename T>
class Proxy<T, std::enable_if_t<
            std::is_class<T>::value && !std::is_base_of<ClassWrapper, T>::value
        >> 
    : public ProxyImplBase
    , public operators::AbstractOperatorForwarder<Proxy<T>, T>
{
    static_assert(std::is_trivial<T>::value, "only trivial structs are supported");
    REMODEL_PROXY_FORWARD_CTORS
protected: // Implementation of AbstractOperatorForwarder
    T& valueRef() override              { return *static_cast<T*>(this->rawPtr()); }
    const T& valueCRef() const override { return *static_cast<const T*>(this->crawPtr()); }
public:
    T& get()                            { return this->valueRef(); }
    const T& get() const                { return this->valueCRef(); }

    T* operator -> ()                   { return &get(); }
    const T* operator -> () const       { return &get(); }
};

// ---------------------------------------------------------------------------------------------- //
// [Proxy] for wrapped structs/classes                                                            //
// ---------------------------------------------------------------------------------------------- //

template<typename T>
class Proxy<T, std::enable_if_t<std::is_base_of<ClassWrapper, T>::value>>
    : public ProxyImplBase
{
    REMODEL_PROXY_FORWARD_CTORS
public:
    T get() { return wrapper_cast<T>(this->rawPtr()); }
    
    // REMODEL_WRAPPER macro adds overloaded -> operator returning this to class wrappers,
    // allowing the -> operator to work.
    T operator -> () { return get(); }
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

// ---------------------------------------------------------------------------------------------- //
// [Proxy] for rvalue-references                                                                  //
// ---------------------------------------------------------------------------------------------- //

template<typename T>
class Proxy<T*>
{
    static_assert(utils::BlackBoxConsts<T>::false_, "pointer-fields are not supported");
};

#undef REMODEL_PROXY_FORWARD_CTORS

} // namespace internal

// ============================================================================================== //
// Remodeling classes                                                                             //
// ============================================================================================== //

// ---------------------------------------------------------------------------------------------- //
// [Field]                                                                                        //
// ---------------------------------------------------------------------------------------------- //

template<typename T>
class Field : public internal::Proxy<T>
{
public:
    Field(ClassWrapper *parent, typename internal::Proxy<T>::PtrGetter ptrGetter)
        : internal::Proxy<T>(parent, ptrGetter)
    {
    
    }

    Field(ClassWrapper *parent, ptrdiff_t offset)
        : internal::Proxy<T>(parent, OffsGetter(offset))
    {

    }

    operator const T&   () const { return this->valueCRef(); }
    operator T&         ()       { return this->valueRef(); }

    T& operator = (const Field& rhs)
    {
        return this->valueRef() = rhs.valueCRef();
    }

    T& operator = (const T& rhs)
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

template<typename> class VirtualFunction;
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
// Classes that may be used to place objects in a global or module level scope                    //
// ============================================================================================== //

// ---------------------------------------------------------------------------------------------- //
// [Global]                                                                                       //
// ---------------------------------------------------------------------------------------------- //

class Global : public ClassWrapper
{
    REMODEL_WRAPPER(Global)

    Global() : ClassWrapper(nullptr) {}
public:
    Global* instance()
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
    utils::Optional<Module> getModule(const char* moduleName)
    {
        auto modulePtr = platform::obtainModuleHandle(moduleName);
        if (!modulePtr) return utils::Empty;
        return wrapper_cast<Module>(modulePtr);
    }
};

// ============================================================================================== //

} // namespace remodel

#endif //_REMODEL_REMODEL_HPP_
