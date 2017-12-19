// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX               Intel hardware intrinsic Code Generator                     XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/
#include "jitpch.h"
#ifdef _MSC_VER
#pragma hdrstop
#endif

#if FEATURE_HW_INTRINSICS

#include "emit.h"
#include "codegen.h"
#include "sideeffects.h"
#include "lower.h"
#include "gcinfo.h"
#include "gcinfoencoder.h"

//------------------------------------------------------------------------
// genIsTableDrivenHWIntrinsic:
//
// Arguments:
//    category - category of a HW intrinsic
//
// Return Value:
//    returns true if this category can be table-driven in CodeGen
//
static bool genIsTableDrivenHWIntrinsic(HWIntrinsicCategory category, HWIntrinsicFlag flags)
{
    // TODO - make more categories to the table-driven framework
    // HW_Category_Helper and HW_Flag_MultiIns usually need manual codegen
    const bool tableDrivenCategory =
        category == HW_Category_SimpleSIMD || category == HW_Category_MemoryLoad || category == HW_Category_SIMDScalar;
    const bool tableDrivenFlag = (flags & HW_Flag_MultiIns) == 0;
    return tableDrivenCategory && tableDrivenFlag;
}

void CodeGen::genHWIntrinsic(GenTreeHWIntrinsic* node)
{
    NamedIntrinsic      intrinsicID = node->gtHWIntrinsicId;
    InstructionSet      isa         = Compiler::isaOfHWIntrinsic(intrinsicID);
    HWIntrinsicCategory category    = Compiler::categoryOfHWIntrinsic(intrinsicID);
    HWIntrinsicFlag     flags       = Compiler::flagsOfHWIntrinsic(intrinsicID);
    int                 ival        = Compiler::ivalOfHWIntrinsic(intrinsicID);
    int                 numArgs     = Compiler::numArgsOfHWIntrinsic(intrinsicID);

    assert((flags & HW_Flag_NoCodeGen) == 0);

    if (genIsTableDrivenHWIntrinsic(category, flags))
    {
        GenTree*  op1        = node->gtGetOp1();
        GenTree*  op2        = node->gtGetOp2();
        regNumber targetReg  = node->gtRegNum;
        var_types targetType = node->TypeGet();
        var_types baseType   = node->gtSIMDBaseType;

        regNumber op1Reg = REG_NA;
        regNumber op2Reg = REG_NA;
        emitter*  emit   = getEmitter();

        assert(numArgs >= 0);
        instruction ins = Compiler::insOfHWIntrinsic(intrinsicID, baseType);
        assert(ins != INS_invalid);
        emitAttr simdSize = (emitAttr)(node->gtSIMDSize);
        assert(simdSize != 0);

        switch (numArgs)
        {
            case 1:
                genConsumeOperands(node);
                op1Reg = op1->gtRegNum;
                if (category == HW_Category_MemoryLoad)
                {
                    emit->emitIns_R_AR(ins, simdSize, targetReg, op1Reg, 0);
                }
                else if (category == HW_Category_SIMDScalar && (flags & HW_Flag_CopyUpperBits) != 0)
                {
                    emit->emitIns_SIMD_R_R_R(ins, simdSize, targetReg, op1Reg, op1Reg);
                }
                else
                {

                    emit->emitIns_R_R(ins, simdSize, targetReg, op1Reg);
                }
                break;

            case 2:
                genConsumeOperands(node);
                if (ival != -1)
                {
                    genHWIntrinsic_R_R_RM_I(node, ins);
                }
                else if (category == HW_Category_MemoryLoad)
                {
                    emit->emitIns_SIMD_R_R_AR(ins, emitTypeSize(TYP_SIMD16), targetReg, op1->gtRegNum, op2->gtRegNum);
                }
                else
                {
                    genHWIntrinsic_R_R_RM(node, ins);
                }
                break;
            case 3:
            {
                assert(op1->OperIsList());
                assert(op1->gtGetOp2()->OperIsList());
                assert(op1->gtGetOp2()->gtGetOp2()->OperIsList());

                GenTreeArgList* argList = op1->AsArgList();
                op1                     = argList->Current();
                genConsumeRegs(op1);
                op1Reg = op1->gtRegNum;

                argList = argList->Rest();
                op2     = argList->Current();
                genConsumeRegs(op2);
                op2Reg = op2->gtRegNum;

                argList      = argList->Rest();
                GenTree* op3 = argList->Current();
                genConsumeRegs(op3);
                regNumber op3Reg = op3->gtRegNum;

                emit->emitIns_SIMD_R_R_R_R(ins, simdSize, targetReg, op1Reg, op2Reg, op3Reg);
                break;
            }

            default:
                unreached();
                break;
        }
        genProduceReg(node);
        return;
    }

    switch (isa)
    {
        case InstructionSet_SSE:
            genSSEIntrinsic(node);
            break;
        case InstructionSet_SSE2:
            genSSE2Intrinsic(node);
            break;
        case InstructionSet_SSE3:
            genSSE3Intrinsic(node);
            break;
        case InstructionSet_SSSE3:
            genSSSE3Intrinsic(node);
            break;
        case InstructionSet_SSE41:
            genSSE41Intrinsic(node);
            break;
        case InstructionSet_SSE42:
            genSSE42Intrinsic(node);
            break;
        case InstructionSet_AVX:
            genAVXIntrinsic(node);
            break;
        case InstructionSet_AVX2:
            genAVX2Intrinsic(node);
            break;
        case InstructionSet_AES:
            genAESIntrinsic(node);
            break;
        case InstructionSet_BMI1:
            genBMI1Intrinsic(node);
            break;
        case InstructionSet_BMI2:
            genBMI2Intrinsic(node);
            break;
        case InstructionSet_FMA:
            genFMAIntrinsic(node);
            break;
        case InstructionSet_LZCNT:
            genLZCNTIntrinsic(node);
            break;
        case InstructionSet_PCLMULQDQ:
            genPCLMULQDQIntrinsic(node);
            break;
        case InstructionSet_POPCNT:
            genPOPCNTIntrinsic(node);
            break;
        default:
            unreached();
            break;
    }
}

