/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <fraze/ast/AST.h>
#include <fraze/ast/ASTVisitor.h>
#include <fraze/program/JITProgram.h>
#include <fraze/program/MIRContext.h>
#include <mir.h>

namespace fraze {

class JITCodeGenerator : public ASTVisitor
{
    MIR_item_t currentFunctionItem{};
    MIR_op_t exprValueOp{};
    uint32_t registerCount{};

    std::unordered_map<const Type*, sptr<TypeInfo>> typeInfo;

    // Keyed on the function's type, because overloads share a qualified name.
    std::unordered_map<Type*, MIR_item_t> functionItems;
    std::unordered_map<std::string, MIR_item_t> protoItems;

    struct ImportedFunction
    {
        MIR_item_t item;
        void* entryAddress;
    };

    // The externs and runtime helpers a call reached, kept apart from 'functionItems' because MIR_load_external
    // binds an import to its address instead of resolving it against the module's definitions.
    std::unordered_map<std::string, ImportedFunction> importedFunctions;

    // The parameters and locals of the function being generated.
    std::unordered_map<Definition*, MIR_reg_t> variableRegisters;

    // The labels of the function being generated, each created by whichever of the label and its first goto
    // the generator reaches first.
    std::unordered_map<const LabelStatement*, MIR_insn_t> labelInsns;

    // The generated functions, paired with the qualified name the host invokes them by.
    std::vector<std::pair<std::string_view, MIR_item_t>> generatedFunctions;

    void AppendInsn(MIR_insn_t insn);
    std::string NewRegisterName();
    MIR_reg_t NewRegister(MIR_type_t type);
    MIR_type_t MIRTypeOf(Type* type, const SourceLocation& loc);
    std::string GetFunctionName(const sptr<FunctionDefinition>& def);
    MIR_type_t GetResultType(const sptr<FunctionDefinition>& def);
    std::vector<MIR_type_t> GetParameterTypes(const sptr<FunctionDefinition>& def);
    MIR_item_t GetProtoItem(MIR_type_t resultType, const std::vector<MIR_type_t>& paramTypes);
    MIR_item_t GetFunctionItem(const sptr<FunctionDefinition>& def);
    MIR_item_t GetImportItem(const std::string& name, void* entryAddress);
    MIR_insn_t GetLabelInsn(const LabelStatement* label);

    MIR_op_t EmitGlobalOp(const sptr<VariableDefinition>& def, const SourceLocation& loc);
    MIR_op_t EmitVariableOp(const sptr<IdentifierExpression>& node);
    MIR_op_t EmitExpressionValue(const sptr<Expression>& expr);
    MIR_op_t EmitArithmetic(
        TokenType operation, MIR_type_t operandType, MIR_op_t leftOp, MIR_op_t rightOp, const SourceLocation& loc);
    MIR_op_t EmitConversion(const sptr<Expression>& value, Type* resultType, const SourceLocation& loc);
    MIR_op_t EmitShortCircuit(const sptr<BinaryExpression>& node);
    void EmitConditionalBranch(const sptr<Expression>& condition, MIR_insn_t target, bool expectedValue);
    MIR_op_t EmitStore(const sptr<Expression>& target, MIR_op_t valueOp, const SourceLocation& loc);
    [[noreturn]] void ThrowUnsupported(const ASTNode& node);

    // Both live only while Visit(ASTRoot) runs, because the program that owns the context is local to it.
    // Compiled code takes the program's address, its globals' and its string literals' as immediates, none of
    // which the program itself can outlive.
    MIR_context_t context = nullptr;
    JITProgram* currentProgram = nullptr;

public:
    sptr<JITProgram> program;

    virtual void VisitChildNode(const sptr<ASTNode>& node) override;

    virtual void Visit(const sptr<ASTRoot>& node) override;
    virtual void Visit(const sptr<FunctionDefinition>& node) override;
    virtual void Visit(const sptr<AsExpression>& node) override;
    virtual void Visit(const sptr<AssignExpression>& node) override;
    virtual void Visit(const sptr<BinaryExpression>& node) override;
    virtual void Visit(const sptr<BooleanLiteralExpression>& node) override;
    virtual void Visit(const sptr<CallExpression>& node) override;
    virtual void Visit(const sptr<CastExpression>& node) override;
    virtual void Visit(const sptr<CheckSiteExpression>& node) override;
    virtual void Visit(const sptr<ConvertExpression>& node) override;
    virtual void Visit(const sptr<DefaultValueExpression>& node) override;
    virtual void Visit(const sptr<IdentifierExpression>& node) override;
    virtual void Visit(const sptr<IntegerLiteralExpression>& node) override;
    virtual void Visit(const sptr<NullLiteralExpression>& node) override;
    virtual void Visit(const sptr<NumberLiteralExpression>& node) override;
    virtual void Visit(const sptr<StringLiteralExpression>& node) override;
    virtual void Visit(const sptr<PostfixExpression>& node) override;
    virtual void Visit(const sptr<PrefixExpression>& node) override;
    virtual void Visit(const sptr<TernaryExpression>& node) override;
    virtual void Visit(const sptr<BlockStatement>& node) override;
    virtual void Visit(const sptr<EmptyStatement>& node) override;
    virtual void Visit(const sptr<ExpressionStatement>& node) override;
    virtual void Visit(const sptr<ForStatement>& node) override;
    virtual void Visit(const sptr<GotoStatement>& node) override;
    virtual void Visit(const sptr<IfStatement>& node) override;
    virtual void Visit(const sptr<LabelStatement>& node) override;
    virtual void Visit(const sptr<ReturnStatement>& node) override;
    virtual void Visit(const sptr<SwitchStatement>& node) override;
    virtual void Visit(const sptr<VariableDefinitionStatement>& node) override;
    virtual void Visit(const sptr<WhileStatement>& node) override;

    virtual void Visit(const sptr<ArrayCountExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<AwaitExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<FoldExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<IndexExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<IsExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<NewExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<SizeOfExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<TypeLiteralExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<TypeOfExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<AssertStatement>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<ExposeStatement>& node) override { ThrowUnsupported(*node); }
};

} // fraze
