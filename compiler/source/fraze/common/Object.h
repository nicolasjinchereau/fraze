/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <unordered_map>
#include <variant>
#include <fraze/common/Pointers.h>
#include <fraze/common/Extensions.h>
#include <fraze/memory/Heap.h>
#include <fraze/memory/IAllocator.h>

namespace fraze {

enum class WordType
{
    Void = -1,
    Object, // 'null' literals or native classes which inherit from Object
    Boolean,
    Integer,
    Number,
    Array,
    String,
    Class,
    Reference,
};

const std::unordered_map<WordType, const std::string> WordTypeNames
{
    { WordType::Object,    "Object" },
    { WordType::Boolean,   "Boolean" },
    { WordType::Integer,   "Integer" },
    { WordType::Number,    "Number" },
    { WordType::Array,     "Array" },
    { WordType::String,    "String" },
    { WordType::Class,     "Class" },
    { WordType::Reference, "Reference" },
};

class Word;

using Boolean  = bool;
using Integer  = int64_t;
using Number  = double;
using Reference = Word*;

template<class T = void>
class Array;

template<typename T>
struct _IsArrayImpl : std::false_type {};

template<typename T>
struct _IsArrayImpl<Array<T>> : std::true_type {};

template<typename T>
inline constexpr bool IsArray = _IsArrayImpl<T>::value;

class Program;
class Class;
class String;
class ScopedAllocator;

struct TypeInfo;
struct SectionInfo;
struct BasicTypeInfo;
struct FieldInfo;
struct FunctionInfo;
struct ClassInfo;
struct InterfaceInfo;
struct StructInfo;
struct EnumInfo;
struct ArrayInfo;

class Object
{
    friend Program;
    friend ScopedAllocator;
protected:
    const TypeInfo* info = nullptr;

public:
    Object(const TypeInfo* info)
        : info(info)
    {
        assert(info);
    }

    Object(const Object&) = delete;
    Object& operator=(const Object&) = delete;

    const TypeInfo* GetTypeInfo() const {
        return info;
    }

    template<class T, typename... Args> requires std::is_convertible_v<T*, Object*>
    static T* Create(IAllocator& allocator, size_t size, Args&&... args)
    {
        assert(size != 0);
        T* val = reinterpret_cast<T*>(allocator.Allocate(size));
        return std::construct_at(val, std::forward<Args>(args)...);
    }

    struct Deleter
    {
        void operator()(Object* object)
        {
            assert(object);
            std::destroy_at(object);
            ::operator delete(object);
        }
    };

    template<class T, typename... Args> requires std::is_convertible_v<T*, Object*>
    static std::unique_ptr<T, Deleter> Create(size_t size, Args&&... args)
    {
        assert(size != 0);
        T* val = reinterpret_cast<T*>(::operator new(size));
        std::construct_at(val, std::forward<Args>(args)...);
        return std::unique_ptr<T, Deleter>(val);
    }
};

template<class T>
concept ObjectSubclass = std::is_base_of_v<Object, std::remove_cvref_t<T>>;

class Word
{
    union {
        uint64_t storage;
        Integer integer;
        Number number;
        Object* object;
        Reference reference;
    };

    friend Program;
public:
    Word() { object = nullptr; }
    Word(std::nullptr_t) : Word() { }
    Word(Boolean value) { storage = static_cast<uint64_t>(value); }
    Word(Integer value) { integer = value; }
    Word(Number value) { number = value; }
    Word(Reference value) { reference = value; }
    Word(Object* value) { object = value; }

    Word(const Word& obj) = default;
    Word& operator=(const Word& obj) = default;

    static Word Raw(uint64_t value) {
        Word ret;
        ret.storage = value;
        return ret;
    }

    void Set(std::nullptr_t) { object = nullptr; }
    void Set(Boolean value) { storage = static_cast<uint64_t>(value); }
    void Set(Integer value) { integer = value; }
    void Set(Number value) { number = value; }
    void Set(Reference value) { reference = value; }
    void Set(Object* value) { object = value; }

    Boolean GetBoolean() const { return static_cast<bool>(storage); }
    Integer GetInteger() const { return integer; }
    Number GetNumber() const { return number; }
    Object* GetObject() const { return object; }
    Array<>* GetArray() const;
    Class* GetClass() const;
    String* GetString() const;
    Reference GetReference() const { return reference; }

    bool operator==(const Word& other) const {
        return storage == other.storage;
    }