void CodeGen::genHWIntrinsic_R_R_RM(GenTreeHWIntrinsic* node, instruction ins)
{
    var_types targetType = node->TypeGet();
    regNumber targetReg  = node->gtRegNum;
    GenTree*  op1        = node->gtGetOp1();
    GenTree*  op2        = node->gtGetOp2();
    emitAttr  simdSize   = (emitAttr)(node->gtSIMDSize);
    emitter*  emit       = getEmitter();

    // TODO-XArch-CQ: Commutative operations can have op1 be contained
    // TODO-XArch-CQ: Non-VEX encoded instructions can have both ops contained

    regNumber op1Reg = op1->gtRegNum;

    assert(targetReg != REG_NA);
    assert(op1Reg != REG_NA);

    if (op2->isContained() || op2->isUsedFromSpillTemp())
    {
        TempDsc* tmpDsc = nullptr;
        unsigned varNum = BAD_VAR_NUM;
        unsigned offset = (unsigned)-1;

        if (op2->isUsedFromSpillTemp())
        {
            assert(op2->IsRegOptional());

            tmpDsc = getSpillTempDsc(op2);
            varNum = tmpDsc->tdTempNum();
            offset = 0;

            compiler->tmpRlsTemp(tmpDsc);
        }
        else if (op2->isIndir())
        {
            GenTreeIndir* memIndir = op2->AsIndir();
            GenTree*      memBase  = memIndir->gtOp1;

            switch (memBase->OperGet())
            {
                case GT_LCL_VAR_ADDR:
                {
                    varNum = memBase->AsLclVarCommon()->GetLclNum();
                    offset = 0;

                    // Ensure that all the GenTreeIndir values are set to their defaults.
                    assert(memBase->gtRegNum == REG_NA);
                    assert(!memIndir->HasIndex());
                    assert(memIndir->Scale() == 1);
                    assert(memIndir->Offset() == 0);

                    break;
                }

                case GT_CLS_VAR_ADDR:
                {
                    emit->emitIns_SIMD_R_R_C(ins, simdSize, targetReg, op1Reg, memBase->gtClsVar.gtClsVarHnd, 0);
                    return;
                }

                default:
                {
                    emit->emitIns_SIMD_R_R_A(ins, simdSize, targetReg, op1Reg, memIndir);
                    return;
                }
            }
        }
        else
        {
            switch (op2->OperGet())
            {
                case GT_LCL_FLD:
                {
                    GenTreeLclFld* lclField = op2->AsLclFld();

                    varNum = lclField->GetLclNum();
                    offset = lclField->gtLclFld.gtLclOffs;
                    break;
                }

                case GT_LCL_VAR:
                {
                    assert(op2->IsRegOptional() || !compiler->lvaTable[op2->gtLclVar.gtLclNum].lvIsRegCandidate());
                    varNum = op2->AsLclVar()->GetLclNum();
                    offset = 0;
                    break;
                }

                default:
                    unreached();
                    break;
            }
        }

        // Ensure we got a good varNum and offset.
        // We also need to check for `tmpDsc != nullptr` since spill temp numbers
        // are negative and start with -1, which also happens to be BAD_VAR_NUM.
        assert((varNum != BAD_VAR_NUM) || (tmpDsc != nullptr));
        assert(offset != (unsigned)-1);

        emit->emitIns_SIMD_R_R_S(ins, simdSize, targetReg, op1Reg, varNum, offset);
    }
    else
    {
        emit->emitIns_SIMD_R_R_R(ins, simdSize, targetReg, op1Reg, op2->gtRegNum);
    }
}

void CodeGen::genHWIntrinsic_R_R_RM_I(GenTreeHWIntrinsic* node, instruction ins)
{
    var_types targetType = node->TypeGet();
    regNumber targetReg  = node->gtRegNum;
    GenTree*  op1        = node->gtGetOp1();
    GenTree*  op2        = node->gtGetOp2();
    int       ival       = Compiler::ivalOfHWIntrinsic(node->gtHWIntrinsicId);
    emitter*  emit       = getEmitter();

    // TODO-XArch-CQ: Commutative operations can have op1 be contained
    // TODO-XArch-CQ: Non-VEX encoded instructions can have both ops contained

    regNumber op1Reg = op1->gtRegNum;

    assert(targetReg != REG_NA);
    assert(op1Reg != REG_NA);

    if (op2->isContained() || op2->isUsedFromSpillTemp())
    {
        TempDsc* tmpDsc = nullptr;
        unsigned varNum = BAD_VAR_NUM;
        unsigned offset = (unsigned)-1;

        if (op2->isUsedFromSpillTemp())
        {
            assert(op2->IsRegOptional());

            tmpDsc = getSpillTempDsc(op2);
            varNum = tmpDsc->tdTempNum();
            offset = 0;

            compiler->tmpRlsTemp(tmpDsc);
        }
        else if (op2->isIndir())
        {
            GenTreeIndir* memIndir = op2->AsIndir();
            GenTree*      memBase  = memIndir->gtOp1;

            switch (memBase->OperGet())
            {
                case GT_LCL_VAR_ADDR:
                {
                    varNum = memBase->AsLclVarCommon()->GetLclNum();
                    offset = 0;

                    // Ensure that all the GenTreeIndir values are set to their defaults.
                    assert(memBase->gtRegNum == REG_NA);
                    assert(!memIndir->HasIndex());
                    assert(memIndir->Scale() == 1);
                    assert(memIndir->Offset() == 0);

                    break;
                }

                case GT_CLS_VAR_ADDR:
                {
                    emit->emitIns_SIMD_R_R_C_I(ins, emitTypeSize(targetType), targetReg, op1Reg,
                                               memBase->gtClsVar.gtClsVarHnd, 0, ival);
                    return;
                }

                default:
                {
                    emit->emitIns_SIMD_R_R_A_I(ins, emitTypeSize(targetType), targetReg, op1Reg, memIndir, ival);
                    return;
                }
            }
        }
        else
        {
            switch (op2->OperGet())
            {
                case GT_LCL_FLD:
                {
                    GenTreeLclFld* lclField = op2->AsLclFld();

                    varNum = lclField->GetLclNum();
                    offset = lclField->gtLclFld.gtLclOffs;
                    break;
                }

                case GT_LCL_VAR:
                {
                    assert(op2->IsRegOptional() || !compiler->lvaTable[op2->gtLclVar.gtLclNum].lvIsRegCandidate());
                    varNum = op2->AsLclVar()->GetLclNum();
                    offset = 0;
                    break;
                }

                default:
                    unreached();
                    break;
            }
        }

        // Ensure we got a good varNum and offset.
        // We also need to check for `tmpDsc != nullptr` since spill temp numbers
        // are negative and start with -1, which also happens to be BAD_VAR_NUM.
        assert((varNum != BAD_VAR_NUM) || (tmpDsc != nullptr));
        assert(offset != (unsigned)-1);

        emit->emitIns_SIMD_R_R_S_I(ins, emitTypeSize(targetType), targetReg, op1Reg, varNum, offset, ival);
    }
    else
    {
        emit->emitIns_SIMD_R_R_R_I(ins, emitTypeSize(targetType), targetReg, op1Reg, op2->gtRegNum, ival);
    }
}

