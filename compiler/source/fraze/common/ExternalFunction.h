/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <tuple>
#include <utility>
#include <fraze/common/Object.h>
#include <fraze/common/Platform.h>

namespace fraze {

class Program;

struct Operation;
struct StackFrame;

template<typename... Args>
concept FirstArgIsProgram = requires
{
    requires sizeof...(Args) >= 1;
    requires std::is_same_v<
        typename std::tuple_element<0, std::tuple<Args...>>::type,
            Program*
    >;
};

template<typename... Args>
struct ObjectArgsOnly {
    using type = std::tuple<Args...>;
};

template<typename... Rest>
struct ObjectArgsOnly<Program*, Rest...> {
    using type = std::tuple<Rest...>;
};

class IExternalFunction
{
public:
    virtual ~IExternalFunction(){}
    virtual void Invoke(Program* program, std::span<Word> ret, std::span<Word*> args) = 0;
    virtual std::span<WordType> GetParamTypes() = 0;
    virtual WordType GetReturnType() = 0;
    virtual size_t GetParamCount() const = 0;
};

template<class Ret, class... Args>
class ExternalFunction : public IExternalFunction
{
public:
    std::string qualifiedName;
    Ret(*func)(Args...);
    using ObjectArgs = typename ObjectArgsOnly<Args...>::type;
    constexpr static size_t ParamCount = std::tuple_size<ObjectArgs>::value;

    ExternalFunction(const std::string& qualifiedName, Ret(*func)(Args...))
        : qualifiedName(qualifiedName), func(func){}

    virtual void Invoke(Program* program, std::span<Word> result, std::span<Word*> args) override {
        assert(GetParamCount() == args.size());

        if constexpr(!std::is_void_v<Ret>) {
            assert((sizeof(Ret) / sizeof(Word)) == result.size());
        }

        InvokeImpl(program, result, args, std::make_index_sequence<ParamCount>());
    }

    virtual std::span<WordType> GetParamTypes() override {
        return GetParamTypesImpl(std::make_index_sequence<ParamCount>());
    }

    virtual WordType GetReturnType() override {
        using ReturnType = std::remove_pointer_t<std::remove_cvref_t<Ret>>;
        if constexpr(!std::is_void_v<ReturnType>)
            return Word::GetType<ReturnType>();
        else
            return WordType::Void;
    }

    virtual size_t GetParamCount() const override {
        return ParamCount;
    }

private:
    template<size_t... Is>
    void InvokeImpl(Program* program, std::span<Word> result, std::span<Word*> args, std::index_sequence<Is...>)
    {
        if constexpr(std::is_void_v<Ret>)
        {
            if constexpr(FirstArgIsProgram<Args...>)
                func( program, args[Is]->Get<std::remove_cvref_t<std::tuple_element_t<Is, ObjectArgs>>>()... );
            else
                func( args[Is]->Get<std::remove_cvref_t<std::tuple_element_t<Is, ObjectArgs>>>()... );
        }
        else
        {
            if constexpr(FirstArgIsProgram<Args...>)
            {
                Ret* returnAddress = reinterpret_cast<Ret*>(result.data());
                *returnAddress = func( program, args[Is]->Get<std::remove_cvref_t<std::tuple_element_t<Is, ObjectArgs>>>()... );
            }
            else
            {
                Ret* returnAddress = reinterpret_cast<Ret*>(result.data());
                *returnAddress = func( args[Is]->Get<std::remove_cvref_t<std::tuple_element_t<Is, ObjectArgs>>>()... );
            }
        }
    }

    template<size_t... Is>
    std::span<WordType> GetParamTypesImpl(std::index_sequence<Is...>) {
        static std::array<WordType, sizeof...(Is)> types {
            Word::GetType<std::remove_cvref_t<std::tuple_element_t<Is, ObjectArgs>>>()...
        };
        return types;
    }
};

using IntrinsicFunction = void (*)(const Operation& op, Word* stackTop, Word* bp);

} // fraze
