/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/def/Definition.h>
#include <fraze/ast/type/Type.h>
#include <string>

namespace fraze {

class Definition;

class TypeSpecifier : public ASTNode
{
public:
    Scope* scope{}; // starting point of upward type search
    shared_string baseTypeName;
    std::vector<sptr<TypeSpecifier>> templateArgs;
    int arrayDimensions{};
    Type* type{};

    TypeSpecifier(
        const SourceLocation& loc,
        Scope* scope,
        const shared_string& baseTypeName,
        int arrayDimensions = 0)
        : ASTNode(loc)
        , scope(scope)
        , baseTypeName(baseTypeName)
        , arrayDimensions(arrayDimensions)
    {
        assert(scope);
    }

    TypeSpecifier(
        const SourceLocation& loc,
        Scope* scope,
        const shared_string& baseTypeName,
        const std::vector<sptr<TypeSpecifier>>& templateArgs,
        int arrayDimensions = 0)
        : ASTNode(loc)
        , scope(scope)
        , baseTypeName(baseTypeName)
        , templateArgs(templateArgs)
        , arrayDimensions(arrayDimensions)
    {
        assert(scope);
    }

    TypeSpecifier(const SourceLocation& loc, Type* type);
    TypeSpecifier(const TypeSpecifier& other);

    static std::vector<sptr<TypeSpecifier>> CloneTypeSpecs(ScopeStack& scopes, const std::vector<sptr<TypeSpecifier>>& typeSpecs)
    {
        std::vector<sptr<TypeSpecifier>> ret;
        ret.reserve(typeSpecs.size());

        for(auto& typeSpec : typeSpecs)
        {
            auto copy = typeSpec->Clone(scopes, nullptr)->ToTypeSpecifier();
            ret.push_back(copy);
        }
        
        return ret;
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<TypeSpecifier>(
            loc, scopes.GetCurrent(), baseTypeName, CloneTypeSpecs(scopes, templateArgs), arrayDimensions);
        
        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<TypeSpecifier> ToTypeSpecifier() override {
        return self();
    }

    shared_string GetBaseTypeName(bool qualified = false) const
    {
        std::string_view view = baseTypeName;

        if(!qualified)
        {
            size_t dot = view.find_last_of('.');
            if(dot != std::string_view::npos)
                view.remove_prefix(dot + 1);
        }
        
        return shared_string(view);
    }

    shared_string GetTemplateArgs(bool qualified = false) const
    {
        std::string ret;

        if(!templateArgs.empty())
        {
            ret += "<";

            size_t i = 0;
            for(auto& arg : templateArgs)
            {
                if(i++ > 0)
                    ret += ",";

                ret += arg->GetTypeName(qualified);
            }

            ret += ">";
        }

        return shared_string(std::move(ret));
    }

    shared_string GetArrayDims() const
    {
        std::string ret;

        for(auto i : std::views::iota(0, arrayDimensions))
            ret += "[]";

        return shared_string(std::move(ret));
    }

    shared_string GetElementTypeName(bool qualified = false) const
    {
        std::string ret;
        ret += GetBaseTypeName(qualified);
        ret += GetTemplateArgs(true);
        return shared_string(std::move(ret));
    }

    shared_string GetTypeName(bool qualified = false) const
    {
        std::string ret;
        ret += GetBaseTypeName(qualified);
        ret += GetTemplateArgs(true);
        ret += GetArrayDims();
        return shared_string(std::move(ret));
    }

    bool IsVoid() const {
        return baseTypeName == "void" && arrayDimensions == 0;
    }

    Type* GetType() const {
        return type;
    }
};

} // fraze