void CodeGen::genSSEIntrinsic(GenTreeHWIntrinsic* node)
{
    NamedIntrinsic intrinsicID = node->gtHWIntrinsicId;
    GenTree*       op1         = node->gtGetOp1();
    GenTree*       op2         = node->gtGetOp2();
    GenTree*       op3         = nullptr;
    GenTree*       op4         = nullptr;
    regNumber      targetReg   = node->gtRegNum;
    var_types      targetType  = node->TypeGet();
    var_types      baseType    = node->gtSIMDBaseType;
    instruction    ins         = Compiler::insOfHWIntrinsic(intrinsicID, node->gtSIMDBaseType);

    regNumber op1Reg = REG_NA;
    regNumber op2Reg = REG_NA;
    regNumber op3Reg = REG_NA;
    regNumber op4Reg = REG_NA;
    emitter*  emit   = getEmitter();

    if ((op1 != nullptr) && !op1->OperIsList())
    {
        op1Reg = op1->gtRegNum;
        genConsumeOperands(node);
    }

    switch (intrinsicID)
    {
        case NI_SSE_ConvertToVector128SingleScalar:
        {
            assert(node->TypeGet() == TYP_SIMD16);
            assert(node->gtSIMDBaseType == TYP_FLOAT);
            assert(Compiler::ivalOfHWIntrinsic(intrinsicID) == -1);

            instruction ins = Compiler::insOfHWIntrinsic(intrinsicID, node->gtSIMDBaseType);
            genHWIntrinsic_R_R_RM(node, ins);
            break;
        }
        case NI_SSE_CompareEqualOrderedScalar:
        case NI_SSE_CompareEqualUnorderedScalar:
        {
            assert(baseType == TYP_FLOAT);
            op2Reg           = op2->gtRegNum;
            regNumber tmpReg = node->GetSingleTempReg();

            emit->emitIns_R_R(ins, emitTypeSize(TYP_SIMD16), op1Reg, op2Reg);
            emit->emitIns_R(INS_setpo, EA_1BYTE, targetReg);
            emit->emitIns_R(INS_sete, EA_1BYTE, tmpReg);
            emit->emitIns_R_R(INS_and, EA_1BYTE, tmpReg, targetReg);
            emit->emitIns_R(INS_setne, EA_1BYTE, targetReg);
            emit->emitIns_R_R(INS_movzx, EA_1BYTE, targetReg, targetReg);
            break;
        }

        case NI_SSE_CompareGreaterThanOrderedScalar:
        case NI_SSE_CompareGreaterThanUnorderedScalar:
        {
            assert(baseType == TYP_FLOAT);
            op2Reg = op2->gtRegNum;

            emit->emitIns_R_R(ins, emitTypeSize(TYP_SIMD16), op1Reg, op2Reg);
            emit->emitIns_R(INS_seta, EA_1BYTE, targetReg);
            emit->emitIns_R_R(INS_movzx, EA_1BYTE, targetReg, targetReg);
            break;
        }

        case NI_SSE_CompareGreaterThanOrEqualOrderedScalar:
        case NI_SSE_CompareGreaterThanOrEqualUnorderedScalar:
        {
            assert(baseType == TYP_FLOAT);
            op2Reg = op2->gtRegNum;

            emit->emitIns_R_R(ins, emitTypeSize(TYP_SIMD16), op1Reg, op2Reg);
            emit->emitIns_R(INS_setae, EA_1BYTE, targetReg);
            emit->emitIns_R_R(INS_movzx, EA_1BYTE, targetReg, targetReg);
            break;
        }

        case NI_SSE_CompareLessThanOrderedScalar:
        case NI_SSE_CompareLessThanUnorderedScalar:
        {
            assert(baseType == TYP_FLOAT);
            op2Reg = op2->gtRegNum;

            emit->emitIns_R_R(ins, emitTypeSize(TYP_SIMD16), op2Reg, op1Reg);
            emit->emitIns_R(INS_seta, EA_1BYTE, targetReg);
            emit->emitIns_R_R(INS_movzx, EA_1BYTE, targetReg, targetReg);
            break;
        }

        case NI_SSE_CompareLessThanOrEqualOrderedScalar:
        case NI_SSE_CompareLessThanOrEqualUnorderedScalar:
        {
            assert(baseType == TYP_FLOAT);
            op2Reg = op2->gtRegNum;

            emit->emitIns_R_R(ins, emitTypeSize(TYP_SIMD16), op2Reg, op1Reg);
            emit->emitIns_R(INS_setae, EA_1BYTE, targetReg);
            emit->emitIns_R_R(INS_movzx, EA_1BYTE, targetReg, targetReg);
            break;
        }

        case NI_SSE_CompareNotEqualOrderedScalar:
        case NI_SSE_CompareNotEqualUnorderedScalar:
        {
            assert(baseType == TYP_FLOAT);
            op2Reg = op2->gtRegNum;

            regNumber tmpReg = node->GetSingleTempReg();

            emit->emitIns_R_R(ins, emitTypeSize(TYP_SIMD16), op1Reg, op2Reg);
            emit->emitIns_R(INS_setpe, EA_1BYTE, targetReg);
            emit->emitIns_R(INS_setne, EA_1BYTE, tmpReg);
            emit->emitIns_R_R(INS_or, EA_1BYTE, tmpReg, targetReg);
            emit->emitIns_R(INS_setne, EA_1BYTE, targetReg);
            emit->emitIns_R_R(INS_movzx, EA_1BYTE, targetReg, targetReg);
            break;
        }

        case NI_SSE_ConvertToSingle:
        case NI_SSE_StaticCast:
        {
            assert(op2 == nullptr);
            if (op1Reg != targetReg)
            {
                emit->emitIns_R_R(ins, emitTypeSize(TYP_SIMD16), targetReg, op1Reg);
            }
            break;
        }

        case NI_SSE_MoveMask:
        {
            assert(baseType == TYP_FLOAT);
            assert(op2 == nullptr);

            emit->emitIns_R_R(ins, emitTypeSize(TYP_INT), targetReg, op1Reg);
            break;
        }

        case NI_SSE_SetScalar:
        {
            assert(baseType == TYP_FLOAT);
            assert(op2 == nullptr);

            if (op1Reg == targetReg)
            {
                regNumber tmpReg = node->GetSingleTempReg();
                emit->emitIns_R_R(INS_movaps, emitTypeSize(TYP_SIMD16), tmpReg, op1Reg);
                op1Reg = tmpReg;
            }

            emit->emitIns_SIMD_R_R_R(INS_xorps, emitTypeSize(TYP_SIMD16), targetReg, targetReg, targetReg);
            emit->emitIns_SIMD_R_R_R(INS_movss, emitTypeSize(TYP_SIMD16), targetReg, targetReg, op1Reg);
            break;
        }

        case NI_SSE_SetZeroVector128:
        {
            assert(baseType == TYP_FLOAT);
            assert(op1 == nullptr);
            assert(op2 == nullptr);
            emit->emitIns_SIMD_R_R_R(INS_xorps, emitTypeSize(TYP_SIMD16), targetReg, targetReg, targetReg);
            break;
        }

        case NI_SSE_Shuffle:
        {
            GenTreeArgList* argList;

            // Shuffle takes 3 operands, so op1 should be an arg list with two
            // additional node in the chain.
            assert(baseType == TYP_FLOAT);
            assert(op1->OperIsList());
            assert(op1->AsArgList()->Rest() != nullptr);
            assert(op1->AsArgList()->Rest()->Rest() != nullptr);
            assert(op1->AsArgList()->Rest()->Rest()->Rest() == nullptr);
            assert(op2 == nullptr);

            argList = op1->AsArgList();
            op1     = argList->Current();
            op1Reg  = op1->gtRegNum;
            genConsumeRegs(op1);

            argList = argList->Rest();
            op2     = argList->Current();
            op2Reg  = op2->gtRegNum;
            genConsumeRegs(op2);

            argList = argList->Rest();
            op3     = argList->Current();
            genConsumeRegs(op3);

            if (op3->IsCnsIntOrI())
            {
                ssize_t ival = op3->AsIntConCommon()->IconValue();
                emit->emitIns_SIMD_R_R_R_I(INS_shufps, emitTypeSize(TYP_SIMD16), targetReg, op1Reg, op2Reg, (int)ival);
            }
            else
            {
                // We emit a fallback case for the scenario when op3 is not a constant. This should normally
                // happen when the intrinsic is called indirectly, such as via Reflection. However, it can
                // also occur if the consumer calls it directly and just doesn't pass a constant value.

                const unsigned jmpCount = 256;
                BasicBlock*    jmpTable[jmpCount];

                unsigned jmpTableBase = emit->emitBBTableDataGenBeg(jmpCount, true);
                unsigned jmpTableOffs = 0;

                // Emit the jump table

                JITDUMP("\n      J_M%03u_DS%02u LABEL   DWORD\n", Compiler::s_compMethodsCount, jmpTableBase);

                for (unsigned i = 0; i < jmpCount; i++)
                {
                    jmpTable[i] = genCreateTempLabel();
                    JITDUMP("            DD      L_M%03u_BB%02u\n", Compiler::s_compMethodsCount, jmpTable[i]->bbNum);
                    emit->emitDataGenData(i, jmpTable[i]);
                }

                emit->emitDataGenEnd();

                // Compute and jump to the appropriate offset in the switch table

                regNumber baseReg = node->ExtractTempReg();   // the start of the switch table
                regNumber offsReg = node->GetSingleTempReg(); // the offset into the switch table

                emit->emitIns_R_C(INS_lea, emitTypeSize(TYP_I_IMPL), offsReg, compiler->eeFindJitDataOffs(jmpTableBase),
                                  0);

                emit->emitIns_R_ARX(INS_mov, EA_4BYTE, offsReg, offsReg, op3->gtRegNum, 4, 0);
                emit->emitIns_R_L(INS_lea, EA_PTR_DSP_RELOC, compiler->fgFirstBB, baseReg);
                emit->emitIns_R_R(INS_add, EA_PTRSIZE, offsReg, baseReg);
                emit->emitIns_R(INS_i_jmp, emitTypeSize(TYP_I_IMPL), offsReg);

                // Emit the switch table entries

                BasicBlock* switchTableBeg = genCreateTempLabel();
                BasicBlock* switchTableEnd = genCreateTempLabel();

                genDefineTempLabel(switchTableBeg);

                for (unsigned i = 0; i < jmpCount; i++)
                {
                    genDefineTempLabel(jmpTable[i]);
                    emit->emitIns_SIMD_R_R_R_I(INS_shufps, emitTypeSize(TYP_SIMD16), targetReg, op1Reg, op2Reg, i);
                    emit->emitIns_J(INS_jmp, switchTableEnd);
                }

                genDefineTempLabel(switchTableEnd);
            }
            break;
        }

        default:
            unreached();
            break;
    }

    genProduceReg(node);
}

