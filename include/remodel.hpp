#ifndef _REMODEL_REMODEL_HPP_
#define _REMODEL_REMODEL_HPP_

#include <functional>

#include "utils.hpp"
#include "operators.hpp"
#include "config.hpp"

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
    std::size_t m_rawSize = 0;
protected:
    template<typename T> T* ptr(ptrdiff_t offs)
    {
        return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_raw) + offs);
    }
protected:
    explicit ClassWrapper(void* raw)
        : m_raw(raw)
    {}
public:
    virtual ~ClassWrapper() = default;

    ClassWrapper(const ClassWrapper& other)
        : m_raw(other.m_raw)
    {}

    ClassWrapper& operator = (const ClassWrapper& other)
    {
        m_raw = other.m_raw;
        return *this;
    }

    // disable address-of operator to avoid confusion (use addressOfObj/addressOfWrapper instead)
    ClassWrapper& operator & () = delete;

    void* addressOfObj()
    {
        return ptr<void>(0);
    }
};

// Macro forwarding constructors.
#define REMODEL_WRAPPER(classname)                                                                 \
    protected:                                                                                     \
        template<typename wrapperT>                                                                \
        friend wrapperT remodel::wrapperCast(void *raw);                                           \
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
    private:

// ============================================================================================== //
// Casting function(s) and out-of-class "operators"                                               //
// ============================================================================================== //

template<typename wrapperT>
inline wrapperT wrapperCast(void *raw)
{
    wrapperT nrvo(raw);
    return nrvo;
}

template<typename wrapperT>
inline void* addressOfObj(wrapperT& wrapper)
{
    static_assert(std::is_base_of<ClassWrapper, wrapperT>::value,
        "addressOfObj is only supported for class-wrappers");

    return wrapper.addressOfObj();
}

template<typename wrapperT>
inline void* addressOfWrapper(wrapperT& wrapper)
{
    static_assert(std::is_base_of<ClassWrapper, wrapperT>::value,
        "addressOfWrapper is only supported for class-wrappers and fields");

    return wrapper.addressOfWrapper();
}

// ============================================================================================== //
// [OffsGetter]                                                                                   //
// ============================================================================================== //

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

// ============================================================================================== //
// [VFtableGetter]                                                                                //
// ============================================================================================== //

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
// [ProxyImplBase]                                                                                //
// ============================================================================================== //

namespace internal
{

class ProxyImplBase
{
protected:
    using PtrGetter = std::function<void*(void* rawBasePtr)>;
protected:
    PtrGetter m_ptrGetter;
    ClassWrapper *m_parent;
protected:
    ProxyImplBase(ClassWrapper *parent, PtrGetter ptrGetter)
        : m_ptrGetter(ptrGetter)
        , m_parent(parent)
    {}
protected:
    ProxyImplBase(const ProxyImplBase&) = delete;
    ProxyImplBase& operator = (const ProxyImplBase&) { return *this; }
protected:
    void* rawPtr()              { return this->m_ptrGetter(this->m_parent->m_raw); }
    const void* crawPtr() const { return this->m_ptrGetter(this->m_parent->m_raw); }
public:
    virtual ~ProxyImplBase() = default;

    // Disable address-of operator to avoid confusion.
    // TODO: add functions to get address of fields
    ProxyImplBase* operator & () = delete;
};

// ============================================================================================== //
// [Proxy]                                                                                        //
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

// Fallthrough.
template<typename T, typename=void>
class Proxy
{
    static_assert(utils::BlackBoxConsts<T>::false_, "this types is not supported for wrapping");
};

// Arithmetic types.
template<typename T>
class Proxy<T, std::enable_if_t<std::is_arithmetic<T>::value>>
    : public ProxyImplBase
    , public Operators::ForwardByFlags<
        Proxy<T>, 
        T, 
        (Operators::ARITHMETIC | Operators::BITWISE) 
            & ~(std::is_floating_point<T>::value ? Operators::BITWISE_NOT : 0)
            & ~(std::is_unsigned<T>::value ? Operators::UNARY_MINUS : 0)
    >
{
    REMODEL_PROXY_FORWARD_CTORS
protected: // Implementation of AbstractOperatorForwarder
    T& valueRef() override              { return *static_cast<T*>(this->rawPtr()); }
    const T& valueCRef() const override { return *static_cast<const T*>(this->crawPtr()); }
};

// Field implementation for arrays of non-wrapped types
template<typename T>
class Proxy<T[]>
{
    static_assert(utils::BlackBoxConsts<T>::false_, 
        "unknown size array struct fields are not permitted by the standard");
};

// Field implementation for known-size arrays of non-wrapped types
template<typename T, std::size_t N>
class Proxy<T[N]>
    : public ProxyImplBase
    , public Operators::ForwardByFlags<
        Proxy<T[N]>,
        T[N],
        Operators::ARRAY_SUBSCRIPT 
            | Operators::INDIRECTION 
            | Operators::SUBTRACT
            | Operators::ADD // addition with integer constants
    >
{
    static_assert(std::is_trivial<T>::value, "arary fields may only be created of trivial types");
    REMODEL_PROXY_FORWARD_CTORS
protected: // Implementation of AbstractOperatorForwarder
    using TN = T[N];
    TN& valueRef() override              { return *static_cast<TN*>(this->rawPtr()); }
    const TN& valueCRef() const override { return *static_cast<const TN*>(this->crawPtr()); }
};

// Field implementation for non-wrapped trivial structs/classes
template<typename T>
class Proxy<T, std::enable_if_t<
            std::is_class<T>::value && !std::is_base_of<ClassWrapper, T>::value
        >> 
    : public ProxyImplBase
    , public Operators::AbstractOperatorForwarder<Proxy<T>, T>
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

// Field implementation for wrapped structs/classes
template<typename T>
class Proxy<T, std::enable_if_t<std::is_base_of<ClassWrapper, T>::value>>
    : public ProxyImplBase
{
    REMODEL_PROXY_FORWARD_CTORS
public:
    T get() { return wrapperCast<T>(this->rawPtr()); }
    
    // REMODEL_WRAPPER macro adds overloaded -> operator returning this to class wrappers,
    // allowing the -> operator to work.
    T operator -> () { return get(); }
};

template<typename T>
class Proxy<T&>
{
    static_assert(utils::BlackBoxConsts<T>::false_, "reference-fields are not supported");
};

template<typename T>
class Proxy<T&&>
{
    static_assert(utils::BlackBoxConsts<T>::false_, "rvalue-reference-fields are not supported");
};

template<typename T>
class Proxy<T*>
{
    static_assert(utils::BlackBoxConsts<T>::false_, "pointer-fields are not supported");
};

#undef REMODEL_PROXY_FORWARD_CTORS

} // namespace internal

