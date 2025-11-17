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
    : heap(this)
{
    stack = dynamic_array<Word, Heap::BlockSize>(StackSize);
    rsp = stack.data() - 1;
    rbp = stack.data();
    rip = 0;
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

std::size_t Program::GetStackSize() const {
    return std::size_t(rsp + 1 - stack.data());
}

void Program::Initialize()
{
    rsp += globalCount;
    rbp = rsp + 1;

    // RUN GLOBAL INIT
    for(auto& ti : typeInfo)
    {
        if(auto sect = ti->ToSectionInfo())
        {
            Operation* ops = code.data();

            // return storage
            *(++rsp) = Word(nullptr);

            // no args to push

            // context
            *(++rsp) = Word(nullptr);

// Call + Prologue
            // Push a sentinel value instead of the instruction pointer
            // since we have no real return address here in C++.
            std::size_t previousInstructionPointer = rip;
            *(++rsp) = Word::Raw(DONE_INSTR);
            rip = sect->codeStart;

            // save/bump base pointer
            *(++rsp) = Word(rbp);
            rbp = rsp + 1;
            // no locals

            while(rip != DONE_INSTR)
            {
                Operation& op = ops[rip];
#if FRAZE_HEAP_DEBUG
                SourceLocation& loc = locations[rip];
                heap.SetLocation(&loc);
#endif
                Execute( op );

#if FRAZE_HEAP_DEBUG
                heap.SetLocation(nullptr);
#endif
            }

            // OpCode::Return will pop rbp, context and args
            
            // OpCode::Return restored the sentinel from the stack,
            // so restore the real instruction pointer here.
            rip = previousInstructionPointer;

            --rsp; // return storage
            assert(GetStackSize() == globalCount);
        }
    }

    assert(GetStackSize() == globalCount);
    
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

    Operation* ops = code.data();

#ifndef NDEBUG
    size_t previousStackSize = GetStackSize();
    Word* previousBasePointer = rbp;
#endif

    // return storage
    rsp += funcInfo->returnSize;

    // args
    for(auto& arg : std::views::reverse(args))
        *(++rsp) = arg;

    // context
    *(++rsp) = Word(nullptr);

// Call + Prologue
    // Push a sentinel value instead of the instruction pointer
    // since we have no real return address here in C++.
    std::size_t previousInstructionPointer = rip;
    *(++rsp) = Word::Raw(DONE_INSTR);
    rip = funcInfo->codeStart;

    // save/bump base pointer and allocate locals
    *(++rsp) = Word(rbp);
    rbp = rsp + 1;

    // allocate locals
    rsp += funcInfo->localSize;

    while(rip != DONE_INSTR)
    {
        Operation& op = ops[rip];

#if FRAZE_PRINT_EXECUTED_CODE || FRAZE_HEAP_DEBUG
        SourceLocation& loc = locations[rip];
#endif

#if FRAZE_PRINT_EXECUTED_CODE
        std::cout << std::setw(4) << std::setfill(' ') << loc.line << ", ";
        std::cout << std::setw(3) << std::setfill(' ') << loc.column << ", ";
        PrintOperation(rip, std::cout);
        std::cout << std::endl;
#endif

#if FRAZE_HEAP_DEBUG
        heap.SetLocation(&loc);
#endif
            
#if FRAZE_CODE_PROFILING
        auto start = std::chrono::high_resolution_clock::now();
#endif // FRAZE_CODE_PROFILING

        Execute( op );

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

    // OpCode::Return restored the sentinel from the stack,
    // so restore the real instruction pointer here.
    rip = previousInstructionPointer;

    assert(rbp == previousBasePointer);

    // OpCode::Return will pop rbp, context and args
    
    Word result = *(rsp--); // assume 1-word return value for now

    assert(GetStackSize() == previousStackSize);

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
    VERIFY_HANDLER_INDEX(CallVirtual);
    VERIFY_HANDLER_INDEX(Return);
    VERIFY_HANDLER_INDEX(CallExternal);
    VERIFY_HANDLER_INDEX(CallIntrinsic);
    VERIFY_HANDLER_INDEX(Jump);
    VERIFY_HANDLER_INDEX(JumpIf);
    VERIFY_HANDLER_INDEX(JumpIfNot);
    VERIFY_HANDLER_INDEX(Goto);
    VERIFY_HANDLER_INDEX(Assert);
    VERIFY_HANDLER_INDEX(NullCheck);
    VERIFY_HANDLER_INDEX(BoundsCheck);
    VERIFY_HANDLER_INDEX(ObjectTypeCheck);
}

void Program::Execute_NoOp(const Operation& op)
{
    ++rip;
}

void Program::Execute_PushLiteral(const Operation& op)
{
    *(++rsp) = *(data.data() + op.arg1_u64);
    ++rip;
}

void Program::Execute_PushContext(const Operation& op)
{
    Word* top = rsp;
    *(++top) = *(rbp - 3);
    rsp = top;
    ++rip;
}

void Program::Execute_PushLocal(const Operation& op)
{
    assert(op.arg2_u64 == 0);
    *(++rsp) = *(rbp + op.arg1_u64);
    ++rip;
}

void Program::Execute_PushLocalN(const Operation& op)
{
    Word* src = rbp + op.arg1_u64;
    Word* dest = rsp + 1;

    for(std::size_t i = 0; i != op.arg2_u64; ++i)
        dest[i] = src[i];

    rsp = dest + (op.arg2_u64 - 1);
    ++rip;
}

void Program::Execute_PushLocalAddr(const Operation& op)
{
    *(++rsp) = Word(rbp + op.arg1_u64);
    ++rip;
}

void Program::Execute_PopLocal(const Operation& op)
{
    assert(op.arg2_u64 == 0);
    *(rbp + op.arg1_u64) = *(rsp--);
    ++rip;
}

void Program::Execute_PopLocalN(const Operation& op)
{
    assert(op.arg2_u64 != 0);
    
    auto dest = rbp + op.arg1_u64;
    auto src = rsp + 1 - op.arg2_u64;
    
    for(size_t i = 0; i != op.arg2_u64; ++i)
        dest[i] = src[i];

    rsp -= op.arg2_u64;
    ++rip;
}

void Program::Execute_PushGlobal(const Operation& op)
{
    assert(op.arg2_u64 != 0);

    Word* dest = rsp + 1;
    for(size_t i = 0; i != op.arg2_u64; ++i)
        dest[i] = this->stack[ op.arg1_u64 + i ];

    rsp = dest + (op.arg2_u64 - 1);
    ++rip;
}

void Program::Execute_PushGlobalAddr(const Operation& op)
{
    *(++rsp) = &this->stack[ op.arg1_u64 ];
    ++rip;
}

void Program::Execute_PopGlobal(const Operation& op)
{
    assert(op.arg2_u64 != 0);

    Word* top = rsp;
    Word* global = &this->stack[ op.arg1_u64 ];
    Word* value = top + 1 - op.arg2_u64;

    for(size_t i = 0; i != op.arg2_u64; ++i)
        global[i] = value[i];

    rsp = top - op.arg2_u64;
    ++rip;
}

void Program::Execute_PushArgument(const Operation& op)
{
    const uint64_t argIndex = op.arg1_u64;
    const uint64_t argSize = op.arg2_u64;
    
    assert(argSize == 0);

    *(++rsp) = *(rbp - 4 - argIndex);
    ++rip;
}

void Program::Execute_PushArgumentN(const Operation& op)
{
    const uint64_t argIndex = op.arg1_u64;
    const uint64_t argSize = op.arg2_u64;

    assert(argSize != 0);

    Word* src = rbp - 4 - argIndex - (argSize - 1);
    Word* dest = rsp + 1;

    for(std::size_t i = 0; i != argSize; ++i)
        dest[i] = src[i];

    rsp = dest + (argSize - 1);
    ++rip;
}

void Program::Execute_PushArgumentAddr(const Operation& op)
{
    const uint64_t argIndex = op.arg1_u64;
    const uint64_t argSize = op.arg2_u64;

    *(++rsp) = { rbp - 4 - argIndex - (argSize - 1) };
    ++rip;
}

void Program::Execute_PopArgument(const Operation& op)
{
    const uint64_t argIndex = op.arg1_u64;
    const uint64_t argSize = op.arg2_u64;

    assert(argSize != 0);

    Word* top = rsp;
    auto arg = rbp - 4 - argIndex - (argSize - 1);
    auto value = top + 1 - argSize;

    for(size_t i = 0; i != argSize; ++i)
        arg[i] = value[i];

    rsp = top - argSize;
    ++rip;
}

void Program::Execute_PushField(const Operation& op)
{
    assert(op.arg2_u64 != 0);

    Word* top = rsp;
    Class* object = static_cast<Class*>((top--)->object);

    for(auto& field : object->GetFields(op.arg1_u64, op.arg2_u64))
        *(++top) = field;

    rsp = top;
    ++rip;
}

void Program::Execute_PushFieldAddr(const Operation& op)
{
    Word* top = rsp;
    Class* object = static_cast<Class*>(top->object);
    *top = object->GetFieldRef(op.arg1_u64);
    ++rip;
}

void Program::Execute_PopField(const Operation& op)
{
    assert(op.arg2_u64 != 0);

    Word* top = rsp;
    Class* object = static_cast<Class*>((top--)->object);
    auto values = top + 1 - op.arg2_u64;
    object->SetFields(op.arg1_u64, { values, values + op.arg2_u64 });
    rsp = top - op.arg2_u64;
    ++rip;
}

void Program::Execute_PushRefField(const Operation& op)
{
    *rsp = rsp->reference[op.arg1_u64];
    ++rip;
}

void Program::Execute_PushRefFieldN(const Operation& op)
{
    Word* top = rsp;
    Word* reference = (top--)->reference;

    assert(op.arg2_u64 != 0);
    
    Word* src = reference + op.arg1_u64;
    Word* dest = top + 1;

    for(std::size_t i = 0; i < op.arg2_u64; ++i)
        dest[i] = src[i];

    rsp = dest + (op.arg2_u64 - 1);
    ++rip;
}

void Program::Execute_PushRefFieldAddr(const Operation& op)
{
    Word* top = rsp;
    Reference reference = top->reference;
    top->reference = &reference[op.arg1_u64];
    ++rip;
}

void Program::Execute_PopRefField(const Operation& op)
{
    assert(op.arg2_u64 != 0);

    Word* top = rsp;
    Reference reference = (top--)->reference;

    auto values = top + 1 - op.arg2_u64;
    for(size_t i = 0; i != op.arg2_u64; ++i)
        reference[op.arg1_u64 + i] = values[i];

    rsp = top - op.arg2_u64;
    ++rip;
}

void Program::Execute_PushElement(const Operation& op)
{
    Word* top = rsp;
    
    Integer elementIndex = (top--)->integer;
    Array<>* arr = static_cast<Array<>*>((top--)->object);

    assert(op.arg1_u64 == arr->GetElementSize());

    auto wordIndex = elementIndex * arr->GetElementSize();

    Word* src = &arr->At(wordIndex);
    Word* dest = top + 1;

    std::size_t count = op.arg1_u64;

    for(std::size_t i = 0; i != count; ++i)
        dest[i] = src[i];

    rsp = top + count;

    ++rip;
}

void Program::Execute_PushElementAddr(const Operation& op)
{
    Word* top = rsp;

    Integer elementIndex = (top--)->integer;
    Array<>* arr = static_cast<Array<>*>((top--)->object);

    auto wordIndex = elementIndex * arr->GetElementSize();
    *(++top) = Word( &arr->At(wordIndex) );

    rsp = top;
    ++rip;
}

void Program::Execute_PopElement(const Operation& op)
{
    assert(op.arg1_u64 != 0);

    Word* top = rsp;

    Word* value = top + 1 - op.arg1_u64;
    Integer elementIndex = (value - 1)->integer;
    Array<>* arr = (value - 2)->GetArray();

    assert(op.arg1_u64 == arr->GetElementSize());

    auto wordIndex = elementIndex * op.arg1_u64;

    for(size_t i = 0; i != op.arg1_u64; ++i)
        arr->At(wordIndex + i) = value[i];

    rsp = top - (op.arg1_u64 + 2) ;

    ++rip;
}

void Program::Execute_PushOffset(const Operation& op)
{
    Word* top = rsp;
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

    rsp = top;
    ++rip;
}

void Program::Execute_PopOffset(const Operation& op)
{
    *(rsp - op.arg1_u64) = *(rsp--);
    ++rip;
}

void Program::Execute_PushBoolean(const Operation& op)
{
    (++rsp)->storage = static_cast<uint64_t>(op.arg1_i64 != 0);
    ++rip;
}

void Program::Execute_PushInteger(const Operation& op)
{
    (++rsp)->integer = op.arg1_i64;
    ++rip;
}

void Program::Execute_PushNumber(const Operation& op)
{
    (++rsp)->number = op.arg1_f64;
    ++rip;
}

void Program::Execute_PushNull(const Operation& op)
{
    (++rsp)->object = nullptr;
    ++rip;
}

void Program::Execute_PushCount(const Operation& op)
{
    Word* top = rsp;
    Array<>* arr = top->GetArray();
    top->integer = arr->GetCount();
    ++rip;
}

void Program::Execute_PushSize(const Operation& op)
{
    Word* top = rsp;
    Array<>* arr = top->GetArray();
    top->integer = arr->GetSize();
    ++rip;
}

void Program::Execute_Pop(const Operation& op)
{
    rsp -= op.arg1_u64;
    ++rip;
}

void Program::Execute_Reserve(const Operation& op)
{
    rsp += op.arg1_u64;
    ++rip;
}

void Program::Execute_NewArray(const Operation& op)
{
    Word* top = rsp;

    ArrayInfo* arrayInfo = typeInfo[op.arg1_u64]->ToArrayInfo();
    StructInfo* structInfo = arrayInfo->elementType->ToStructInfo();
    size_t elementSize = structInfo ? structInfo->size : size_t(1);

    Integer length = top->integer;
    top->object = Array<>::New(heap, arrayInfo, length * elementSize);

    ++rip;
}

void Program::Execute_NewClass(const Operation& op)
{
    Word* top = rsp;
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
    rsp = top;

    ++rip;
}

void Program::Execute_LogicalOr(const Operation& op)
{
    Word* top = rsp;
    Boolean rhs = static_cast<Boolean>((top--)->storage);
    Boolean lhs = static_cast<Boolean>((top--)->storage);
    (++top)->storage = static_cast<uint64_t>(lhs || rhs);
    rsp = top;
    ++rip;
}

void Program::Execute_LogicalAnd(const Operation& op)
{
    Word* top = rsp;
    Boolean rhs = static_cast<Boolean>((top--)->storage);
    Boolean lhs = static_cast<Boolean>((top--)->storage);
    (++top)->storage = static_cast<uint64_t>(lhs && rhs);
    rsp = top;
    ++rip;
}

void Program::Execute_BitOr(const Operation& op)
{
    Word* top = rsp;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs | rhs;
    rsp = top;
    ++rip;
}

void Program::Execute_BitXor(const Operation& op)
{
    Word* top = rsp;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs ^ rhs;
    rsp = top;
    ++rip;
}

void Program::Execute_BitAnd(const Operation& op)
{
    Word* top = rsp;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs & rhs;
    rsp = top;
    ++rip;
}

void Program::Execute_BitNot(const Operation& op)
{
    Word* top = rsp;
    top->integer = ~top->integer;
    ++rip;
}

void Program::Execute_LeftShift(const Operation& op)
{
    Word* top = rsp;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs << rhs;
    rsp = top;
    ++rip;
}

void Program::Execute_RightShift(const Operation& op)
{
    Word* top = rsp;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs >> rhs;
    rsp = top;
    ++rip;
}

void Program::Execute_Equal(const Operation& op)
{
    assert(op.arg1_u64 == 0);
    Word* top = rsp;
    uint64_t rhs = (top--)->storage;
    uint64_t lhs = top->storage;
    top->storage = (lhs == rhs);
    rsp = top;
    ++rip;
}

void Program::Execute_EqualN(const Operation& op)
{
    assert(op.arg1_u64 == 1);

    Word* rhs = rsp + 1 - op.arg1_u64;
    Word* lhs = rhs - op.arg1_u64;

    uint64_t i = 0;

    for( ; i != op.arg1_u64; ++i)
    {
        if(rhs[i] != lhs[i])
            break;
    }

    lhs->storage = static_cast<uint64_t>(i == op.arg1_u64);
    rsp = lhs;
    ++rip;
}

void Program::Execute_StringEqual(const Operation& op)
{
    Word* top = rsp;
    String* rhs = static_cast<String*>((top--)->object);
    String* lhs = static_cast<String*>(top->object); // skip decrement
    Word res = Word( lhs == rhs || (lhs && rhs && lhs->GetView() == rhs->GetView()) );
    *top = res; // skip increment
    rsp = top;
    ++rip;
}

void Program::Execute_IsInstance(const Operation& op)
{
    auto targetTypeInfo = typeInfo[op.arg1_u64];
    assert(targetTypeInfo);

    Word* top = rsp;
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
    ++rip;
}

void Program::Execute_LessInt(const Operation& op)
{
    Word* top = rsp;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->storage = static_cast<uint64_t>(lhs < rhs);
    rsp = top;
    ++rip;
}

void Program::Execute_LessNum(const Operation& op)
{
    Word* top = rsp;
    Number rhs = (top--)->number;
    Number lhs = (top--)->number;
    (++top)->storage = static_cast<uint64_t>(lhs < rhs);
    rsp = top;
    ++rip;
}

void Program::Execute_LessEqualInt(const Operation& op)
{
    Word* top = rsp;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->storage = static_cast<uint64_t>(lhs <= rhs);
    rsp = top;
    ++rip;
}

void Program::Execute_LessEqualNum(const Operation& op)
{
    Word* top = rsp;
    Number rhs = (top--)->number;
    Number lhs = (top--)->number;
    (++top)->storage = static_cast<uint64_t>(lhs <= rhs);
    rsp = top;
    ++rip;
}

void Program::Execute_GreaterInt(const Operation& op)
{
    Word* top = rsp;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->storage = static_cast<uint64_t>(lhs > rhs);
    rsp = top;
    ++rip;
}

void Program::Execute_GreaterNum(const Operation& op)
{
    Word* top = rsp;
    Number rhs = (top--)->number;
    Number lhs = (top--)->number;
    (++top)->storage = static_cast<uint64_t>(lhs > rhs);
    rsp = top;
    ++rip;
}

void Program::Execute_GreaterEqualInt(const Operation& op)
{
    Word* top = rsp;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->storage = static_cast<uint64_t>(lhs >= rhs);
    rsp = top;
    ++rip;
}

void Program::Execute_GreaterEqualNum(const Operation& op)
{
    Word* top = rsp;
    Number rhs = (top--)->number;
    Number lhs = (top--)->number;
    (++top)->storage = static_cast<uint64_t>(lhs >= rhs);
    rsp = top;
    ++rip;
}

void Program::Execute_AddInt(const Operation& op)
{
    Word* top = rsp;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs + rhs;
    rsp = top;
    ++rip;
}

void Program::Execute_AddNum(const Operation& op)
{
    Word* top = rsp;
    Number rhs = (top--)->number;
    Number lhs = (top--)->number;
    (++top)->number = lhs + rhs;
    rsp = top;
    ++rip;
}

void Program::Execute_SubInt(const Operation& op)
{
    Word* top = rsp;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs - rhs;
    rsp = top;
    ++rip;
}

void Program::Execute_SubNum(const Operation& op)
{
    Word* top = rsp;
    Number rhs = (top--)->number;
    Number lhs = (top--)->number;
    (++top)->number = lhs - rhs;
    rsp = top;
    ++rip;
}

void Program::Execute_MulInt(const Operation& op)
{
    Word* top = rsp;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs * rhs;
    rsp = top;
    ++rip;
}

void Program::Execute_MulNum(const Operation& op)
{
    Word* top = rsp;
    Number rhs = (top--)->number;
    Number lhs = (top--)->number;
    (++top)->number = lhs * rhs;
    rsp = top;
    ++rip;
}

void Program::Execute_DivInt(const Operation& op)
{
    Word* top = rsp;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs / rhs;
    rsp = top;
    ++rip;
}

void Program::Execute_DivNum(const Operation& op)
{
    Word* top = rsp;
    Number rhs = (top--)->number;
    Number lhs = (top--)->number;
    (++top)->number = lhs / rhs;
    rsp = top;
    ++rip;
}

void Program::Execute_ModInt(const Operation& op)
{
    Word* top = rsp;
    Integer rhs = (top--)->integer;
    Integer lhs = (top--)->integer;
    (++top)->integer = lhs % rhs;
    rsp = top;
    ++rip;
}

void Program::Execute_ModNum(const Operation& op)
{
    Word* top = rsp;
    Number rhs = (top--)->number;
    Number lhs = (top--)->number;
    (++top)->number = fmod(lhs, rhs);
    rsp = top;
    ++rip;
}

void Program::Execute_ConvIntToNum(const Operation& op)
{
    rsp->number = static_cast<Number>(rsp->integer);
    ++rip;
}

void Program::Execute_ConvNumToInt(const Operation& op)
{
    rsp->integer = static_cast<Integer>(rsp->number);
    ++rip;
}

void Program::Execute_ConvBoolToStr(const Operation& op)
{
    bool value = static_cast<bool>(rsp->storage);
    rsp->object = String::New(heap, value ? "true" : "false");
    ++rip;
}

void Program::Execute_ConvIntToStr(const Operation& op)
{
    Word* top = rsp;
    Integer value = top->integer;

    std::array<char, 32> buffer;
    auto ret = std::to_chars(
        buffer.data(), buffer.data() + buffer.size(), value);
    assert(ret.ec == std::errc());

    top->object = String::New(heap, std::string_view(buffer.data(), ret.ptr));
    ++rip;
}

void Program::Execute_ConvNumToStr(const Operation& op)
{
    Word* top = rsp;
    Number value = top->number;

    std::array<char, 1080> buffer;
    auto ret = std::to_chars(
        buffer.data(), buffer.data() + buffer.size(), value, std::chars_format::fixed);
    assert(ret.ec == std::errc());

    top->object = String::New(heap, std::string_view(buffer.data(), ret.ptr));
    ++rip;
}

void Program::Execute_ConvEnumToStr(const Operation& op)
{
    Word* top = rsp;
    Integer value = top->integer;
    auto* enumInfo = typeInfo[op.arg1_u64]->ToEnumInfo();
    auto it = std::ranges::find_if(enumInfo->members, [&](auto m){ return m.second == value; });
    assert(it != enumInfo->members.end());
    top->object = String::New(heap, it->first);
    ++rip;
}

void Program::Execute_ConvObjToType(const Operation& op)
{
    // for object to reference type conversions, we just need to
    // check if the object is of the target type; if not, set to null
    Word* top = rsp;
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

    ++rip;
}

void Program::Execute_ConvRefToStruct(const Operation& op)
{
    Word* top = rsp;
    Reference reference = (top--)->reference;

    assert(op.arg1_u64 != 0);

    for(size_t i = 0; i != op.arg1_u64; ++i)
        *(++top) = reference[i];

    rsp = top;
    ++rip;
}

void Program::Execute_Box(const Operation& op)
{
    Word* top = rsp;
    Box* box = Box::New(heap, *top, (WordType)op.arg1_u64);
    top->object = box;
    ++rip;
}

void Program::Execute_Unbox(const Operation& op)
{
    Word* top = rsp;
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
    ++rip;
}

void Program::Execute_StringConcat(const Operation& op)
{
    Word* top = rsp;

    String* rhs = (top--)->GetString();
    String* lhs = (top--)->GetString();
    std::string_view lhsView = lhs->GetView();
    std::string_view rhsView = rhs->GetView();
    String* result = String::New(heap, lhsView, rhsView);
    (++top)->object = result;
    rsp = top;
    ++rip;
}

void Program::Execute_Dup(const Operation& op)
{
    Word* top = rsp;
    *(++top) = *top;
    rsp = top;
    ++rip;
}

void Program::Execute_DupN(const Operation& op)
{
    Word* top = rsp;
    Word* src = top + 1 - op.arg1_u64;
    Word* dest = top + 1;

    for (std::size_t i = 0; i < op.arg1_u64; ++i)
        dest[i] = src[i];

    rsp = dest + (op.arg1_u64 - 1);
    ++rip;
}

void Program::Execute_Call(const Operation& op)
{
    auto info = typeInfo[op.arg1_u64]->ToFunctionInfo();
    assert(info);

    Word* top = rsp;

    *(++top) = Word::Raw(rip);
    rip = info->codeStart;

    *(++top) = { rbp };
    rbp = top + 1;
    rsp = top + info->localSize;
}

void Program::Execute_CallVirtual(const Operation& op)
{
    Word* top = rsp; // starts at context pointer

    const uint64_t interfaceFuncID = op.arg1_u64;
    const uint64_t interfaceType = op.arg2_u64;

    auto interfaceFuncInfo = typeInfo[interfaceFuncID]->ToFunctionInfo();
    assert(interfaceFuncInfo);

    Class* obj = top->GetClass();
    size_t actualFuncID = obj->GetFunctionID(interfaceType, interfaceFuncInfo->offset);

    auto info = typeInfo[actualFuncID]->ToFunctionInfo();
    assert(info);

    *(++top) = Word::Raw(rip);
    rip = info->codeStart;

    *(++top) = { rbp };
    rbp = top + 1;
    rsp = top + info->localSize;
}

void Program::Execute_Return(const Operation& op)
{
    const uint64_t argsSize = op.arg1_u64;
    const uint64_t returnSize = op.arg2_u64;

    Word* top = rsp;

    Word* returnStorageStart = rbp - 3 - argsSize - returnSize;
    Word* returnValueEnd = top + 1;
    Word* returnValueStart = returnValueEnd - returnSize;

    while(returnValueStart != returnValueEnd)
    {
        *(returnStorageStart++) = *(returnValueStart++);
    }

    top = rbp - 1;
    rbp = (top--)->reference;

    rip = (top--)->storage;

    rsp = top - 1 - argsSize;

    if(rip != DONE_INSTR)
        ++rip;
}

void Program::Execute_CallExternal(const Operation& op)
{
    auto info = typeInfo[op.arg1_u64]->ToFunctionInfo();
    assert(info);

    Word* top = rsp; // context pointer

    *(++top) = Word::Raw(rip);
    rip = 0; // no code address for external function

    *(++top) = { rbp };
    rbp = top + 1;
    // no locals

    constexpr int MaxArgs = 32;
    std::array<Word*, MaxArgs> argPointerBuffer;
    std::span<Word*> argPointers;
    std::span<Word> result;

    if(info->paramSize != 0)
    {
        int i = 0;
        for(auto& param : info->params)
            argPointerBuffer[i++] = rbp - 3 - param.offset - param.size;

        argPointers = std::span<Word*>(argPointerBuffer.begin(), argPointerBuffer.begin() + i);
    }

    Word* returnStorageStart = rbp - 3 - info->paramSize - info->returnSize;
    Word* returnStorageEnd = returnStorageStart + info->returnSize;
    result = std::span<Word>(returnStorageStart, returnStorageEnd);

    // commit the changes here in case native calls back into the VM
    rsp = top;
    info->externalFunction->Invoke(this, result, argPointers);

    top = rbp - 1;
    rbp = (top--)->reference;

    rip = (top--)->storage;

    rsp = top - 1 - info->paramSize;

    ++rip;
}

void Program::Execute_CallIntrinsic(const Operation& op)
{
    // op.arg1_u32a: type id;
    // op.arg1_u32b: intrinsic id;
    // op.arg2_u32a: return size;
    // op.arg2_u32b: args size;

    const uint32_t intrinsicID = op.arg1_u32b;
    const uint32_t argsSize = op.arg2_u32b;

    Word* top = rsp;

    *(++top) = Word::Raw(rip);
    rip = 0; // no code address for external function

    *(++top) = { rbp };
    rbp = top + 1;

    // commit the changes here in case native calls back into the VM
    rsp = top;
    intrinsics[intrinsicID](op, top, rbp);

    top = rbp - 1;
    rbp = (top--)->reference;

    rip = (top--)->storage;

    rsp = top - 1 - argsSize;

    ++rip;
}

void Program::Execute_Jump(const Operation& op)
{
    rip = op.arg1_u64;
}

void Program::Execute_JumpIf(const Operation& op)
{
    Boolean value = static_cast<Boolean>((rsp--)->storage);
    if(value)
        rip = op.arg1_u64;
    else
        ++rip;
}

void Program::Execute_JumpIfNot(const Operation& op)
{
    Boolean value = static_cast<Boolean>((rsp--)->storage);
    if(!value)
        rip = op.arg1_u64;
    else
        ++rip;
}

void Program::Execute_Goto(const Operation& op)
{
    rip = (rsp--)->integer;
}

void Program::Execute_Assert(const Operation& op)
{
    String* message = (rsp--)->GetString();

    std::string msgText;

    if(message)
        msgText = std::format("assertion failed: {}", message->GetView());
    else
        msgText = "assertion failed.";

    Throw(locations[rip], "{}", msgText);
    ++rip;
}

void Program::Execute_NullCheck(const Operation& op)
{
    if(rsp->storage == 0)
    {
        Throw(locations[rip], "object reference is null");
    }

    ++rip;
}

void Program::Execute_BoundsCheck(const Operation& op)
{
    Word* top = rsp;

    Array<>* arr = (top - 1)->GetArray();
    Integer index = top->integer;
    Integer count = static_cast<Integer>(arr->GetCount());

    if(index < 0 || index >= count)
    {
        Throw(locations[rip], "index out of range");
    }

    ++rip;
}

void Program::Execute_ObjectTypeCheck(const Operation& op)
{
    Word* top = rsp;
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

        Throw(locations[rip], "Object is not of type '{}'", type);
    }

    ++rip;
}

void Program::Execute(const Operation& op)
{
    (this->*handlers[static_cast<size_t>(op.code)])(op);
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
 