void CodeGen::genSSE2Intrinsic(GenTreeHWIntrinsic* node)
{
    NamedIntrinsic intrinsicID = node->gtHWIntrinsicId;
    GenTree*       op1         = node->gtGetOp1();
    GenTree*       op2         = node->gtGetOp2();
    ssize_t        imm8        = node->gtImmValue;
    regNumber      targetReg   = node->gtRegNum;
    var_types      targetType  = node->TypeGet();
    var_types      baseType    = node->gtSIMDBaseType;
    instruction    ins         = INS_invalid;

    regNumber op1Reg = op1->gtRegNum;
    regNumber op2Reg = REG_NA;
    emitter*  emit   = getEmitter();

    genConsumeOperands(node);

    switch (intrinsicID)
    {
        case NI_SSE2_Add:
        {
            assert(node->TypeGet() == TYP_SIMD16);

            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_addpd;
                    break;
                case TYP_INT:
                case TYP_UINT:
                    ins = INS_paddd;
                    break;
                case TYP_LONG:
                case TYP_ULONG:
                    ins = INS_paddq;
                    break;
                case TYP_BYTE:
                case TYP_UBYTE:
                    ins = INS_paddb;
                    break;
                case TYP_SHORT:
                case TYP_USHORT:
                    ins = INS_paddw;
                    break;
                default:
                    unreached();
                    break;
            }

            genHWIntrinsic_R_R_RM(node, ins);
            break;
        }

        case NI_SSE2_AddSaturate:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_BYTE:
                    ins = INS_paddsb;
                    break;
                case TYP_UBYTE:
                    ins = INS_paddusb;
                    break;
                case TYP_SHORT:
                    ins = INS_paddsw;
                    break;
                case TYP_USHORT:
                    ins = INS_paddusw;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_And:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_INT:
                case TYP_UINT:
                case TYP_LONG:
                case TYP_ULONG:
                case TYP_BYTE:
                case TYP_UBYTE:
                case TYP_SHORT:
                case TYP_USHORT:
                    ins = INS_pand;
                    break;
                case TYP_DOUBLE:
                    ins = INS_andpd;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(INS_pand, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_AndNot:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_INT:
                case TYP_UINT:
                case TYP_LONG:
                case TYP_ULONG:
                case TYP_BYTE:
                case TYP_UBYTE:
                case TYP_SHORT:
                case TYP_USHORT:
                    ins = INS_pandn;
                    break;
                case TYP_DOUBLE:
                    ins = INS_andnpd;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_Average:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_BYTE:
                    ins = INS_pavgb;
                    break;
                case TYP_USHORT:
                    ins = INS_pavgw;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_CompareEqual:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    // Instruction with immediate byte operand controlling comparison type
                    ins = INS_cmppd;
                    emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, 0, TYP_SIMD16);
                    break;
                case TYP_INT:
                case TYP_UINT:
                    ins = INS_pcmpeqd;
                    break;
                case TYP_BYTE:
                case TYP_UBYTE:
                    ins = INS_pcmpeqb;
                    break;
                case TYP_SHORT:
                case TYP_USHORT:
                    ins = INS_pcmpeqw;
                    break;
                default:
                    unreached();
                    break;
            }

            if (ins != INS_cmppd)
            {
                emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            }
            break;
        }

        case NI_SSE2_CompareGreaterThan:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    // Instruction with immediate byte operand controlling comparison type
                    ins = INS_cmppd;
                    emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, 5, TYP_SIMD16);
                    break;
                case TYP_INT:
                    ins = INS_pcmpgtd;
                    break;
                case TYP_BYTE:
                    ins = INS_pcmpgtb;
                    break;
                case TYP_SHORT:
                    ins = INS_pcmpgtw;
                    break;
                default:
                    unreached();
                    break;
            }

            if (ins != INS_cmppd)
            {
                emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            }
            break;
        }

        case NI_SSE2_CompareGreaterThanOrEqual:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_cmppd;
                    break;
                default:
                    unreached();
                    break;
            }

            // Instruction with immediate byte operand controlling comparison type
            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, 6, TYP_SIMD16);
            break;
        }

        case NI_SSE2_CompareLessThan:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    // Instruction with immediate byte operand controlling comparison type
                    ins = INS_cmppd;
                    emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op2Reg, op1Reg, 5, TYP_SIMD16);
                    break;
                case TYP_INT:
                    ins = INS_pcmpgtd;
                    break;
                case TYP_BYTE:
                    ins = INS_pcmpgtb;
                    break;
                case TYP_SHORT:
                    ins = INS_pcmpgtw;
                    break;
                default:
                    unreached();
                    break;
            }

            if (ins != INS_cmppd)
            {
                emit->emitIns_SIMD_R_R_R(ins, targetReg, op2Reg, op1Reg, TYP_SIMD16);
            }
            break;
        }

        case NI_SSE2_CompareLessThanOrEqual:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_cmppd;
                    break;
                default:
                    unreached();
                    break;
            }

            // Instruction with immediate byte operand controlling comparison type
            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, 2, TYP_SIMD16);
            break;
        }

        case NI_SSE2_CompareNotEqual:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_cmppd;
                    break;
                default:
                    unreached();
                    break;
            }

            // Instruction with immediate byte operand controlling comparison type
            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, 4, TYP_SIMD16);
            break;
        }

        case NI_SSE2_CompareNotGreaterThan:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_cmppd;
                    break;
                default:
                    unreached();
                    break;
            }

            // Instruction with immediate byte operand controlling comparison type
            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, 2, TYP_SIMD16);
            break;
        }

        // This case is according to description identical to the above one
        case NI_SSE2_CompareNotGreaterThanOrEqual:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_cmppd;
                    break;
                default:
                    unreached();
                    break;
            }

            // Instruction with immediate byte operand controlling comparison type
            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, 2, TYP_SIMD16);
            break;
        }

        case NI_SSE2_CompareNotLessThan:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_cmppd;
                    break;
                default:
                    unreached();
                    break;
            }

            // Instruction with immediate byte operand controlling comparison type
            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, 5, TYP_SIMD16);
            break;
        }

        case NI_SSE2_CompareNotLessThanOrEqual:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_cmppd;
                    break;
                default:
                    unreached();
                    break;
            }

            // Instruction with immediate byte operand controlling comparison type
            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, 6, TYP_SIMD16);
            break;
        }

        case NI_SSE2_CompareOrdered:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_cmppd;
                    break;
                default:
                    unreached();
                    break;
            }

            // Instruction with immediate byte operand controlling comparison type
            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, 7, TYP_SIMD16);
            break;
        }

        case NI_SSE2_CompareUnordered:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_cmppd;
                    break;
                default:
                    unreached();
                    break;
            }

            // Instruction with immediate byte operand controlling comparison type
            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, 3, TYP_SIMD16);
            break;
        }

        case NI_SSE2_ConvertToVector128Int32:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_FLOAT:
                    ins = INS_cvtps2dq;
                    break;
                case TYP_DOUBLE:
                    ins = INS_cvtpd2dq;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_ConvertToVector128Double:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_FLOAT:
                    ins = INS_cvtps2pd;
                    break;
                case TYP_INT:
                    ins = INS_cvtdq2pd;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_ConvertToVector128Single:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_cvtpd2ps;
                    break;
                case TYP_INT:
                    ins = INS_cvtdq2ps;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_ConvertToVector128Int32WithTruncation:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_FLOAT:
                    ins = INS_cvttps2dq;
                    break;
                case TYP_DOUBLE:
                    ins = INS_cvttpd2dq;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_Divide:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_divpd;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_ExtractInt16:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_SHORT:
                    ins = INS_pextrw;
                    break;
                default:
                    unreached();
                    break;
            }
            // This should be emitIns_SIMD_R_R_I where target is r32/r64, source xmm, imm8
            // Upper bits of r32/r64 targets are zeroed
            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, imm8, TYP_SIMD16);
            break;
        }

        case NI_SSE2_ExtractUInt16:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_USHORT:
                    ins = INS_pextrw;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, imm8, TYP_SIMD16);
            break;
        }

        case NI_SSE2_InsertInt16:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_SHORT:
                    ins = INS_pinsrw;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, imm8, TYP_SIMD16);
            break;
        }

        case NI_SSE2_InsertUInt16:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_USHORT:
                    ins = INS_pinsrw;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, imm8, TYP_SIMD16);
            break;
        }

        case NI_SSE2_Max:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_UBYTE:
                    ins = INS_pmaxub;
                    break;
                case TYP_SHORT:
                    ins = INS_pmaxsw;
                    break;
                case TYP_DOUBLE:
                    ins = INS_maxpd;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_Min:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_UBYTE:
                    ins = INS_pminub;
                    break;
                case TYP_SHORT:
                    ins = INS_pminsw;
                    break;
                case TYP_DOUBLE:
                    ins = INS_minpd;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_Multiply:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_UINT:
                    ins = INS_pmuludq;
                    break;
                case TYP_DOUBLE:
                    ins = INS_mulpd;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_MultiplyHigh:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_SHORT:
                    ins = INS_pmulhw;
                    break;
                case TYP_USHORT:
                    ins = INS_pmulhuw;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_MultiplyLow:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_SHORT:
                    ins = INS_pmullw;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_MultiplyHorizontalAdd:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_SHORT:
                    ins = INS_pmaddwd;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_Or:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_orpd;
                    break;
                case TYP_INT:
                case TYP_UINT:
                case TYP_LONG:
                case TYP_ULONG:
                case TYP_BYTE:
                case TYP_UBYTE:
                case TYP_SHORT:
                case TYP_USHORT:
                    ins = INS_por;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_PackSignedSaturate:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_SHORT:
                    ins = INS_packsswb;
                    break;
                case TYP_INT:
                    ins = INS_packssdw;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_PackUnsignedSaturate:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_USHORT:
                    ins = INS_packuswb;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_SumAbsoluteDifferences:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_UBYTE:
                    ins = INS_psadbw;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_Shuffle:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_INT:
                case TYP_UINT:
                    ins = INS_pshufd;
                    break;
                case TYP_DOUBLE:
                    ins = INS_shufpd;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, imm8, TYP_SIMD16);
            break;
        }

        case NI_SSE2_ShuffleHigh:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_INT:
                case TYP_UINT:
                    ins = INS_pshufhw;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, imm8, TYP_SIMD16);
            break;
        }

        case NI_SSE2_ShuffleLow:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_INT:
                case TYP_UINT:
                    ins = INS_pshuflw;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, imm8, TYP_SIMD16);
            break;
        }

        case NI_SSE2_ShiftLeftLogical:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_SHORT:
                case TYP_USHORT:
                    ins = INS_psllw;
                    break;
                case TYP_INT:
                case TYP_UINT:
                    ins = INS_pslld;
                    break;
                case TYP_LONG:
                case TYP_ULONG:
                    ins = INS_psllq;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, imm8, TYP_SIMD16);
            break;
        }

        case NI_SSE2_ShiftLeftLogical128BitLane:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
            case TYP_SHORT:
            case TYP_USHORT:
            case TYP_INT:
            case TYP_UINT:
            case TYP_LONG:
            case TYP_ULONG:
                ins = INS_pslldq;
                break;
            default:
                unreached();
                break;
            }

            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, imm8, TYP_SIMD16);
            break;
        }

        case NI_SSE2_ShiftRightArithmetic:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
            case TYP_SHORT:
                ins = INS_psraw;
                break;
            case TYP_INT:
                ins = INS_psrad;
                break;
            default:
                unreached();
                break;
            }

            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, imm8, TYP_SIMD16);
            break;
        }

        case NI_SSE2_ShiftRightLogical:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
            case TYP_SHORT:
            case TYP_USHORT:
                ins = INS_psrlw;
                break;
            case TYP_INT:
            case TYP_UINT:
                ins = INS_psrld;
                break;
            case TYP_LONG:
            case TYP_ULONG:
                ins = INS_psrlq;
                break;
            default:
                unreached();
                break;
            }

            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, imm8, TYP_SIMD16);
            break;
        }

        case NI_SSE2_ShiftRightLogical128BitLane:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
            case TYP_SHORT:
            case TYP_USHORT:
            case TYP_INT:
            case TYP_UINT:
            case TYP_LONG:
            case TYP_ULONG:
                ins = INS_psrldq;
                break;
            default:
                unreached();
                break;
            }

            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, imm8, TYP_SIMD16);
            break;
        }

        case NI_SSE2_Sqrt:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_sqrtpd;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_Subtract:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_subpd;
                    break;
                case TYP_INT:
                case TYP_UINT:
                    ins = INS_psubd;
                    break;
                case TYP_LONG:
                case TYP_ULONG:
                    ins = INS_psubq;
                    break;
                case TYP_BYTE:
                case TYP_UBYTE:
                    ins = INS_psubb;
                    break;
                case TYP_SHORT:
                case TYP_USHORT:
                    ins = INS_psubw;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_SubtractSaturate:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_BYTE:
                    ins = INS_psubsb;
                    break;
                case TYP_UBYTE:
                    ins = INS_psubusb;
                    break;
                case TYP_SHORT:
                    ins = INS_psubsw;
                    break;
                case TYP_USHORT:
                    ins = INS_psubusw;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_UnpackHigh:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_unpckhpd;
                    break;
                case TYP_INT:
                case TYP_UINT:
                    ins = INS_punpckhdq;
                    break;
                case TYP_LONG:
                case TYP_ULONG:
                    ins = INS_punpckhqdq;
                    break;
                case TYP_BYTE:
                case TYP_UBYTE:
                    ins = INS_punpckhbw;
                    break;
                case TYP_SHORT:
                case TYP_USHORT:
                    ins = INS_punpckhwd;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_UnpackLow:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_unpcklpd;
                    break;
                case TYP_INT:
                case TYP_UINT:
                    ins = INS_punpckldq;
                    break;
                case TYP_LONG:
                case TYP_ULONG:
                    ins = INS_punpcklqdq;
                    break;
                case TYP_BYTE:
                case TYP_UBYTE:
                    ins = INS_punpcklbw;
                    break;
                case TYP_SHORT:
                case TYP_USHORT:
                    ins = INS_punpcklwd;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_Xor:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_xorpd;
                    break;
                case TYP_INT:
                case TYP_UINT:
                case TYP_LONG:
                case TYP_ULONG:
                case TYP_BYTE:
                case TYP_UBYTE:
                case TYP_SHORT:
                case TYP_USHORT:
                    ins = INS_pxor;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_AddSaturate:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_BYTE:
                    ins = INS_paddsb;
                    break;
                case TYP_UBYTE:
                    ins = INS_paddusb;
                    break;
                case TYP_SHORT:
                    ins = INS_paddsw;
                    break;
                case TYP_USHORT:
                    ins = INS_paddusw;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_And:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_INT:
                case TYP_UINT:
                case TYP_LONG:
                case TYP_ULONG:
                case TYP_BYTE:
                case TYP_UBYTE:
                case TYP_SHORT:
                case TYP_USHORT:
                    ins = INS_pand;
                    break;
                case TYP_DOUBLE:
                    ins = INS_andpd;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(INS_pand, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_AndNot:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_INT:
                case TYP_UINT:
                case TYP_LONG:
                case TYP_ULONG:
                case TYP_BYTE:
                case TYP_UBYTE:
                case TYP_SHORT:
                case TYP_USHORT:
                    ins = INS_pandn;
                    break;
                case TYP_DOUBLE:
                    ins = INS_andnpd;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_Average:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_BYTE:
                    ins = INS_pavgb;
                    break;
                case TYP_USHORT:
                    ins = INS_pavgw;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_CompareEqual:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    // Instruction with immediate byte operand controlling comparison type
                    ins = INS_cmppd;
                    emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, 0, TYP_SIMD16);
                    break;
                case TYP_INT:
                case TYP_UINT:
                    ins = INS_pcmpeqd;
                    break;
                case TYP_BYTE:
                case TYP_UBYTE:
                    ins = INS_pcmpeqb;
                    break;
                case TYP_SHORT:
                case TYP_USHORT:
                    ins = INS_pcmpeqw;
                    break;
                default:
                    unreached();
                    break;
            }

            if (ins != INS_cmppd)
            {
                emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            }
            break;
        }

        case NI_SSE2_CompareGreaterThan:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    // Instruction with immediate byte operand controlling comparison type
                    ins = INS_cmppd;
                    emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, 5, TYP_SIMD16);
                    break;
                case TYP_INT:
                    ins = INS_pcmpgtd;
                    break;
                case TYP_BYTE:
                    ins = INS_pcmpgtb;
                    break;
                case TYP_SHORT:
                    ins = INS_pcmpgtw;
                    break;
                default:
                    unreached();
                    break;
            }

            if (ins != INS_cmppd)
            {
                emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            }
            break;
        }

        case NI_SSE2_CompareGreaterThanOrEqual:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_cmppd;
                    break;
                default:
                    unreached();
                    break;
            }

            // Instruction with immediate byte operand controlling comparison type
            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, 6, TYP_SIMD16);
            break;
        }

        case NI_SSE2_CompareLessThan:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    // Instruction with immediate byte operand controlling comparison type
                    ins = INS_cmppd;
                    emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op2Reg, op1Reg, 5, TYP_SIMD16);
                    break;
                case TYP_INT:
                    ins = INS_pcmpgtd;
                    break;
                case TYP_BYTE:
                    ins = INS_pcmpgtb;
                    break;
                case TYP_SHORT:
                    ins = INS_pcmpgtw;
                    break;
                default:
                    unreached();
                    break;
            }

            if (ins != INS_cmppd)
            {
                emit->emitIns_SIMD_R_R_R(ins, targetReg, op2Reg, op1Reg, TYP_SIMD16);
            }
            break;
        }

        case NI_SSE2_CompareLessThanOrEqual:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_cmppd;
                    break;
                default:
                    unreached();
                    break;
            }

            // Instruction with immediate byte operand controlling comparison type
            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, 2, TYP_SIMD16);
            break;
        }

        case NI_SSE2_CompareNotEqual:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_cmppd;
                    break;
                default:
                    unreached();
                    break;
            }

            // Instruction with immediate byte operand controlling comparison type
            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, 4, TYP_SIMD16);
            break;
        }

        case NI_SSE2_CompareNotGreaterThan:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_cmppd;
                    break;
                default:
                    unreached();
                    break;
            }

            // Instruction with immediate byte operand controlling comparison type
            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, 2, TYP_SIMD16);
            break;
        }

        // This case is according to description identical to the above one
        case NI_SSE2_CompareNotGreaterThanOrEqual:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_cmppd;
                    break;
                default:
                    unreached();
                    break;
            }

            // Instruction with immediate byte operand controlling comparison type
            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, 2, TYP_SIMD16);
            break;
        }

        case NI_SSE2_CompareNotLessThan:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_cmppd;
                    break;
                default:
                    unreached();
                    break;
            }

            // Instruction with immediate byte operand controlling comparison type
            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, 5, TYP_SIMD16);
            break;
        }

        case NI_SSE2_CompareNotLessThanOrEqual:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_cmppd;
                    break;
                default:
                    unreached();
                    break;
            }

            // Instruction with immediate byte operand controlling comparison type
            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, 6, TYP_SIMD16);
            break;
        }

        case NI_SSE2_CompareOrdered:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_cmppd;
                    break;
                default:
                    unreached();
                    break;
            }

            // Instruction with immediate byte operand controlling comparison type
            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, 7, TYP_SIMD16);
            break;
        }

        case NI_SSE2_CompareUnordered:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_cmppd;
                    break;
                default:
                    unreached();
                    break;
            }

            // Instruction with immediate byte operand controlling comparison type
            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, 3, TYP_SIMD16);
            break;
        }

        case NI_SSE2_ConvertToVector128Int32:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_FLOAT:
                    ins = INS_cvtps2dq;
                    break;
                case TYP_DOUBLE:
                    ins = INS_cvtpd2dq;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_ConvertToVector128Double:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_FLOAT:
                    ins = INS_cvtps2pd;
                    break;
                case TYP_INT:
                    ins = INS_cvtdq2pd;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_ConvertToVector128Single:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_cvtpd2ps;
                    break;
                case TYP_INT:
                    ins = INS_cvtdq2ps;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_ConvertToVector128Int32WithTruncation:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_FLOAT:
                    ins = INS_cvttps2dq;
                    break;
                case TYP_DOUBLE:
                    ins = INS_cvttpd2dq;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_Divide:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_divpd;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_ExtractInt16:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_SHORT:
                    ins = INS_pextrw;
                    break;
                default:
                    unreached();
                    break;
            }
            // This should be emitIns_SIMD_R_R_I where target is r32/r64, source xmm, imm8
            // Upper bits of r32/r64 targets are zeroed
            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, imm8, TYP_SIMD16);
            break;
        }

        case NI_SSE2_ExtractUInt16:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_USHORT:
                    ins = INS_pextrw;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, imm8, TYP_SIMD16);
            break;
        }

        case NI_SSE2_InsertInt16:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_SHORT:
                    ins = INS_pinsrw;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, imm8, TYP_SIMD16);
            break;
        }

        case NI_SSE2_InsertUInt16:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_USHORT:
                    ins = INS_pinsrw;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, imm8, TYP_SIMD16);
            break;
        }

        case NI_SSE2_Max:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_UBYTE:
                    ins = INS_pmaxub;
                    break;
                case TYP_SHORT:
                    ins = INS_pmaxsw;
                    break;
                case TYP_DOUBLE:
                    ins = INS_maxpd;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_Min:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_UBYTE:
                    ins = INS_pminub;
                    break;
                case TYP_SHORT:
                    ins = INS_pminsw;
                    break;
                case TYP_DOUBLE:
                    ins = INS_minpd;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_Multiply:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_UINT:
                    ins = INS_pmuludq;
                    break;
                case TYP_DOUBLE:
                    ins = INS_mulpd;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_MultiplyHigh:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_SHORT:
                    ins = INS_pmulhw;
                    break;
                case TYP_USHORT:
                    ins = INS_pmulhuw;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_MultiplyLow:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_SHORT:
                    ins = INS_pmullw;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_MultiplyHorizontalAdd:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_SHORT:
                    ins = INS_pmaddwd;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_Or:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_orpd;
                    break;
                case TYP_INT:
                case TYP_UINT:
                case TYP_LONG:
                case TYP_ULONG:
                case TYP_BYTE:
                case TYP_UBYTE:
                case TYP_SHORT:
                case TYP_USHORT:
                    ins = INS_por;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_PackSignedSaturate:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_SHORT:
                    ins = INS_packsswb;
                    break;
                case TYP_INT:
                    ins = INS_packssdw;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_PackUnsignedSaturate:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_USHORT:
                    ins = INS_packuswb;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_SumAbsoluteDifferences:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_UBYTE:
                    ins = INS_psadbw;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_Shuffle:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_INT:
                case TYP_UINT:
                    ins = INS_pshufd;
                    break;
                case TYP_DOUBLE:
                    ins = INS_shufpd;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, imm8, TYP_SIMD16);
            break;
        }

        case NI_SSE2_ShuffleHigh:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_INT:
                case TYP_UINT:
                    ins = INS_pshufhw;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, imm8, TYP_SIMD16);
            break;
        }

        case NI_SSE2_ShuffleLow:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_INT:
                case TYP_UINT:
                    ins = INS_pshuflw;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, imm8, TYP_SIMD16);
            break;
        }

        case NI_SSE2_ShiftLeftLogical:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_SHORT:
                case TYP_USHORT:
                    ins = INS_psllw;
                    break;
                case TYP_INT:
                case TYP_UINT:
                    ins = INS_pslld;
                    break;
                case TYP_LONG:
                case TYP_ULONG:
                    ins = INS_psllq;
                    break;
                default:
                    unreached();
                    break;
            }

            if (imm8 >= 0)
            {
                emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, imm8, TYP_SIMD16);
            }
            else
            {
                emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            }
            break;
        }

        case NI_SSE2_ShiftLeftLogical128BitLane:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_SHORT:
                case TYP_USHORT:
                case TYP_INT:
                case TYP_UINT:
                case TYP_LONG:
                case TYP_ULONG:
                    ins = INS_pslldq;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, imm8, TYP_SIMD16);
            break;
        }

        case NI_SSE2_ShiftRightArithmetic:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_SHORT:
                    ins = INS_psraw;
                    break;
                case TYP_INT:
                    ins = INS_psrad;
                    break;
                default:
                    unreached();
                    break;
            }

            if (imm8 >= 0)
            {
                emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, imm8, TYP_SIMD16);
            }
            else
            {
                emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            }
            break;
        }

        case NI_SSE2_ShiftRightLogical:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_SHORT:
                case TYP_USHORT:
                    ins = INS_psrlw;
                    break;
                case TYP_INT:
                case TYP_UINT:
                    ins = INS_psrld;
                    break;
                case TYP_LONG:
                case TYP_ULONG:
                    ins = INS_psrlq;
                    break;
                default:
                    unreached();
                    break;
            }

            if (imm8 >= 0)
            {
                emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, imm8, TYP_SIMD16);
            }
            else
            {
                emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            }
            break;
        }

        case NI_SSE2_ShiftRightLogical128BitLane:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_SHORT:
                case TYP_USHORT:
                case TYP_INT:
                case TYP_UINT:
                case TYP_LONG:
                case TYP_ULONG:
                    ins = INS_psrldq;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R_I(ins, targetReg, op1Reg, op2Reg, imm8, TYP_SIMD16);
            break;
        }

        case NI_SSE2_Sqrt:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_sqrtpd;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_Subtract:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_subpd;
                    break;
                case TYP_INT:
                case TYP_UINT:
                    ins = INS_psubd;
                    break;
                case TYP_LONG:
                case TYP_ULONG:
                    ins = INS_psubq;
                    break;
                case TYP_BYTE:
                case TYP_UBYTE:
                    ins = INS_psubb;
                    break;
                case TYP_SHORT:
                case TYP_USHORT:
                    ins = INS_psubw;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_SubtractSaturate:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_BYTE:
                    ins = INS_psubsb;
                    break;
                case TYP_UBYTE:
                    ins = INS_psubusb;
                    break;
                case TYP_SHORT:
                    ins = INS_psubsw;
                    break;
                case TYP_USHORT:
                    ins = INS_psubusw;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_UnpackHigh:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_unpckhpd;
                    break;
                case TYP_INT:
                case TYP_UINT:
                    ins = INS_punpckhdq;
                    break;
                case TYP_LONG:
                case TYP_ULONG:
                    ins = INS_punpckhqdq;
                    break;
                case TYP_BYTE:
                case TYP_UBYTE:
                    ins = INS_punpckhbw;
                    break;
                case TYP_SHORT:
                case TYP_USHORT:
                    ins = INS_punpckhwd;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_UnpackLow:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_unpcklpd;
                    break;
                case TYP_INT:
                case TYP_UINT:
                    ins = INS_punpckldq;
                    break;
                case TYP_LONG:
                case TYP_ULONG:
                    ins = INS_punpcklqdq;
                    break;
                case TYP_BYTE:
                case TYP_UBYTE:
                    ins = INS_punpcklbw;
                    break;
                case TYP_SHORT:
                case TYP_USHORT:
                    ins = INS_punpcklwd;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        case NI_SSE2_Xor:
        {
            op2Reg = op2->gtRegNum;

            instruction ins;
            switch (baseType)
            {
                case TYP_DOUBLE:
                    ins = INS_xorpd;
                    break;
                case TYP_INT:
                case TYP_UINT:
                case TYP_LONG:
                case TYP_ULONG:
                case TYP_BYTE:
                case TYP_UBYTE:
                case TYP_SHORT:
                case TYP_USHORT:
                    ins = INS_pxor;
                    break;
                default:
                    unreached();
                    break;
            }

            emit->emitIns_SIMD_R_R_R(ins, targetReg, op1Reg, op2Reg, TYP_SIMD16);
            break;
        }

        default:
            unreached();
            break;
    }

    genProduceReg(node);
}