    bool operator!=(const Word& other) const {
        return storage != other.storage;
    }

    template<class T>
    T* GetData() { return reinterpret_cast<T*>(this); }

    template<class T>
    const T* GetData() const { return reinterpret_cast<const T*>(this); }
    
    template<class T>
    static constexpr bool IsMutableGetType =
        IsArray<T> ||
        std::is_same_v<T, Class> ||
        std::is_same_v<T, Reference> ||
        (
            !IsArray<T> &&
            !std::is_same_v<T, String> &&
            !std::is_same_v<T, Class> &&
            std::is_base_of_v<Object, std::remove_cvref_t<T>>
        );

    template<class T, class Self>
    static auto& GetImpl(Self& self);

    template<class T>
    auto& Get() const;

    template<class T> requires Word::IsMutableGetType<T>
    auto& Get();

    template<class T>
    static constexpr WordType GetWordType();
};

template<class T>
concept WordValue = std::constructible_from<Word, std::remove_cvref_t<T>>;

template<>
class Array<void> : public Object
{
protected:
    size_t length = 0;
    Word* GetData() const;
    Word& GetWord(size_t i) const;
public:
    Array(const ArrayInfo* info, size_t length);

    static Array<>* New(IAllocator& allocator, const ArrayInfo* info, size_t length);

    size_t GetSize() const;
    size_t GetElementSize() const;
    size_t GetCount() const;

    Word& At(size_t index);
    const Word& At(size_t index) const;

    const ArrayInfo* GetInfo() const;
};

template<class T>
class Array : public Array<void>
{
    T* GetTypedData() {
        return reinterpret_cast<T*>(GetData());
    }

    const T* GetTypedData() const {
        return reinterpret_cast<const T*>(GetData());
    }
public:
    typedef T ElementType;

    T& GetElement(size_t index) {
        assert(index >= 0);
        assert(index * GetElementSize() < length);
        return GetTypedData()[index];
    }

    const T& GetElement(size_t index) const {
        assert(index >= 0);
        assert(index * GetElementSize() < length);
        return GetTypedData()[index];
    }

    T& operator[](size_t index) {
        return GetElement(index);
    }

    const T& operator[](size_t index) const {
        return GetElement(index);
    }

    T* begin() {
        return GetTypedData();
    }
    
    const T* begin() const {
        return GetTypedData();
    }

    T* end() {
        return reinterpret_cast<T*>(GetData() + length);
    }

    const T* end() const {
        return reinterpret_cast<const T*>(GetData() + length);
    }
};

class Class : public Object
{
    Word& GetWord(size_t i) const;
public:
    Class(const ClassInfo* info);

    static Class* New(IAllocator& allocator, const ClassInfo* info);

    std::string_view GetName() const;
    size_t GetFieldCount() const;
    Word GetField(size_t index) const;
    Word GetField(std::string_view name) const;
    Word GetFieldRef(size_t index) const;
    std::span<Word> GetFields(size_t index, size_t size) const;
    void SetField(size_t index, const Word& val);
    void SetFields(size_t index, const std::span<Word>& values);
    void SetField(std::string_view name, const Word& val);
    void SetField(std::string_view name, const std::span<Word>& val);
    const ClassInfo* GetInfo() const;
    size_t GetFunctionID(size_t interfaceID, size_t offset);

    void SetField(std::string_view name, Object* val) {
        SetField(name, Word(val));
    }

    template<class T, class S = std::remove_cvref_t<T>>
        requires(
            std::is_class_v<T> &&
            std::is_trivially_copyable_v<T> &&
            !is_span_v<S> &&
            !is_any_of_v<S, Boolean, Integer, Number, Word> &&
            !std::is_assignable_v<Object*, S> &&
            !IsArray<S>
        )
    void SetField(std::string_view name, T& val) {
        static_assert(sizeof(T) % sizeof(Word) == 0);
        std::span<Word> valWords = { reinterpret_cast<Word*>(&val), reinterpret_cast<Word*>(&val + 1) };
        SetField(name, valWords);
    }
};

class String : public Object
{
    size_t length = 0;
public:
    String(const TypeInfo* info, size_t length);
    String(const TypeInfo* info, std::string_view str);
    String(const TypeInfo* info, std::string_view left, std::string_view right);

    static String* New(IAllocator& allocator, std::string_view str);
    static String* New(IAllocator& allocator, size_t length);
    static String* New(IAllocator& allocator, std::string_view left, std::string_view right);
    