// ============================================================================================== //
// [Field]                                                                                        //
// ============================================================================================== //

template<typename T>
class Field : public internal::Proxy<T>
{
public:
    Field(ClassWrapper *parent, internal::ProxyImplBase::PtrGetter ptrGetter)
        : internal::Proxy<T>(parent, ptrGetter)
    {}

    Field(ClassWrapper *parent, ptrdiff_t offset)
        : internal::Proxy<T>(parent, OffsGetter(offset))
    {}

    operator const T&   () const { return valueCRef();  }
    operator T&         ()       { return valueRef();   }

    T& operator = (const Field& rhs)
    {
        return this->valueRef() = rhs.valueCRef();
    }

    T& operator = (const T& rhs)
    {
        return this->valueRef() = rhs;
    }
};

// ============================================================================================== //
// [Function]                                                                                     //
// ============================================================================================== //

template<typename> class Function;
#define REMODEL_DEF_FUNCTION(callingConv)                                                          \
    template<typename retT, typename... argsT>                                                     \
    class Function<retT (callingConv*)(argsT...)>                                                  \
        : public internal::ProxyImplBase                                                           \
    {                                                                                              \
    protected:                                                                                     \
        using functionPtr = retT(callingConv*)(argsT...);                                          \
    public:                                                                                        \
        Function(void* rawBase, PtrGetter ptrGetter)                                              \
            : FieldImplBase(rawBase, ptrGetter)                                                    \
        {}                                                                                         \
                                                                                                   \
        functionPtr get()                                                                          \
        {                                                                                          \
            return (functionPtr)m_ptrGetter(m_rawBase, 0, 0);                                      \
        }                                                                                          \
                                                                                                   \
        retT operator () (argsT... args)                                                           \
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

// ============================================================================================== //
// [VirtualFunction]                                                                              //
// ============================================================================================== //

// While a user might want to use @c Function to feed a function requiring a @c
// this argument manually, it often makes sense to automatically use the @c raw
// pointer as @c this. The @c this argument is added to the function signature
// by this class, NOT by the user. @c raw() is passed as @c this on calls.
// Compared to @c Function, it also comes with a special constructor accepting
// a VFTable index directly.

template<typename> class VirtualFunction;
#define REMODEL_DEF_VIRT_FUNCTION(callingConv)                                                     \
    template<typename retT, typename... argsT>                                                     \
    class VirtualFunction<retT (callingConv*)(argsT...)>                                           \
        : public internal::ProxyImplBase                                                           \
    {                                                                                              \
    protected:                                                                                     \
        using functionPtr = retT(callingConv*)(void *thiz, argsT... args);                         \
    public:                                                                                        \
        VirtualFunction(void* rawBase, PtrGetter ptrGetter)                                       \
            : FieldImplBase(rawBase, ptrGetter)                                                    \
        {}                                                                                         \
                                                                                                   \
        VirtualFunction(void* rawBase, int vftableIdx)                                            \
            : FieldImplBase(rawBase, VFTableGetter(vftableIdx))                                    \
        {}                                                                                         \
                                                                                                   \
        functionPtr get()                                                                          \
        {                                                                                          \
            return (functionPtr)m_ptrGetter(m_rawBase, 0, 0);                                      \
        }                                                                                          \
                                                                                                   \
        retT operator () (argsT... args)                                                           \
        {                                                                                          \
            return get()(m_rawBase, args...);                                                      \
        }                                                                                          \
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
// [GlobalSpace]                                                                                  //
// ============================================================================================== //

class GlobalScope : public ClassWrapper
{
    REMODEL_WRAPPER(GlobalScope)

    GlobalScope() : ClassWrapper(nullptr) {}
public:
    GlobalScope* instance()
    {
        static GlobalScope thiz;
        return thiz.addressOfWrapper();
    }
};

// ============================================================================================== //

} // namespace Remodel

#endif //_REMODEL_REMODEL_HPP_