void CodeGen::genSSE3Intrinsic(GenTreeHWIntrinsic* node)
{
    NYI("Implement SSE3 intrinsic code generation");
}

void CodeGen::genSSSE3Intrinsic(GenTreeHWIntrinsic* node)
{
    NYI("Implement SSSE3 intrinsic code generation");
}

void CodeGen::genSSE41Intrinsic(GenTreeHWIntrinsic* node)
{
    NYI("Implement SSE41 intrinsic code generation");
}

void CodeGen::genSSE42Intrinsic(GenTreeHWIntrinsic* node)
{
    NamedIntrinsic intrinsicID = node->gtHWIntrinsicId;
    GenTree*       op1         = node->gtGetOp1();
    GenTree*       op2         = node->gtGetOp2();
    regNumber      targetReg   = node->gtRegNum;
    assert(targetReg != REG_NA);
    var_types targetType = node->TypeGet();
    var_types baseType   = node->gtSIMDBaseType;

    regNumber op1Reg = op1->gtRegNum;
    regNumber op2Reg = op2->gtRegNum;
    genConsumeOperands(node);

    switch (intrinsicID)
    {
        case NI_SSE42_Crc32:
            if (op1Reg != targetReg)
            {
                inst_RV_RV(INS_mov, targetReg, op1Reg, targetType, emitTypeSize(targetType));
            }

            if (baseType == TYP_UBYTE || baseType == TYP_USHORT) // baseType is the type of the second argument
            {
                assert(targetType == TYP_INT);
                inst_RV_RV(INS_crc32, targetReg, op2Reg, baseType, emitTypeSize(baseType));
            }
            else
            {
                assert(op1->TypeGet() == op2->TypeGet());
                assert(targetType == TYP_INT || targetType == TYP_LONG);
                inst_RV_RV(INS_crc32, targetReg, op2Reg, targetType, emitTypeSize(targetType));
            }

            break;
        default:
            unreached();
            break;
    }
    genProduceReg(node);
}

