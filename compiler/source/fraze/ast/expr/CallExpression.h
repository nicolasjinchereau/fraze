/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <string>
#include <vector>
#include <fraze/ast/expr/Expression.h>

namespace fraze {

class CallExpression : public Expression
{
public:
    sptr<Expression> target;
    std::vector<sptr<Expression>> arguments;
    
    CallExpression(const SourceLocation& loc, Scope* scope, const sptr<Expression>& target)
        : Expression(loc, scope), target(target)
    {
    }

    template<class Args>
    CallExpression(const SourceLocation& loc, Scope* scope, const sptr<Expression>& target, Args&& args)
        : Expression(loc, scope), target(target), arguments(std::forward<Args>(args))
    {
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<CallExpression>(loc, scopes.GetCurrent(),
                target->Clone(scopes, nullptr)->ToExpression());

        copy->pushAsRef = pushAsRef;

        for(auto& arg : arguments)
        {
            auto argCopy = arg->Clone(scopes, nullptr)->ToExpression();
            copy->arguments.push_back(argCopy);
        }

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<CallExpression> ToCallExpression() override
    {
        return self();
    }

    virtual Type* EvaluateType() override
    {
        if(auto ident = target->ToIdentifierExpression())
        {
            if(auto func = ident->targetDef->ToFunctionDefinition())
                return func->returnType->GetType();
        }

        auto targetType = target->EvaluateType();
        
        if(targetType->GetDefinition())
        {
            if (auto functorClass = targetType->GetDefinition()->ToFunctorClassDefinition())
            {
                auto invokeFunc = functorClass->GetFunction("invoke");
                return invokeFunc->returnType->GetType();
            }
            else if (auto functorInterface = targetType->GetDefinition()->ToFunctorInterfaceDefinition())
            {
                auto invokeFunc = functorInterface->GetFunction("invoke");
                return invokeFunc->returnType->GetType();
            }
        }

        ENFORCE(false, target->loc, "invalid call target");
        return nullptr;
    }
};

} // fraze
