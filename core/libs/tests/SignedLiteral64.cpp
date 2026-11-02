#include "Translation/TranslationContext.hpp"
#include "RdnaDecoder/RdnaInstruction.hpp"
#include <array>
#include <cstdint>
#include <stdexcept>

using namespace ShaderRecompiler;
static void Require(bool value) { if (!value) throw std::runtime_error("signed literal 64-bit regression"); }
static RdnaOperand Literal(std::uint32_t value) {
    RdnaOperand operand{};
    operand.kind = RdnaOperandKind::LiteralConstant;
    operand.value = value;
    return operand;
}
static RdnaOperand Zero() {
    RdnaOperand operand{};
    operand.kind = RdnaOperandKind::IntegerInlineConstant;
    return operand;
}
static RdnaOperand Register(RdnaOperandKind kind, std::uint32_t reg) {
    RdnaOperand operand{};
    operand.kind = kind;
    operand.reg = reg;
    return operand;
}
static RdnaInstruction Make(RdnaOpcode op, RdnaInstructionFamily family, const RdnaOperand& destination, const std::array<RdnaOperand, 3>& sources, std::uint32_t count) {
    RdnaInstruction instruction{};
    instruction.op = op;
    instruction.family = family;
    instruction.destination = destination;
    instruction.sourceCount = count;
    instruction.source0 = sources[0];
    instruction.source1 = sources[1];
    instruction.source2 = sources[2];
    return instruction;
}
static bool HasLiteralPair(IrBlock& block, std::uint32_t high) {
    for (const auto* value : block.Instructions()) {
        if (value->Opcode() != IrOpcode::CompositeConstructU64) continue;
        const IrValue* low = value->Argument(0u)->Resolve();
        const IrValue* upper = value->Argument(1u)->Resolve();
        if (low->HasImmediate() && upper->HasImmediate() && low->ImmediateU32() == 0x80000000u && upper->ImmediateU32() == high) return true;
    }
    return false;
}
static void Check(const RdnaInstruction& instruction, std::uint32_t expectedHigh) {
    IrProgram program;
    auto& block = program.CreateBlock();
    program.SetEntryBlock(block);
    TranslationContext context(program, block, 256u);
    context.TranslateInstruction(instruction);
    Require(HasLiteralPair(block, expectedHigh));
}

int main() {
    const RdnaOperand none{};
    Check(Make(RdnaOpcode::VCmpLtI64, RdnaInstructionFamily::VOPC, Register(RdnaOperandKind::ScalarRegister, 24u), {Literal(0x80000000u), Zero(), none}, 2u), 0xffffffffu);
    Check(Make(RdnaOpcode::VCmpEqI64, RdnaInstructionFamily::VOPC, Register(RdnaOperandKind::VccLo, 0u), {Literal(0x80000000u), Zero(), none}, 2u), 0xffffffffu);
    Check(Make(RdnaOpcode::VCmpNeI64, RdnaInstructionFamily::VOPC, Register(RdnaOperandKind::VccLo, 0u), {Zero(), Literal(0x80000000u), none}, 2u), 0xffffffffu);
    Check(Make(RdnaOpcode::VAshrrevI64, RdnaInstructionFamily::VOP3, Register(RdnaOperandKind::VectorRegister, 12u), {Zero(), Literal(0x80000000u), none}, 2u), 0xffffffffu);
    Check(Make(RdnaOpcode::SAshrI64, RdnaInstructionFamily::SOP2, Register(RdnaOperandKind::ScalarRegister, 12u), {Literal(0x80000000u), Zero(), none}, 2u), 0xffffffffu);
    Check(Make(RdnaOpcode::SBfeI64, RdnaInstructionFamily::SOP2, Register(RdnaOperandKind::ScalarRegister, 12u), {Literal(0x80000000u), Zero(), none}, 2u), 0xffffffffu);
    Check(Make(RdnaOpcode::SFlbitI32I64, RdnaInstructionFamily::SOP1, Register(RdnaOperandKind::ScalarRegister, 12u), {Literal(0x80000000u), none, none}, 1u), 0xffffffffu);
    Check(Make(RdnaOpcode::VMadI64I32, RdnaInstructionFamily::VOP3, Register(RdnaOperandKind::VectorRegister, 12u), {Zero(), Zero(), Literal(0x80000000u)}, 3u), 0xffffffffu);
    Check(Make(RdnaOpcode::VCmpLtU64, RdnaInstructionFamily::VOPC, Register(RdnaOperandKind::VccLo, 0u), {Literal(0x80000000u), Zero(), none}, 2u), 0u);
    Check(Make(RdnaOpcode::VLshrrevB64, RdnaInstructionFamily::VOP3, Register(RdnaOperandKind::VectorRegister, 12u), {Zero(), Literal(0x80000000u), none}, 2u), 0u);
    Check(Make(RdnaOpcode::SLshrB64, RdnaInstructionFamily::SOP2, Register(RdnaOperandKind::ScalarRegister, 12u), {Literal(0x80000000u), Zero(), none}, 2u), 0u);
    Check(Make(RdnaOpcode::VMadU64U32, RdnaInstructionFamily::VOP3, Register(RdnaOperandKind::VectorRegister, 12u), {Zero(), Zero(), Literal(0x80000000u)}, 3u), 0u);
    return 0;
}