void CodeGen::genAVXIntrinsic(GenTreeHWIntrinsic* node)
{
    NamedIntrinsic intrinsicID = node->gtHWIntrinsicId;
    var_types      baseType    = node->gtSIMDBaseType;
    instruction    ins         = INS_invalid;

    genConsumeOperands(node);

    switch (intrinsicID)
    {
        default:
            unreached();
            break;
    }

    genProduceReg(node);
}

void CodeGen::genAVX2Intrinsic(GenTreeHWIntrinsic* node)
{
    NamedIntrinsic intrinsicID = node->gtHWIntrinsicId;
    var_types      baseType    = node->gtSIMDBaseType;
    instruction    ins         = INS_invalid;

    genConsumeOperands(node);

    switch (intrinsicID)
    {
        default:
            unreached();
            break;
    }

    genProduceReg(node);
}

void CodeGen::genAESIntrinsic(GenTreeHWIntrinsic* node)
{
    NYI("Implement AES intrinsic code generation");
}

void CodeGen::genBMI1Intrinsic(GenTreeHWIntrinsic* node)
{
    NYI("Implement BMI1 intrinsic code generation");
}

void CodeGen::genBMI2Intrinsic(GenTreeHWIntrinsic* node)
{
    NYI("Implement BMI2 intrinsic code generation");
}

