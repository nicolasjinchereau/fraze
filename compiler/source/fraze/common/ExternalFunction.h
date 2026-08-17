/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
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

        using RetU = std::remove_cvref_t<Ret>;
        if constexpr(!std::is_void_v<RetU>) {
            if constexpr(std::is_same_v<RetU, Boolean>)
                assert(result.size() == 1);
            else
                assert(result.size() == (sizeof(RetU) / sizeof(Word)));
        }

        InvokeImpl(program, result, args, std::make_index_sequence<ParamCount>());
    }

    virtual std::span<WordType> GetParamTypes() override {
        return GetParamTypesImpl(std::make_index_sequence<ParamCount>());
    }

    virtual WordType GetReturnType() override {
        using ReturnType = std::remove_pointer_t<std::remove_cvref_t<Ret>>;
        if constexpr(!std::is_void_v<ReturnType>)
            return Word::GetWordType<ReturnType>();
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
            Ret retVal;

            if constexpr(FirstArgIsProgram<Args...>)
                retVal = func( program, args[Is]->Get<std::remove_cvref_t<std::tuple_element_t<Is, ObjectArgs>>>()... );
            else
                retVal = func( args[Is]->Get<std::remove_cvref_t<std::tuple_element_t<Is, ObjectArgs>>>()... );

            if constexpr(std::is_same_v<std::remove_cvref_t<Ret>, Boolean>)
            {
                assert(result.size() == 1);
                result[0].Set(retVal);
            }
            else
            {
                Ret* returnAddress = reinterpret_cast<Ret*>(result.data());
                *returnAddress = retVal;
            }
        }
    }

    template<size_t... Is>
    std::span<WordType> GetParamTypesImpl(std::index_sequence<Is...>) {
        static std::array<WordType, sizeof...(Is)> types {
            Word::GetWordType<std::remove_cvref_t<std::tuple_element_t<Is, ObjectArgs>>>()...
        };
        return types;
    }
};

using IntrinsicFunction = void (*)(Word* basePointer);

} // fraze
