// This file is a part of Julia. License is MIT: https://julialang.org/license

#define DEBUG_TYPE "combine_muladd"
#undef DEBUG
#include "llvm-version.h"

#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Pass.h>
#include <llvm/Support/Debug.h>
#include "fix_llvm_assert.h"

#include "julia.h"

using namespace llvm;

/**
 * Combine
 * ```
 * %v0 = fmul ... %a, %b
 * %v = fadd ... %v0, %c
 * ```
 * to
 * `%v = call ... @llvm.fmuladd.<...>(... %a, ... %b, ... %c)`
 * when `%v0` has no other use and at least one of them allows unsafe arithmetic
 */

struct CombineMulAdd : public FunctionPass {
    static char ID;
    CombineMulAdd() : FunctionPass(ID)
    {}

private:
    bool runOnFunction(Function &F) override;
};

static inline bool allowContract(Instruction *inst)
{
#if JL_LLVM_VERSION >= 50000
    return inst->hasAllowContract();
#else
    return inst->hasUnsafeAlgebra();
#endif
}

static bool checkCombine(Module *m, Instruction *addOp, Value *maybeMul, Value *addend,
                         bool negadd, bool negres, bool unsafe_add)
{
    auto mulOp = dyn_cast<Instruction>(maybeMul);
    if (!mulOp || mulOp->getOpcode() != Instruction::FMul)
        return false;
    FastMathFlags fmf = addOp->getFastMathFlags();
    if (!unsafe_add) {
        if (!allowContract(mulOp))
            return false;
        // Update muladd.ll test when this #ifdef is removed
#if JL_LLVM_VERSION >= 50000
        fmf.setAllowContract(true);
#endif
    }
    if ((!unsafe_add && !allowContract(mulOp)) || !mulOp->hasOneUse())
        return false;
    IRBuilder<> builder(m->getContext());
    builder.SetInsertPoint(addOp);
    auto mul1 = mulOp->getOperand(0);
    auto mul2 = mulOp->getOperand(1);
    Value *muladdf = Intrinsic::getDeclaration(m, Intrinsic::fmuladd, addOp->getType());
    if (negadd) {
        auto newaddend = builder.CreateFNeg(addend);
        // Might be a const
        if (auto neginst = dyn_cast<Instruction>(newaddend))
            neginst->copyFastMathFlags(fmf);
        addend = newaddend;
    }
    Instruction *newv = builder.CreateCall(muladdf, {mul1, mul2, addend});
    newv->copyFastMathFlags(fmf);
    if (negres) {
        // Shouldn't be a constant
        newv = cast<Instruction>(builder.CreateFNeg(newv));
        newv->copyFastMathFlags(fmf);
    }
    addOp->replaceAllUsesWith(newv);
    addOp->eraseFromParent();
    mulOp->eraseFromParent();
    return true;
}

bool CombineMulAdd::runOnFunction(Function &F)
{
    Module *m = F.getParent();
    for (auto &BB: F) {
        for (auto it = BB.begin(); it != BB.end();) {
            auto &I = *it;
            it++;
            switch (I.getOpcode()) {
            case Instruction::FAdd: {
                bool unsafe_add = I.hasUnsafeAlgebra();
                checkCombine(m, &I, I.getOperand(0), I.getOperand(1),
                             false, false, unsafe_add) ||
                    checkCombine(m, &I, I.getOperand(1), I.getOperand(0),
                                 false, false, unsafe_add);
                break;
            }
            case Instruction::FSub: {
                bool unsafe_add = I.hasUnsafeAlgebra();
                checkCombine(m, &I, I.getOperand(0), I.getOperand(1),
                             true, false, unsafe_add) ||
                    checkCombine(m, &I, I.getOperand(1), I.getOperand(0),
                                 true, true, unsafe_add);
                break;
            }
            default:
                break;
            }
        }
    }
    return true;
}

char CombineMulAdd::ID = 0;
static RegisterPass<CombineMulAdd> X("CombineMulAdd", "Combine mul and add to muladd",
                                     false /* Only looks at CFG */,
                                     false /* Analysis Pass */);

Pass *createCombineMulAddPass()
{
    return new CombineMulAdd();
}
