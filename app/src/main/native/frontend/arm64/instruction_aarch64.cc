//
// Created by 甘尧 on 2019-09-24.
//

#include "instruction_aarch64.h"
#include "instruction_decode.h"

using namespace Instruction::A64;

InstructionA64::InstructionA64() : backup_({}) {
    SetOpcode(OpcodeA64::UN_DECODED);
}

void InstructionA64::SetOpcode(OpcodeA64 opcode) {
    opcode_ = static_cast<u32>(opcode);
}

OpcodeA64 InstructionA64::GetOpcode() {
    return OpcodeA64(opcode_);
}

bool InstructionA64::Invalid() {
    return GetOpcode() == OpcodeA64::INVALID;
}

bool InstructionA64::Disassemble(AArch64Inst &inst) {
    if (GetOpcode() == OpcodeA64::UN_DECODED) {
        SetOpcode(DefaultDecoder::DecodeOpCode(inst.raw, TypeOfA64()));
    } else {
        backup_ = inst;
    }
    return true;
}

bool InstructionA64::Assemble() {
    *pc_ = backup_;
    return true;
}


//Branch
InstrA64Branch::InstrA64Branch() {
    type_ = Branch;
}

Condition InstrA64Branch::GetCond() const {
    return cond_;
}

void InstrA64Branch::SetCond(Condition cond) {
    cond_ = cond;
}

bool InstrA64Branch::HasCond() const {
    return cond_ != AL;
}

VAddr InstrA64Branch::GetTarget() const {
    return target_;
}

void InstrA64Branch::SetTarget(VAddr target) {
    target_ = target;
}

bool InstrA64Branch::IsAbs() const {
    return is_abs_;
}

s32 InstrA64Branch::GetOffset() const {
    return offset_;
}

void InstrA64Branch::SetOffset(s32 offset) {
    is_abs_ = false;
    offset_ = offset;
}

bool InstrA64Branch::IsLink() const {
    return link_;
}

void InstrA64Branch::SetLink(bool link) {
    link_ = link;
}

bool InstrA64Branch::Disassemble(AArch64Inst &inst) {
    InstructionA64::Disassemble(inst);
    switch (GetOpcode()) {
        case OpcodeA64::B_cond:
            SetCond(Condition(inst.cond));
            DECODE_OFFSET(inst.bch_cond_offset, 19, 2);
            break;
        case OpcodeA64::BL:
            link_ = true;
        case OpcodeA64::B:
            DECODE_OFFSET(inst.bch_ucond_offset, 26, 2);
            break;
        case OpcodeA64::RET:
            break;
        case OpcodeA64::CBNZ_64: case OpcodeA64::CBZ_64: case OpcodeA64::CBZ_32: case OpcodeA64::CBNZ_32:
            DECODE_OFFSET(inst.bch_ucond_offset, 26, 2);
            rt_ = XReg(inst.Rt);
        case OpcodeA64::BLR:
            link_ = true;
        case OpcodeA64::BR:
            rn_ = XReg(inst.Rn);
            break;
        default:
            return false;
    }
    return true;
}

bool InstrA64Branch::Assemble() {
    *pc_ = InstructionTableA64::Get().GetInstrInfo(GetOpcode()).mask_pair.second;
    switch (GetOpcode()) {
        case OpcodeA64::B_cond:
            pc_->cond = cond_;
            pc_->bch_cond_offset = ENCODE_OFFSET(19, 2);
            break;
        case OpcodeA64::BL: case OpcodeA64::B:
            pc_->bch_ucond_offset = ENCODE_OFFSET(26, 2);
            break;
        case OpcodeA64::RET:
            break;
        case OpcodeA64::CBNZ_64: case OpcodeA64::CBZ_64: case OpcodeA64::CBZ_32: case OpcodeA64::CBNZ_32:
            pc_->Rt = static_cast<u32>(rt_);
            pc_->bch_cond_offset = ENCODE_OFFSET(19, 2);
            break;
        case OpcodeA64::BLR: case OpcodeA64::BR:
            pc_->Rn = static_cast<u32>(rn_);
            break;
        case OpcodeA64::TBZ: case OpcodeA64::TBNZ:
            break;
        default:
            return false;
    }
    return true;
}

XReg InstrA64Branch::GetRt() const {
    return rt_;
}

void InstrA64Branch::SetRt(XReg rt) {
    rt_ = rt;
}

XReg InstrA64Branch::GetRn() const {
    return rn_;
}

void InstrA64Branch::SetRn(XReg rn) {
    rn_ = rn;
}


// Exception Generating
InstrA64ExpGen::InstrA64ExpGen() {
    type_ = System;
}

bool InstrA64ExpGen::Disassemble(AArch64Inst &inst) {
    InstructionA64::Disassemble(inst);
    to_exception_level_ = ExceptionLevel(inst.exp_gen_ll);
    SetImm(static_cast<u16>(inst.exp_gen_num));
    switch (GetOpcode()) {
        case OpcodeA64::SVC:
            break;
        case OpcodeA64::HVC:
        case OpcodeA64::SMC:
            break;
        default:
            break;
    }
    return true;
}

