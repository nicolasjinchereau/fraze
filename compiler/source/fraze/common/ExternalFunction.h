/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <new>
#include <span>
#include <string>
#include <tuple>
#include <utility>
#include <fraze/common/Object.h>
#include <fraze/common/Platform.h>
#include <fraze/common/jump/LandingPad.h>

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

// How a parameter crosses the boundary to a JIT shim: a scalar by value and everything else by address, so the
// shim's signature holds no aggregate for MIR to classify.
template<class T>
struct JITParam
{
    using Param = std::remove_cvref_t<T>;

    using type =
        std::conditional_t<std::is_class_v<Param>, Param*,
        std::conditional_t<std::is_pointer_v<Param>, Param,
        std::conditional_t<std::is_same_v<Param, Number>, Number, Integer>>>;

    static decltype(auto) AsArgument(type value)
    {
        if constexpr(std::is_class_v<Param>)
            return (*value);
        else if constexpr(std::is_pointer_v<Param>)
            return value;
        else
            return Param(value);
    }
};

// A result that fits a register is returned, and a struct is constructed in storage the caller passes, since MIR
// returns no aggregate.
template<class T>
struct JITResult
{
    using Result = std::remove_cvref_t<T>;

    static constexpr bool isReturnedByAddress = std::is_class_v<Result>;

    using type =
        std::conditional_t<std::is_void_v<Result> || isReturnedByAddress, void,
        std::conditional_t<std::is_pointer_v<Result>, Result,
        std::conditional_t<std::is_same_v<Result, Number>, Number, Integer>>>;
};

class IExternalFunction
{
public:
    virtual ~IExternalFunction(){}
    virtual void Invoke(Program* program, Word* argsEnd) = 0;

    // The address of the shim a JIT calls instead of Invoke, whose signature JITParam and JITResult define.
    virtual void* GetJITEntry() = 0;
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

    virtual void* GetJITEntry() override
    {
        if constexpr(JITResult<Ret>::isReturnedByAddress)
            return reinterpret_cast<void*>(&JITEntry<ObjectArgs>::CallWithResultStorage);
        else
            return reinterpret_cast<void*>(&JITEntry<ObjectArgs>::Call);
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
    // The JIT's entry point for Func: Program* leads, and every parameter arrives in the form JITParam gives it.
    template<class Tuple>
    struct JITEntry;

    template<class... Params>
    struct JITEntry<std::tuple<Params...>>
    {
        static typename JITResult<Ret>::type Call(Program* program, typename JITParam<Params>::type... args)
        {
            if constexpr(std::is_void_v<typename JITResult<Ret>::type>)
                ApplyFunc(program, args...);
            else
                return typename JITResult<Ret>::type(ApplyFunc(program, args...));
        }

        static void CallWithResultStorage(
            Program* program, typename JITResult<Ret>::Result* result, typename JITParam<Params>::type... args)
        {
            ::new (static_cast<void*>(result)) typename JITResult<Ret>::Result(ApplyFunc(program, args...));
        }

    private:
        // Compiled code has no unwind data, so an exception out of Func can't travel back through it. It is
        // carried to the innermost landing pad instead, by a jump made after the 'catch' has been left.
        static Ret ApplyFunc(Program* program, typename JITParam<Params>::type... args)
        {
            LandingPad* pad = nullptr;

            try
            {
                if constexpr(FirstArgIsProgram<Args...>)
                    return Func(program, JITParam<Params>::AsArgument(args)...);
                else
                    return Func(JITParam<Params>::AsArgument(args)...);
            }
            catch(...)
            {
                pad = LandingPad::GetInnermost();
                assert(pad);
                pad->StoreFailure(std::current_exception());
            }

            pad->JumpToFailureHandling();
        }
    };

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
