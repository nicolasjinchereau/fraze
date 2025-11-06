/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/program/Program.h>
#include <fraze/common/ExternalFunction.h>
#include <fraze/common/Platform.h>
#include <ranges>
#include <tuple>
#include <print>

namespace fraze {

constexpr size_t DONE_INSTR = static_cast<size_t>(-1);

Program::Program()
{
    stack.reserve(1024 * 1024 / sizeof(Word));
    stackFrames.reserve(2048);
    codePointers.reserve(2048);
}

TypeInfo* Program::GetTypeInfo(const std::string& qualifiedName)
{
    for(auto& ti : typeInfo)
    {
        if(ti->qualifiedName == qualifiedName)
            return ti.get();
    }

    return nullptr;
}

void Program::Report()
{
    heap.Report();
}

void Program::PinMemory(const void* p)
{
    heap.PinMemory(static_cast<const std::byte*>(p));
}

void Program::UnpinMemory(const void* p)
{
    heap.UnpinMemory(static_cast<const std::byte*>(p));
}

void Program::Collect()
{
    heap.Collect();
}

void Program::Initialize()
{
    heap.SetStack(&stack);

    stack.clear();
    stack.resize(globalCount);

    // RUN GLOBAL INIT
    for(auto& ti : typeInfo)
    {
        if(auto sect = ti->ToSectionInfo())
        {
            stack.push(Word(nullptr)); // return storage
            stack.push(Word(nullptr)); // context

            stackFrames.push(StackFrame(stack.end(), 1, 0));
            codePointers.push(DONE_INSTR);

            Operation* ops = code.data();
            Word*& stackTop = stack.top_ptr();
            StackFrame* fp = &stackFrames.top();
            size_t ip = sect->codeStart;

            while(ip != DONE_INSTR)
            {
                Operation& op = ops[ip];

#if FRAZE_HEAP_DEBUG
                heap.SetLocation(&op.loc);
#endif
                Execute( op, stackTop, fp, ip );

#if FRAZE_HEAP_DEBUG
                heap.SetLocation(nullptr);
#endif
            }

            size_t remainingSize = stack.size();
            assert(remainingSize == globalCount + 1);
            stack.pop();

            assert(codePointers.empty());
            assert(stackFrames.empty());
        }
    }

    assert(stack.size() == globalCount);
    
    initialized = true;
}

Word Program::InvokeImpl(const std::string& qualifiedFuncName, const std::span<Word>& args)
{
    if(!initialized)
    {
        Initialize();
    }

    auto typeInfo = GetTypeInfo(qualifiedFuncName);
    ENFORCE(typeInfo != nullptr, SourceLocation(), "function not found: {}", qualifiedFuncName);

    auto funcInfo = typeInfo->ToFunctionInfo();
    ENFORCE(funcInfo->paramSize == args.size(), SourceLocation(), "wrong number of args: {}", args.size());

    size_t numStackFrames = stackFrames.size();
    size_t numCodePointers = codePointers.size();
    size_t stackSize = stack.size();

    stack.grow(funcInfo->returnSize); // return storage
    stack.push(Word(nullptr)); // context

    for(auto& arg : args)
        stack.push(arg);

    stackFrames.push(StackFrame(stack.end(), funcInfo->returnSize, funcInfo->paramSize));
    stack.grow(funcInfo->localSize);
    codePointers.push(DONE_INSTR);

    Operation* ops = code.data();
    Word*& stackTop = stack.top_ptr();
    StackFrame* fp = &stackFrames.top();
    size_t ip = funcInfo->codeStart;

    while(ip != DONE_INSTR)
    {
        Operation& op = ops[ip];

#if FRAZE_PRINT_EXECUTED_CODE
        std::cout << std::setw(4) << std::setfill(' ') << op.loc.line << ", ";
        std::cout << std::setw(3) << std::setfill(' ') << op.loc.column << ", ";
        PrintOperation(ip, std::cout);
        std::cout << std::endl;
#endif

#if FRAZE_HEAP_DEBUG
        heap.SetLocation(&op.loc);
#endif
            
#if FRAZE_CODE_PROFILING
        auto start = std::chrono::high_resolution_clock::now();
#endif // FRAZE_CODE_PROFILING

        Execute( op, stackTop, fp, ip );

#if FRAZE_CODE_PROFILING
        auto end = std::chrono::high_resolution_clock::now();
        auto codeIndex = static_cast<int>(op.code);
        opcodeTotalNanos[codeIndex] += duration_cast<std::chrono::nanoseconds>(end - start).count();
        opcodeTotalCount[codeIndex] += 1;
#endif // FRAZE_CODE_PROFILING

#if FRAZE_HEAP_DEBUG
        heap.SetLocation(nullptr);
#endif
    }

    assert(codePointers.size() == numCodePointers);
    assert(stackFrames.size() == numStackFrames);
    assert(stack.size() == (stackSize + funcInfo->returnSize)); // stackSize + (return value)

    Word result = stack.top();
    stack.pop();
    
    return result;
}

#if FRAZE_CODE_PROFILING
void Program::DumpCodeProfile(std::ostream& stream)
{
    struct InstructionStats
    {
        OpCode code;
        uint64_t totalNanos;
        uint64_t totalCount;
        double nanosPerCall;
        double percentOfTotalTime;
    };

    std::vector<InstructionStats> counts;
    counts.reserve(static_cast<size_t>(OpCode::COUNT));
    uint64_t totalExecutionNanos = 0;

    for(size_t i = 0; i != static_cast<size_t>(OpCode::COUNT); ++i)
    {
        uint64_t totalCount = opcodeTotalCount[i];
        if(totalCount == 0)
            continue;

        OpCode code = static_cast<OpCode>(i);
        uint64_t totalNanos = opcodeTotalNanos[i];
        double nanosPerCall = static_cast<double>(totalNanos) / totalCount;

        totalExecutionNanos += totalNanos;

        counts.push_back(InstructionStats{ code, totalCount, totalNanos, nanosPerCall, 0.0 });
    }

    for(auto& item : counts) {
        item.percentOfTotalTime = static_cast<double>(item.totalNanos * 100) / totalExecutionNanos;
    }

    std::ranges::sort(counts, [](const InstructionStats& a, const InstructionStats& b){
        return a.totalNanos > b.totalNanos;
    });

    stream
        << std::left << std::setw(20) << "Code"
        << std::left << std::setw(16) << "Total Count"
        << std::left << std::setw(20) << "Fraction of Time"
        << std::left << "Nanos Per Call"
        << std::endl;

    for(auto& item : counts)
    {
        stream
            << std::left << std::setw(20) << OpCodeNames[item.code]
            << std::left << std::setw(16) << item.totalCount
            << std::left << std::setw(20) << std::fixed << std::setprecision(8) << item.percentOfTotalTime
            << std::left << std::fixed << std::setprecision(3) << item.nanosPerCall
            << std::endl;
    }

    stream << std::endl;
}
#endif // FRAZE_CODE_PROFILING

void Program::VerifyHandlers()
{
#define VERIFY_HANDLER_INDEX(name) \
    static_assert(Program::handlers[static_cast<size_t>(OpCode::name)] == &Program::Execute_##name);

    VERIFY_HANDLER_INDEX(NoOp);
    VERIFY_HANDLER_INDEX(PushLiteral);
    VERIFY_HANDLER_INDEX(PushContext);
    VERIFY_HANDLER_INDEX(PushLocal);
    VERIFY_HANDLER_INDEX(PushLocalN);
    VERIFY_HANDLER_INDEX(PushLocalAddr);
    VERIFY_HANDLER_INDEX(PopLocal);
    VERIFY_HANDLER_INDEX(PushGlobal);
    VERIFY_HANDLER_INDEX(PushGlobalAddr);
    VERIFY_HANDLER_INDEX(PopGlobal);
    VERIFY_HANDLER_INDEX(PushArgument);
    VERIFY_HANDLER_INDEX(PushArgumentN);
    VERIFY_HANDLER_INDEX(PushArgumentAddr);
    VERIFY_HANDLER_INDEX(PopArgument);
    VERIFY_HANDLER_INDEX(PushField);
    VERIFY_HANDLER_INDEX(PushFieldAddr);
    VERIFY_HANDLER_INDEX(PopField);
    VERIFY_HANDLER_INDEX(PushRefField);
    VERIFY_HANDLER_INDEX(PushRefFieldN);
    VERIFY_HANDLER_INDEX(PushRefFieldAddr);
    VERIFY_HANDLER_INDEX(PopRefField);
    VERIFY_HANDLER_INDEX(PushElement);
    VERIFY_HANDLER_INDEX(PushElementAddr);
    VERIFY_HANDLER_INDEX(PopElement);
    VERIFY_HANDLER_INDEX(PushOffset);
    VERIFY_HANDLER_INDEX(PopOffset);
    VERIFY_HANDLER_INDEX(PushBoolean);
    VERIFY_HANDLER_INDEX(PushInteger);
    VERIFY_HANDLER_INDEX(PushNumber);
    VERIFY_HANDLER_INDEX(PushNull);
    VERIFY_HANDLER_INDEX(PushCount);
    VERIFY_HANDLER_INDEX(PushSize);
    VERIFY_HANDLER_INDEX(Pop);
    VERIFY_HANDLER_INDEX(Reserve);
    VERIFY_HANDLER_INDEX(NewArray);
    VERIFY_HANDLER_INDEX(NewClass);
    VERIFY_HANDLER_INDEX(LogicalOr);
    VERIFY_HANDLER_INDEX(LogicalAnd);
    VERIFY_HANDLER_INDEX(BitOr);
    VERIFY_HANDLER_INDEX(BitXor);
    VERIFY_HANDLER_INDEX(BitAnd);
    VERIFY_HANDLER_INDEX(BitNot);
    VERIFY_HANDLER_INDEX(LeftShift);
    VERIFY_HANDLER_INDEX(RightShift);
    VERIFY_HANDLER_INDEX(Equal);
    VERIFY_HANDLER_INDEX(EqualN);
    VERIFY_HANDLER_INDEX(StringEqual);
    VERIFY_HANDLER_INDEX(IsInstance);
    VERIFY_HANDLER_INDEX(LessInt);
    VERIFY_HANDLER_INDEX(LessNum);
    VERIFY_HANDLER_INDEX(LessEqualInt);
    VERIFY_HANDLER_INDEX(LessEqualNum);
    VERIFY_HANDLER_INDEX(GreaterInt);
    VERIFY_HANDLER_INDEX(GreaterNum);
    VERIFY_HANDLER_INDEX(GreaterEqualInt);
    VERIFY_HANDLER_INDEX(GreaterEqualNum);
    VERIFY_HANDLER_INDEX(AddInt);
    VERIFY_HANDLER_INDEX(AddNum);
    VERIFY_HANDLER_INDEX(SubInt);
    VERIFY_HANDLER_INDEX(SubNum);
    VERIFY_HANDLER_INDEX(MulInt);
    VERIFY_HANDLER_INDEX(MulNum);
    VERIFY_HANDLER_INDEX(DivInt);
    VERIFY_HANDLER_INDEX(DivNum);
    VERIFY_HANDLER_INDEX(ModInt);
    VERIFY_HANDLER_INDEX(ModNum);
    VERIFY_HANDLER_INDEX(ConvIntToNum);
    VERIFY_HANDLER_INDEX(ConvNumToInt);
    VERIFY_HANDLER_INDEX(ConvBoolToStr);
    VERIFY_HANDLER_INDEX(ConvIntToStr);
    VERIFY_HANDLER_INDEX(ConvNumToStr);
    VERIFY_HANDLER_INDEX(ConvEnumToStr);
    VERIFY_HANDLER_INDEX(ConvObjToType);
    VERIFY_HANDLER_INDEX(ConvRefToStruct);
    VERIFY_HANDLER_INDEX(Box);
    VERIFY_HANDLER_INDEX(Unbox);
    VERIFY_HANDLER_INDEX(StringConcat);
    VERIFY_HANDLER_INDEX(Dup);
    VERIFY_HANDLER_INDEX(DupN);
    VERIFY_HANDLER_INDEX(Call);
    VERIFY_HANDLER_INDEX(CallExternal);
    VERIFY_HANDLER_INDEX(CallIntrinsic);
    VERIFY_HANDLER_INDEX(CallVirtual);
    VERIFY_HANDLER_INDEX(Jump);
    VERIFY_HANDLER_INDEX(JumpIf);
    VERIFY_HANDLER_INDEX(JumpIfNot);
    VERIFY_HANDLER_INDEX(Goto);
    VERIFY_HANDLER_INDEX(Return);
    VERIFY_HANDLER_INDEX(Assert);
    VERIFY_HANDLER_INDEX(NullCheck);
    VERIFY_HANDLER_INDEX(BoundsCheck);
    VERIFY_HANDLER_INDEX(ObjectTypeCheck);
}

void Program::Execute_NoOp(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    ++ip;
}

void Program::Execute_PushLiteral(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    *(++stackTop) = *(data.data() + op.arg1_u64);
    ++ip;
}

void Program::Execute_PushContext(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    *(++top) = *(fp->start - fp->paramCount - 1);
    stackTop = top;
    ++ip;
}

void Program::Execute_PushLocal(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip)
{
    assert(op.arg2_u64 == 0);
    *(++stackTop) = *(fp->start + op.arg1_u64);
    ++ip;
}

void Program::Execute_PushLocalN(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip)
{
    Word* src = fp->start + op.arg1_u64;
    Word* dest = stackTop + 1;

    for(std::size_t i = 0; i != op.arg2_u64; ++i)
        dest[i] = src[i];

    stackTop = dest + (op.arg2_u64 - 1);
    ++ip;
}

void Program::Execute_PushLocalAddr(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip)
{
    *(++stackTop) = Word(fp->start + op.arg1_u64);
    ++ip;
}

void Program::Execute_PopLocal(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip)
{
    assert(op.arg2_u64 == 0);
    *(fp->start + op.arg1_u64) = *(stackTop--);
    ++ip;
}

void Program::Execute_PopLocalN(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip)
{
    assert(op.arg2_u64 != 0);
    
    auto dest = fp->start + op.arg1_u64;
    auto src = stackTop + 1 - op.arg2_u64;
    
    for(size_t i = 0; i != op.arg2_u64; ++i)
        dest[i] = src[i];

    stackTop -= op.arg2_u64;
    ++ip;
}

void Program::Execute_PushGlobal(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    assert(op.arg2_u64 != 0);

    Word* dest = stackTop + 1;
    for(size_t i = 0; i != op.arg2_u64; ++i)
        dest[i] = this->stack[ op.arg1_u64 + i ];

    stackTop = dest + (op.arg2_u64 - 1);
    ++ip;
}

void Program::Execute_PushGlobalAddr(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    *(++stackTop) = &this->stack[ op.arg1_u64 ];
    ++ip;
}

void Program::Execute_PopGlobal(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    assert(op.arg2_u64 != 0);

    Word* top = stackTop;
    Word* global = &this->stack[ op.arg1_u64 ];
    Word* value = top + 1 - op.arg2_u64;

    for(size_t i = 0; i != op.arg2_u64; ++i)
        global[i] = value[i];

    stackTop = top - op.arg2_u64;
    ++ip;
}

void Program::Execute_PushArgument(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip)
{
    assert(op.arg2_u64 == 0);
    *(++stackTop) = *(fp->start - fp->paramCount + op.arg1_u64);
    ++ip;
}

void Program::Execute_PushArgumentN(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip)
{
    assert(op.arg2_u64 != 0);

    Word* src = fp->start - fp->paramCount + op.arg1_u64;
    Word* dest = stackTop + 1;

    for(std::size_t i = 0; i != op.arg2_u64; ++i)
        dest[i] = src[i];

    stackTop = dest + (op.arg2_u64 - 1);
    ++ip;
}

void Program::Execute_PushArgumentAddr(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip)
{
    *(++stackTop) = fp->start - fp->paramCount + op.arg1_u64;
    ++ip;
}

void Program::Execute_PopArgument(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip)
{
    assert(op.arg2_u64 != 0);

    Word* top = stackTop;
    auto arg = fp->start - fp->paramCount + op.arg1_u64;
    auto value = top + 1 - op.arg2_u64;

    for(size_t i = 0; i != op.arg2_u64; ++i)
        arg[i] = value[i];

    stackTop = top - op.arg2_u64;
    ++ip;
}

void Program::Execute_PushField(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    assert(op.arg2_u64 != 0);

    Word* top = stackTop;
    Class* object = static_cast<Class*>((top--)->object);

    for(auto& field : object->GetFields(op.arg1_u64, op.arg2_u64))
        *(++top) = field;

    stackTop = top;
    ++ip;
}

void Program::Execute_PushFieldAddr(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Class* object = static_cast<Class*>(top->object);
    *top = object->GetFieldRef(op.arg1_u64);
    ++ip;
}

void Program::Execute_PopField(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    assert(op.arg2_u64 != 0);

    Word* top = stackTop;
    Class* object = static_cast<Class*>((top--)->object);
    auto values = top + 1 - op.arg2_u64;
    object->SetFields(op.arg1_u64, { values, values + op.arg2_u64 });
    stackTop = top - op.arg2_u64;
    ++ip;
}

void Program::Execute_PushRefField(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    *stackTop = stackTop->reference[op.arg1_u64];
    ++ip;
}

void Program::Execute_PushRefFieldN(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Word* reference = (top--)->reference;

    assert(op.arg2_u64 != 0);
    
    Word* src = reference + op.arg1_u64;
    Word* dest = top + 1;

    for(std::size_t i = 0; i < op.arg2_u64; ++i)
        dest[i] = src[i];

    stackTop = dest + (op.arg2_u64 - 1);
    ++ip;
}

void Program::Execute_PushRefFieldAddr(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Reference reference = top->reference;
    top->reference = &reference[op.arg1_u64];
    ++ip;
}

void Program::Execute_PopRefField(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    assert(op.arg2_u64 != 0);

    Word* top = stackTop;
    Reference reference = (top--)->reference;

    auto values = top + 1 - op.arg2_u64;
    for(size_t i = 0; i != op.arg2_u64; ++i)
        reference[op.arg1_u64 + i] = values[i];

    stackTop = top - op.arg2_u64;
    ++ip;
}

void Program::Execute_PushElement(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    
    Integer elementIndex = (top--)->integer;
    Array<>* arr = static_cast<Array<>*>((top--)->object);

    assert(op.arg1_u64 == arr->GetElementSize());

    auto wordIndex = elementIndex * arr->GetElementSize();

    Word* src = &arr->At(wordIndex);
    Word* dest = top + 1;

    std::size_t count = op.arg1_u64;

    for(std::size_t i = 0; i != count; ++i)
        dest[i] = src[i];

    stackTop = top + count;

    ++ip;
}

void Program::Execute_PushElementAddr(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;

    Integer elementIndex = (top--)->integer;
    Array<>* arr = static_cast<Array<>*>((top--)->object);

    auto wordIndex = elementIndex * arr->GetElementSize();
    *(++top) = Word( &arr->At(wordIndex) );

    stackTop = top;
    ++ip;
}

void Program::Execute_PopElement(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    assert(op.arg1_u64 != 0);

    Word* top = stackTop;

    Word* value = top + 1 - op.arg1_u64;
    Integer elementIndex = (value - 1)->integer;
    Array<>* arr = (value - 2)->GetArray();

    assert(op.arg1_u64 == arr->GetElementSize());

    auto wordIndex = elementIndex * op.arg1_u64;

    for(size_t i = 0; i != op.arg1_u64; ++i)
        arr->At(wordIndex + i) = value[i];

    stackTop = top - (op.arg1_u64 + 2) ;

    ++ip;
}

void Program::Execute_PushOffset(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    auto valueRef = top - op.arg1_u64;

    if(op.arg2_u64 != 0)
    {
        for(size_t i = 0; i != op.arg2_u64; ++i)
            *(++top) = valueRef[i];
    }
    else // push a reference to the var at offset
    {
        *(++top) = valueRef;
    }

    stackTop = top;
    ++ip;
}

void Program::Execute_PopOffset(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    *(stackTop - op.arg1_u64) = *(stackTop--);
    ++ip;
}

void Program::Execute_PushBoolean(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    (++stackTop)->storage = static_cast<uint64_t>(op.arg1_i64 != 0);
    ++ip;
}

void Program::Execute_PushInteger(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    (++stackTop)->integer = op.arg1_i64;
    ++ip;
}

void Program::Execute_PushNumber(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    (++stackTop)->number = op.arg1_f64;
    ++ip;
}

void Program::Execute_PushNull(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    (++stackTop)->object = nullptr;
    ++ip;
}

void Program::Execute_PushCount(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Array<>* arr = top->GetArray();
    top->integer = arr->GetCount();
    ++ip;
}

void Program::Execute_PushSize(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Array<>* arr = top->GetArray();
    top->integer = arr->GetSize();
    ++ip;
}

void Program::Execute_Pop(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    stackTop -= op.arg1_u64;
    ++ip;
}

void Program::Execute_Reserve(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    stackTop += op.arg1_u64;
    ++ip;
}

void Program::Execute_NewArray(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;

    ArrayInfo* arrayInfo = typeInfo[op.arg1_u64]->ToArrayInfo();
    StructInfo* structInfo = arrayInfo->elementType->ToStructInfo();
    size_t elementSize = structInfo ? structInfo->size : size_t(1);

    Integer length = top->integer;
    top->object = Array<>::New(heap, arrayInfo, length * elementSize);

    ++ip;
}

void Program::Execute_NewClass(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    auto* classInfo = typeInfo[op.arg1_u64]->ToClassInfo();
    Class* instance = Class::New(heap, classInfo->ToClassInfo());

    Word* end = top + 1;
    Word* firstArg = end - classInfo->size;

    for(size_t i = 0; i != classInfo->size; ++i)
    {
        Word& arg = *(firstArg + i);
        instance->SetField(i, arg);
    }
    
    top -= classInfo->size;
    (++top)->object = instance;
    stackTop = top;

    ++ip;
}

void Program::Execute_LogicalOr(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Boolean rhs = static_cast<Boolean>((top--)->storage);
    Boolean lhs = static_cast<Boolean>((top--)->storage);
    (++top)->storage = static_cast<uint64_t>(lhs || rhs);
    stackTop = top;
    ++ip;
}

void Program::Execute_LogicalAnd(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Boolean rhs = static_cast<Boolean>((top--)->storage);
    Boolean lhs = static_cast<Boolean>((top--)->storage);
    (++top)->storage = static_cast<uint64_t>(lhs && rhs);
    stackTop = top;
    ++ip;
}

void Program::Execute_BitOr(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs | rhs;
    stackTop = top;
    ++ip;
}

void Program::Execute_BitXor(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs ^ rhs;
    stackTop = top;
    ++ip;
}

void Program::Execute_BitAnd(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs & rhs;
    stackTop = top;
    ++ip;
}

void Program::Execute_BitNot(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    top->integer = ~top->integer;
    ++ip;
}

void Program::Execute_LeftShift(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs << rhs;
    stackTop = top;
    ++ip;
}

void Program::Execute_RightShift(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs >> rhs;
    stackTop = top;
    ++ip;
}

void Program::Execute_Equal(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    assert(op.arg1_u64 == 0);
    Word* top = stackTop;
    uint64_t rhs = (top--)->storage;
    uint64_t lhs = top->storage;
    top->storage = (lhs == rhs);
    stackTop = top;
    ++ip;
}

void Program::Execute_EqualN(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    assert(op.arg1_u64 == 1);

    Word* rhs = stackTop + 1 - op.arg1_u64;
    Word* lhs = rhs - op.arg1_u64;

    uint64_t i = 0;

    for( ; i != op.arg1_u64; ++i)
    {
        if(rhs[i] != lhs[i])
            break;
    }

    lhs->storage = static_cast<uint64_t>(i == op.arg1_u64);
    stackTop = lhs;
    ++ip;
}

void Program::Execute_StringEqual(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    String* rhs = static_cast<String*>((top--)->object);
    String* lhs = static_cast<String*>(top->object); // skip decrement
    Word res = Word( lhs == rhs || (lhs && rhs && lhs->GetView() == rhs->GetView()) );
    *top = res; // skip increment
    stackTop = top;
    ++ip;
}

void Program::Execute_IsInstance(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    auto targetTypeInfo = typeInfo[op.arg1_u64];
    assert(targetTypeInfo);

    Word* top = stackTop;
    Object* value = top->object;

    bool isInstance = false;

    if(value == nullptr)
    {
        if (auto bt = targetTypeInfo->ToBasicTypeInfo(); bt && bt->qualifiedName == "null")
            isInstance = true;
    }
    else
    {
        WordType valueWordType = value->GetType();

        if (valueWordType == WordType::Class)
        {
            auto valueTypeInfo = static_cast<Class*>(value)->GetInfo();

            if(auto targetClassInfo = targetTypeInfo->ToClassInfo())
            {
                if (valueTypeInfo->id == targetClassInfo->id)
                    isInstance = true;
            }
            else if (auto targetInterfaceInfo = targetTypeInfo->ToInterfaceInfo())
            {
                for (auto& itf : valueTypeInfo->interfaces)
                {
                    if (itf == targetInterfaceInfo->id)
                    {
                        isInstance = true;
                        break;
                    }
                }
            }
        }
        else if(valueWordType == WordType::Array)
        {
            auto valueTypeInfo = static_cast<Array<>*>(value)->GetInfo();

            if (auto targetArrInfo = targetTypeInfo->ToArrayInfo())
            {
                if (valueTypeInfo->id == targetArrInfo->id)
                    isInstance = true;
            }
        }
        else if(valueWordType == WordType::String)
        {
            if (auto bt = targetTypeInfo->ToBasicTypeInfo(); bt && bt->qualifiedName == "string")
                isInstance = true;
        }
        else if (valueWordType == WordType::Boolean) // boxed
        {
            if (auto bt = targetTypeInfo->ToBasicTypeInfo(); bt && bt->qualifiedName == "bool")
                isInstance = true;
        }
        else if (valueWordType == WordType::Integer) // boxed
        {
            if (auto bt = targetTypeInfo->ToBasicTypeInfo(); bt && bt->qualifiedName == "int")
                isInstance = true;
        }
        else if (valueWordType == WordType::Number) // boxed
        {
            if (auto bt = targetTypeInfo->ToBasicTypeInfo(); bt && bt->qualifiedName == "num")
                isInstance = true;
        }
    }

    top->storage = isInstance ? 1 : 0;
    ++ip;
}

void Program::Execute_LessInt(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->storage = static_cast<uint64_t>(lhs < rhs);
    stackTop = top;
    ++ip;
}

void Program::Execute_LessNum(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Number rhs = (top--)->number;
    Number lhs = (top--)->number;
    (++top)->storage = static_cast<uint64_t>(lhs < rhs);
    stackTop = top;
    ++ip;
}

void Program::Execute_LessEqualInt(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->storage = static_cast<uint64_t>(lhs <= rhs);
    stackTop = top;
    ++ip;
}

void Program::Execute_LessEqualNum(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Number rhs = (top--)->number;
    Number lhs = (top--)->number;
    (++top)->storage = static_cast<uint64_t>(lhs <= rhs);
    stackTop = top;
    ++ip;
}

void Program::Execute_GreaterInt(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->storage = static_cast<uint64_t>(lhs > rhs);
    stackTop = top;
    ++ip;
}

void Program::Execute_GreaterNum(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Number rhs = (top--)->number;
    Number lhs = (top--)->number;
    (++top)->storage = static_cast<uint64_t>(lhs > rhs);
    stackTop = top;
    ++ip;
}

void Program::Execute_GreaterEqualInt(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->storage = static_cast<uint64_t>(lhs >= rhs);
    stackTop = top;
    ++ip;
}

void Program::Execute_GreaterEqualNum(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Number rhs = (top--)->number;
    Number lhs = (top--)->number;
    (++top)->storage = static_cast<uint64_t>(lhs >= rhs);
    stackTop = top;
    ++ip;
}

void Program::Execute_AddInt(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs + rhs;
    stackTop = top;
    ++ip;
}

void Program::Execute_AddNum(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Number rhs = (top--)->number;
    Number lhs = (top--)->number;
    (++top)->number = lhs + rhs;
    stackTop = top;
    ++ip;
}

void Program::Execute_SubInt(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs - rhs;
    stackTop = top;
    ++ip;
}

void Program::Execute_SubNum(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Number rhs = (top--)->number;
    Number lhs = (top--)->number;
    (++top)->number = lhs - rhs;
    stackTop = top;
    ++ip;
}

void Program::Execute_MulInt(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs * rhs;
    stackTop = top;
    ++ip;
}

void Program::Execute_MulNum(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Number rhs = (top--)->number;
    Number lhs = (top--)->number;
    (++top)->number = lhs * rhs;
    stackTop = top;
    ++ip;
}

void Program::Execute_DivInt(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs / rhs;
    stackTop = top;
    ++ip;
}

void Program::Execute_DivNum(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Number rhs = (top--)->number;
    Number lhs = (top--)->number;
    (++top)->number = lhs / rhs;
    stackTop = top;
    ++ip;
}

void Program::Execute_ModInt(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs % rhs;
    stackTop = top;
    ++ip;
}

void Program::Execute_ModNum(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Number rhs = (top--)->number;
    Number lhs = (top--)->number;
    (++top)->number = fmod(lhs, rhs);
    stackTop = top;
    ++ip;
}

void Program::Execute_ConvIntToNum(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    stackTop->number = static_cast<Number>(stackTop->integer);
    ++ip;
}

void Program::Execute_ConvNumToInt(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    stackTop->integer = static_cast<Integer>(stackTop->number);
    ++ip;
}

void Program::Execute_ConvBoolToStr(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    bool value = static_cast<bool>(stackTop->storage);
    stackTop->object = String::New(heap, value ? "true" : "false");
    ++ip;
}

void Program::Execute_ConvIntToStr(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Integer value = top->integer;

    std::array<char, 32> buffer;
    auto ret = std::to_chars(
        buffer.data(), buffer.data() + buffer.size(), value);
    assert(ret.ec == std::errc());

    top->object = String::New(heap, std::string_view(buffer.data(), ret.ptr));
    ++ip;
}

void Program::Execute_ConvNumToStr(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Number value = top->number;

    std::array<char, 1080> buffer;
    auto ret = std::to_chars(
        buffer.data(), buffer.data() + buffer.size(), value, std::chars_format::fixed);
    assert(ret.ec == std::errc());

    top->object = String::New(heap, std::string_view(buffer.data(), ret.ptr));
    ++ip;
}

void Program::Execute_ConvEnumToStr(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Integer value = top->integer;
    auto* enumInfo = typeInfo[op.arg1_u64]->ToEnumInfo();
    auto it = std::ranges::find_if(enumInfo->members, [&](auto m){ return m.second == value; });
    assert(it != enumInfo->members.end());
    top->object = String::New(heap, it->first);
    ++ip;
}

void Program::Execute_ConvObjToType(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    // for object to reference type conversions, we just need to
    // check if the object is of the target type; if not, set to null
    Word* top = stackTop;
    Object* obj = top->object;
    auto& targetInfo = typeInfo[op.arg1_u64];

    switch(obj->GetType())
    {
    case WordType::Class:
    {
        auto valueClassInfo = ((Class*)obj)->GetInfo();

        if(auto targetClassInfo = targetInfo->ToClassInfo())
        {
            if(valueClassInfo->id != targetClassInfo->id)
                top->object = nullptr;
        }
        else if(auto targetItfInfo = targetInfo->ToInterfaceInfo())
        {
            bool match = false;

            for(auto& implementedItf : valueClassInfo->interfaces)
            {
                if(implementedItf == targetItfInfo->id)
                {
                    match = true;
                    break;
                }
            }

            if(!match)
                top->object = nullptr;
        }
        break;
    }
    case WordType::Array:
        if(((Array<>*)obj)->GetInfo()->id != targetInfo->id)
        {
            top->object = nullptr;
        }
        break;
    case WordType::String:
        if(targetInfo->ToBasicTypeInfo() == nullptr || targetInfo->qualifiedName != "string")
        {
            top->object = nullptr;
        }
        break;
    default:
        top->object = nullptr;
        break;
    }

    ++ip;
}

void Program::Execute_ConvRefToStruct(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Reference reference = (top--)->reference;

    assert(op.arg1_u64 != 0);

    for(size_t i = 0; i != op.arg1_u64; ++i)
        *(++top) = reference[i];

    stackTop = top;
    ++ip;
}

void Program::Execute_Box(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Box* box = Box::New(heap, *top, (WordType)op.arg1_u64);
    top->object = box;
    ++ip;
}

void Program::Execute_Unbox(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Box* box = static_cast<Box*>(top->object);

    switch((WordType)op.arg1_u64)
    {
    case WordType::Boolean:
        top->storage = box->GetValue().storage;
        break;
    case WordType::Integer:
        top->integer = box->GetValue().integer;
        break;
    case WordType::Number:
        top->number = box->GetValue().number;
        break;
    }
    ++ip;
}

void Program::Execute_StringConcat(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    // don't decrement top until after the allocation
    String* rhs = (top)->GetString();
    String* lhs = (top - 1)->GetString();
    String* result = String::New(heap, lhs->GetView(), rhs->GetView());
    --top;
    top->object = result;
    stackTop = top;
    ++ip;
}

void Program::Execute_Dup(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    *(++top) = *top;
    stackTop = top;
    ++ip;
}

void Program::Execute_DupN(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Word* src = top + 1 - op.arg1_u64;
    Word* dest = top + 1;

    for (std::size_t i = 0; i < op.arg1_u64; ++i)
        dest[i] = src[i];

    stackTop = dest + (op.arg1_u64 - 1);
    ++ip;
}

void Program::Execute_Call(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip)
{
    auto info = typeInfo[op.arg1_u64]->ToFunctionInfo();
    assert(info);

    Word* top = stackTop;
    stackFrames.push(StackFrame(top + 1, info->returnSize, info->paramSize));
    fp = &stackFrames.top();
    top += info->localSize;
    stackTop = top;
    codePointers.push( ip );
    ip = info->codeStart;
}

void Program::Execute_CallExternal(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    auto info = typeInfo[op.arg1_u64]->ToFunctionInfo();
    assert(info);

    Word* top = stackTop;
    Word* stackEnd = top + 1;

    constexpr int MaxArgs = 32;
    std::array<Word*, MaxArgs> wordBuffer;
    std::span<Word> result;
    std::span<Word*> args;

    if(info->paramSize != 0)
    {
        Word* begin = stackEnd - info->paramSize;
        
        int i = 0;
        for(auto& param : info->params)
            wordBuffer[i++] = begin + param.first;

        args = std::span<Word*>(wordBuffer.begin(), wordBuffer.begin() + i);
    }

    Word* returnStart = stackEnd - info->paramSize - 1 - info->returnSize;
    Word* returnEnd = returnStart + info->returnSize;
    result = std::span<Word>(returnStart, returnEnd);

    info->externalFunction->Invoke(this, result, args);
    stackTop = top - (info->paramSize + 1);

    ++ip;
}

void Program::Execute_CallIntrinsic(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    // op.arg1_u32a: type id;
    // op.arg1_u32b: intrinsic id;
    // op.arg2_u32a: return size;
    // op.arg2_u32b: params size;

    StackFrame stackFrame(stackTop + 1, op.arg2_u32a, op.arg2_u32b);
    intrinsics[op.arg1_u32b](op, stackTop, &stackFrame);
    ++ip;
}

void Program::Execute_CallVirtual(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Word* stackEnd = top + 1;

    uint64_t interfaceFuncID = op.arg1_u64;
    uint64_t interfaceType = op.arg2_u64;

    auto interfaceFuncInfo = typeInfo[interfaceFuncID]->ToFunctionInfo();
    assert(interfaceFuncInfo);

    Class* obj = (stackEnd - interfaceFuncInfo->paramSize - 1)->GetClass();
    size_t actualFuncID = obj->GetFunctionID(interfaceType, interfaceFuncInfo->offset);

    auto info = typeInfo[actualFuncID]->ToFunctionInfo();
    assert(info);

    stackFrames.push(StackFrame(stackEnd, info->returnSize, info->paramSize));
    fp = &stackFrames.top();
    stackTop = top + info->localSize;
    codePointers.push( ip );
    ip = info->codeStart;
}

void Program::Execute_Jump(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    ip = op.arg1_u64;
}

void Program::Execute_JumpIf(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Boolean value = static_cast<Boolean>((stackTop--)->storage);
    if(value)
        ip = op.arg1_u64;
    else
        ++ip;
}

void Program::Execute_JumpIfNot(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Boolean value = static_cast<Boolean>((stackTop--)->storage);
    if(!value)
        ip = op.arg1_u64;
    else
        ++ip;
}

void Program::Execute_Goto(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    ip = (stackTop--)->integer;
}

void Program::Execute_Return(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    Word* end = top + 1;

    // stack size after popping locals, args, and context, but leaving return value storage
    Word* prevStackTop = fp->start - fp->paramCount - 1;
    size_t shrinkAmount = stackTop - prevStackTop + 1;

    Word* returnStorageStart = prevStackTop - fp->returnSize;
    Word* returnValueStart = end - fp->returnSize;
    Word* returnValueEnd = end;

    while(returnValueStart != returnValueEnd)
    {
        *(returnStorageStart++) = *(returnValueStart++);
    }

    // pop locals, args and context
    stackTop = top - shrinkAmount;

    stackFrames.pop();
    fp = !stackFrames.empty() ? &stackFrames.top() : nullptr;
    ip = codePointers.pull();

    if(ip != DONE_INSTR)
        ++ip;
}

void Program::Execute_Assert(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    String* message = (stackTop--)->GetString();

    std::string msgText;

    if(message)
        msgText = std::format("assertion failed: {}", message->GetView());
    else
        msgText = "assertion failed.";

    Throw(op.loc, "{}", msgText);
    ++ip;
}

void Program::Execute_NullCheck(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    if(stackTop->storage == 0)
    {
        Throw(op.loc, "object reference is null");
    }

    ++ip;
}

void Program::Execute_BoundsCheck(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;

    Array<>* arr = (top - 1)->GetArray();
    Integer index = top->integer;
    Integer count = static_cast<Integer>(arr->GetCount());

    if(index < 0 || index >= count)
    {
        Throw(op.loc, "index out of range");
    }

    ++ip;
}

void Program::Execute_ObjectTypeCheck(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT _, size_t& RESTRICT ip)
{
    Word* top = stackTop;
    auto objectType = top->object->GetType();
    auto targetType = static_cast<WordType>(op.arg1_u64);

    if (objectType != targetType)
    {
        const char* type{};

        if(targetType == WordType::Boolean)
            type = "bool";
        else if(targetType == WordType::Integer)
            type = "int";
        else if(targetType == WordType::Number)
            type = "num";
        
        assert(type);

        Throw(op.loc, "Object is not of type '{}'", type);
    }

    ++ip;
}

void Program::Execute(const Operation& op, Word*& RESTRICT stackTop, StackFrame*& RESTRICT fp, size_t& RESTRICT ip)
{
    (this->*handlers[static_cast<size_t>(op.code)])(op, stackTop, fp, ip);
}

void Program::Print(bool printData = true, bool printCode = true)
{
    if(printData)
    {
        std::cout << "DATA:" << std::endl;

        // print data
    
        for(int i = 0; i != data.size(); ++i)
        {
            WordType type = dataTypes[i];
            std::cout << std::setw(4) << std::setfill('0') << i << ": "
                << WordTypeNames.at(type) << ", " << GetLiteralValue(i) << std::endl;
        }

        std::cout << std::endl;
    }

    if(printCode)
    {
        // print code
        for(auto& ti : typeInfo)
        {
            auto func = ti->ToFunctionInfo();
            if(!func)
                continue;

            std::cout << "FUNCTION: " << func->qualifiedName << std::endl;

            if(!func->externalFunction)
            {
                for(auto i = func->codeStart; i != func->codeEnd; ++i) {
                    PrintOperation(i, std::cout);
                    std::cout << std::endl;
                }
            }
            else
            {
                std::cout << "<external>" << std::endl;
            }

            std::cout << std::endl;
        }
    }
}

std::string Program::GetLiteralValue(uint64_t index)
{
    Word value = data[index];
    WordType type = dataTypes[index];

    switch(type)
    {
    case WordType::Object:
        return "null";
    case WordType::Boolean:
        return value.GetBoolean() ? "true" : "false";
    case WordType::Integer:
        return std::to_string(value.integer);
    case WordType::Number:
        return std::to_string(value.number);
    case WordType::String:
        return "\"" + std::string(value.GetString()->GetView()) + "\"";
    default:
        return "?";
    }
}

void Program::PrintOperation(size_t index, std::ostream& stream)
{
    const Operation& op = code[index];
    assert(OpCodeNames.contains(op.code));

    stream << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << index << ": " << std::dec;

    switch(op.code)
    {
    case OpCode::Jump:
    case OpCode::JumpIf:
    case OpCode::JumpIfNot:
        stream << OpCodeNames[op.code] << ", "
            << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << op.arg1_u64 << std::dec;
        break;

    case OpCode::PushLiteral:
        stream << OpCodeNames[op.code] << ", "
            << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << op.arg1_u64
            << " [" << GetLiteralValue(op.arg1_u64) << "]" << std::dec;
        break;

    case OpCode::PushBoolean:
        stream << OpCodeNames[op.code] << ", " << std::boolalpha << (op.arg1_u64 != 0);
        break;

    case OpCode::PushNumber:
        stream << OpCodeNames[op.code] << ", " << static_cast<Number>(op.arg1_f64);
        break;

    case OpCode::NewClass:
        stream << OpCodeNames[op.code] << ", " << typeInfo[op.arg1_u64]->qualifiedName;
        break;

    //case OpCode::PushField:
    //{
    //    stream << OpCodeNames[op.code] << ", " << op.arg1_u64
    //        << " [" << GetLiteralValue(op.arg1_u64) << "]" << std::dec;
    //    break;
    //}
    case OpCode::Call:
    case OpCode::CallExternal:
        stream << OpCodeNames[op.code] << ", " << typeInfo[op.arg1_u64]->qualifiedName;
        break;

    case OpCode::CallIntrinsic:
        stream << OpCodeNames[op.code] << ", " << typeInfo[op.arg1_u32a]->qualifiedName;
        break;

    case OpCode::CallVirtual:
        stream << OpCodeNames[op.code] << ", " << typeInfo[op.arg1_u64]->qualifiedName;
        break;

    case OpCode::NewArray:
        stream << OpCodeNames[op.code] << ", " << op.arg1_u64
               << " [" << typeInfo[op.arg1_u64]->ToArrayInfo()->elementType->qualifiedName << "]";
        break;

    case OpCode::PushContext:
    case OpCode::PushNull:
    case OpCode::PushSize:
    case OpCode::Dup:
    case OpCode::Return:
    case OpCode::LogicalOr:
    case OpCode::LogicalAnd:
    case OpCode::BitOr:
    case OpCode::BitXor:
    case OpCode::BitAnd:
    case OpCode::LeftShift:
    case OpCode::RightShift:
    case OpCode::Equal:
    case OpCode::LessInt:
    case OpCode::LessNum:
    case OpCode::LessEqualInt:
    case OpCode::LessEqualNum:
    case OpCode::GreaterInt:
    case OpCode::GreaterNum:
    case OpCode::GreaterEqualInt:
    case OpCode::GreaterEqualNum:
    case OpCode::AddInt:
    case OpCode::AddNum:
    case OpCode::SubInt:
    case OpCode::SubNum:
    case OpCode::MulInt:
    case OpCode::MulNum:
    case OpCode::DivInt:
    case OpCode::DivNum:
    case OpCode::ModInt:
    case OpCode::ModNum:
    case OpCode::ConvIntToNum:
    case OpCode::ConvNumToInt:
    case OpCode::ConvBoolToStr:
    case OpCode::ConvIntToStr:
    case OpCode::ConvNumToStr:
    case OpCode::ConvEnumToStr:
    case OpCode::ConvRefToStruct:
    case OpCode::StringConcat:
    case OpCode::Assert:
    case OpCode::NullCheck:
    case OpCode::BoundsCheck:
        stream << OpCodeNames[op.code];
        break;

    case OpCode::PushLocalN:
    case OpCode::PopLocalN:
    case OpCode::PushGlobal:
    case OpCode::PopGlobal:
    case OpCode::PushArgumentN:
    case OpCode::PopArgument:
    case OpCode::PushField:
    case OpCode::PopField:
    case OpCode::PushRefFieldN:
    case OpCode::PopRefField:
    case OpCode::PushElement:
        stream << OpCodeNames[op.code] << ", " << op.arg1_u64 << ", " << op.arg2_u64;
        break;

    case OpCode::PushInteger:
        stream << OpCodeNames[op.code] << ", " << op.arg1_i64;
        break;

    case OpCode::BitNot:
        stream << OpCodeNames[op.code] << ", " << op.arg1_i64;
        break;

    case OpCode::NoOp:
        stream << OpCodeNames[op.code];

        if(op.arg1_cstr)
            stream << ", " << op.arg1_cstr;

        if(op.arg2_cstr)
        {
            if(op.arg1_cstr)
                stream << ", ";

            stream << op.arg2_cstr;
        }
        break;

    case OpCode::EqualN:
    case OpCode::PushLocal:
    case OpCode::PushLocalAddr:
    case OpCode::PopLocal:
    case OpCode::PushGlobalAddr:
    case OpCode::PushArgument:
    case OpCode::PushArgumentAddr:
    case OpCode::PushFieldAddr:
    case OpCode::PushRefField:
    case OpCode::PushRefFieldAddr:
    case OpCode::PushElementAddr:
    case OpCode::PopElement:
    case OpCode::Pop:
    default:
        stream << OpCodeNames[op.code] << ", " << op.arg1_u64;
        break;
    }
}

} // fraze
 