void CodeGen::genFMAIntrinsic(GenTreeHWIntrinsic* node)
{
    NYI("Implement FMA intrinsic code generation");
}

void CodeGen::genLZCNTIntrinsic(GenTreeHWIntrinsic* node)
{
    NamedIntrinsic intrinsicID = node->gtHWIntrinsicId;
    GenTree*       op1         = node->gtGetOp1();
    regNumber      targetReg   = node->gtRegNum;
    assert(targetReg != REG_NA);
    var_types targetType = node->TypeGet();
    regNumber op1Reg     = op1->gtRegNum;
    genConsumeOperands(node);

    assert(intrinsicID == NI_LZCNT_LeadingZeroCount);

    inst_RV_RV(INS_lzcnt, targetReg, op1Reg, targetType, emitTypeSize(targetType));

    genProduceReg(node);
}

void CodeGen::genPCLMULQDQIntrinsic(GenTreeHWIntrinsic* node)
{
    NYI("Implement PCLMULQDQ intrinsic code generation");
}

void CodeGen::genPOPCNTIntrinsic(GenTreeHWIntrinsic* node)
{
    NamedIntrinsic intrinsicID = node->gtHWIntrinsicId;
    GenTree*       op1         = node->gtGetOp1();
    regNumber      targetReg   = node->gtRegNum;
    assert(targetReg != REG_NA);
    var_types targetType = node->TypeGet();
    regNumber op1Reg     = op1->gtRegNum;
    genConsumeOperands(node);

    assert(intrinsicID == NI_POPCNT_PopCount);

    inst_RV_RV(INS_popcnt, targetReg, op1Reg, targetType, emitTypeSize(targetType));

    genProduceReg(node);
}

#endif // FEATURE_HW_INTRINSICS
