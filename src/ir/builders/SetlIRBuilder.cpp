#include <iostream>
#include <sstream>
#include <stdexcept>

#include <SetlIRBuilder.h>
#include <Registers.h>
#include <SMT2Lib.h>
#include <SymbolicExpression.h>


SetlIRBuilder::SetlIRBuilder(uint64 address, const std::string &disassembly):
  BaseIRBuilder(address, disassembly) {
}


void SetlIRBuilder::imm(AnalysisProcessor &ap, Inst &inst) const {
  OneOperandTemplate::stop(this->disas);
}


void SetlIRBuilder::reg(AnalysisProcessor &ap, Inst &inst) const {
  SymbolicExpression *se;
  smt2lib::smtAstAbstractNode *expr, *sf, *of;
  uint64 reg     = this->operands[0].getValue();
  uint64 regSize = this->operands[0].getSize();

  /* Create the flag SMT semantic */
  sf = ap.buildSymbolicFlagOperand(ID_SF);
  of = ap.buildSymbolicFlagOperand(ID_OF);

  /* Finale expr */
  expr = smt2lib::ite(
            smt2lib::equal(
              smt2lib::bvxor(sf, of),
              smt2lib::bvtrue()),
            smt2lib::bv(1, BYTE_SIZE_BIT),
            smt2lib::bv(0, BYTE_SIZE_BIT));

  /* Create the symbolic expression */
  se = ap.createRegSE(inst, expr, reg, regSize);

  /* Apply the taint via the concretization */
  if (ap.getFlagValue(ID_SF) ^ ap.getFlagValue(ID_OF)) {
    if (ap.isRegTainted(ID_SF) == TAINTED)
      ap.assignmentSpreadTaintRegReg(se, reg, ID_SF);
    else
      ap.assignmentSpreadTaintRegReg(se, reg, ID_OF);
  }

}


void SetlIRBuilder::mem(AnalysisProcessor &ap, Inst &inst) const {
  SymbolicExpression *se;
  smt2lib::smtAstAbstractNode *expr, *sf, *of;
  uint64 mem     = this->operands[0].getValue();
  uint64 memSize = this->operands[0].getSize();

  /* Create the flag SMT semantic */
  sf = ap.buildSymbolicFlagOperand(ID_SF);
  of = ap.buildSymbolicFlagOperand(ID_OF);

  /* Finale expr */
  expr = smt2lib::ite(
            smt2lib::equal(
              smt2lib::bvxor(sf, of),
              smt2lib::bvtrue()),
            smt2lib::bv(1, BYTE_SIZE_BIT),
            smt2lib::bv(0, BYTE_SIZE_BIT));

  /* Create the symbolic expression */
  se = ap.createMemSE(inst, expr, mem, memSize);

  /* Apply the taint via the concretization */
  if (ap.getFlagValue(ID_SF) ^ ap.getFlagValue(ID_OF)) {
    if (ap.isRegTainted(ID_SF) == TAINTED)
      ap.assignmentSpreadTaintMemReg(se, mem, ID_SF, memSize);
    else
      ap.assignmentSpreadTaintMemReg(se, mem, ID_OF, memSize);
  }

}


void SetlIRBuilder::none(AnalysisProcessor &ap, Inst &inst) const {
  OneOperandTemplate::stop(this->disas);
}


Inst *SetlIRBuilder::process(AnalysisProcessor &ap) const {
  this->checkSetup();

  Inst *inst = new Inst(ap.getThreadID(), this->address, this->disas);

  try {
    this->templateMethod(ap, *inst, this->operands, "SETL");
    ap.incNumberOfExpressions(inst->numberOfExpressions()); /* Used for statistics */
    ControlFlow::rip(*inst, ap, this->nextAddress);
  }
  catch (std::exception &e) {
    delete inst;
    throw;
  }

  return inst;
}

