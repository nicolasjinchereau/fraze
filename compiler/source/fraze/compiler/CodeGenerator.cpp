/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <ranges>
#include <print>
#include <fraze/compiler/CodeGenerator.h>
#include <fraze/compiler/Compiler.h>
#include <fraze/compiler/RuntimeTypeInfo.h>

namespace fraze {

size_t CodeGenerator::Emit(nullptr_t data) {
    size_t ret = program->data.size();
    program->data.push_back(data);
    program->dataTypes.push_back(WordType::Object);
    return ret;
}

size_t CodeGenerator::Emit(Boolean data) {
    size_t ret = program->data.size();
    program->data.push_back(data);
    program->dataTypes.push_back(WordType::Boolean);
    return ret;
}

size_t CodeGenerator::Emit(Integer data) {
    size_t ret = program->data.size();
    program->data.push_back(data);
    program->dataTypes.push_back(WordType::Integer);
    return ret;
}

size_t CodeGenerator::Emit(Number data) {
    size_t ret = program->data.size();
    program->data.push_back(data);
    program->dataTypes.push_back(WordType::Number);
    return ret;
}

size_t CodeGenerator::Emit(String* data) {
    size_t ret = program->data.size();
    program->data.push_back(data);
    program->dataTypes.push_back(WordType::String);
    return ret;
}

void CodeGenerator::PopExpression(const sptr<Expression>& node, const sptr<Expression>& source) // pop into 'node'
{
    // how many words to pop
    size_t varSize = 1;
    
    auto targetType = node->EvaluateType();
    if(targetType->IsStruct())
    {
        varSize = typeInfo[targetType]->ToStructInfo()->size;
    }

    if(auto ident = node->ToIdentifierExpression())
    {
        if(auto varDef = ident->targetDef->ToVariableDefinition())
        {
            bool hasContext = false;
            bool isContextStruct = false;

            if(ident->context && Expression::IsValueExpression(ident->context))
            {
                // IdentifierExpression: emits Push(Local|Global|Argument) or nothing for SectionDefinition
                auto sz = program->code.size();
                VisitChild(ident->context);
                assert(program->code.size() > sz);
                hasContext = true;

                if(ident->context->EvaluateType()->IsStruct())
                {
                    isContextStruct = true;
                }
            }

            if(hasContext)
            {
                if(isContextStruct)
                {
                    ENFORCE(!ident->context || !ident->context->ToCallExpression(), ident->context->loc, "Cannot directly modify a struct returned from a function");
                }

                uint64_t offset = FieldOffset(ident->context, varDef->offset);

                if(varSize > 1)
                    Emit(node->loc, OpCode::PopWordN, offset, varSize);
                else
                    Emit(node->loc, OpCode::PopWord, offset);
            }
            else
            {
                if(varDef->isStatic)
                {
                    Emit(node->loc, OpCode::PopGlobal, varDef->offset, varSize);
                }
                else
                {
                    if(varSize > 1)
                        Emit(node->loc, OpCode::PopLocalN, varDef->offset, varSize);
                    else
                        Emit(node->loc, OpCode::PopLocal, varDef->offset);
                }
            }
        }
        else if(auto paramDef = ident->targetDef->ToParameterDefinition())
        {
            ENFORCE(!paramDef->isReference, ident->loc, "cannot assign to '{}'", paramDef->name);
            Emit(node->loc, OpCode::PopArgument, paramDef->offset, varSize);
        }
        else
        {
            // is a function, class, section, or built-in type
        }
    }
    else if(auto ind = node->ToIndexExpression())
    {
        // should leave an Array on the stack which can be indexed
        VisitChild(ind->target);

        // should leave an integer on the stack by which the array can be indexed
        VisitChild(ind->arg);

        // replace the array and index with the address of the element
        Emit(node->loc, OpCode::PushIndexAddr, (uint64_t)ArrayDataOffset, varSize);

        // store the value pushed by the caller into the element
        if(varSize > 1)
            Emit(node->loc, OpCode::PopWordN, 0ull, varSize);
        else
            Emit(node->loc, OpCode::PopWord, 0ull);
    }
}

void CodeGenerator::EmitFieldInitializer(sptr<Expression>& value, size_t fieldOffset, size_t fieldSize)
{
    uint64_t offset = ClassDataOffset + fieldOffset;

    // evaluate the field's value above the object reference
    VisitChild(value);

    // push a copy of the object reference from below the value
    Emit(value->loc, OpCode::PushOffset, fieldSize, 1ull);

    // store the value in the field, leaving the original object reference
    if(fieldSize > 1)
        Emit(value->loc, OpCode::PopWordN, offset, (uint64_t)fieldSize);
    else
        Emit(value->loc, OpCode::PopWord, offset);
}

void CodeGenerator::EmitConversion(sptr<Expression>& value, const sptr<TypeSpecifier>& resultTypeSpec)
{
    Type* sourceType = value->EvaluateType();
    Type* resultType = resultTypeSpec->type;

    assert(sourceType);
    assert(resultType);

    // evaluate the source expression and convert it in place
    VisitChild(value);

    if(resultType->IsBoolean())
    {
        if(sourceType->IsObject())
        {
            // unbox boolean value
            Emit(value->loc, OpCode::PushWord, (uint64_t)ClassDataOffset);
        }
    }
    else if(resultType->IsInteger())
    {
        if(sourceType->IsObject())
        {
            // unbox integer value
            Emit(value->loc, OpCode::PushWord, (uint64_t)ClassDataOffset);
        }
        else if(sourceType->IsNumber())
        {
            Emit(value->loc, OpCode::ConvNumToInt);
        }
        // enum is already integer
    }
    else if(resultType->IsNumber())
    {
        if(sourceType->IsObject())
        {
            // unbox number value
            Emit(value->loc, OpCode::PushWord, (uint64_t)ClassDataOffset);
        }
        if(sourceType->IsInteger())
        {
            Emit(value->loc, OpCode::ConvIntToNum);
        }
    }
    else if(resultType->IsString())
    {
        if(sourceType->IsObject())
        {
            // should be lowered to Type.AsInstance or a cast in semantic analyzer
            assert(0);
        }
        else if(sourceType->IsBoolean())
        {
            //Emit(value->loc, OpCode::ConvBoolToStr);
            assert(0);
        }
        else if(sourceType->IsInteger())
        {
            //Emit(value->loc, OpCode::ConvIntToStr);
            assert(0);
        }
        else if(sourceType->IsNumber())
        {
            //Emit(value->loc, OpCode::ConvNumToStr);
            assert(0);
        }
        else if(sourceType->IsEnum())
        {
            //Emit(value->loc, OpCode::ConvEnumToStr, typeInfo[sourceType]->id);
            assert(0);
        }
    }
    else if(resultType->IsInterface())
    {
        if(sourceType->IsObject() || sourceType->IsClass() || sourceType->IsInterface())
        {
            // should be lowered to Type.AsInstance or a cast in semantic analyzer
            assert(0);
        }
    }
    else if(resultType->IsClass())
    {
        if(sourceType->IsObject() || sourceType->IsInterface())
        {
            // should be lowered to Type.AsInstance or a cast in semantic analyzer
            assert(0);
        }
    }
    else if(resultType->IsArray())
    {
        if(sourceType->IsObject())
        {
            // should be lowered to Type.AsInstance in semantic analyzer
            assert(0);
        }
    }
}

// Emits 'condition' such that one result jumps and the other falls through, depending on which
// jump list is given. Normally the condition leaves its result on the stack and gets a JumpIf or
// JumpIfNot, but &&, || and ! leave nothing on the stack here: they recurse with a destination per
// operand, so the first operand that decides the outcome performs a jump. The code a jump lands on
// hasn't been generated yet, so each jump's index is collected in the caller's list, and the caller
// fills in the destination once it knows where that code starts.
void CodeGenerator::EmitConditionalJumps(const sptr<Expression>& condition, std::vector<size_t>* trueJumpIndices, std::vector<size_t>* falseJumpIndices)
{
    // one result jumps, the other falls through
    assert(!trueJumpIndices != !falseJumpIndices);

    if(auto binaryExpr = condition->ToBinaryExpression())
    {
        if(binaryExpr->operation == TokenType::LogicalAnd || binaryExpr->operation == TokenType::LogicalOr)
        {
            // 'left' decides the result by itself when it short circuits: false for &&, true for ||
            bool isAnd = binaryExpr->operation == TokenType::LogicalAnd;
            auto shortCircuitJumpIndices = isAnd ? falseJumpIndices : trueJumpIndices;

            if(shortCircuitJumpIndices)
            {
                // the short circuit jumps to the same destination as 'right'
                EmitConditionalJumps(binaryExpr->left, trueJumpIndices, falseJumpIndices);
                EmitConditionalJumps(binaryExpr->right, trueJumpIndices, falseJumpIndices);
            }
            else
            {
                // the short circuit continues below instead, so 'left' jumps past 'right'
                std::vector<size_t> jumpIndicesPastRight;
                EmitConditionalJumps(binaryExpr->left, isAnd ? nullptr : &jumpIndicesPastRight, isAnd ? &jumpIndicesPastRight : nullptr);
                EmitConditionalJumps(binaryExpr->right, trueJumpIndices, falseJumpIndices);
                PatchJumps(jumpIndicesPastRight, program->code.size());
            }

            return;
        }
    }
    else if(auto prefixExpr = condition->ToPrefixExpression())
    {
        if(prefixExpr->operation == TokenType::LogicalNot)
        {
            // negation only swaps which result jumps
            EmitConditionalJumps(prefixExpr->arg, falseJumpIndices, trueJumpIndices);
            return;
        }
    }

    VisitChildNode(condition);

    auto jumpIndices = trueJumpIndices ? trueJumpIndices : falseJumpIndices;
    jumpIndices->push_back(program->code.size());
    Emit(condition->loc, trueJumpIndices ? OpCode::JumpIf : OpCode::JumpIfNot, -1);
}

/*****************************
*            ROOT            *
*****************************/

void CodeGenerator::Visit(const sptr<ASTRoot>& node)
{
    program = spnew<Program>();
    
    auto compiler = Compiler::GetActiveCompiler();
    if(!compiler)
        throw Exception("no active compiler");

    auto rtti = RuntimeTypeInfo(node, compiler->types);
    typeInfo = std::move(rtti.typeInfoByType);
    program->typeInfo = std::move(rtti.allTypeInfo);
    program->intrinsics = std::move(rtti.intrinsics);
    program->globalCount = rtti.globalSize;

    ASTVisitor::Visit(node);
}

/*****************************
*         DEFINITIONS        *
*****************************/

void CodeGenerator::Visit(const sptr<BasicTypeDefinition>& node) {
    ASTVisitor::Visit(node);
}

void CodeGenerator::Visit(const sptr<ClassDefinition>& node)
{
    if(node->IsTemplateDeclaration())
        return;

    ASTVisitor::Visit(node);
}

void CodeGenerator::Visit(const sptr<EnumDefinition>& node)
{
}

void CodeGenerator::Visit(const sptr<EnumMemberDefinition>& node)
{
}

void CodeGenerator::Visit(const sptr<FunctionDefinition>& node)
{
    if(node->IsTemplateDeclaration())
        return;

    auto* type = typeInfo[node->type]->ToFunctionInfo();
    type->codeStart = (uint32_t)program->code.size();
    VisitChild(node->body);
    type->codeEnd = (uint32_t)program->code.size();
}

void CodeGenerator::Visit(const sptr<InterfaceDefinition>& node)
{
    if(node->IsTemplateDeclaration())
        return;

    ASTVisitor::Visit(node);
}

void CodeGenerator::Visit(const sptr<ParameterDefinition>& node) {
    ASTVisitor::Visit(node);
}

void CodeGenerator::Visit(const sptr<SectionDefinition>& node)
{
    SectionInfo* type = typeInfo[node->type]->ToSectionInfo();

    type->codeStart = (uint32_t)program->code.size();

    for (auto& stmt : node->statements)
        VisitChild(stmt);

    type->codeEnd = (uint32_t)program->code.size();

    for (auto& def : node->scope->definitions)
    {
        if(!def->ToVariableDefinition())
            VisitChild(def);
    }
}

void CodeGenerator::Visit(const sptr<StructDefinition>& node)
{
    if(node->IsTemplateDeclaration())
        return;

    ASTVisitor::Visit(node);
}

void CodeGenerator::Visit(const sptr<TemplateDefinition>& node) {
    ASTVisitor::Visit(node);
}

void CodeGenerator::Visit(const sptr<TemplateParameterDefinition>& node) {
    ASTVisitor::Visit(node);
}

void CodeGenerator::Visit(const sptr<VariableDefinition>& node)
{
    VisitChild(node->typeSpec);

    if(node->initializer)
    {
        VisitChild(node->initializer);
    }
    else
    {
        Type* type = node->typeSpec->type;
        
        if(type->IsNullable()) // object, array, class, interface, string, functor
            Emit(node->loc, OpCode::PushNull);
        else if(type == Type::Get("bool"))
            Emit(node->loc, OpCode::PushBoolean, 0);
        else if(type == Type::Get("int"))
            Emit(node->loc, OpCode::PushInteger, 0);
        else if(type == Type::Get("num"))
            Emit(node->loc, OpCode::PushNumber, 0.0);
    }
}

/*****************************
*         EXPRESSIONS        *
*****************************/


void CodeGenerator::Visit(const sptr<AsExpression>& node)
{
    Type* valueType = node->value->EvaluateType();
    Type* targetType = node->typeSpec->GetType();

    if (valueType == targetType)
    {
        VisitChild(node->value);
    }
    else
    {
        EmitConversion(node->value, node->typeSpec);
    }
}

void CodeGenerator::Visit(const sptr<AssignExpression>& node)
{
    Type* type = node->left->EvaluateType();
    Type* otherType = node->right->EvaluateType();
    assert(type);
    assert(otherType);
    assert(type == otherType || type->IsNullable() && otherType->IsNull());

    switch(node->operation)
    {
    case TokenType::Assign:
        VisitChild(node->right);
        break;

    case TokenType::AddAssign:
        VisitChild(node->left);
        VisitChild(node->right);

        if (type->IsInteger())
            Emit(node->loc, OpCode::AddInt);
        else if (type->IsNumber())
            Emit(node->loc, OpCode::AddNum);
        else if (type->IsString())
            // should have been lowered to (left = String.Concat(left, right))
            assert(0);
        else
            assert(0);
        break;

    case TokenType::SubAssign:
        VisitChild(node->left);
        VisitChild(node->right);

        if(type->IsInteger())
            Emit(node->loc, OpCode::SubInt);
        else if(type->IsNumber())
            Emit(node->loc, OpCode::SubNum);
        else
            assert(0);
        break;

    case TokenType::MulAssign:
        VisitChild(node->left);
        VisitChild(node->right);
        
        if(type->IsInteger())
            Emit(node->loc, OpCode::MulInt);
        else if(type->IsNumber())
            Emit(node->loc, OpCode::MulNum);
        else
            assert(0);
        break;

    case TokenType::DivAssign:
        VisitChild(node->left);
        VisitChild(node->right);
        
        if(type->IsInteger())
            Emit(node->loc, OpCode::DivInt);
        else if(type->IsNumber())
            Emit(node->loc, OpCode::DivNum);
        else
            assert(0);
        break;

    case TokenType::ModAssign:
        VisitChild(node->left);
        VisitChild(node->right);

        if(type->IsInteger())
            Emit(node->loc, OpCode::ModInt);
        else if(type->IsNumber())
            Emit(node->loc, OpCode::ModNum);
        else
            assert(0);
        break;

    case TokenType::LeftShiftAssign:
        VisitChild(node->left);
        VisitChild(node->right);

        assert(type->IsInteger());
        Emit(node->loc, OpCode::LeftShift);
        break;

    case TokenType::RightShiftAssign:
        VisitChild(node->left);
        VisitChild(node->right);

        assert(type->IsInteger());
        Emit(node->loc, OpCode::RightShift);
        break;

    case TokenType::BitAndAssign:
        VisitChild(node->left);
        VisitChild(node->right);

        assert(type->IsInteger());
        Emit(node->loc, OpCode::BitAnd);
        break;

    case TokenType::BitOrAssign:
        VisitChild(node->left);
        VisitChild(node->right);

        assert(type->IsInteger());
        Emit(node->loc, OpCode::BitOr);
        break;

    case TokenType::BitXorAssign:
        VisitChild(node->left);
        VisitChild(node->right);

        assert(type->IsInteger());
        Emit(node->loc, OpCode::BitXor);
        break;
    }

    // duplicate 'right' so the assignment yields a value
    if(!TryCancelExpressionStatementPop(node.get()))
    {
        auto targetType = node->EvaluateType();
        size_t varSize = typeInfo[targetType]->GetSize();
        if(varSize > 1)
            Emit(node->loc, OpCode::DupN, varSize);
        else
            Emit(node->loc, OpCode::Dup);
    }

    // pop the value into 'left'
    PopExpression(node->left, node->right);
}

void CodeGenerator::Visit(const sptr<BinaryExpression>& node)
{
    if(node->operation == TokenType::LogicalAnd || node->operation == TokenType::LogicalOr)
    {
        // the result is kept, so generate the jumps a condition would make, then push what they decided
        std::vector<size_t> falseJumpIndices;
        EmitConditionalJumps(node, nullptr, &falseJumpIndices);

        Emit(node->loc, OpCode::PushBoolean, 1);
        size_t jumpOverFalseValueCodeIndex = program->code.size();
        Emit(node->loc, OpCode::Jump, -1);

        PatchJumps(falseJumpIndices, program->code.size());

        Emit(node->loc, OpCode::PushBoolean, 0);
        program->code[jumpOverFalseValueCodeIndex].arg1_u64 = program->code.size();
        return;
    }

    // push args to stack
    VisitChild(node->left);
    VisitChild(node->right);

    Type* type = node->left->EvaluateType();
    Type* otherType = node->right->EvaluateType();
    assert(type);
    assert(otherType);
    assert(type == otherType);

    switch(node->operation)
    {
    case TokenType::BitOr:
        assert(type->IsInteger() || type->IsEnum());
        Emit(node->loc, OpCode::BitOr);
        break;
    case TokenType::BitXor:
        assert(type->IsInteger() || type->IsEnum());
        Emit(node->loc, OpCode::BitXor);
        break;
    case TokenType::BitAnd:
        assert(type->IsInteger() || type->IsEnum());
        Emit(node->loc, OpCode::BitAnd);
        break;
    case TokenType::BitTest:
        assert(type->IsInteger() || type->IsEnum());
        Emit(node->loc, OpCode::BitAnd);
        Emit(node->loc, OpCode::PushInteger, 0);
        Emit(node->loc, OpCode::GreaterInt);
        break;
    case TokenType::LeftShift:
        assert(type->IsInteger() || type->IsEnum());
        Emit(node->loc, OpCode::LeftShift);
        break;
    case TokenType::RightShift:
        assert(type->IsInteger() || type->IsEnum());
        Emit(node->loc, OpCode::RightShift);
        break;

    case TokenType::Equal:
        if(type->IsString())
        {
            // should have been lowered to String.Equals(left, right)
            assert(0);
        }
        else
        {
            size_t size = typeInfo[type]->GetSize();
            if(size > 1)
                Emit(node->loc, OpCode::EqualN, size);
            else
                Emit(node->loc, OpCode::Equal);
        }
        break;

    case TokenType::NotEqual:
        if(type->IsString())
        {
            // should have been lowered to !String.Equals(left, right)
            assert(0);
        }
        else
        {
            size_t size = typeInfo[type]->GetSize();
            if(size > 1)
                Emit(node->loc, OpCode::NotEqualN, size);
            else
                Emit(node->loc, OpCode::NotEqual);
        }
        break;
    
    case TokenType::Less:
        if(type->IsInteger())
            Emit(node->loc, OpCode::LessInt);
        else if(type->IsNumber())
            Emit(node->loc, OpCode::LessNum);
        else
            assert(0);
        break;
    case TokenType::LessEqual:
        if(type->IsInteger())
            Emit(node->loc, OpCode::LessEqualInt);
        else if(type->IsNumber())
            Emit(node->loc, OpCode::LessEqualNum);
        else
            assert(0);
        break;
    case TokenType::Greater:
        if(type->IsInteger())
            Emit(node->loc, OpCode::GreaterInt);
        else if(type->IsNumber())
            Emit(node->loc, OpCode::GreaterNum);
        else
            assert(0);
        break;
    case TokenType::GreaterEqual:
        if(type->IsInteger())
            Emit(node->loc, OpCode::GreaterEqualInt);
        else if(type->IsNumber())
            Emit(node->loc, OpCode::GreaterEqualNum);
        else
            assert(0);
        break;

    case TokenType::Add:
        if (type->IsInteger())
            Emit(node->loc, OpCode::AddInt);
        else if (type->IsNumber())
            Emit(node->loc, OpCode::AddNum);
        else if (type->IsString())
            // should have been lowered to String.Concat(left, right)
            assert(0);
        else
            assert(0);
        break;
    case TokenType::Sub:
        if(type->IsInteger())
            Emit(node->loc, OpCode::SubInt);
        else if(type->IsNumber())
            Emit(node->loc, OpCode::SubNum);
        else
            assert(0);
        break;
    case TokenType::Mul:
        if(type->IsInteger())
            Emit(node->loc, OpCode::MulInt);
        else if(type->IsNumber())
            Emit(node->loc, OpCode::MulNum);
        else
            assert(0);
        break;
    case TokenType::Div:
        if(type->IsInteger())
            Emit(node->loc, OpCode::DivInt);
        else if(type->IsNumber())
            Emit(node->loc, OpCode::DivNum);
        else
            assert(0);
        break;
    case TokenType::Mod:
        if(type->IsInteger())
            Emit(node->loc, OpCode::ModInt);
        else if(type->IsNumber())
            Emit(node->loc, OpCode::ModNum);
        else
            assert(0);
        break;
    }
}

void CodeGenerator::Visit(const sptr<BooleanLiteralExpression>& node) {
    Emit(node->loc, OpCode::PushBoolean, node->value ? 1 : 0);
}

void CodeGenerator::Visit(const sptr<CachedExpression>& node)
{
    ASTVisitor::Visit(node);
}

void CodeGenerator::Visit(const sptr<CallExpression>& node)
{
    auto targetType = node->target->EvaluateType();
    
    if(targetType->IsFunction())
    {
        auto ident = node->target->ToIdentifierExpression();
        auto func = ident->targetDef->ToFunctionDefinition();
        auto funcInfo = typeInfo[func->type]->ToFunctionInfo();
        auto interfaceID = size_t(-1);

        if(func->HasImplicitThisParam())
        {
            Type* contextType = node->arguments[0]->EvaluateType();
            if(contextType->IsInterface())
            {
                interfaceID = typeInfo[contextType]->ToInterfaceInfo()->id;
            }
        }

        // return storage
        EmitReserve(node->loc, funcInfo->returnSize);

        // args (reverse order)
        for(auto& arg : std::views::reverse(node->arguments))
        {
            VisitChild(arg);
        }

        // call
        if(interfaceID != size_t(-1))
        {
            Emit(node->loc, OpCode::CallVirtual, funcInfo->id, interfaceID);
        }
        else
        {
            if(funcInfo->isExternal)
            {
                if(funcInfo->externalFunction)
                {
                    Emit(node->loc, OpCode::CallExternal, funcInfo->id);
                }
                else if(funcInfo->intrinsicID != -1)
                {
                    Operation op(OpCode::CallIntrinsic);
                    op.arg1_u32a = (uint32_t)funcInfo->id;
                    op.arg1_u32b = (uint32_t)funcInfo->intrinsicID;
                    op.arg2_u32a = funcInfo->returnSize;
                    op.arg2_u32b = funcInfo->paramSize;
                    Emit(node->loc, op);
                }
                else
                {
                    assert(0);
                }
            }
            else
            {
                Emit(node->loc, OpCode::Call, funcInfo->id);
            }
        }
    }
    else if(targetType->IsFunctorInterface())
    {
        auto functorInterface = targetType->GetDefinition()->ToFunctorInterfaceDefinition();
        auto invokeFunc = functorInterface->GetFunction("invoke");
        auto invokeFuncInfo = typeInfo[invokeFunc->type]->ToFunctionInfo();
        auto interfaceTypeInfo = typeInfo[functorInterface->type]->ToInterfaceInfo();
        
        // return storage
        EmitReserve(node->loc, invokeFuncInfo->returnSize);

        // args (reverse order)
        for(auto& arg : std::views::reverse(node->arguments))
        {
            // emit data and emit Push(literal)
            VisitChild(arg);
        }

        // call invoke function for this functor type
        Emit(node->loc, OpCode::CallVirtual, invokeFuncInfo->id, interfaceTypeInfo->id);
    }
    else if(targetType->IsFunctorClass())
    {
        auto functorClass = targetType->GetDefinition()->ToFunctorClassDefinition();
        auto invokeFunc = functorClass->GetFunction("invoke");
        auto invokeFuncInfo = typeInfo[invokeFunc->type]->ToFunctionInfo();

        // return storage
        EmitReserve(node->loc, invokeFuncInfo->returnSize);

        // args (reverse order)
        for(auto& arg : std::views::reverse(node->arguments))
        {
            // emit data and emit Push(literal)
            VisitChild(arg);
        }

        // call invoke function for this functor type
        Emit(node->loc, OpCode::Call, invokeFuncInfo->id);
    }
    else
    {
        ENFORCE(false, node->loc, "invalid call target");
    }
}

void CodeGenerator::Visit(const sptr<CastExpression>& node)
{
    VisitChild(node->value);
}

void CodeGenerator::Visit(const sptr<ConvertExpression>& node)
{
    EmitConversion(node->value, node->resultTypeSpec);
}

void CodeGenerator::Visit(const sptr<DefaultValueExpression>& node)
{
    Type* type = node->typeSpec->type;

    if(type->IsNullable())
        Emit(node->loc, OpCode::PushNull);
    else if(type->IsStruct())
    {
        // each field is default-initialized so that the struct has a well defined value
        auto structDef = type->GetDefinition()->ToStructDefinition();

        for(const auto& field : structDef->GetChildren<VariableDefinition>([](auto& f) { return !f->isStatic; }))
            VisitChild(field->initializer);
    }
    else if(type->IsBoolean())
        Emit(node->loc, OpCode::PushBoolean, 0);
    else if(type->IsInteger())
        Emit(node->loc, OpCode::PushInteger, 0);
    else if(type->IsNumber())
        Emit(node->loc, OpCode::PushNumber, 0.0);
    else if(type->IsEnum())
        Emit(node->loc, OpCode::PushInteger, 0);
    else
        assert(0);
}

void CodeGenerator::Visit(const sptr<FoldExpression>& node)
{
    auto& statements = node->body->statements;

    for(size_t i = 0; i != statements.size() - 1; ++i)
        VisitChild(statements[i]);

    auto finalStatement = statements.back()->ToExpressionStatement();
    VisitChild(finalStatement->expression);

    // expression result left on stack
}

void CodeGenerator::Visit(const sptr<IdentifierExpression>& node)
{
    // how many words to push
    size_t pushSize = 1;

    auto targetType = node->EvaluateType();
    if(targetType->IsStruct())
    {
        auto structDef = targetType->GetDefinition()->ToStructDefinition();
        pushSize = node->pushAsRef ? 0 : structDef->size;
    }

    if(auto varDef = node->targetDef->ToVariableDefinition())
    {
        bool hasContext = false;

        if(node->context && Expression::IsValueExpression(node->context))
        {
            auto sz = program->code.size();
            VisitChild(node->context);
            assert(program->code.size() > sz);
            hasContext = true;
        }

        if(hasContext)
        {
            uint64_t offset = FieldOffset(node->context, varDef->offset);

            if(pushSize == 0)
            {
                Emit(node->loc, OpCode::PushWordAddr, offset);
            }
            else if(pushSize == 1)
            {
                Emit(node->loc, OpCode::PushWord, offset);
            }
            else
            {
                assert(pushSize > 1);
                Emit(node->loc, OpCode::PushWordN, offset, pushSize);
            }
        }
        else
        {
            if(varDef->isStatic)
            {
                if(pushSize > 0)
                    Emit(node->loc, OpCode::PushGlobal, varDef->offset, pushSize);
                else
                    Emit(node->loc, OpCode::PushGlobalAddr, varDef->offset);
            }
            else
            {
                if(pushSize == 0)
                {
                    Emit(node->loc, OpCode::PushLocalAddr, varDef->offset);
                }
                else if(pushSize == 1)
                {
                    Emit(node->loc, OpCode::PushLocal, varDef->offset);
                }
                else
                {
                    assert(pushSize > 1);
                    Emit(node->loc, OpCode::PushLocalN, varDef->offset, pushSize);
                }
            }
        }
    }
    else if(auto paramDef = node->targetDef->ToParameterDefinition())
    {
        if(paramDef->isReference)
        {
            // the parameter holds the address of the struct, so it is one word
            Emit(node->loc, OpCode::PushArgument, paramDef->offset);

            // dereference it unless the address itself is what's wanted
            if(pushSize == 1)
                Emit(node->loc, OpCode::PushWord, 0ull);
            else if(pushSize > 1)
                Emit(node->loc, OpCode::PushWordN, 0ull, pushSize);
        }
        else if(pushSize == 0)
        {
            Emit(node->loc, OpCode::PushArgumentAddr, paramDef->offset, paramDef->size);
        }
        else if(pushSize == 1)
        {
            Emit(node->loc, OpCode::PushArgument, paramDef->offset);
        }
        else
        {
            Emit(node->loc, OpCode::PushArgumentN, paramDef->offset, pushSize);
        }
    }
    else if(auto enumMemberDef = node->targetDef->ToEnumMemberDefinition())
    {
        VisitChild(enumMemberDef->value);
    }
    else if(node->targetDef->ToClassDefinition() || node->targetDef->ToStructDefinition())
    {
        // a type name used to qualify a static member emits nothing
    }
    else
    {
        // identifier not implemented
        assert(0);
    }
}

void CodeGenerator::Visit(const sptr<IndexExpression>& node)
{
    auto arrayType = node->target->EvaluateType();
    auto elementType = arrayType->GetElementType();

    uint64_t elementSize = 1;
    if(elementType->IsStruct())
        elementSize = typeInfo[elementType]->ToStructInfo()->size;

    // struct elements can be pushed by address so they can be assigned to or mutated in place
    uint64_t pushSize = (elementType->IsStruct() && node->pushAsRef) ? 0 : elementSize;

    // should leave an Array on the stack which can be indexed
    VisitChild(node->target);

    // should leave an integer on the stack by which the array can be indexed
    VisitChild(node->arg);

    // replace the array and index with the address of the element
    Emit(node->loc, OpCode::PushIndexAddr, (uint64_t)ArrayDataOffset, elementSize);

    // replace the address with the value it points to
    if(pushSize == 1)
        Emit(node->loc, OpCode::PushWord, 0ull);
    else if(pushSize > 1)
        Emit(node->loc, OpCode::PushWordN, 0ull, pushSize);
}

// NOTE: 'await' must be the whole of a statement or the whole of a variable
// initializer. Suspending emits a Return that discards the operand stack, so any
// values already pushed for an enclosing expression are lost and the resumed code
// runs with an underflowed stack. Locals are safe because Parser hoists a
// coroutine's locals into fields of the state object, but partially evaluated
// expressions are not. There is currently no diagnostic for this.
void CodeGenerator::Visit(const sptr<AwaitExpression>& node)
{
    auto awaitableType = Type::Get("Awaitable");
    size_t awaitableInterfaceID = typeInfo[awaitableType]->ToInterfaceInfo()->id;

    auto awaiterType = Type::Get("Awaiter");
    size_t awaiterInterfaceID = typeInfo[awaiterType]->ToInterfaceInfo()->id;

    auto exprType = node->expression->EvaluateType();
    auto taskDef = exprType->GetDefinition()->ToInterfaceDefinition();
    size_t taskInterfaceID = typeInfo[exprType]->ToInterfaceInfo()->id;

    auto contextDef = node->context->EvaluateType()->GetDefinition();
    auto awaited = contextDef->GetVariable("$awaited");
    uint64_t awaitedOffset = FieldOffset(node->context, awaited->offset);

    // push Task<T> and save to temporary
    VisitChild(node->expression);
    VisitChild(node->context);
    Emit(node->loc, OpCode::PopWord, awaitedOffset);

    // if( awaitable.IsDone() )
    auto isDoneFunc = taskDef->GetFunction("IsDone");
    auto isDoneFuncInfo = typeInfo[isDoneFunc->type]->ToFunctionInfo();
    auto isDoneFuncID = isDoneFuncInfo->id;
    EmitReserve(node->loc, isDoneFuncInfo->returnSize);
    VisitChild(node->context);
    Emit(node->loc, OpCode::PushWord, awaitedOffset);
    Emit(node->loc, OpCode::CallVirtual, isDoneFuncID, taskInterfaceID);
    size_t jump1 = program->code.size();
    Emit(node->loc, OpCode::JumpIfNot, -1);
    
    // push awaitable.GetValue()
    auto getValueFunc = taskDef->GetFunction("GetValue");
    auto getvalueFuncInfo = typeInfo[getValueFunc->type]->ToFunctionInfo();
    auto getValueFuncID = getvalueFuncInfo->id;
    if(!getValueFunc->returnType->IsVoid())
    {
        EmitReserve(node->loc, getvalueFuncInfo->returnSize);
        VisitChild(node->context);
        Emit(node->loc, OpCode::PushWord, awaitedOffset);
        Emit(node->loc, OpCode::CallVirtual, getValueFuncID, taskInterfaceID);
    }
    else
    {
        EmitReserve(node->loc, getvalueFuncInfo->returnSize);
    }
    // jump to end
    size_t jump2 = program->code.size();
    Emit(node->loc, OpCode::Jump, -1);
    
    program->code[jump1].arg1_u64 = program->code.size();
    // awaitable.SetAwaiter(this)
    auto setAwaiterType = Type::Get("Awaitable.SetAwaiter");
    auto setAwaiterFuncInfo = typeInfo[setAwaiterType]->ToFunctionInfo();
    auto setAwaiterFuncID = setAwaiterFuncInfo->id;
    EmitReserve(node->loc, setAwaiterFuncInfo->returnSize);
    VisitChild(node->context); // push this frame's task as the 'awaiter' arg
    VisitChild(node->context);
    Emit(node->loc, OpCode::PushWord, awaitedOffset);
    Emit(node->loc, OpCode::CallVirtual, setAwaiterFuncID, awaitableInterfaceID);

    size_t paramSize = 0;
    size_t returnSize = 1;
    if(auto func = node->scope->owner->ToFunctionDefinition())
    {
        paramSize = func->paramSize;
        returnSize = typeInfo[func->type]->ToFunctionInfo()->returnSize;
    }

    // store the resume location in $position and return
    size_t resumeLocation = program->code.size();
    Emit(node->loc, OpCode::PushInteger, -1);
    VisitChild(node->context);
    Emit(node->loc, OpCode::PopWord, FieldOffset(node->context, contextDef->GetVariable("$position")->offset));
    if(returnSize != 0)
        Emit(node->loc, OpCode::PushNull);
    Emit(node->loc, OpCode::Return, paramSize, returnSize);
    program->code[resumeLocation].arg1_u64 = program->code.size();
    
    // push $awaited.GetValue()
    if(!getValueFunc->returnType->IsVoid())
    {
        EmitReserve(node->loc, getvalueFuncInfo->returnSize);
        VisitChild(node->context);
        Emit(node->loc, OpCode::PushWord, awaitedOffset);
        Emit(node->loc, OpCode::CallVirtual, getValueFuncID, taskInterfaceID);
    }
    else
    {
        EmitReserve(node->loc, getvalueFuncInfo->returnSize);
    }
    program->code[jump2].arg1_u64 = program->code.size();
}

void CodeGenerator::Visit(const sptr<ArrayCountExpression>& node)
{
    auto elementType = node->array->EvaluateType()->GetElementType();

    uint64_t elementSize = 1;
    if(elementType->IsStruct())
        elementSize = typeInfo[elementType]->ToStructInfo()->size;

    // should leave an Array on the stack
    VisitChild(node->array);

    // the array header stores its length in words
    Emit(node->loc, OpCode::PushWord, (uint64_t)ArrayLengthOffset);

    if(elementSize != 1)
    {
        Emit(node->loc, OpCode::PushInteger, static_cast<Integer>(elementSize));
        Emit(node->loc, OpCode::DivInt);
    }
}

void CodeGenerator::Visit(const sptr<CheckSiteExpression>& node)
{
    auto siteId = static_cast<Integer>(program->checkSites.size());
    program->checkSites.push_back(CheckSite{ node->message, node->loc });
    Emit(node->loc, OpCode::PushInteger, siteId);
}

void CodeGenerator::Visit(const sptr<IntegerLiteralExpression>& node) {
    Emit(node->loc, OpCode::PushInteger, node->value);
}

void CodeGenerator::Visit(const sptr<IsExpression>& node)
{
    Type* valueType = node->value->EvaluateType();
    Type* targetType = node->typeSpec->GetType();

    if (valueType->IsNullable())
    {
        // IsExpression for nullables should be converted to a
        // call to Type.IsInstance() during semantic analysis.
        assert(0);
    }
    else
    {
        Emit(node->loc, OpCode::PushBoolean, (valueType == targetType) ? 1 : 0);
    }
}

void CodeGenerator::Visit(const sptr<NewExpression>& node)
{
    Type* type = node->EvaluateType();

    if(type->IsArray())
    {
        // create the new array: Type.NewArray(typeID, length)
        assert(node->allocExpression);
        VisitChild(node->allocExpression);

        if(!node->argumentExpression) // new Type[]{ initializers }
        {
            size_t i = 0;

            // initialize elements
            for(auto& element : node->arguments)
            {
                uint64_t size = 1;

                auto elementType = element->EvaluateType();
                if(elementType->IsStruct())
                    size = typeInfo[elementType]->ToStructInfo()->size;

                // push element
                VisitChild(element);

                // push a copy of the array from below the element
                Emit(node->loc, OpCode::PushOffset, size, 1ull);

                // push array index
                Emit(node->loc, OpCode::PushInteger, i);

                // replace the array copy and index with the address of the element
                Emit(node->loc, OpCode::PushIndexAddr, (uint64_t)ArrayDataOffset, size);

                // store element in array
                if(size > 1)
                    Emit(node->loc, OpCode::PopWordN, 0ull, size);
                else
                    Emit(node->loc, OpCode::PopWord, 0ull);

                ++i;
            }

            // array should be left on top of stack
        }
    }
    else if(type->IsClass())
    {
        auto classDef = type->GetDefinition()->ToClassDefinition();
        ENFORCE(classDef != nullptr, node->loc, "expected class type");

        // use arguments passed to initializer
        auto fields = classDef->GetChildren<VariableDefinition>([](auto& f) { return !f->isStatic; });
        ENFORCE(node->arguments.size() <= fields.count(), node->loc, "too many arguments");

        // instantiate the class: Type.NewClass(typeID)
        assert(node->allocExpression);
        VisitChild(node->allocExpression);

        auto currentArg = node->arguments.begin();

        for(const auto& field : fields)
        {
            if(currentArg != node->arguments.end())
            {
                // store the initializer arg in its field
                EmitFieldInitializer(*currentArg, field->offset, field->size);
                ++currentArg;
            }
            else
            {
                EmitFieldInitializer(field->initializer, field->offset, field->size);
            }
        }
    }
    else if(type->IsStruct())
    {
        auto structDef = type->GetDefinition()->ToStructDefinition();
        ENFORCE(structDef != nullptr, node->loc, "expected struct type");

        auto fields = structDef->GetChildren<VariableDefinition>();
        
        // use arguments passed to initializer
        ENFORCE(node->arguments.size() <= fields.count(), node->loc, "too many arguments");
        
        auto currentField = fields.begin();
        auto currentArg = node->arguments.begin();

        // push initializer args onto the stack
        for( ; currentArg != node->arguments.end(); ++currentArg, ++currentField)
            VisitChild(*currentArg);

        // default-initialize any fields not passed to initializer
        for( ; currentField != fields.end(); ++currentField)
            VisitChild((*currentField)->initializer);
    }
}

void CodeGenerator::Visit(const sptr<NullLiteralExpression>& node) {
    Emit(node->loc, OpCode::PushNull);
}

void CodeGenerator::Visit(const sptr<NumberLiteralExpression>& node) {
    Emit(node->loc, OpCode::PushNumber, node->value);
}

void CodeGenerator::Visit(const sptr<PostfixExpression>& node)
{
    Type* type = node->arg->EvaluateType();
    assert(type);

    VisitChild(node->arg);

    // keep the old value as the result
    if(!TryCancelExpressionStatementPop(node.get()))
        Emit(node->loc, OpCode::Dup);

    switch(node->operation)
    {
    case TokenType::Increment:
        if(type->IsInteger())
        {
            Emit(node->loc, OpCode::PushInteger, 1);
            Emit(node->loc, OpCode::AddInt);
        }
        else if(type->IsNumber())
        {
            Emit(node->loc, OpCode::PushNumber, 1.0);
            Emit(node->loc, OpCode::AddNum);
        }
        else
        {
            assert(0);
        }
        break;
    case TokenType::Decrement:
        if(type->IsInteger())
        {
            Emit(node->loc, OpCode::PushInteger, 1);
            Emit(node->loc, OpCode::SubInt);
        }
        else if(type->IsNumber())
        {
            Emit(node->loc, OpCode::PushNumber, 1.0);
            Emit(node->loc, OpCode::SubNum);
        }
        else
        {
            assert(0);
        }
        break;
    default:
        assert(0);
        break;
    }

    PopExpression(node->arg, node->arg);

    // old value left on stack, unless discarded
}

void CodeGenerator::Visit(const sptr<PrefixExpression>& node)
{
    Type* type = node->arg->EvaluateType();
    assert(type);

    bool rmw = false;

    switch (node->operation)
    {
    case TokenType::Add:
        VisitChild(node->arg);
        break;
    case TokenType::Sub:
        if (type->IsInteger())
        {
            Emit(node->loc, OpCode::PushInteger, 0);
            VisitChild(node->arg);
            Emit(node->loc, OpCode::SubInt);
        }
        else if (type->IsNumber())
        {
            Emit(node->loc, OpCode::PushNumber, 0.0);
            VisitChild(node->arg);
            Emit(node->loc, OpCode::SubNum);
        }
        else
        {
            assert(0);
        }
        break;
    case TokenType::Increment:
        if (type->IsInteger())
        {
            VisitChild(node->arg);
            Emit(node->loc, OpCode::PushInteger, 1);
            Emit(node->loc, OpCode::AddInt);
        }
        else if (type->IsNumber())
        {
            VisitChild(node->arg);
            Emit(node->loc, OpCode::PushNumber, 1.0);
            Emit(node->loc, OpCode::AddNum);
        }
        else
        {
            assert(0);
        }
        rmw = true;
        break;
    case TokenType::Decrement:
        if (type->IsInteger())
        {
            VisitChild(node->arg);
            Emit(node->loc, OpCode::PushInteger, 1);
            Emit(node->loc, OpCode::SubInt);
        }
        else if (type->IsNumber())
        {
            VisitChild(node->arg);
            Emit(node->loc, OpCode::PushNumber, 1.0);
            Emit(node->loc, OpCode::SubNum);
        }
        else
        {
            assert(0);
        }
        rmw = true;
        break;
    case TokenType::BitNot:
        VisitChild(node->arg);
        Emit(node->loc, OpCode::BitNot);
        break;
    case TokenType::LogicalNot:
        VisitChild(node->arg);
        Emit(node->loc, OpCode::PushBoolean, 0);
        Emit(node->loc, OpCode::Equal);
        break;
    }

    if (rmw)
    {
        // keep the new value as the result
        if(!TryCancelExpressionStatementPop(node.get()))
            Emit(node->loc, OpCode::Dup);

        PopExpression(node->arg, node->arg);
    }
}

void CodeGenerator::Visit(const sptr<SizeOfExpression>& node)
{
    size_t size = typeInfo[node->typeSpec->type]->GetSize();
    Emit(node->loc, OpCode::PushInteger, static_cast<Integer>(size));
}

void CodeGenerator::Visit(const sptr<StringLiteralExpression>& node) {
    program->staticObjects.push_back(String::New(program.get(), node->value));
    auto index = Emit((String*)program->staticObjects.back().get());
    Emit(node->loc, OpCode::PushLiteral, index);
}

void CodeGenerator::Visit(const sptr<TernaryExpression>& node)
{
    std::vector<size_t> falseJumpIndices;
    EmitConditionalJumps(node->condition, nullptr, &falseJumpIndices);

    VisitChild(node->trueValue);

    size_t jumpOverFalseValueCodeIndex = program->code.size();
    Emit(node->loc, OpCode::Jump, -1);

    PatchJumps(falseJumpIndices, program->code.size());

    VisitChild(node->falseValue);
    program->code[jumpOverFalseValueCodeIndex].arg1_u64 = program->code.size();
}

void CodeGenerator::Visit(const sptr<TypeLiteralExpression>& node)
{
    TypeInfo* type = program->GetTypeInfo(node->value);
    Emit(node->loc, OpCode::PushInteger, static_cast<Integer>(type->id));
}

void CodeGenerator::Visit(const sptr<TypeOfExpression>& node)
{
    assert(false); // this should have been replaced by a TypeLiteralExpression during semantic analysis
}

/****************************
*         SPECIFIERS        *
****************************/

void CodeGenerator::Visit(const sptr<TypeSpecifier>& node) {
    ASTVisitor::Visit(node);
}

/****************************
*         STATEMENTS        *
****************************/

void CodeGenerator::Visit(const sptr<AssertStatement>& node) {
    // should have been lowered to Debug.Assert() during semantic analysis
    assert(0);
}

void CodeGenerator::Visit(const sptr<BlockStatement>& node) {
    ASTVisitor::Visit(node);
}

void CodeGenerator::Visit(const sptr<EmptyStatement>& node)
{
    ASTVisitor::Visit(node);
}

void CodeGenerator::Visit(const sptr<ExposeStatement>& node) {
    ASTVisitor::Visit(node);
}

void CodeGenerator::Visit(const sptr<ExpressionStatement>& node)
{
    // let the expression cancel this pop, then restore any pending pop from an enclosing
    // statement (a FoldExpression can put statements inside an expression)
    auto enclosing = std::exchange(pendingPop, node->expression.get());
    ASTVisitor::Visit(node);
    bool popCancelled = std::exchange(pendingPop, enclosing) == nullptr;

    auto exprType = node->expression->EvaluateType();
    if(popCancelled || exprType->IsVoid())
    {
        // nothing was left on the stack
    }
    else if(exprType->IsStruct())
    {
        auto structInfo = typeInfo[exprType]->ToStructInfo();
        Emit(node->loc, OpCode::Pop, structInfo->size);
    }
    else
    {
        Emit(node->loc, OpCode::Pop, 1);
    }
}

void CodeGenerator::Visit(const sptr<ForStatement>& node)
{
    if(node->init)
        VisitChild(node->init);

    size_t conditionCodeStart = program->code.size();

    // without a condition, the loop never exits through the top
    std::vector<size_t> loopExitJumpIndices;
    if(node->condition)
        EmitConditionalJumps(node->condition, nullptr, &loopExitJumpIndices);

    VisitChild(node->body);

    if(node->iterate)
        VisitChild(node->iterate);

    Emit(node->loc, OpCode::Jump, conditionCodeStart);

    PatchJumps(loopExitJumpIndices, program->code.size());
}

void CodeGenerator::Visit(const sptr<GotoStatement>& node)
{
    // the code location
    VisitChild(node->expression);

    // jump to location on stack top
    Emit(node->loc, OpCode::Goto);
}

void CodeGenerator::Visit(const sptr<IfStatement>& node)
{
    // jump over the true branch when the condition is false
    std::vector<size_t> jumpIndicesOverTrueBranch;
    EmitConditionalJumps(node->condition, nullptr, &jumpIndicesOverTrueBranch);

    // emit true branch
    VisitChild(node->trueBranch);
    size_t trueBranchCodeEnd = program->code.size();

    if(node->falseBranch)
    {
        // jump over false branch after true branch
        size_t jumpOverFalseCodeStart = program->code.size();
        Emit(node->loc, OpCode::Jump, -1);

        // update true branch code end
        trueBranchCodeEnd = program->code.size();

        // emit false branch
        size_t falseBranchCodeStart = program->code.size();
        VisitChild(node->falseBranch);
        size_t falseBranchCodeEnd = program->code.size();

        // fix up jump code pointer
        program->code[jumpOverFalseCodeStart].arg1_u64 = falseBranchCodeEnd;
    }

    PatchJumps(jumpIndicesOverTrueBranch, trueBranchCodeEnd);
}

void CodeGenerator::Visit(const sptr<ReturnStatement>& node)
{
    size_t paramSize = 0;
    size_t returnSize = 1;
    if(auto func = node->enclosingScope->owner->ToFunctionDefinition())
    {
        paramSize = func->paramSize;
        returnSize = typeInfo[func->type]->ToFunctionInfo()->returnSize;
    }

    if(node->context)
    {
        auto awaitableType = Type::Get("Awaitable");
        size_t awaitableInterfaceID = typeInfo[awaitableType]->ToInterfaceInfo()->id;
        auto resumeAwaiterFuncType = Type::Get("Awaitable.ResumeAwaiter");
        auto resumeAwaiterFuncInfo = typeInfo[resumeAwaiterFuncType]->ToFunctionInfo();
        auto resumeAwaiterFuncID = resumeAwaiterFuncInfo->id;
        auto contextDef = node->context->EvaluateType()->GetDefinition();

        // this.$value = node.expression;
        if(node->expression)
        {
            auto valueField = contextDef->GetVariable("$value");
            VisitChild(node->expression);
            VisitChild(node->context);

            uint64_t valueOffset = FieldOffset(node->context, valueField->offset);

            if(valueField->size > 1)
                Emit(node->loc, OpCode::PopWordN, valueOffset, valueField->size);
            else
                Emit(node->loc, OpCode::PopWord, valueOffset);
        }

        // this.$position = -1;
        auto positionField = contextDef->GetVariable("$position");
        Emit(node->loc, OpCode::PushInteger, -1);
        VisitChild(node->context);
        Emit(node->loc, OpCode::PopWord, FieldOffset(node->context, positionField->offset));

        // this.ResumeAwaiter();
        EmitReserve(node->loc, resumeAwaiterFuncInfo->returnSize);
        VisitChild(node->context);
        Emit(node->loc, OpCode::CallVirtual, resumeAwaiterFuncID, awaitableInterfaceID);

        // done!
        if(returnSize != 0)
            Emit(node->loc, OpCode::PushNull);
        Emit(node->loc, OpCode::Return, paramSize, returnSize);
    }
    else
    {
        if(node->expression)
            VisitChild(node->expression);
        else if(returnSize != 0)
            Emit(node->loc, OpCode::PushNull);

        Emit(node->loc, OpCode::Return, paramSize, returnSize);
    }
}

void CodeGenerator::Visit(const sptr<VariableDefinitionStatement>& node)
{
    // Push[*] <value>
    VisitChild(node->variableDefinition->initializer);

    // pop literal into variable
    if(node->variableDefinition->isStatic)
    {
        Emit(node->loc, OpCode::PopGlobal, node->variableDefinition->offset, node->variableDefinition->size);
    }
    else
    {
        if(node->variableDefinition->size > 1)
            Emit(node->loc, OpCode::PopLocalN, node->variableDefinition->offset, node->variableDefinition->size);
        else
            Emit(node->loc, OpCode::PopLocal, node->variableDefinition->offset);
    }
}

void CodeGenerator::Visit(const sptr<WhileStatement>& node)
{
    size_t conditionCodeStart = program->code.size();

    // without a condition, the loop never exits through the top
    std::vector<size_t> loopExitJumpIndices;
    if(node->condition)
        EmitConditionalJumps(node->condition, nullptr, &loopExitJumpIndices);

    VisitChild(node->body);

    Emit(node->loc, OpCode::Jump, conditionCodeStart);

    PatchJumps(loopExitJumpIndices, program->code.size());
}

} // fraze