    // for static strings
    static std::unique_ptr<String, Object::Deleter> New(Program* program, std::string_view str);

    bool IsEmpty() const;
    char* GetChar(size_t i) const;

    std::string_view GetView() const;

    inline operator std::string_view() const {
        return GetView();
    }
};

inline Array<>* Word::GetArray() const {
    return static_cast<Array<>*>(object);
}

inline Class* Word::GetClass() const {
    return static_cast<Class*>(object);
}

inline String* Word::GetString() const {
    return static_cast<String*>(object);
}

template<class T, class Self>
inline auto& Word::GetImpl(Self& self)
{
    using U = std::remove_cvref_t<T>;

    if constexpr (std::is_same_v<U, Boolean>) {
        return *reinterpret_cast<copy_const_t<Self, Boolean>*>(&self.storage);
    }
    else if constexpr (std::is_same_v<U, Integer>) {
        return self.integer;
    }
    else if constexpr (std::is_same_v<U, Number>) {
        return self.number;
    }
    else if constexpr (std::is_same_v<U, String>) {
        return *static_cast<copy_const_t<Self, String*>>(self.object);
    }
    else if constexpr (IsArray<U>) {
        return *static_cast<copy_const_t<Self, U*>>(self.object);
    }
    else if constexpr (std::is_same_v<U, Class>) {
        return *static_cast<copy_const_t<Self, Class*>>(self.object);
    }
    else if constexpr (std::is_same_v<U, Reference>) {
        return self.reference;
    }
    else if constexpr (std::is_base_of_v<Object, U>) {
        return *static_cast<copy_const_t<Self, U*>>(self.object);
    }
    else if constexpr (std::is_pointer_v<U> && std::is_base_of_v<Object, std::remove_cv_t<std::remove_pointer_t<U>>>) {
        using PtrRef = std::conditional_t<
            std::is_const_v<Self>,
            std::add_lvalue_reference_t<std::add_const_t<U>>,
            std::add_lvalue_reference_t<U>
        >;
        return reinterpret_cast<PtrRef>(self.object);
    }
    else if constexpr (std::is_class_v<U> && std::is_trivially_copyable_v<U>) {
        return *reinterpret_cast<copy_const_t<Self, U>*>(&self);
    }
    else if constexpr (std::is_enum_v<U> && std::is_same_v<std::underlying_type_t<U>, Integer>) {
        return *reinterpret_cast<copy_const_t<Self, U>*>(&self.integer);
    }
    else {
        static_assert(!std::is_same_v<U, U>, "Unsupported type for Word::Get<T>().");
    }
}

template<class T>
inline auto& Word::Get() const {
    return GetImpl<T>(*this);
}

template<class T> requires Word::IsMutableGetType<T>
inline auto& Word::Get() {
    return GetImpl<T>(*this);
}

template<class T>
constexpr WordType Word::GetWordType()
{
    using U = std::remove_cvref_t<T>;

    if constexpr (std::is_same_v<U, Boolean>) {
        return WordType::Boolean;
    }
    else if constexpr (std::is_same_v<U, Integer>) {
        return WordType::Integer;
    }
    else if constexpr (std::is_same_v<U, Number>) {
        return WordType::Number;
    }
    else if constexpr (std::is_same_v<U, String>) {
        return WordType::String;
    }
    else if constexpr (IsArray<U>) {
        return WordType::Array;
    }
    else if constexpr (std::is_same_v<U, Class>) {
        return WordType::Class;
    }
    else if constexpr (std::is_base_of_v<Object, U> && !IsArray<U> && !std::is_same_v<U, String> && !std::is_same_v<U, Class>) {
        return WordType::Object;
    }
    else if constexpr (std::is_pointer_v<U> && std::is_base_of_v<Object, std::remove_cv_t<std::remove_pointer_t<U>>>) {
        return WordType::Object;
    }
    else if constexpr (std::is_same_v<U, Reference>) {
        return WordType::Reference;
    }
    else if constexpr (std::is_class_v<U> && std::is_trivially_copyable_v<U>) {
        return WordType::Reference; // struct
    }
    else if constexpr (std::is_enum_v<U> && std::is_same_v<std::underlying_type_t<U>, Integer>) {
        return WordType::Integer;
    }
    else if constexpr (std::is_void_v<U>) {
        return WordType::Void;
    }
    else {
        static_assert(!std::is_same_v<U, U>, "Unsupported type for Word::GetWordType<T>().");
    }
}

} // fraze