bool InstrA64ExpGen::Assemble() {
    *pc_ = InstructionTableA64::Get().GetInstrInfo(GetOpcode()).mask_pair.second;
    pc_->exp_gen_num = GetImm();
    return true;
}

bool InstrA64ExpGen::Excutable(ExceptionLevel cur_lel) {
    return to_exception_level_ - cur_lel == 1;
}

u16 InstrA64ExpGen::GetImm() const {
    return imm_;
}

void InstrA64ExpGen::SetImm(u16 imm) {
    imm_ = imm;
}


//System

InstrA64System::InstrA64System() {
    type_ = System;
}

const SystemRegister &InstrA64System::GetSystemRegister() const {
    return system_register_;
}

void InstrA64System::SetSystemRegister(const SystemRegister &systemRegister) {
    system_register_ = systemRegister;
}

XReg InstrA64System::GetRt() const {
    return rt_;
}

void InstrA64System::SetRt(XReg rt) {
    rt_ = rt;
}

bool InstrA64System::Disassemble(AArch64Inst &inst) {
    InstructionA64::Disassemble(inst);
    switch (GetOpcode()) {
        case OpcodeA64::MRS: case OpcodeA64::MSR_imm: case OpcodeA64::MSR_reg:
            rt_ = XReg(inst.Rd);
            system_register_ = SystemRegister(static_cast<u16>(inst.system_register));
            break;
    }
    return true;
}

bool InstrA64System::Assemble() {
    *pc_ = InstructionTableA64::Get().GetInstrInfo(GetOpcode()).mask_pair.second;
    switch (GetOpcode()) {
        case OpcodeA64::MRS: case OpcodeA64::MSR_imm: case OpcodeA64::MSR_reg:
            pc_->Rd = static_cast<u32>(rt_);
            pc_->system_register = system_register_.Value();
            break;
    }
    return true;
}

// PC Relate Addressing
InstrA64PCRelAddr::InstrA64PCRelAddr() {
    type_ = NormalCalculations;
}

s32 InstrA64PCRelAddr::GetOffset() const {
    return offset_;
}

void InstrA64PCRelAddr::SetOffset(s32 offset) {
    offset_ = offset;
}

bool InstrA64PCRelAddr::PageAlign() const {
    return page_align_;
}

VAddr InstrA64PCRelAddr::GetTarget() {
    VAddr base = reinterpret_cast<VAddr>(GetPC());
    if (PageAlign()) {
        base = RoundDown(base, PAGE_SIZE);
    }
    return base + GetOffset();
}

bool InstrA64PCRelAddr::Disassemble(AArch64Inst &inst) {
    InstructionA64::Disassemble(inst);
    switch (GetOpcode()) {
        case OpcodeA64::ADRP:
            page_align_ = true;
        case OpcodeA64::ADR:
            u64 imm = (inst.immhi << 2) | inst.immlo;
            DECODE_OFFSET(imm, 21, 0);
            if (page_align_) {
                offset_ <<= A64_PAGE_SIZE;
            }
            break;
    }
    return true;
}

bool InstrA64PCRelAddr::Assemble() {
    return InstructionA64::Assemble();
}



//Add & Sub Immediate
InstrA64AddSubImm::InstrA64AddSubImm() : rd_() {}

bool InstrA64AddSubImm::Disassemble(AArch64Inst &inst) {
    InstructionA64::Disassemble(inst);
    is_sub_ = inst.addsub_imm_update_sub == 1;
    is_64bit = inst.addsub_imm_update_64bit == 1;
    update_flag_ = inst.addsub_imm_update_flag == 1;
    shift_ = inst.shift == 1;
    operand_.shift_extend_imm_ = static_cast<s32>(static_cast<u64>(inst.dp_imm)
            << ((shift_) ? PAGE_OFFSET : 0));
    if (is_64bit) {
        rd_ = XREG(inst.Rd);
        operand_.reg_ = XREG(inst.Rn);
    } else {
        rd_ = WREG(inst.Rd);
        operand_.reg_ = WREG(inst.Rn);
    }
    return true;
}

bool InstrA64AddSubImm::Assemble() {
    return InstructionA64::Assemble();
}

bool InstrA64AddSubImm::IsSub() const {
    return is_sub_;
}

bool InstrA64AddSubImm::IsUpdateFlag() const {
    return update_flag_;
}

bool InstrA64AddSubImm::Is64Bit() const {
    return is_64bit;
}

GeneralRegister &InstrA64AddSubImm::GetRd() {
    return rd_;
}

void InstrA64AddSubImm::SetRd(GeneralRegister rd) {
    rd_ = rd;
}

const Operand &InstrA64AddSubImm::GetOperand() const {
    return operand_;
}

void InstrA64AddSubImm::SetOperand(const Operand &operand) {
    operand_ = operand;
}
