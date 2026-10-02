/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <new>
#include <span>
#include <string>
#include <tuple>
#include <utility>
#include <fraze/common/Object.h>
#include <fraze/common/Platform.h>

namespace fraze {

class Program;

template<typename... Args>
concept FirstArgIsProgram = requires
{
    requires sizeof...(Args) >= 1;
    requires std::is_same_v<get_type<0, Args...>, Program*>;
};

template<typename... Args>
struct ObjectArgsOnly {
    using type = std::tuple<Args...>;
};

template<typename... Rest>
struct ObjectArgsOnly<Program*, Rest...> {
    using type = std::tuple<Rest...>;
};

// Holds each parameter's size in words and its offset from the end of the argument block, where the VM pushes the
// first argument last, so the first parameter has offset 0.
template<class Tuple>
struct ParamLayout;

template<class... Params>
struct ParamLayout<std::tuple<Params...>>
{
    static constexpr std::array<uint32_t, sizeof...(Params)> sizes { Word::GetWordSize<Params>()... };
    static constexpr uint32_t totalSize = (0u + ... + Word::GetWordSize<Params>());

    static constexpr std::array<uint32_t, sizeof...(Params)> offsets = [] {
        std::array<uint32_t, sizeof...(Params)> result{};
        uint32_t offset = 0;
        for(size_t i = 0; i != result.size(); ++i) {
            result[i] = offset;
            offset += sizes[i];
        }
        return result;
    }();
};

class IExternalFunction
{
public:
    virtual ~IExternalFunction(){}
    virtual void Invoke(Program* program, Word* argsEnd) = 0;
    virtual std::span<WordType> GetParamTypes() = 0;
    virtual std::span<const uint32_t> GetParamSizes() = 0;
    virtual WordType GetReturnType() = 0;
    virtual uint32_t GetReturnSize() = 0;
    virtual size_t GetParamCount() const = 0;
};

// Binds the C++ function Func, which is a template argument so that Invoke calls it directly and the C++ compiler can
// inline it there. FuncType defaults to Func's own type, which the specialization below splits into Ret and Args.
template<auto Func, class FuncType = decltype(Func)>
class ExternalFunction;

template<auto Func, class Ret, class... Args>
class ExternalFunction<Func, Ret(*)(Args...)> : public IExternalFunction
{
public:
    std::string qualifiedName;
    using ObjectArgs = typename ObjectArgsOnly<Args...>::type;
    using Layout = ParamLayout<ObjectArgs>;
    constexpr static size_t ParamCount = std::tuple_size<ObjectArgs>::value;
    constexpr static uint32_t ReturnSize = Word::GetWordSize<Ret>();

    ExternalFunction(const std::string& qualifiedName)
        : qualifiedName(qualifiedName){}

    // Reads each argument at its fixed offset below argsEnd, which is one past the first argument, calls Func with
    // them, and constructs its result in the return storage below the arguments.
    virtual void Invoke(Program* program, Word* argsEnd) override
    {
        constexpr auto paramIndices = std::make_index_sequence<ParamCount>();

        auto call = [&]<size_t... Is>(std::index_sequence<Is...>) -> Ret {
            if constexpr(FirstArgIsProgram<Args...>)
                return Func( program, GetArg<Is>(argsEnd)... );
            else
                return Func( GetArg<Is>(argsEnd)... );
        };

        using RetU = std::remove_cv_t<Ret>;

        if constexpr(std::is_void_v<RetU>)
        {
            call(paramIndices);
        }
        else
        {
            Word* result = argsEnd - Layout::totalSize - ReturnSize;

            if constexpr(std::is_same_v<RetU, Boolean>)
                result->Set(call(paramIndices));
            else
                ::new (static_cast<void*>(result)) RetU(call(paramIndices));
        }
    }

    virtual std::span<WordType> GetParamTypes() override {
        return GetParamTypesImpl(std::make_index_sequence<ParamCount>());
    }

    virtual std::span<const uint32_t> GetParamSizes() override {
        return Layout::sizes;
    }

    virtual WordType GetReturnType() override {
        using ReturnType = std::remove_pointer_t<std::remove_cvref_t<Ret>>;
        if constexpr(!std::is_void_v<ReturnType>)
            return Word::GetWordType<ReturnType>();
        else
            return WordType::Void;
    }

    virtual uint32_t GetReturnSize() override {
        return ReturnSize;
    }

    virtual size_t GetParamCount() const override {
        return ParamCount;
    }

private:
    // Returns parameter I as a reference to its words on the stack.
    template<size_t I>
    static decltype(auto) GetArg(Word* argsEnd)
    {
        using Param = std::remove_cvref_t<std::tuple_element_t<I, ObjectArgs>>;
        return (argsEnd - Layout::offsets[I] - Layout::sizes[I])->Get<Param>();
    }

    template<size_t... Is>
    std::span<WordType> GetParamTypesImpl(std::index_sequence<Is...>) {
        static std::array<WordType, sizeof...(Is)> types {
            Word::GetWordType<std::remove_cvref_t<std::tuple_element_t<Is, ObjectArgs>>>()...
        };
        return types;
    }
};

} // fraze
