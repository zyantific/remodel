#ifndef _REMODEL_REMODEL_HPP_
#define _REMODEL_REMODEL_HPP_

#include <functional>

#include "utils.hpp"
#include "operators.hpp"
#include "config.hpp"

namespace Remodel
{

using RawPtr = void*;

// ============================================================================================== //
// [ClassWrapper]                                                                                 //
// ============================================================================================== //

class ClassWrapper
{
    template<typename, typename> friend class Field;
protected:
    RawPtr m_raw = nullptr;
    std::size_t m_rawSize = 0;
protected:
    template<typename T> T* ptr(ptrdiff_t offs)
    {
        return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_raw) + offs);
    }
protected:
    explicit ClassWrapper(RawPtr raw)
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
public:
    ClassWrapper* thiz() 
    { 
        return this;
    }
};

// Macro forwarding constructors.
#define REMODEL_WRAPPER(classname)                                                                 \
    protected:                                                                                     \
        template<typename wrapperT>                                                                \
        friend wrapperT Remodel::wrapperCast(void *raw);                                           \
        explicit classname(RawPtr raw)                                                             \
            : ClassWrapper(raw) {}                                                                 \
    public:                                                                                        \
        classname(const classname& other)                                                          \
            : ClassWrapper(other) {}                                                               \
        classname& operator = (const classname& other)                                             \
            { this->ClassWrapper::operator = (other); return *this; }                              \
    private:


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

    void* operator () (RawPtr raw, int idx, std::size_t elementSize)
    {
        return reinterpret_cast<void*>(
            reinterpret_cast<uintptr_t>(raw) + m_offs + idx * elementSize
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
    explicit VFTableGetter(unsigned vftableIdx)
        : m_vftableIdx(vftableIdx)
    {}

    RawPtr operator () (RawPtr raw, int idx, std::size_t elementSize)
    {
        if (idx != 0 || elementSize != 0) {
            Utils::fatalError("VFTable getter does not support array semantics");
        }

        return reinterpret_cast<void*>(
            *reinterpret_cast<uintptr_t*>(
                *reinterpret_cast<uintptr_t*>(raw) + m_vftableIdx * sizeof(uintptr_t)
                )
            );
    }
};

// ============================================================================================== //
// [FieldImplBase]                                                                                //
// ============================================================================================== //

namespace Internal
{

class FieldImplBase
{
protected:
    using PtrGetter = std::function<RawPtr(RawPtr rawBasePtr, int idx, std::size_t elementSize)>;
protected:
    PtrGetter m_ptrGetter;
    ClassWrapper *m_parent;
protected:
    FieldImplBase(ClassWrapper *parent, PtrGetter ptrGetter)
        : m_ptrGetter(ptrGetter)
        , m_parent(parent)
    {}

    FieldImplBase(ClassWrapper *parent, ptrdiff_t offset)
        : m_ptrGetter(OffsGetter(offset))
        , m_parent(parent)
    {}

    FieldImplBase(const FieldImplBase &other)
        : m_ptrGetter(other.m_ptrGetter)
        , m_parent(other.m_parent)
    {}

    FieldImplBase& operator = (const FieldImplBase &rhs)
    {
        m_ptrGetter = rhs.m_ptrGetter;
        m_parent = rhs.m_parent;
        return *this;
    }
public:
    virtual ~FieldImplBase() = default;

    // Disable address-of operator to avoid confusion.
    // TODO: add functions to get address of fields
    FieldImplBase* operator & () = delete;
};

} // namespace Internal

// ============================================================================================== //
// [Field]                                                                                        //
// ============================================================================================== //

#define REMODEL_FIELD_FORWARD_CTORS                                                                \
    Field(ClassWrapper *parent, PtrGetter ptrGetter)                                               \
        : FieldImplBase(parent, ptrGetter)                                                         \
    {}                                                                                             \
                                                                                                   \
    Field(ClassWrapper *parent, ptrdiff_t offset)                                                  \
        : FieldImplBase(parent, offset)                                                            \
    {}

template<typename T, typename=void>
class Field : public Internal::FieldImplBase
{
    static_assert(std::is_trivial<T>::value, "only trivial non-wrapped types are supported");
public:
    REMODEL_FIELD_FORWARD_CTORS
    ~Field() override = default;
protected:
    T& valueRef() /*override*/ 
    { 
        return *static_cast<T*>(m_ptrGetter(m_parent->m_raw, 0, 0)); 
    }

    const T& valueCRef() const /*override*/
    {
        return *static_cast<const T*>(m_ptrGetter(m_parent->m_raw, 0, 0)); 
    }
public:
    operator const T&   () const { return valueCRef();  }
    operator T&         ()       { return valueRef();   }
};

template<typename T>
class Field<T&>
{
    static_assert(sizeof(T) != sizeof(T), "reference-fields are not implemented, yet");
};

template<typename T>
class Field<T&&>
{
    static_assert(sizeof(T) != sizeof(T), "lvalue-reference-fields are not supported");
};

template<typename T>
class Field<T*> : public Internal::FieldImplBase
{
public:
    REMODEL_FIELD_FORWARD_CTORS


};

template<typename T>
class Field<T[]>
{
    static_assert(sizeof(T) != sizeof(T), "array-fields are not implemented, yet (1)");
};

// Field implementation for arrays of wrapped types
template<typename T, std::size_t N>
class Field<T[N], std::enable_if_t<std::is_base_of<ClassWrapper, T>::value>>
{
    static_assert(sizeof(T) != sizeof(T), "array-fields are not implemented, yet (2)");
};

// Field implementation for arrays of non-wrapped types
template<typename T, std::size_t N>
class Field<T[N], std::enable_if_t<!std::is_base_of<ClassWrapper, T>::value>>
{
    static_assert(sizeof(T) != sizeof(T), "array-fields are not implemented, yet (2)");
};

// Field implementation for non-wrapped trivial structs/classes
template<typename T>
class Field<T, std::enable_if_t<
        std::is_class<T>::value && !std::is_base_of<ClassWrapper, T>::value
        >> 
    : public Internal::FieldImplBase
{
    static_assert(std::is_trivial<T>::value, "only trivial structs are supported");
public:
    REMODEL_FIELD_FORWARD_CTORS

};

// Field implementation for wrapped structs/classes
template<typename T>
class Field<T, std::enable_if_t<std::is_base_of<ClassWrapper, T>::value>>
    : public Internal::FieldImplBase
{
public:
    REMODEL_FIELD_FORWARD_CTORS


};

#undef REMODEL_FIELD_FORWARD_CTORS

// ============================================================================================== //
// [Function]                                                                                     //
// ============================================================================================== //

template<typename> class Function;
#define REMODEL_DEF_FUNCTION(callingConv)                                                          \
    template<typename retT, typename... argsT>                                                     \
    class Function<retT (callingConv*)(argsT...)>                                                  \
        : public Internal::FieldImplBase                                                           \
    {                                                                                              \
    protected:                                                                                     \
        using functionPtr = retT(callingConv*)(argsT...);                                          \
    public:                                                                                        \
        Function(RawPtr rawBase, PtrGetter ptrGetter)                                              \
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
        : public Internal::FieldImplBase                                                           \
    {                                                                                              \
    protected:                                                                                     \
        using functionPtr = retT(callingConv*)(void *thiz, argsT... args);                         \
    public:                                                                                        \
        VirtualFunction(RawPtr rawBase, PtrGetter ptrGetter)                                       \
            : FieldImplBase(rawBase, ptrGetter)                                                    \
        {}                                                                                         \
                                                                                                   \
        VirtualFunction(RawPtr rawBase, int vftableIdx)                                            \
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
    GlobalScope() : ClassWrapper(nullptr) {}
public:
    GlobalScope* instance()
    {
        static GlobalScope thiz;
        return &thiz;
    }
};

// ============================================================================================== //
// Casting function(s)                                                                            //
// ============================================================================================== //

template<typename wrapperT> 
inline wrapperT wrapperCast(void *raw)
{
    wrapperT nrvo(raw);
    return nrvo;
}

// ============================================================================================== //

} // namespace Remodel

#endif //_REMODEL_REMODEL_HPP_