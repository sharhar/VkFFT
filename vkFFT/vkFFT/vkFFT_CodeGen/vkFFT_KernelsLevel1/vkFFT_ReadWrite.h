// This file is part of VkFFT
//
// Copyright (C) 2021 - present Dmitrii Tolmachev <dtolm96@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, &including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, &iNCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
#ifndef VKFFT_READWRITE_H
#define VKFFT_READWRITE_H

#include "vkFFT/vkFFT_Structs/vkFFT_Structs.h"
#include "vkFFT/vkFFT_CodeGen/vkFFT_StringManagement/vkFFT_StringManager.h"
#include "vkFFT/vkFFT_CodeGen/vkFFT_MathUtils/vkFFT_MathUtils.h"
#include "vkFFT/vkFFT_CodeGen/vkFFT_KernelsLevel0/vkFFT_KernelUtils.h"
#include "vkFFT/vkFFT_CodeGen/vkFFT_KernelsLevel0/vkFFT_Zeropad.h"
#include "vkFFT/vkFFT_CodeGen/vkFFT_KernelsLevel0/vkFFT_MemoryManagement/vkFFT_MemoryTransfers/vkFFT_Transfers.h"
#include "vkFFT/vkFFT_CodeGen/vkFFT_KernelsLevel1/PrePostProcessing/vkFFT_R2R.h"
#include "vkFFT/vkFFT_CodeGen/vkFFT_KernelsLevel1/PrePostProcessing/vkFFT_R2C.h"
static inline void setReadToRegisters(VkFFTSpecializationConstantsLayout* sc, int readType) {
	if (sc->res != VKFFT_SUCCESS) return;
	switch (readType % 10) {
	case 0: //single_c2c
	{
		if ((sc->localSize[1].data.i > 1) || ((sc->performR2C) && (sc->actualInverse) && (sc->numAxisUploads == 1) && (!sc->forceCallbackVersionRealTransforms)) || ((sc->fftDim.data.i>1)&&((sc->localSize[0].data.i * sc->stageRadix[0] * (sc->registers_per_thread_per_radix[sc->stageRadix[0]] / sc->stageRadix[0]) > sc->fftDim.data.i) || (sc->rader_generator[0] > 0))))
			sc->readToRegisters = 0;
		else
			sc->readToRegisters = 1;
		break;
	}
	case 1: //grouped_c2c
	{
		if ((sc->fftDim.data.i>1)&&(((sc->localSize[1].data.i * sc->stageRadix[0] * (sc->registers_per_thread_per_radix[sc->stageRadix[0]] / sc->stageRadix[0]) > sc->fftDim.data.i) || (sc->rader_generator[0] > 0))))
			sc->readToRegisters = 0;
		else
			sc->readToRegisters = 1;
		break;
	}
	case 2: //single_c2c_strided
	{
		if ((sc->fftDim.data.i>1)&&(((sc->localSize[1].data.i * sc->stageRadix[0] * (sc->registers_per_thread_per_radix[sc->stageRadix[0]] / sc->stageRadix[0]) > sc->fftDim.data.i) || (sc->rader_generator[0] > 0))))
			sc->readToRegisters = 0;
		else
			sc->readToRegisters = 1;
		break;
	}
	}
	if (sc->axis_id == 0) {
		switch (readType / 10) {
		case 50://single_r2c
		{
			if ((sc->stridedSharedLayout) || (sc->localSize[1].data.i > 1) || (sc->fftDim.data.i == 1) || ((sc->fftDim.data.i > 1) && ((sc->localSize[0].data.i * sc->stageRadix[0] * (sc->registers_per_thread_per_radix[sc->stageRadix[0]] / sc->stageRadix[0]) > sc->fftDim.data.i) || (sc->rader_generator[0] > 0))))
				sc->readToRegisters = 0;
			else
				sc->readToRegisters = 1;
			break;
		}
		case 60: //single_c2r
		{
			sc->readToRegisters = 0;
			/*if ((sc->rader_generator[0] > 0) || ((sc->fftDim.data.i % sc->localSize[0].data.i) && (!sc->stridedSharedLayout)) || ((sc->fftDim.data.i % sc->localSize[1].data.i) && (sc->stridedSharedLayout)))
				sc->readToRegisters = 0;
			else
				sc->readToRegisters = 1;*/
			break;
		}
		}
	}
	if (sc->numAxisUploads == 1) {
		switch (readType / 10) {
		case 110: case 130: case 142:
		{
			sc->readToRegisters = 0;
			break;
		}
		case 140:
		{
			if (sc->performDST == 4)
				sc->readToRegisters = 1;
			else {
#if(((VKFFT_BACKEND==3)||(VKFFT_BACKEND==4)||(VKFFT_BACKEND==5)))
				sc->readToRegisters = 1;
#else
				sc->readToRegisters = 0;
#endif
			}
			break;
		}
		case 120:
		{
			sc->readToRegisters = 1;
			break;
		}
		}
	}
	return;
}
static inline void setWriteFromRegisters(VkFFTSpecializationConstantsLayout* sc, int writeType) {
	if (sc->res != VKFFT_SUCCESS) return;
	switch (writeType % 10) {
	case 0: //single_c2c
	{
		if ((sc->localSize[1].data.i > 1) || ((sc->fftDim.data.i>1)&&((sc->localSize[0].data.i * sc->stageRadix[sc->numStages - 1] * (sc->registers_per_thread_per_radix[sc->stageRadix[sc->numStages - 1]] / sc->stageRadix[sc->numStages - 1]) > sc->fftDim.data.i) || (sc->rader_generator[sc->numStages - 1] > 0)))) {
			sc->writeFromRegisters = 0;
		}
		else
			sc->writeFromRegisters = 1;
		break;
	}
	case 1: //grouped_c2c
	{
		if ((sc->fftDim.data.i>1)&&(((sc->localSize[1].data.i * sc->stageRadix[sc->numStages - 1] * (sc->registers_per_thread_per_radix[sc->stageRadix[sc->numStages - 1]] / sc->stageRadix[sc->numStages - 1]) > sc->fftDim.data.i) || (sc->rader_generator[sc->numStages - 1] > 0)))) {
			sc->writeFromRegisters = 0;
		}
		else
			sc->writeFromRegisters = 1;
		break;
	}
	case 2: //single_c2c_strided
	{
		if ((sc->fftDim.data.i>1)&&(((sc->localSize[1].data.i * sc->stageRadix[sc->numStages - 1] * (sc->registers_per_thread_per_radix[sc->stageRadix[sc->numStages - 1]] / sc->stageRadix[sc->numStages - 1]) > sc->fftDim.data.i) || (sc->rader_generator[sc->numStages - 1] > 0)))) {
			sc->writeFromRegisters = 0;
		}
		else
			sc->writeFromRegisters = 1;
		break;
	}
	}
	if (sc->axis_id == 0) {
		switch (writeType / 10) {
		case 50://single_r2c
		{
			sc->writeFromRegisters = 0;
			break;
		}
		case 60: //single_c2r
		{
			if ((sc->stridedSharedLayout) || (sc->localSize[1].data.i > 1) || (sc->fftDim.data.i == 1) || ((sc->fftDim.data.i > 1) && ((sc->localSize[0].data.i * sc->stageRadix[sc->numStages - 1] * (sc->registers_per_thread_per_radix[sc->stageRadix[sc->numStages - 1]] / sc->stageRadix[sc->numStages - 1]) > sc->fftDim.data.i) || (sc->rader_generator[sc->numStages - 1] > 0)))) {
				sc->writeFromRegisters = 0;
			}
			else
				sc->writeFromRegisters = 1;
			break;
		}
		}
	}
	if (sc->numAxisUploads == 1) {
		switch (writeType / 10) {
		case 110: case 120: case 130: case 140: case 142:
		{
			sc->writeFromRegisters = 0;
			break;
		}
		}
	}
	return;
}
static inline void appendOffset(VkFFTSpecializationConstantsLayout* sc, int readWrite, int type) {
	if (sc->res != VKFFT_SUCCESS) return;
	PfContainer temp_int = VKFFT_ZERO_INIT;
	temp_int.type = 31;

	PfContainer* bufferStride = (readWrite) ? sc->outputStride : sc->inputStride;

	if (sc->numFFTdims > 2) {
		if (sc->numCoordinates * sc->matrixConvolution * sc->numBatches.data.i > 1) {
			if (sc->performWorkGroupShift[2]) {
				PfMul(sc, &sc->tempInt, &sc->workGroupShiftZ, &sc->gl_WorkGroupSize_z, 0);
				PfAdd(sc, &sc->tempInt, &sc->tempInt, &sc->gl_GlobalInvocationID_z);
				PfMod(sc, &sc->tempInt, &sc->tempInt, &sc->dispatchZactualFFTSize);
			}
			else {
				PfMod(sc, &sc->tempInt, &sc->gl_GlobalInvocationID_z, &sc->dispatchZactualFFTSize);
			}
		}
		else {
			if (sc->performWorkGroupShift[2]) {
				PfMul(sc, &sc->tempInt, &sc->workGroupShiftZ, &sc->gl_WorkGroupSize_z, 0);
				PfAdd(sc, &sc->tempInt, &sc->tempInt, &sc->gl_GlobalInvocationID_z);
			}
			else {
                PfMov(sc, &sc->tempInt, &sc->gl_GlobalInvocationID_z);
			}
		}
        
        /*if (sc->axis_id == 2)
            checkZeropad(sc, &sc->tempInt, 1);
        else
            checkZeropad(sc, &sc->tempInt, 2);
        PfMul(sc, &sc->tempInt, &sc->tempInt, &bufferStride[2], 0);
        PfAdd(sc, &sc->shiftZ, &sc->shiftZ, &sc->tempInt);*/
        int locStrideOrder = 2;
        for (int i = 1; i < sc->numFFTdims; i++){
            if (((i != sc->axis_id)&&(sc->axis_id > 0)) || ((i>1) && (sc->axis_id == 0))) {
                PfMod(sc, &sc->inoutID_y, &sc->tempInt, &sc->size[i]);
                checkZeropad_otherAxes(sc, &sc->inoutID_y, i);
                PfMul(sc, &sc->inoutID_y, &sc->inoutID_y, &bufferStride[locStrideOrder], 0);
                PfAdd(sc, &sc->shiftZ, &sc->shiftZ, &sc->inoutID_y);
				if ((i!=(sc->numFFTdims-1) && (sc->axis_id != (sc->numFFTdims-1))) || ((i!=(sc->numFFTdims-2)) && (sc->axis_id == (sc->numFFTdims-1))))
					PfDiv(sc, &sc->tempInt, &sc->tempInt, &sc->size[i]);
                locStrideOrder++;
            }
        }
	}
	pfINT maxCoordinate = sc->numCoordinates * sc->matrixConvolution;
	if (sc->numCoordinates * sc->matrixConvolution > 1) {
		PfDiv(sc, &sc->tempInt, &sc->gl_GlobalInvocationID_z, &sc->dispatchZactualFFTSize);
		temp_int.data.i = maxCoordinate;
		PfMod(sc, &sc->tempInt, &sc->tempInt, &temp_int);
		PfMul(sc, &sc->tempInt, &sc->tempInt, &bufferStride[sc->numFFTdims], 0);
		PfAdd(sc, &sc->shiftZ, &sc->shiftZ, &sc->tempInt);
	}
	if ((sc->matrixConvolution > 1) && (sc->convolutionStep)) {
		maxCoordinate = 1;
		PfMul(sc, &temp_int, &sc->coordinate, &bufferStride[sc->numFFTdims], 0);
		PfAdd(sc, &sc->shiftZ, &sc->shiftZ, &temp_int);
	}
	if ((sc->numBatches.data.i > 1) || (sc->numKernels.data.i > 1)) {
		if (sc->convolutionStep && (sc->numKernels.data.i > 1)) {
			PfMul(sc, &sc->tempInt, &sc->batchID, &bufferStride[sc->numFFTdims+1], 0);
			PfAdd(sc, &sc->shiftZ, &sc->shiftZ, &sc->tempInt);
		}
		else {
			temp_int.data.i = sc->dispatchZactualFFTSize.data.i * maxCoordinate;
			PfDiv(sc, &sc->tempInt, &sc->gl_GlobalInvocationID_z, &temp_int);
			PfMul(sc, &sc->tempInt, &sc->tempInt, &bufferStride[sc->numFFTdims+1], 0);
			PfAdd(sc, &sc->shiftZ, &sc->shiftZ, &sc->tempInt);

		}
	}
	if (readWrite) {
		if (sc->outputOffset.type < 100) {
			temp_int.data.i = sc->outputOffset.data.i / sc->outputNumberByteSize;
			PfAdd(sc, &sc->shiftZ, &sc->shiftZ, &temp_int);
		}
		else {
			if (sc->outputOffset.type == 101) {
				if (sc->performPostCompilationOutputOffset) {
					PfAdd(sc, &sc->shiftZ, &sc->shiftZ, &sc->outputOffset);
				}
			}
		}
	}
	else {
		if (sc->inputOffset.type < 100) {
			temp_int.data.i = sc->inputOffset.data.i / sc->inputNumberByteSize;
			PfAdd(sc, &sc->shiftZ, &sc->shiftZ, &temp_int);
		}
		else {
			if (sc->inputOffset.type == 101) {
				if (sc->performPostCompilationInputOffset) {
					PfAdd(sc, &sc->shiftZ, &sc->shiftZ, &sc->inputOffset);
				}
			}
		}
	}
	return;
}

static inline void appendKernelOffset(VkFFTSpecializationConstantsLayout* sc, int readWrite, int type) {
	if (sc->res != VKFFT_SUCCESS) return;
	PfContainer temp_int = VKFFT_ZERO_INIT;
	temp_int.type = 31;

	PfContainer* bufferStride = sc->inputStride;
	PfContainer batching_localSize = VKFFT_ZERO_INIT;
	batching_localSize.type = 31;

	if (sc->stridedSharedLayout) {
		batching_localSize.data.i = sc->localSize[0].data.i;
	}
	else {
		batching_localSize.data.i = sc->localSize[1].data.i;
	}

	if (type == 1) {
		if (sc->axis_id == 0) {
			if (sc->size[1].data.i > 1) {
				if (sc->performWorkGroupShift[1]) {
					PfAdd(sc, &sc->blockInvocationID, &sc->gl_WorkGroupID_y, &sc->workGroupShiftY);
					temp_int.data.i = sc->inputStride[1].data.i;
					PfMul(sc, &sc->blockInvocationID, &sc->blockInvocationID, &temp_int, 0);
				}
				else
				{
					PfMov(sc, &sc->blockInvocationID, &sc->gl_WorkGroupID_y);
					temp_int.data.i = sc->inputStride[1].data.i;
					PfMul(sc, &sc->blockInvocationID, &sc->blockInvocationID, &temp_int, 0);
				}
			}
		}
		else {
			PfSetToZero(sc, &sc->blockInvocationID);
		}
	}
	else {
		if (sc->size[1].data.i > 1) {
			if (sc->numAxisUploads != 1) {
				if (sc->performWorkGroupShift[1]) {
					PfAdd(sc, &sc->blockInvocationID, &sc->gl_WorkGroupID_y, &sc->workGroupShiftY);
					temp_int.data.i = sc->inputStride[1].data.i;
					PfMul(sc, &sc->blockInvocationID, &sc->blockInvocationID, &temp_int, 0);
				}
				else
				{
					PfMov(sc, &sc->blockInvocationID, &sc->gl_WorkGroupID_y);
					temp_int.data.i = sc->inputStride[1].data.i;
					PfMul(sc, &sc->blockInvocationID, &sc->blockInvocationID, &temp_int, 0);
				}
			}
		}
		else {
			PfSetToZero(sc, &sc->blockInvocationID);
		}
	}
    if (sc->numFFTdims > 2) {
		if (sc->numCoordinates * sc->matrixConvolution * sc->numBatches.data.i > 1) {
			if (sc->performWorkGroupShift[2]) {
				PfMul(sc, &sc->tempInt, &sc->workGroupShiftZ, &sc->gl_WorkGroupSize_z, 0);
				PfAdd(sc, &sc->tempInt, &sc->tempInt, &sc->gl_GlobalInvocationID_z);
				PfMod(sc, &sc->tempInt, &sc->tempInt, &sc->dispatchZactualFFTSize);
                //PfMul(sc, &sc->tempInt, &sc->tempInt, &bufferStride[2], 0);
                //PfAdd(sc, &sc->blockInvocationID, &sc->blockInvocationID, &sc->tempInt);
			}
			else {
				PfMod(sc, &sc->tempInt, &sc->gl_GlobalInvocationID_z, &sc->dispatchZactualFFTSize);
				//PfMul(sc, &sc->tempInt, &sc->tempInt, &bufferStride[2], 0);
				//PfAdd(sc, &sc->blockInvocationID, &sc->blockInvocationID, &sc->tempInt);
			}
           
		}
		else {
			if (sc->performWorkGroupShift[2]) {
				PfMul(sc, &sc->tempInt, &sc->workGroupShiftZ, &sc->gl_WorkGroupSize_z, 0);
				PfAdd(sc, &sc->tempInt, &sc->tempInt, &sc->gl_GlobalInvocationID_z);
				//PfMul(sc, &sc->tempInt, &sc->tempInt, &bufferStride[2], 0);
				//PfAdd(sc, &sc->blockInvocationID, &sc->blockInvocationID, &sc->tempInt);
			}
			else {
                PfMov(sc, &sc->tempInt, &sc->gl_GlobalInvocationID_z);
				//PfMul(sc, &sc->tempInt, &sc->gl_GlobalInvocationID_z, &bufferStride[2], 0);
				//PfAdd(sc, &sc->blockInvocationID, &sc->blockInvocationID, &sc->tempInt);
			}
		}
        int locStrideOrder = 2;
        for (int i = 1; i < sc->numFFTdims; i++){
            if (((i != sc->axis_id)&&(sc->axis_id > 0)) || ((i>1) && (sc->axis_id == 0))) {
                PfMod(sc, &sc->inoutID_y, &sc->tempInt, &sc->size[i]);
                checkZeropad_otherAxes(sc, &sc->inoutID_y, i);
                PfMul(sc, &sc->inoutID_y, &sc->inoutID_y, &bufferStride[locStrideOrder], 0);
                PfAdd(sc, &sc->shiftZ, &sc->shiftZ, &sc->inoutID_y);
				if ((i!=(sc->numFFTdims-1) && (sc->axis_id != (sc->numFFTdims-1))) || ((i!=(sc->numFFTdims-2)) && (sc->axis_id == (sc->numFFTdims-1))))
					PfDiv(sc, &sc->tempInt, &sc->tempInt, &sc->size[i]);
                locStrideOrder++;
            }
        }
	}
	pfINT maxCoordinate = sc->numCoordinates * sc->matrixConvolution;
	if (sc->numCoordinates * sc->matrixConvolution > 1) {
		PfDiv(sc, &sc->tempInt, &sc->gl_GlobalInvocationID_z, &sc->dispatchZactualFFTSize);
		temp_int.data.i = maxCoordinate;
		PfMod(sc, &sc->tempInt, &sc->tempInt, &temp_int);
		PfMul(sc, &sc->tempInt, &sc->tempInt, &bufferStride[sc->numFFTdims], 0);
		PfAdd(sc, &sc->blockInvocationID, &sc->blockInvocationID, &sc->tempInt);
	}
	if ((sc->matrixConvolution > 1) && (sc->convolutionStep)) {
		maxCoordinate = 1;
		PfMul(sc, &temp_int, &sc->coordinate, &bufferStride[sc->numFFTdims], 0);
		PfAdd(sc, &sc->blockInvocationID, &sc->blockInvocationID, &temp_int);
	}
	if ((sc->numBatches.data.i > 1) || (sc->numKernels.data.i > 1)) {
		if (sc->convolutionStep && (sc->numKernels.data.i > 1)) {
			PfMul(sc, &sc->tempInt, &sc->batchID, &sc->inputStride[sc->numFFTdims+1], 0);
			PfAdd(sc, &sc->blockInvocationID, &sc->blockInvocationID, &sc->tempInt);
		}
		else {
			temp_int.data.i = sc->dispatchZactualFFTSize.data.i * maxCoordinate;
			PfDiv(sc, &sc->tempInt, &sc->gl_GlobalInvocationID_z, &temp_int);
			PfMul(sc, &sc->tempInt, &sc->tempInt, &bufferStride[sc->numFFTdims+1], 0);
			PfAdd(sc, &sc->blockInvocationID, &sc->blockInvocationID, &sc->tempInt);

		}
	}
	if (sc->kernelOffset.type < 100) {
		temp_int.data.i = sc->kernelOffset.data.i / sc->kernelNumberByteSize;
		PfAdd(sc, &sc->blockInvocationID, &sc->blockInvocationID, &temp_int);
	}
	else {
		if (sc->kernelOffset.type == 101) {
			if (sc->performPostCompilationKernelOffset) {
				PfAdd(sc, &sc->blockInvocationID, &sc->blockInvocationID, &sc->kernelOffset);
			}
		}
	}
	return;
}

static inline void appendReadWriteDataVkFFT_nonstrided(VkFFTSpecializationConstantsLayout* sc, int readWrite, int type) {
	if (sc->res != VKFFT_SUCCESS) return;
	//&sc->tempIntLen = sprintf(&sc->tempIntStr, "	return;\n");
	//char shiftX[500] = "";
	//if (&sc->performWorkGroupShift[0])
	//	sprintf(shiftX, " + consts.workGroupShiftX ");
	PfContainer temp_int = VKFFT_ZERO_INIT;
	temp_int.type = 31;
	PfContainer temp_int1 = VKFFT_ZERO_INIT;
	temp_int1.type = 31;
	if ((!sc->writeFromRegisters) && (readWrite == 1))
		appendBarrierVkFFT(sc, __FILE__, __LINE__);
	//move to initialization
	//char shiftY[100] = "";
	//if (&sc->performWorkGroupShift[1])
	//	sprintf(shiftY, " + consts.workGroupShiftY ");

	//&sc->shiftY = &sc->workGroupShiftX;
	PfContainer localSize = VKFFT_ZERO_INIT;
	localSize.type = 31;

	PfContainer batching_localSize = VKFFT_ZERO_INIT;
	batching_localSize.type = 31;

	PfContainer* localInvocationID = VKFFT_ZERO_INIT;
	PfContainer* batchingInvocationID = VKFFT_ZERO_INIT;

	if (sc->stridedSharedLayout) {
		batching_localSize.data.i = sc->localSize[0].data.i;
		localSize.data.i = sc->localSize[1].data.i;
		localInvocationID = &sc->gl_LocalInvocationID_y;
		batchingInvocationID = &sc->gl_LocalInvocationID_x;
	}
	else {
		batching_localSize.data.i = sc->localSize[1].data.i;
		localSize.data.i = sc->localSize[0].data.i;
		localInvocationID = &sc->gl_LocalInvocationID_x;
		batchingInvocationID = &sc->gl_LocalInvocationID_y;
	}

	PfContainer used_registers = VKFFT_ZERO_INIT;
	used_registers.type = 31;

	PfContainer* bufferStride = (readWrite) ? sc->outputStride : sc->inputStride;

	int recalculateAtEveryStep_inoutID = 0;
	if ((sc->zeropad[readWrite]) || ((sc->numAxisUploads > 1) && (sc->zeropadBluestein[readWrite])) || ((type / 10) == 111) || ((type / 10) == 121) || ((type / 10) == 131) || ((type / 10) == 141) || ((type / 10) == 143) || ((type / 10) == 70) || ((type / 10) == 80)) {
		recalculateAtEveryStep_inoutID = 1;
	}

	PfContainer mult = VKFFT_ZERO_INIT;
	mult.type = 31;

	PfContainer fftDim = VKFFT_ZERO_INIT;
	fftDim.type = 31;
	if (((sc->zeropad[readWrite]) || ((sc->zeropadBluestein[readWrite])) || (((type / 10) == 111)&&(sc->performDST == 1))) && (!readWrite)) {
		if (sc->readToRegisters == 0) {
			appendSetSMToZero(sc);
			appendBarrierVkFFT(sc, __FILE__, __LINE__);
		}
	}
	if (sc->zeropadBluestein[readWrite]) {
		if (sc->numAxisUploads == 1) {
			if (readWrite) {
				fftDim.data.i = sc->fft_zeropad_Bluestein_left_write[sc->axis_id].data.i;
			}
			else {
				fftDim.data.i = sc->fft_zeropad_Bluestein_left_read[sc->axis_id].data.i;
			}
			if ((type / 10) == 141) fftDim.data.i /= 2;
			if (((type / 10) == 111)&&(sc->performDCT == 1)) fftDim.data.i = 2 * fftDim.data.i - 2;
			if (((type / 10) == 111)&&(sc->performDST == 1)) fftDim.data.i = 2 * fftDim.data.i + 2;
		}
		else {
			fftDim.data.i = sc->fftDim.data.i;
		}
	}
	else
		fftDim.data.i = sc->fftDim.data.i;

	if ((((type / 10) == 60) && (readWrite == 0)) || (((type / 10) == 50) && (readWrite == 1))) {
		temp_int.data.i = 2;
		PfDiv(sc, &fftDim, &fftDim, &temp_int);
		PfInc(sc, &fftDim);
	}
	else if (((type / 10) == 110) && (!(sc->zeropadBluestein[readWrite] && (sc->numAxisUploads == 1))))  {
		if(sc->performDST > 0)
			fftDim.data.i = (fftDim.data.i - 2) / 2;
		else
			fftDim.data.i = (fftDim.data.i + 2) / 2;
	}
	else if (((type / 10) == 140) && (readWrite == 0)) {
		fftDim.data.i = 2 * fftDim.data.i;
	}

	if (sc->mergeSequencesR2C)
		mult.data.i = 2;
	else
		mult.data.i = 1;

	//prepare offsets
	if (readWrite == 0) {
		if (sc->performWorkGroupShift[0]) {
			PfAdd(sc, &sc->shiftX, &sc->gl_WorkGroupID_x, &sc->workGroupShiftX);
		}
		else {
			PfMov(sc, &sc->shiftX, &sc->gl_WorkGroupID_x);
		}
		if (sc->size[1].data.i > 1) {
			if (sc->numAxisUploads == 1) {
				if (sc->performWorkGroupShift[1]) {
					PfAdd(sc, &sc->shiftY, &sc->gl_WorkGroupID_y, &sc->workGroupShiftY);
					temp_int.data.i = mult.data.i * batching_localSize.data.i;
					PfMul(sc, &sc->shiftY, &sc->shiftY, &temp_int, 0);
					checkZeropad_otherAxes(sc, &sc->shiftY, 1);
				}
				else {
					PfMov(sc, &sc->shiftY, &sc->gl_WorkGroupID_y);
					temp_int.data.i = mult.data.i * batching_localSize.data.i;
					PfMul(sc, &sc->shiftY, &sc->shiftY, &temp_int, 0);
					checkZeropad_otherAxes(sc, &sc->shiftY, 1);
				}
				PfSetToZero(sc, &sc->shiftZ);
			}
			else {
				if (sc->performWorkGroupShift[1]) {
					PfAdd(sc, &sc->shiftY, &sc->gl_WorkGroupID_y, &sc->workGroupShiftY);
					checkZeropad_otherAxes(sc, &sc->shiftY, 1);
					temp_int.data.i = sc->inputStride[1].data.i;
					PfMul(sc, &sc->shiftZ, &sc->shiftY, &temp_int, 0);
				}
				else
				{
					PfMov(sc, &sc->shiftY, &sc->gl_WorkGroupID_y);
					checkZeropad_otherAxes(sc, &sc->shiftY, 1);
					temp_int.data.i = sc->inputStride[1].data.i;
					PfMul(sc, &sc->shiftZ, &sc->shiftY, &temp_int, 0);
				}
			}
		}
		else {
			PfSetToZero(sc, &sc->shiftZ);
		}
		appendOffset(sc, readWrite, type);

	}
	else {
        int isInputStrideNotEqualToOutput = 0;
        for (int i = 0; i < sc->numFFTdims+2; i++){
            if ((sc->inputStride[i].type>100)||(sc->outputStride[i].type>100)||(sc->inputStride[i].data.i != sc->outputStride[i].data.i)) {
                isInputStrideNotEqualToOutput = 1;
            }
        }
		if ((isInputStrideNotEqualToOutput) || (sc->performPostCompilationInputOffset) || (sc->performPostCompilationOutputOffset) || ((sc->inputOffset.data.i != sc->outputOffset.data.i) && (sc->inputOffset.type < 100) && (sc->outputOffset.type < 100)) || ((sc->convolutionStep) && (sc->matrixConvolution > 1)) || (sc->batchID.data.i > 0)) {
			if ((sc->size[1].data.i > 1) && (sc->numAxisUploads != 1)) {
				if (sc->performWorkGroupShift[1]) {
					temp_int.data.i = sc->outputStride[1].data.i;
					PfMul(sc, &sc->shiftZ, &sc->shiftY, &temp_int, 0);
				}
				else
				{
					temp_int.data.i = sc->outputStride[1].data.i;
					PfMul(sc, &sc->shiftZ, &sc->shiftY, &temp_int, 0);
				}
			}
			else {
				PfSetToZero(sc, &sc->shiftZ);
			}
			appendOffset(sc, readWrite, type);
		}
	}

	if ((((type / 10) == 60) && (readWrite == 0)) || (((type / 10) == 50) && (readWrite == 1))) {
		PfMul(sc, &used_registers, &fftDim, &mult, 0);
		mult.data.i = 1;
	}
	else {
		PfMov(sc, &used_registers, &fftDim);
	}

	PfDivCeil(sc, &used_registers, &used_registers, &localSize);

	PfContainer size1 = VKFFT_ZERO_INIT;
	size1.type = 31;
	PfDivCeil(sc, &size1, &sc->size[1], &mult);

	if (sc->registerBoost > 1) {
		temp_int.data.i = sc->registerBoost;
		PfDiv(sc, &used_registers, &used_registers, &temp_int);
	}

	if (sc->fftDim.data.i != sc->fft_dim_full.data.i) {
		if ((sc->reorderFourStep) && (readWrite == 1)) {
			//sc->tempLen = sprintf(sc->tempStr, "		if (((%s + %" PRIu64 " * %s) %% %" PRIu64 " + ((%s%s) / %" PRIu64 ")*%" PRIu64 " < %" PRIu64 ")){\n", sc->gl_LocalInvocationID_x, sc->localSize[0], sc->gl_LocalInvocationID_y, sc->localSize[1], sc->gl_WorkGroupID_x, shiftX, sc->firstStageStartSize / sc->fftDim, sc->localSize[1], sc->fft_dim_full / sc->firstStageStartSize);
			PfMul(sc, &sc->tempInt2, &sc->localSize[0], &sc->gl_LocalInvocationID_y, 0);
			PfAdd(sc, &sc->tempInt2, &sc->tempInt2, &sc->gl_LocalInvocationID_x);
			PfMod(sc, &sc->tempInt2, &sc->tempInt2, &batching_localSize);

			PfDiv(sc, &temp_int, &sc->firstStageStartSize, &sc->fftDim);
			PfDiv(sc, &sc->tempInt, &sc->shiftX, &temp_int);
			PfMul(sc, &sc->tempInt, &sc->tempInt, &batching_localSize, 0);

			PfAdd(sc, &sc->tempInt2, &sc->tempInt2, &sc->tempInt);
			PfDiv(sc, &temp_int, &sc->fft_dim_full, &sc->firstStageStartSize);
			PfIf_lt_start(sc, &sc->tempInt2, &temp_int);
		}
		else {
			PfDiv(sc, &temp_int, &sc->firstStageStartSize, &sc->fftDim);
			PfMod(sc, &sc->tempInt, &sc->shiftX, &temp_int);
			PfMul(sc, &sc->tempInt2, &sc->tempInt, &sc->fftDim, 0);

			PfDiv(sc, &sc->tempInt, &sc->shiftX, &temp_int);
			PfMul(sc, &temp_int, &batching_localSize, &sc->firstStageStartSize, 0);
			PfMul(sc, &sc->tempInt, &sc->tempInt, &temp_int, 0);
			PfAdd(sc, &sc->tempInt2, &sc->tempInt2, &sc->tempInt);

			//sc->tempLen = sprintf(sc->tempStr, "		%s numActiveThreads = ((%s/%" PRIu64 ")==%" PRIu64 ") ? %" PRIu64 " : %" PRIu64 ";\n", uintType, sc->gl_WorkGroupID_x, sc->firstStageStartSize / sc->fftDim, ((pfUINT)floor(sc->fft_dim_full / ((double)sc->localSize[0] * sc->fftDim))) / (sc->firstStageStartSize / sc->fftDim), (pfUINT)pfceil(((sc->fft_dim_full - (sc->firstStageStartSize / sc->fftDim) * ((((pfUINT)floor(sc->fft_dim_full / ((double)sc->localSize[0] * sc->fftDim))) / (sc->firstStageStartSize / sc->fftDim)) * sc->localSize[0] * sc->fftDim)) / (sc->firstStageStartSize / sc->fftDim)) / (double)used_registers_read), sc->localSize[0] * sc->localSize[1]);// sc->fft_dim_full, sc->gl_WorkGroupID_x, shiftX, sc->firstStageStartSize / sc->fftDim, sc->fftDim, sc->gl_WorkGroupID_x, shiftX, sc->firstStageStartSize / sc->fftDim, sc->localSize[0] * sc->firstStageStartSize, sc->fft_dim_full / (sc->localSize[0] * sc->fftDim));
			temp_int.data.i = sc->firstStageStartSize.data.i / sc->fftDim.data.i;
			PfDiv(sc, &sc->tempInt, &sc->gl_WorkGroupID_x, &temp_int);
			temp_int1.data.i = ((pfINT)pffloor(sc->fft_dim_full.data.i / ((pfLD)batching_localSize.data.i * sc->fftDim.data.i))) / (sc->firstStageStartSize.data.i / sc->fftDim.data.i);
			PfIf_eq_start(sc, &sc->tempInt, &temp_int1);
			temp_int1.data.i = ((sc->fft_dim_full.data.i - (sc->firstStageStartSize.data.i / sc->fftDim.data.i) * ((((pfINT)pffloor(sc->fft_dim_full.data.i / ((pfLD)batching_localSize.data.i * sc->fftDim.data.i))) / (sc->firstStageStartSize.data.i / sc->fftDim.data.i)) * batching_localSize.data.i * sc->fftDim.data.i)) / (sc->firstStageStartSize.data.i / sc->fftDim.data.i));
			PfMov(sc, &sc->blockInvocationID, &temp_int1);
			PfIf_else(sc);
			temp_int1.data.i = fftDim.data.i * batching_localSize.data.i;
			PfMov(sc, &sc->blockInvocationID, &temp_int1);
			PfIf_end(sc);


			if (sc->stridedSharedLayout) {

				PfMul(sc, &sc->tempInt, &sc->gl_LocalInvocationID_x, &sc->firstStageStartSize, 0);
				PfAdd(sc, &sc->tempInt, &sc->tempInt, &sc->tempInt2);
			}
			else {
				PfMul(sc, &sc->tempInt, &sc->gl_LocalInvocationID_y, &sc->firstStageStartSize, 0);
				PfAdd(sc, &sc->tempInt, &sc->tempInt, &sc->tempInt2);
			}

			/*sc->useDisableThreads = 1;
			PfIf_ge_start(sc, &sc->tempInt, &sc->fft_dim_full);
			temp_int.data.i = 0;
			PfMov(sc, &sc->disableThreads, &temp_int);
			PfIf_end(sc);*/
		}
	}
	if (sc->useDisableThreads) {
		temp_int.data.i = 0;
		PfIf_gt_start(sc, &sc->disableThreads, &temp_int);
	}
	if (bufferStride[1].data.i == fftDim.data.i) {
		PfMul(sc, &sc->inoutID, &bufferStride[1], &sc->shiftY, 0);
		PfAdd(sc, &sc->inoutID, &sc->inoutID, &sc->shiftZ);
		if (sc->localSize[1].data.i == 1) {
			PfAdd(sc, &sc->inoutID, &sc->inoutID, &sc->gl_LocalInvocationID_x);
		}
		else {
			PfAdd(sc, &sc->inoutID, &sc->inoutID, &sc->gl_LocalInvocationID_x);
			PfMul(sc, &sc->combinedID, &sc->localSize[0], &sc->gl_LocalInvocationID_y, 0);
			PfAdd(sc, &sc->inoutID, &sc->inoutID, &sc->combinedID);
		}
	}
	//for (pfUINT k = 0; k < &sc->registerBoost; k++) {
	for (int k = 0; k < sc->registerBoost; k++) {
		//for (pfUINT i = 0; i < used_registers; i++) {
		for (int i = 0; i < used_registers.data.i; i++) {
			//combined thread numeration
			if (sc->localSize[1].data.i == 1) {
				//&sc->tempIntLen = sprintf(&sc->tempIntStr, "		combinedID = %s + %" PRIu64 ";\n", &sc->gl_LocalInvocationID_x, (i + k * used_registers) * &sc->localSize[0]);
				temp_int.data.i = (k * used_registers.data.i + i) * sc->localSize[0].data.i;

				PfAdd(sc, &sc->combinedID, &sc->gl_LocalInvocationID_x, &temp_int);
			}
			else {
				//&sc->tempIntLen = sprintf(&sc->tempIntStr, "		combinedID = (%s + %" PRIu64 " * %s) + %" PRIu64 ";\n", &sc->gl_LocalInvocationID_x, &sc->localSize[0], &sc->gl_LocalInvocationID_y, (i + k * used_registers) * &sc->localSize[0] * &sc->localSize[1]);
				PfMul(sc, &sc->combinedID, &sc->localSize[0], &sc->gl_LocalInvocationID_y, 0);
				PfAdd(sc, &sc->combinedID, &sc->combinedID, &sc->gl_LocalInvocationID_x);

				temp_int.data.i = (k * used_registers.data.i + i) * sc->localSize[0].data.i * sc->localSize[1].data.i;

				PfAdd(sc, &sc->combinedID, &sc->combinedID, &temp_int);
			}
			//set inoutID - global array index. Two batching options - in consecutive x (if multi-upload), &in y if multidimensional.
			if (sc->fftDim.data.i == sc->fft_dim_full.data.i) {
				//&sc->tempIntLen = sprintf(&sc->tempIntStr, "		inoutID = (combinedID %% %" PRIu64 ") + (combinedID / %" PRIu64 ") * %" PRIu64 ";\n", &sc->fftDim, &sc->fftDim, &sc->inputStride[1]);
				PfMod(sc, &sc->inoutID_x, &sc->combinedID, &fftDim);
				PfDiv(sc, &sc->inoutID_y, &sc->combinedID, &fftDim);
				if (mult.data.i > 1) {
					PfMul(sc, &sc->inoutID_y, &sc->inoutID_y, &mult, 0);
				}
				PfAdd(sc, &sc->inoutID_y, &sc->inoutID_y, &sc->shiftY);
				checkZeropadStart_otherAxes(sc, &sc->inoutID_y, 1);
				//PfMul(sc, &sc->tempInt, &batching_localSize, &sc->shiftY,0);
				//PfAdd(sc, &sc->inoutID_y, &sc->inoutID_y, &sc->tempInt);
				temp_int.data.i = batching_localSize.data.i;
				//we switched to reading 2x more data, but we still need to check out of bounds for odd size1
				if ((sc->mergeSequencesR2C) && (mult.data.i == 1))
					temp_int.data.i *= 2;

				if ((size1.data.i % temp_int.data.i) != 0) {
#if (VKFFT_BACKEND!=2) //AMD compiler fix
					//&sc->tempIntLen = sprintf(&sc->tempIntStr, "		if(combinedID / %" PRIu64 " + (%s%s)*%" PRIu64 "< %" PRIu64 "){\n", &sc->fftDim, &sc->gl_WorkGroupID_y, shiftY, &sc->localSize[0], &sc->size[&sc->axis_id + 1]);
					if ((sc->mergeSequencesR2C) && (sc->size[1].data.i % 2) && (readWrite == 0)) {
						PfIf_ge_start(sc, &sc->inoutID_y, &sc->size[1]);
						temp_int.data.i = sc->size[1].data.i - 1;
						PfMov(sc, &sc->inoutID_y, &temp_int);
						PfIf_end(sc);
					}
					else {
						PfIf_lt_start(sc, &sc->inoutID_y, &sc->size[1]);
					}
#else
					//&sc->tempIntLen = sprintf(&sc->tempIntStr, "		if(!(combinedID / %" PRIu64 " + (%s%s)*%" PRIu64 "< %" PRIu64 ")) %s = 0; {\n", &sc->fftDim, &sc->gl_WorkGroupID_y, shiftY, &sc->localSize[0], &sc->size[&sc->axis_id + 1], &sc->inoutID);
					if ((sc->mergeSequencesR2C) && (sc->size[1].data.i % 2) && (readWrite == 0)) {
						PfIf_ge_start(sc, &sc->inoutID_y, &sc->size[1]);
						temp_int.data.i = sc->size[1].data.i - 1;
						PfMov(sc, &sc->inoutID_y, &temp_int);
						PfIf_end(sc);
					}
					else if (readWrite == 0) {
						PfIf_ge_start(sc, &sc->inoutID_y, &sc->size[1]);
						PfSetToZero(sc, &sc->inoutID_x);
						PfSetToZero(sc, &sc->inoutID_y);
						PfIf_end(sc);
					}
					else {
						PfIf_lt_start(sc, &sc->inoutID_y, &sc->size[1]);
					}
#endif
				}
			}
			else {
				//&sc->tempIntLen = sprintf(&sc->tempIntStr, "		inoutID = (combinedID %% %" PRIu64 ") + (combinedID / %" PRIu64 ") * %" PRIu64 " + ((%s%s) %% %" PRIu64 ") * %" PRIu64 " + ((%s%s) / %" PRIu64 ") * %" PRIu64 ";\n", &sc->fftDim, &sc->fftDim, &sc->firstStageStartSize, &sc->gl_WorkGroupID_x, shiftX, &sc->firstStageStartSize / &sc->fftDim, &sc->fftDim, &sc->gl_WorkGroupID_x, shiftX, &sc->firstStageStartSize / &sc->fftDim, &sc->localSize[0] * &sc->firstStageStartSize);
				if ((sc->reorderFourStep) && (readWrite == 1)) {
					PfMod(sc, &sc->inoutID_x, &sc->combinedID, &batching_localSize);

					temp_int.data.i = sc->firstStageStartSize.data.i / sc->fftDim.data.i;
					PfDiv(sc, &sc->tempInt, &sc->shiftX, &temp_int);
					PfMul(sc, &sc->tempInt, &sc->tempInt, &batching_localSize, 0);

					PfAdd(sc, &sc->inoutID_x, &sc->inoutID_x, &sc->tempInt);

					PfDiv(sc, &sc->tempInt2, &sc->combinedID, &batching_localSize);
					temp_int.data.i = sc->fft_dim_full.data.i / sc->fftDim.data.i;
					PfMul(sc, &sc->tempInt2, &sc->tempInt2, &temp_int, 0);
					temp_int.data.i = sc->firstStageStartSize.data.i / sc->fftDim.data.i;
					PfMod(sc, &sc->tempInt, &sc->shiftX, &temp_int);
					temp_int.data.i = sc->fft_dim_full.data.i / sc->firstStageStartSize.data.i;
					PfMul(sc, &sc->tempInt, &sc->tempInt, &temp_int, 0);
					PfAdd(sc, &sc->tempInt2, &sc->tempInt2, &sc->tempInt);

					PfAdd(sc, &sc->inoutID_x, &sc->inoutID_x, &sc->tempInt2);
				}
				else {
					PfMod(sc, &sc->inoutID_x, &sc->combinedID, &sc->fftDim);

					PfDiv(sc, &sc->tempInt, &sc->combinedID, &sc->fftDim);
					PfMul(sc, &sc->tempInt, &sc->tempInt, &sc->firstStageStartSize, 0);
					PfAdd(sc, &sc->inoutID_x, &sc->inoutID_x, &sc->tempInt);

					PfAdd(sc, &sc->inoutID_x, &sc->inoutID_x, &sc->tempInt2);

					//PfIf_lt_start(sc, &sc->inoutID_x, &sc->size[sc->axis_id]);
				}
			}

			if ((sc->fftDim.data.i != sc->fft_dim_full.data.i) && (!((sc->reorderFourStep) && (readWrite == 1)))) {
				PfIf_lt_start(sc, &sc->combinedID, &sc->blockInvocationID);
			}
			else {
				temp_int.data.i = (k * used_registers.data.i + i + 1) * sc->localSize[0].data.i * sc->localSize[1].data.i;

				temp_int1.data.i = fftDim.data.i * batching_localSize.data.i;

				if ((sc->mergeSequencesR2C) && (mult.data.i == 1))
					temp_int1.data.i *= 2;

				if (temp_int.data.i > temp_int1.data.i) {
					//check that we only read fftDim * local batch data
					//&sc->tempIntLen = sprintf(&sc->tempIntStr, "		if(combinedID < %" PRIu64 "){\n", &sc->fftDim * &sc->localSize[0]);
					PfIf_lt_start(sc, &sc->combinedID, &temp_int1);
				}
			}
			temp_int.data.i = batching_localSize.data.i;
			//we switched to reading 2x more data, but we still need to check out of bounds for odd size1
			if ((sc->mergeSequencesR2C) && (mult.data.i == 1))
				temp_int.data.i *= 2;

			if (!recalculateAtEveryStep_inoutID)
				checkZeropadStart_currentFFTAxis(sc, readWrite, type, &sc->inoutID_x);

			append_inoutID_preprocessing_multiupload_R2R(sc, &sc->inoutID_x, readWrite, type, &sc->stageInvocationID, &sc->sdataID);
			append_inoutID_preprocessing_multiupload_R2C(sc, &sc->inoutID_x, readWrite, type, &sc->stageInvocationID, &sc->sdataID);

#if (VKFFT_BACKEND!=2) //AMD compiler fix
			if ((bufferStride[1].data.i == fftDim.data.i) && (!(((size1.data.i % temp_int.data.i) != 0) && (sc->mergeSequencesR2C) && (sc->size[1].data.i % 2) && (readWrite == 0))) && (!sc->mergeSequencesR2C) && (!recalculateAtEveryStep_inoutID)) {
#else
			if ((bufferStride[1].data.i == fftDim.data.i) && (!(((size1.data.i % temp_int.data.i) != 0) && (readWrite == 0))) && (!sc->mergeSequencesR2C)) {
#endif
				if (k * used_registers.data.i + i > 0) {
					temp_int.data.i = sc->localSize[0].data.i * sc->localSize[1].data.i;
					PfAdd(sc, &sc->inoutID, &sc->inoutID, &temp_int);
				}
			}
			else {
				PfMov(sc, &sc->inoutID, &sc->inoutID_x);
				if (sc->fftDim.data.i == sc->fft_dim_full.data.i) {
					PfMul(sc, &sc->tempInt, &sc->inoutID_y, &bufferStride[1], 0);
					PfAdd(sc, &sc->inoutID, &sc->inoutID, &sc->tempInt);
				}
				PfAdd(sc, &sc->inoutID, &sc->inoutID, &sc->shiftZ);
			}

			if ((sc->readToRegisters && (readWrite == 0)) || (sc->writeFromRegisters && (readWrite == 1))) {
				//no need to calculate register addresses
			}
			else {
				if (sc->stridedSharedLayout) {
					if ((sc->reorderFourStep) && (readWrite == 1)) {
						//sc->tempLen = sprintf(sc->tempStr, "			%s[%s] = %ssdata[(combinedID %% %s)+(combinedID/%s)*sharedStride]%s;\n", outputsStruct, sc->inoutID, convTypeLeft, sc->gl_WorkGroupSize_x, sc->gl_WorkGroupSize_x, convTypeRight);
						PfMod(sc, &sc->sdataID, &sc->combinedID, &sc->localSize[0]);
						PfDiv(sc, &sc->tempInt, &sc->combinedID, &sc->localSize[0]);
						PfMul(sc, &sc->tempInt, &sc->tempInt, &sc->sharedStride, 0);
						PfAdd(sc, &sc->sdataID, &sc->sdataID, &sc->tempInt);
					}
					else {
						//&sc->tempIntLen = sprintf(&sc->tempIntStr, "		sdata[(combinedID %% %" PRIu64 ") * sharedStride + (combinedID / %" PRIu64 ")] = %s%s[%s]%s;\n", &sc->fftDim, &sc->fftDim, convTypeLeft, &inputsStruct, &sc->inoutID, convTypeRight);
						temp_int.data.i = fftDim.data.i;
						if ((sc->mergeSequencesR2C) && (mult.data.i == 1))
							temp_int.data.i *= 2;
						PfMod(sc, &sc->sdataID, &sc->combinedID, &temp_int);

						if (((type / 10) == 140) && (!sc->readToRegisters) && (readWrite == 0)) {
							temp_int1.data.i = 2;
							PfDiv(sc, &sc->sdataID, &sc->sdataID, &temp_int1);
						}

						PfMul(sc, &sc->sdataID, &sc->sdataID, &sc->sharedStride, 0);
						PfDiv(sc, &sc->tempInt, &sc->combinedID, &temp_int);
						PfAdd(sc, &sc->sdataID, &sc->sdataID, &sc->tempInt);
					}
					if ((sc->performDST == 1) && (!sc->forceCallbackVersionRealTransforms))
						PfAdd(sc, &sc->sdataID, &sc->sdataID, &sc->sharedStride);
				}
				else {
					if ((sc->reorderFourStep) && (readWrite == 1)) {
						//sc->tempLen = sprintf(sc->tempStr, "			%s[%s] = %ssdata[(combinedID %% %s)*sharedStride+combinedID/%s]%s;\n", outputsStruct, sc->inoutID, convTypeLeft, sc->gl_WorkGroupSize_y, sc->gl_WorkGroupSize_y, convTypeRight);
						PfMod(sc, &sc->sdataID, &sc->combinedID, &sc->localSize[1]);
						PfMul(sc, &sc->sdataID, &sc->sdataID, &sc->sharedStride, 0);
						PfDiv(sc, &sc->tempInt, &sc->combinedID, &sc->localSize[1]);
						PfAdd(sc, &sc->sdataID, &sc->sdataID, &sc->tempInt);
					}
					else {
						//&sc->tempIntLen = sprintf(&sc->tempIntStr, "		sdata[(combinedID %% %" PRIu64 ") + (combinedID / %" PRIu64 ") * sharedStride] = %s%s[%s]%s;\n", &sc->fftDim, &sc->fftDim, convTypeLeft, &inputsStruct, &sc->inoutID, convTypeRight);
						temp_int.data.i = fftDim.data.i;
						if ((sc->mergeSequencesR2C) && (mult.data.i == 1))
							temp_int.data.i *= 2;
						PfMod(sc, &sc->sdataID, &sc->combinedID, &temp_int);

						if (((type / 10) == 140) && (!sc->readToRegisters) && (readWrite == 0)) {
							temp_int1.data.i = 2;
							PfDiv(sc, &sc->sdataID, &sc->sdataID, &temp_int1);
						}

						PfDiv(sc, &sc->tempInt, &sc->combinedID, &temp_int);
						PfMul(sc, &sc->tempInt, &sc->tempInt, &sc->sharedStride, 0);
						PfAdd(sc, &sc->sdataID, &sc->sdataID, &sc->tempInt);
					}
					if ((sc->performDST == 1) && (!sc->forceCallbackVersionRealTransforms))
						PfInc(sc, &sc->sdataID);
				}
			}
			
			if (readWrite == 0) {
				if ((sc->inputMemoryCode % 10) == 2) {
					if (((type / 10) == 50) || ((type / 10) == 70) || ((type / 10) == 110) || ((type / 10) == 111) || ((type / 10) == 120) || ((type / 10) == 121) || ((type / 10) == 130) || ((type / 10) == 131) || ((type / 10) == 142) || ((type / 10) == 143)) {
						if (recalculateAtEveryStep_inoutID)
							checkZeropadStart_currentFFTAxis(sc, readWrite, type, &sc->inoutID_x);
							
						if (sc->readToRegisters) {
							appendGlobalToRegisters_x(sc, &sc->regIDs[k * sc->registers_per_thread + i], &sc->inputsStruct, &sc->inoutID);

							if (((((sc->performDCT == 3) || (sc->performDST == 3)) && (sc->actualInverse == 0)) || (((sc->performDCT == 2) || (sc->performDST == 2)) && (sc->actualInverse == 1))) && ((type / 10) == 131)) {
								if (recalculateAtEveryStep_inoutID) {
									checkZeropadStart_currentFFTAxis(sc, readWrite, type, &sc->inoutID2);
									if (sc->fftDim.data.i == sc->fft_dim_full.data.i) {
										PfMul(sc, &sc->tempInt, &sc->inoutID_y, &bufferStride[1], 0);
										PfAdd(sc, &sc->inoutID2, &sc->inoutID2, &sc->tempInt);
									}
									PfAdd(sc, &sc->inoutID2, &sc->inoutID2, &sc->shiftZ);
								}
								appendGlobalToRegisters_y(sc, &sc->regIDs[k * sc->registers_per_thread + i], &sc->inputsStruct, &sc->inoutID2);
								if (recalculateAtEveryStep_inoutID)
									checkZeropadEnd_currentFFTAxis(sc, readWrite, type);
							}

							append_processing_multiupload_R2R(sc, &sc->inoutID_x, &sc->regIDs[k * sc->registers_per_thread + i], readWrite, type, 0, 0);
							if (sc->mergeSequencesR2C) {
								if ((sc->size[1].data.i % 2) != 0) {
									temp_int.data.i = sc->size[1].data.i - 1;
									PfIf_lt_start(sc, &sc->inoutID_y, &temp_int);
								}
								PfAdd(sc, &sc->inoutID, &sc->inoutID, &sc->inputStride[1]);
								if ((sc->size[1].data.i % 2) != 0) {
									PfIf_end(sc);
								}
								appendGlobalToRegisters_y(sc, &sc->regIDs[k * sc->registers_per_thread + i], &sc->inputsStruct, &sc->inoutID);
							}
						}
						else {
							appendGlobalToRegisters_x(sc, &sc->temp, &sc->inputsStruct, &sc->inoutID);

							if (((((sc->performDCT == 3) || (sc->performDST == 3)) && (sc->actualInverse == 0)) || (((sc->performDCT == 2) || (sc->performDST == 2)) && (sc->actualInverse == 1))) && ((type / 10) == 131)) {
								if (recalculateAtEveryStep_inoutID) {
									checkZeropadStart_currentFFTAxis(sc, readWrite, type, &sc->inoutID2);
									if (sc->fftDim.data.i == sc->fft_dim_full.data.i) {
										PfMul(sc, &sc->tempInt, &sc->inoutID_y, &bufferStride[1], 0);
										PfAdd(sc, &sc->inoutID2, &sc->inoutID2, &sc->tempInt);
									}
									PfAdd(sc, &sc->inoutID2, &sc->inoutID2, &sc->shiftZ);
								}

								appendGlobalToRegisters_y(sc, &sc->temp, &sc->inputsStruct, &sc->inoutID2);
								if (recalculateAtEveryStep_inoutID)
									checkZeropadEnd_currentFFTAxis(sc, readWrite, type);
							}

							append_processing_multiupload_R2R(sc, &sc->inoutID_x, &sc->temp, readWrite, type, 0, 0);
							if (sc->mergeSequencesR2C) {
								if ((sc->size[1].data.i % 2) != 0) {
									temp_int.data.i = sc->size[1].data.i - 1;
									PfIf_lt_start(sc, &sc->inoutID_y, &temp_int);
								}
								PfAdd(sc, &sc->inoutID, &sc->inoutID, &sc->inputStride[1]);
								if ((sc->size[1].data.i % 2) != 0) {
									PfIf_end(sc);
								}
								appendGlobalToRegisters_y(sc, &sc->temp, &sc->inputsStruct, &sc->inoutID);
							}
							appendRegistersToShared(sc, &sc->sdataID, &sc->temp);
						}

						if (recalculateAtEveryStep_inoutID)
							checkZeropadEnd_currentFFTAxis(sc, readWrite, type);
						
					}
					else  if ((type / 10) == 140) {
						if (recalculateAtEveryStep_inoutID)
							checkZeropadStart_currentFFTAxis(sc, readWrite, type, &sc->inoutID_x);	
						if (sc->readToRegisters) {
							if (i < used_registers.data.i / 2) {
								appendGlobalToRegisters_x(sc, &sc->regIDs[k * sc->registers_per_thread + i], &sc->inputsStruct, &sc->inoutID);
							}
							else {
								appendGlobalToRegisters_y(sc, &sc->regIDs[k * sc->registers_per_thread + i - used_registers.data.i / 2], &sc->inputsStruct, &sc->inoutID);
							}
						}
						else {
							PfMod(sc, &sc->tempInt, &sc->combinedID, &fftDim);
							temp_int.data.i = 2;
							PfMod(sc, &sc->tempInt, &sc->tempInt, &temp_int);
					
							appendGlobalToRegisters_x(sc, &sc->temp, &sc->inputsStruct, &sc->inoutID);
							
							temp_int.data.i = 0;
							PfIf_eq_start(sc, &sc->tempInt, &temp_int);
							appendRegistersToShared_x_x(sc, &sc->sdataID, &sc->temp);
							PfIf_else(sc);
							appendRegistersToShared_y_x(sc, &sc->sdataID, &sc->temp);
							PfIf_end(sc);
						}
						if (recalculateAtEveryStep_inoutID)
							checkZeropadEnd_currentFFTAxis(sc, readWrite, type);
					}
					else  if ((type / 10) == 141) {
						if (recalculateAtEveryStep_inoutID)
							checkZeropadStart_currentFFTAxis(sc, readWrite, type, &sc->inoutID_x);

						appendGlobalToRegisters_x(sc, &sc->regIDs[k * sc->registers_per_thread + i], &sc->inputsStruct, &sc->inoutID);
								
						if (recalculateAtEveryStep_inoutID)
							checkZeropadEnd_currentFFTAxis(sc, readWrite, type);

						if (recalculateAtEveryStep_inoutID) {
							checkZeropadStart_currentFFTAxis(sc, readWrite, type, &sc->inoutID2);
									
							if (sc->fftDim.data.i == sc->fft_dim_full.data.i) {
								PfMul(sc, &sc->tempInt, &sc->inoutID_y, &bufferStride[1], 0);
								PfAdd(sc, &sc->inoutID2, &sc->inoutID2, &sc->tempInt);
							}
							PfAdd(sc, &sc->inoutID2, &sc->inoutID2, &sc->shiftZ);
						}
						appendGlobalToRegisters_y(sc, &sc->regIDs[k * sc->registers_per_thread + i], &sc->inputsStruct, &sc->inoutID2);

						if (recalculateAtEveryStep_inoutID)
							checkZeropadEnd_currentFFTAxis(sc, readWrite, type);
						append_processing_multiupload_R2R(sc, &sc->inoutID_x, &sc->regIDs[k * sc->registers_per_thread + i], readWrite, type, 0, 0);
						if (!sc->readToRegisters) {
							appendRegistersToShared(sc, &sc->sdataID, &sc->regIDs[k * sc->registers_per_thread + i]);
						}
					}
				}
				else {
					if (recalculateAtEveryStep_inoutID)
						checkZeropadStart_currentFFTAxis(sc, readWrite, type, &sc->inoutID_x);
							
					if (sc->readToRegisters) {
						appendGlobalToRegisters(sc, &sc->regIDs[k * sc->registers_per_thread + i], &sc->inputsStruct, &sc->inoutID);
						append_inoutID_processing_multiupload_R2C(sc, &sc->inoutID_x, &sc->regIDs[k * sc->registers_per_thread + i], readWrite, type, &sc->stageInvocationID, 0);	
					}
					else {
						if (sc->performR2C) {
							appendGlobalToRegisters(sc, &sc->temp, &sc->inputsStruct, &sc->inoutID);
							append_inoutID_processing_multiupload_R2C(sc, &sc->inoutID_x, &sc->temp, readWrite, type, &sc->stageInvocationID, 0);	
							appendRegistersToShared(sc, &sc->sdataID, &sc->temp);
						}
						else
							appendGlobalToShared(sc, &sc->sdataID, &sc->inputsStruct, &sc->inoutID);
					}
					if (recalculateAtEveryStep_inoutID)
						checkZeropadEnd_currentFFTAxis(sc, readWrite, type);
				}
			}
			else {
				if ((sc->outputMemoryCode % 10) == 2) {
					if ((type / 10) == 111) {
						if (sc->zeropadBluestein[readWrite]) {
							if (readWrite) {
								temp_int.data.i = sc->fft_zeropad_Bluestein_left_write[sc->axis_id].data.i;
							}
							else {
								//appendSetSMToZero(sc);
								temp_int.data.i = sc->fft_zeropad_Bluestein_left_read[sc->axis_id].data.i;
							}
						}
						else {
							temp_int.data.i = sc->fft_dim_full.data.i;
							if(sc->performDCT)
								temp_int.data.i = (temp_int.data.i+2)/2;
							if(sc->performDST)
								temp_int.data.i = (temp_int.data.i-2)/2;
						}
						PfIf_lt_start(sc, &sc->inoutID_x, &temp_int);
					}
					if (((type / 10) == 60) || ((type / 10) == 80) || ((type / 10) == 110) || ((type / 10) == 111) || ((type / 10) == 120) || ((type / 10) == 121) || ((type / 10) == 130) || ((type / 10) == 131) || ((type / 10) == 142) || ((type / 10) == 143)) {
						if (recalculateAtEveryStep_inoutID)
							checkZeropadStart_currentFFTAxis(sc, readWrite, type, &sc->inoutID_x);
						if (sc->writeFromRegisters) {
							append_processing_multiupload_R2R(sc, &sc->inoutID_x, &sc->regIDs[k * sc->registers_per_thread + i], readWrite, type, 0, 0);
							if (sc->performDST == 1) {
								PfMovNeg(sc, &sc->regIDs[k * sc->registers_per_thread + i].data.c[1], &sc->regIDs[k * sc->registers_per_thread + i].data.c[1]);
								appendRegistersToGlobal_y(sc, &sc->outputsStruct, &sc->inoutID, &sc->regIDs[k * sc->registers_per_thread + i]);
							}
							else
								appendRegistersToGlobal_x(sc, &sc->outputsStruct, &sc->inoutID, &sc->regIDs[k * sc->registers_per_thread + i]);
							if (sc->mergeSequencesR2C) {
								if ((sc->size[1].data.i % 2) != 0) {
									temp_int.data.i = sc->size[1].data.i - 1;
									PfIf_lt_start(sc, &sc->inoutID_y, &temp_int);
								}
								PfAdd(sc, &sc->inoutID, &sc->inoutID, &sc->outputStride[1]);
								appendRegistersToGlobal_y(sc, &sc->outputsStruct, &sc->inoutID, &sc->regIDs[k * sc->registers_per_thread + i]);
								if ((sc->size[1].data.i % 2) != 0) {
									PfIf_end(sc);
								}
							}
						}
						else {
							appendSharedToRegisters(sc, &sc->temp, &sc->sdataID);
							append_processing_multiupload_R2R(sc, &sc->inoutID_x, &sc->temp, readWrite, type, 0, 0);
							if (sc->performDST == 1) {
								PfMovNeg(sc, &sc->temp.data.c[1], &sc->temp.data.c[1]);
								appendRegistersToGlobal_y(sc, &sc->outputsStruct, &sc->inoutID, &sc->temp);
							}
							else
								appendRegistersToGlobal_x(sc, &sc->outputsStruct, &sc->inoutID, &sc->temp);
							if (sc->mergeSequencesR2C) {
								if ((sc->size[1].data.i % 2) != 0) {
									temp_int.data.i = sc->size[1].data.i - 1;
									PfIf_lt_start(sc, &sc->inoutID_y, &temp_int);
								}
								PfAdd(sc, &sc->inoutID, &sc->inoutID, &sc->outputStride[1]);
								if (sc->performDST == 1)
									appendRegistersToGlobal_x(sc, &sc->outputsStruct, &sc->inoutID, &sc->temp);
								else
									appendRegistersToGlobal_y(sc, &sc->outputsStruct, &sc->inoutID, &sc->temp);
								if ((sc->size[1].data.i % 2) != 0) {
									PfIf_end(sc);
								}
							}
						}
						if (recalculateAtEveryStep_inoutID)
							checkZeropadEnd_currentFFTAxis(sc, readWrite, type);
					}
					else  if (((type / 10) == 140) || ((type / 10) == 141)) {
						if (!sc->writeFromRegisters) {
							appendSharedToRegisters(sc, &sc->regIDs[k * sc->registers_per_thread + i], &sc->sdataID);
						}
						if (recalculateAtEveryStep_inoutID)
							checkZeropadStart_currentFFTAxis(sc, readWrite, type, &sc->inoutID_x);
						append_processing_multiupload_R2R(sc, &sc->inoutID_x, &sc->regIDs[k * sc->registers_per_thread + i], readWrite, type, 0, 0);

						if ((sc->fftDim.data.i == 1) && (sc->normalize) && (sc->actualInverse)){ //workaround for DCT/DST-IV of size 2 that has no FFT stages (where normalization typically happens). 
							PfContainer temp_double = VKFFT_ZERO_INIT;
							temp_double.type = 22;
							temp_double.data.d = pfFPinit("0.25");
							PfMul(sc, &sc->regIDs[k * sc->registers_per_thread + i], &sc->regIDs[k * sc->registers_per_thread + i], &temp_double, 0);
						}

						appendRegistersToGlobal_x(sc, &sc->outputsStruct, &sc->inoutID, &sc->regIDs[k * sc->registers_per_thread + i]);
						if (recalculateAtEveryStep_inoutID)
							checkZeropadEnd_currentFFTAxis(sc, readWrite, type);

						if (sc->zeropadBluestein[readWrite]) {
							if (readWrite) {
								temp_int.data.i = sc->fft_zeropad_Bluestein_left_write[sc->axis_id].data.i;
							}
							else {
								//appendSetSMToZero(sc);
								temp_int.data.i = sc->fft_zeropad_Bluestein_left_read[sc->axis_id].data.i;
							}
							if ((type / 10) == 141) temp_int.data.i /= 2;
						}
						else {
							temp_int.data.i = sc->fft_dim_full.data.i;
						}
						
						if (recalculateAtEveryStep_inoutID) {
							if ((type / 10) == 141) {
								temp_int.data.i = 2*temp_int.data.i - 1;
								PfSub(sc, &sc->sdataID, &temp_int, &sc->inoutID_x);
							}
							else
								PfAdd(sc, &sc->sdataID, &sc->inoutID_x, &temp_int);
							checkZeropadStart_currentFFTAxis(sc, readWrite, type, &sc->sdataID);
							if (sc->fftDim.data.i == sc->fft_dim_full.data.i) {
								PfMul(sc, &sc->tempInt, &sc->inoutID_y, &bufferStride[1], 0);
								PfAdd(sc, &sc->sdataID, &sc->sdataID, &sc->tempInt);
							}
							PfAdd(sc, &sc->inoutID, &sc->sdataID, &sc->shiftZ);
							checkZeropadStart_currentFFTAxis(sc, readWrite, type, &sc->sdataID);
						}
						else {
							PfAdd(sc, &sc->inoutID, &sc->inoutID, &temp_int);
						}
						appendRegistersToGlobal_y(sc, &sc->outputsStruct, &sc->inoutID, &sc->regIDs[k * sc->registers_per_thread + i]);
						if (recalculateAtEveryStep_inoutID)
							checkZeropadEnd_currentFFTAxis(sc, readWrite, type);
					}
					if ((type / 10) == 111) {
						PfIf_end(sc);
					}
				}
				else {
					if (recalculateAtEveryStep_inoutID)
						checkZeropadStart_currentFFTAxis(sc, readWrite, type, &sc->inoutID_x);
					if (sc->writeFromRegisters) {
						appendRegistersToGlobal(sc, &sc->outputsStruct, &sc->inoutID, &sc->regIDs[k * sc->registers_per_thread + i]);
					}
					else {
						appendSharedToGlobal(sc, &sc->outputsStruct, &sc->inoutID, &sc->sdataID);
					}
					if (recalculateAtEveryStep_inoutID)
						checkZeropadEnd_currentFFTAxis(sc, readWrite, type);
				}
			}

			append_inoutID_postprocessing_multiupload_R2R(sc, &sc->inoutID_x, readWrite, type, &sc->stageInvocationID);
			append_inoutID_postprocessing_multiupload_R2C(sc, &sc->inoutID_x, readWrite, type, &sc->stageInvocationID);

			if (!recalculateAtEveryStep_inoutID)
				checkZeropadEnd_currentFFTAxis(sc, readWrite, type);

			if ((sc->fftDim.data.i != sc->fft_dim_full.data.i) && (!((sc->reorderFourStep) && (readWrite == 1)))) {
				PfIf_end(sc);
			}
			else {
				temp_int.data.i = (k * used_registers.data.i + i + 1) * sc->localSize[0].data.i * sc->localSize[1].data.i;

				temp_int1.data.i = fftDim.data.i * batching_localSize.data.i;

				if ((sc->mergeSequencesR2C) && (mult.data.i == 1))
					temp_int1.data.i *= 2;

				if (temp_int.data.i > temp_int1.data.i) {
					PfIf_end(sc);
				}
			}
			if (sc->fftDim.data.i == sc->fft_dim_full.data.i) {
				checkZeropadEnd_otherAxes(sc, 1);
				temp_int.data.i = batching_localSize.data.i;
				//we switched to reading 2x more data, but we still need to check out of bounds for odd size1
				if ((sc->mergeSequencesR2C) && (mult.data.i == 1))
					temp_int.data.i *= 2;
				if ((size1.data.i % temp_int.data.i) != 0) {
#if (VKFFT_BACKEND!=2) //AMD compiler fix
					if ((sc->mergeSequencesR2C) && (sc->size[1].data.i % 2) && (readWrite == 0)) {
					}
					else {
						PfIf_end(sc);
					}
#else
					if (readWrite != 0) {
						PfIf_end(sc);
					}
#endif
				}
			}
			else {
				if ((sc->reorderFourStep) && (readWrite == 1)) {
				}
				else {
					//PfIf_end(sc);
				}
			}
			}
		}

	if (sc->useDisableThreads) {
		PfIf_end(sc);
	}
	if ((sc->fftDim.data.i != sc->fft_dim_full.data.i) && ((sc->reorderFourStep) && (readWrite == 1))) {
		PfIf_end(sc);
	}

	return;
	}
static inline void appendReadWriteDataVkFFT_strided(VkFFTSpecializationConstantsLayout* sc, int readWrite, int type) {
	if (sc->res != VKFFT_SUCCESS) return;
	PfContainer temp_int = VKFFT_ZERO_INIT;
	temp_int.type = 31;

	PfContainer temp_int1 = VKFFT_ZERO_INIT;
	temp_int1.type = 31;

	PfContainer used_registers = VKFFT_ZERO_INIT;
	used_registers.type = 31;

	PfContainer* bufferStride = (readWrite) ? sc->outputStride : sc->inputStride;

	PfContainer* local_inoutID;
	if (sc->axis_id > 0)
		local_inoutID = &sc->inoutID_y;
	else
		local_inoutID = &sc->inoutID_x;

	int recalculateAtEveryStep_inoutID = 0;
	if ((sc->zeropad[readWrite]) || ((sc->numAxisUploads > 1) && (sc->zeropadBluestein[readWrite])) || ((type / 10) == 111) || ((type / 10) == 121) || ((type / 10) == 131) || ((type / 10) == 141) || ((type / 10) == 143) || ((type / 10) == 70) || ((type / 10) == 80)) {
		recalculateAtEveryStep_inoutID = 1;
	}
	if ((!sc->writeFromRegisters) && (readWrite == 1))
		appendBarrierVkFFT(sc, __FILE__, __LINE__);
	//char shiftX[500] = "";
	//if (&sc->performWorkGroupShift[0])
	//	sprintf(shiftX, " + consts.workGroupShiftX * %s ", &sc->gl_WorkGroupSize_x);
	PfContainer fftDim = VKFFT_ZERO_INIT;
	fftDim.type = 31;
	if (((sc->zeropad[readWrite]) || ((sc->zeropadBluestein[readWrite])) || (((type / 10) == 111)&&(sc->performDST == 1))) && (!readWrite)) {
		if (sc->readToRegisters == 0) {
			appendSetSMToZero(sc);
			appendBarrierVkFFT(sc, __FILE__, __LINE__);
		}
	}
	if (sc->zeropadBluestein[readWrite]) {
		if (sc->numAxisUploads == 1) {
			if (readWrite) {
				fftDim.data.i = sc->fft_zeropad_Bluestein_left_write[sc->axis_id].data.i;
			}
			else {
				fftDim.data.i = sc->fft_zeropad_Bluestein_left_read[sc->axis_id].data.i;
			}
			if ((type / 10) == 141) fftDim.data.i /= 2;
			if (((type / 10) == 111)&&(sc->performDCT == 1)) fftDim.data.i = 2 * fftDim.data.i - 2;
			if (((type / 10) == 111)&&(sc->performDST == 1)) fftDim.data.i = 2 * fftDim.data.i + 2;
		}
		else {
			fftDim.data.i = sc->fftDim.data.i;
		}
	}
	else
		fftDim.data.i = sc->fftDim.data.i;

	if (((type / 10) == 110) && (!(sc->zeropadBluestein[readWrite] && (sc->numAxisUploads == 1)))) {
		if(sc->performDST > 0)
			fftDim.data.i = (fftDim.data.i - 2) / 2;
		else
			fftDim.data.i = (fftDim.data.i + 2) / 2;
	}
	else if (((type/10) == 140) && (readWrite == 0)) {
		fftDim.data.i = 2 * fftDim.data.i;
	}
	if (readWrite == 0) {
		if (sc->performWorkGroupShift[0]) {
			PfMul(sc, &sc->shiftX, &sc->workGroupShiftX, &sc->gl_WorkGroupSize_x, 0);
			PfAdd(sc, &sc->shiftX, &sc->gl_GlobalInvocationID_x, &sc->shiftX);
		}
		else {
			PfMov(sc, &sc->shiftX, &sc->gl_GlobalInvocationID_x);
		}
		if (sc->axis_id == 0) {
			if (sc->size[1].data.i > 1) {
				if (sc->performWorkGroupShift[1]) {
					PfAdd(sc, &sc->shiftY, &sc->gl_WorkGroupID_y, &sc->workGroupShiftY);
					checkZeropad_otherAxes(sc, &sc->shiftY, 1);
					temp_int.data.i = sc->inputStride[1].data.i;
					PfMul(sc, &sc->shiftZ, &sc->shiftY, &temp_int, 0);
				}
				else
				{
					PfMov(sc, &sc->shiftY, &sc->gl_WorkGroupID_y);
					checkZeropad_otherAxes(sc, &sc->shiftY, 1);
					temp_int.data.i = sc->inputStride[1].data.i;
					PfMul(sc, &sc->shiftZ, &sc->shiftY, &temp_int, 0);
				}
			}
			else
			{
				PfSetToZero(sc, &sc->shiftZ);
			}
		}
		else {
			PfSetToZero(sc, &sc->shiftZ);
		}
		appendOffset(sc, readWrite, type);
		if (sc->axis_id > 0) {
			PfMod(sc, &sc->inoutID_x, &sc->shiftX, &sc->fft_dim_x);
			checkZeropad_otherAxes(sc, &sc->inoutID_x, 0);
			//&sc->tempIntLen = sprintf(&sc->tempIntStr, "		disableThreads = (((%s%s) / %" PRIu64 ") %% (%" PRIu64 ")+((%s%s) / %" PRIu64 ") * (%" PRIu64 ") < %" PRIu64 ") ? 1 : 0;\n", &sc->gl_GlobalInvocationID_x, shiftX, &sc->fft_dim_x, &sc->stageStartSize, &sc->gl_GlobalInvocationID_x, shiftX, &sc->fft_dim_x * &sc->stageStartSize, &sc->fftDim * &sc->stageStartSize, &sc->size[&sc->axis_id]);

			PfDiv(sc, &sc->tempInt2, &sc->shiftX, &sc->fft_dim_x);

			PfMod(sc, &sc->tempInt2, &sc->tempInt2, &sc->stageStartSize);

			PfMul(sc, &temp_int, &sc->fft_dim_x, &sc->stageStartSize, 0);
			PfDiv(sc, &sc->tempInt, &sc->shiftX, &temp_int); // disableThreads - tempInt3

			PfMul(sc, &temp_int, &fftDim, &sc->stageStartSize, 0);
			PfMul(sc, &sc->tempInt, &sc->tempInt, &temp_int, 0);

			PfAdd(sc, &sc->tempInt2, &sc->tempInt2, &sc->tempInt);

			if (sc->numAxisUploads > 1) {
				PfIf_lt_start(sc, &sc->tempInt2, &sc->fft_dim_full);
			}
			else {
				if ((type / 10) == 141) {
					temp_int.data.i = sc->sourceFFTSize.data.i / 2;
					PfIf_lt_start(sc, &sc->tempInt2, &temp_int);
				}else
					PfIf_lt_start(sc, &sc->tempInt2, &sc->sourceFFTSize);	
			}
		}
		else {
			//&sc->tempIntLen = sprintf(&sc->tempIntStr, "		disableThreads = (((%s%s) / %" PRIu64 ") * (%" PRIu64 ") < %" PRIu64 ") ? 1 : 0;\n", &sc->gl_GlobalInvocationID_x, shiftX, &sc->stageStartSize, &sc->stageStartSize * &sc->fftDim, &sc->fft_dim_full);
			PfDiv(sc, &sc->tempInt2, &sc->shiftX, &sc->stageStartSize);
			PfMul(sc, &temp_int, &sc->fftDim, &sc->stageStartSize, 0);
			PfMul(sc, &sc->tempInt2, &sc->tempInt2, &temp_int, 0);

			PfIf_lt_start(sc, &sc->tempInt2, &sc->fft_dim_full);
		}
		//PfIf_gt_start(sc, &sc->disableThreads, &temp_int1);
	}
	else {
		int isInputStrideNotEqualToOutput = 0;
        for (int i = 0; i < sc->numFFTdims+2; i++){
            if ((sc->inputStride[i].type>100)||(sc->outputStride[i].type>100)||(sc->inputStride[i].data.i != sc->outputStride[i].data.i)) {
                isInputStrideNotEqualToOutput = 1;
            }
        }
		if ((isInputStrideNotEqualToOutput) || (sc->performPostCompilationInputOffset) || (sc->performPostCompilationOutputOffset) || ((sc->inputOffset.data.i != sc->outputOffset.data.i) && (sc->inputOffset.type < 100) && (sc->outputOffset.type < 100)) || ((sc->convolutionStep) && (sc->matrixConvolution > 1)) || (sc->batchID.data.i > 0)) {
			if (sc->axis_id == 0) {
				if (sc->size[1].data.i > 1) {
					if (sc->performWorkGroupShift[1]) {
						PfAdd(sc, &sc->shiftY, &sc->gl_WorkGroupID_y, &sc->workGroupShiftY);
						temp_int.data.i = sc->outputStride[1].data.i;
						PfMul(sc, &sc->shiftZ, &sc->shiftY, &temp_int, 0);
					}
					else
					{
						PfMov(sc, &sc->shiftY, &sc->gl_WorkGroupID_y);
						temp_int.data.i = sc->outputStride[1].data.i;
						PfMul(sc, &sc->shiftZ, &sc->shiftY, &temp_int, 0);
					}
				}
				else {
					PfSetToZero(sc, &sc->shiftZ);
				}
			}
			else {
				PfSetToZero(sc, &sc->shiftZ);
			}

			appendOffset(sc, readWrite, type);
		}
		if ((sc->reorderFourStep) && (sc->stageStartSize.data.i == 1) && (sc->numAxisUploads > 1)) {
			PfDiv(sc, &sc->inoutID, &sc->shiftX, &sc->fft_dim_x);
			temp_int.data.i = sc->firstStageStartSize.data.i / fftDim.data.i;
			PfMod(sc, &sc->tempInt2, &sc->inoutID, &temp_int);
			temp_int.data.i = sc->fft_dim_full.data.i / sc->firstStageStartSize.data.i;
			PfMul(sc, &sc->tempInt2, &sc->tempInt2, &temp_int, 0);

			temp_int.data.i = sc->fft_dim_x.data.i * (sc->firstStageStartSize.data.i / fftDim.data.i);
			PfDiv(sc, &sc->tempInt, &sc->shiftX, &temp_int);

			PfAdd(sc, &sc->tempInt2, &sc->tempInt2, &sc->tempInt);
			//sc->tempLen = sprintf(sc->tempStr, "		if (((%s%s) / %" PRIu64 ") %% (%" PRIu64 ")+((%s%s) / %" PRIu64 ") * (%" PRIu64 ") < %" PRIu64 ") {\n", sc->gl_GlobalInvocationID_x, shiftX, sc->fft_dim_x, sc->stageStartSize, sc->gl_GlobalInvocationID_x, shiftX, sc->fft_dim_x * sc->stageStartSize, sc->fftDim * sc->stageStartSize, sc->size[sc->axis_id]);
			PfMod(sc, &sc->tempInt, &sc->inoutID, &sc->stageStartSize);
			temp_int.data.i = sc->fft_dim_x.data.i * sc->stageStartSize.data.i;
			PfDiv(sc, &sc->inoutID, &sc->shiftX, &temp_int);
			temp_int.data.i = fftDim.data.i * sc->stageStartSize.data.i;
			PfMul(sc, &sc->inoutID, &sc->inoutID, &temp_int, 0);
			PfAdd(sc, &sc->tempInt, &sc->tempInt, &sc->inoutID);
			PfIf_lt_start(sc, &sc->tempInt, &sc->fft_dim_full);
		}
		else {
			if (sc->axis_id > 0) {
				if (sc->numAxisUploads > 1) {
					PfIf_lt_start(sc, &sc->tempInt2, &sc->fft_dim_full);
				}
				else {
					if ((type / 10) == 141) {
						temp_int.data.i = sc->sourceFFTSize.data.i / 2;
						PfIf_lt_start(sc, &sc->tempInt2, &temp_int);
					}else
						PfIf_lt_start(sc, &sc->tempInt2, &sc->sourceFFTSize);					
				}
			}
			else {
				PfIf_lt_start(sc, &sc->tempInt2, &sc->fft_dim_full);
			}
		}
	}

	if (sc->useDisableThreads) {
		temp_int.data.i = 0;
		PfIf_gt_start(sc, &sc->disableThreads, &temp_int);
	}

	PfDivCeil(sc, &used_registers, &fftDim, &sc->localSize[1]);

	if (sc->registerBoost > 1) {
		temp_int.data.i = sc->registerBoost;
		PfDiv(sc, &used_registers, &used_registers, &temp_int);
	}
	if (sc->axis_id > 0) {
		//&sc->tempIntLen = sprintf(&sc->tempIntStr, "		inoutID = (%" PRIu64 " * (%s + %" PRIu64 ") + ((%s%s) / %" PRIu64 ") %% (%" PRIu64 ")+((%s%s) / %" PRIu64 ") * (%" PRIu64 "));\n", &sc->stageStartSize, &sc->gl_LocalInvocationID_y, (i + k * used_registers) * &sc->localSize[1], &sc->gl_GlobalInvocationID_x, shiftX, &sc->fft_dim_x, &sc->stageStartSize, &sc->gl_GlobalInvocationID_x, shiftX, &sc->fft_dim_x * &sc->stageStartSize, &sc->fftDim * &sc->stageStartSize);
		if ((readWrite == 1) && (sc->reorderFourStep) && (sc->stageStartSize.data.i == 1) && (sc->numAxisUploads > 1)) {
			temp_int1.data.i = sc->fft_dim_full.data.i / fftDim.data.i;
			PfMul(sc, &sc->inoutID_y, &sc->gl_LocalInvocationID_y, &temp_int1, 0);
			PfAdd(sc, &sc->inoutID_y, &sc->inoutID_y, &sc->tempInt2);
		}
		else {
			PfMul(sc, &sc->inoutID_y, &sc->gl_LocalInvocationID_y, &sc->stageStartSize, 0);
			PfAdd(sc, &sc->inoutID_y, &sc->inoutID_y, &sc->tempInt2);
		}

	}
	else {
		//&sc->tempIntLen = sprintf(&sc->tempIntStr, "		inoutID = (%s%s) %% (%" PRIu64 ") + %" PRIu64 " * (%s + %" PRIu64 ") + ((%s%s) / %" PRIu64 ") * (%" PRIu64 ");\n", &sc->gl_GlobalInvocationID_x, shiftX, &sc->stageStartSize, &sc->stageStartSize, &sc->gl_LocalInvocationID_y, (i + k * used_registers) * &sc->localSize[1], &sc->gl_GlobalInvocationID_x, shiftX, &sc->stageStartSize, &sc->stageStartSize * &sc->fftDim);
		PfMod(sc, &sc->inoutID_x, &sc->shiftX, &sc->stageStartSize);

		PfMul(sc, &sc->tempInt, &sc->gl_LocalInvocationID_y, &sc->stageStartSize, 0);

		PfAdd(sc, &sc->inoutID_x, &sc->inoutID_x, &sc->tempInt);

		PfAdd(sc, &sc->inoutID_x, &sc->inoutID_x, &sc->tempInt2);
	}
	if (!recalculateAtEveryStep_inoutID) {
		PfMov(sc, &sc->inoutID, &sc->inoutID_x);

		if (sc->axis_id > 0) {
			PfMul(sc, &sc->tempInt, &sc->inoutID_y, &bufferStride[1], 0);
			PfAdd(sc, &sc->inoutID, &sc->inoutID, &sc->tempInt);
		}
		PfAdd(sc, &sc->inoutID, &sc->inoutID, &sc->shiftZ);
	}
	//for (pfUINT k = 0; k < &sc->registerBoost; k++) {
	for (int k = 0; k < sc->registerBoost; k++) {
		//for (pfUINT i = 0; i < used_registers; i++) {
		for (int i = 0; i < used_registers.data.i; i++) {

			temp_int1.data.i = (k * used_registers.data.i + i + 1) * sc->localSize[1].data.i;

			if (temp_int1.data.i > fftDim.data.i) {
				//&sc->tempIntLen = sprintf(&sc->tempIntStr, "		if(%s < %" PRIu64 "){\n", &sc->gl_LocalInvocationID_y, &sc->fftDim - (i + k * used_registers) * &sc->localSize[1]);
				temp_int1.data.i = sc->localSize[1].data.i - (temp_int1.data.i - fftDim.data.i);
				PfIf_lt_start(sc, &sc->gl_LocalInvocationID_y, &temp_int1);
			}

			/*if (bufferStride[0].data.i != 1)
				PfMul(sc, &sc->inoutID, &sc->inoutID_x, &bufferStride[0], 0);
			else
				PfMov(sc, &sc->inoutID, &sc->inoutID_x);

			if (sc->axis_id > 0) {
				PfMul(sc, &sc->tempInt, &sc->inoutID_y, &bufferStride[1], 0);
				PfAdd(sc, &sc->inoutID, &sc->inoutID, &sc->tempInt);
			}

			PfAdd(sc, &sc->inoutID, &sc->inoutID, &sc->shiftZ);*/
			if (recalculateAtEveryStep_inoutID) {
				if ((i > 0) && (k == 0)) {
					if (sc->axis_id > 0) {
						//&sc->tempIntLen = sprintf(&sc->tempIntStr, "		inoutID = (%" PRIu64 " * (%s + %" PRIu64 ") + ((%s%s) / %" PRIu64 ") %% (%" PRIu64 ")+((%s%s) / %" PRIu64 ") * (%" PRIu64 "));\n", &sc->stageStartSize, &sc->gl_LocalInvocationID_y, (i + k * used_registers) * &sc->localSize[1], &sc->gl_GlobalInvocationID_x, shiftX, &sc->fft_dim_x, &sc->stageStartSize, &sc->gl_GlobalInvocationID_x, shiftX, &sc->fft_dim_x * &sc->stageStartSize, &sc->fftDim * &sc->stageStartSize);
						if ((readWrite == 1) && (sc->reorderFourStep) && (sc->stageStartSize.data.i == 1) && (sc->numAxisUploads > 1)) {
							temp_int1.data.i = sc->fft_dim_full.data.i / fftDim.data.i * sc->localSize[1].data.i;
							PfAdd(sc, &sc->inoutID_y, &sc->inoutID_y, &temp_int1);
						}
						else {
							temp_int1.data.i = sc->stageStartSize.data.i * sc->localSize[1].data.i;
							PfAdd(sc, &sc->inoutID_y, &sc->inoutID_y, &temp_int1);
						}

					}
					else {
						//&sc->tempIntLen = sprintf(&sc->tempIntStr, "		inoutID = (%s%s) %% (%" PRIu64 ") + %" PRIu64 " * (%s + %" PRIu64 ") + ((%s%s) / %" PRIu64 ") * (%" PRIu64 ");\n", &sc->gl_GlobalInvocationID_x, shiftX, &sc->stageStartSize, &sc->stageStartSize, &sc->gl_LocalInvocationID_y, (i + k * used_registers) * &sc->localSize[1], &sc->gl_GlobalInvocationID_x, shiftX, &sc->stageStartSize, &sc->stageStartSize * &sc->fftDim);
						temp_int1.data.i = sc->stageStartSize.data.i * sc->localSize[1].data.i;
						PfAdd(sc, &sc->inoutID_x, &sc->inoutID_x, &temp_int1);
					}
				}
				
				append_inoutID_preprocessing_multiupload_R2R(sc, local_inoutID, readWrite, type, &sc->stageInvocationID, &sc->sdataID);
				append_inoutID_preprocessing_multiupload_R2C(sc, local_inoutID, readWrite, type, &sc->stageInvocationID, &sc->sdataID);

				PfMov(sc, &sc->inoutID, &sc->inoutID_x);

				if (sc->axis_id > 0) {
					PfMul(sc, &sc->tempInt, &sc->inoutID_y, &bufferStride[1], 0);
					PfAdd(sc, &sc->inoutID, &sc->inoutID, &sc->tempInt);
				}
				PfAdd(sc, &sc->inoutID, &sc->inoutID, &sc->shiftZ);
			}
			else {
				if ((i > 0) || (k > 0)) {
					if (sc->axis_id > 0) {
						//&sc->tempIntLen = sprintf(&sc->tempIntStr, "		inoutID = (%" PRIu64 " * (%s + %" PRIu64 ") + ((%s%s) / %" PRIu64 ") %% (%" PRIu64 ")+((%s%s) / %" PRIu64 ") * (%" PRIu64 "));\n", &sc->stageStartSize, &sc->gl_LocalInvocationID_y, (i + k * used_registers) * &sc->localSize[1], &sc->gl_GlobalInvocationID_x, shiftX, &sc->fft_dim_x, &sc->stageStartSize, &sc->gl_GlobalInvocationID_x, shiftX, &sc->fft_dim_x * &sc->stageStartSize, &sc->fftDim * &sc->stageStartSize);
						if ((readWrite == 1) && (sc->reorderFourStep) && (sc->stageStartSize.data.i == 1) && (sc->numAxisUploads > 1)) {
							temp_int1.data.i = sc->fft_dim_full.data.i / fftDim.data.i * bufferStride[1].data.i * sc->localSize[1].data.i;
							PfAdd(sc, &sc->inoutID, &sc->inoutID, &temp_int1);
						}
						else {
							temp_int1.data.i = sc->stageStartSize.data.i * bufferStride[1].data.i * sc->localSize[1].data.i;
							PfAdd(sc, &sc->inoutID, &sc->inoutID, &temp_int1);
						}

					}
					else {
						//&sc->tempIntLen = sprintf(&sc->tempIntStr, "		inoutID = (%s%s) %% (%" PRIu64 ") + %" PRIu64 " * (%s + %" PRIu64 ") + ((%s%s) / %" PRIu64 ") * (%" PRIu64 ");\n", &sc->gl_GlobalInvocationID_x, shiftX, &sc->stageStartSize, &sc->stageStartSize, &sc->gl_LocalInvocationID_y, (i + k * used_registers) * &sc->localSize[1], &sc->gl_GlobalInvocationID_x, shiftX, &sc->stageStartSize, &sc->stageStartSize * &sc->fftDim);
						temp_int1.data.i = sc->stageStartSize.data.i * sc->localSize[1].data.i;
						PfAdd(sc, &sc->inoutID, &sc->inoutID, &temp_int1);
					}
				}
			}
			temp_int.data.i = (k * used_registers.data.i + i) * sc->localSize[1].data.i;

			if ((sc->readToRegisters && (readWrite == 0)) || (sc->writeFromRegisters && (readWrite == 1))) {
				//if (&sc->inputBufferBlockNum == 1)
				//	&sc->tempIntLen = sprintf(&sc->tempIntStr, "			%s=%s%s[%s]%s;\n", &sc->regIDs[i + k * &sc->registers_per_thread], convTypeLeft, &inputsStruct, &sc->inoutID, convTypeRight);
				//else
				//	&sc->tempIntLen = sprintf(&sc->tempIntStr, "			%s=%sinputBlocks[%s / %" PRIu64 "]%s[%s %% %" PRIu64 "]%s;\n", &sc->regIDs[i + k * &sc->registers_per_thread], convTypeLeft, &sc->inoutID, &sc->inputBufferBlockSize, &inputsStruct, &sc->inoutID, &sc->inputBufferBlockSize, convTypeRight);
			}
			else {
				//if (&sc->inputBufferBlockNum == 1)
				//	&sc->tempIntLen = sprintf(&sc->tempIntStr, "			sdata[%s*(%s+%" PRIu64 ")+%s]=%s%s[%s]%s;\n", &sc->sharedStride, &sc->gl_LocalInvocationID_y, (i + k * used_registers) * &sc->localSize[1], &sc->gl_LocalInvocationID_x, convTypeLeft, &inputsStruct, &sc->inoutID, convTypeRight);
				//else
				//	&sc->tempIntLen = sprintf(&sc->tempIntStr, "			sdata[%s*(%s+%" PRIu64 ")+%s]=%sinputBlocks[%s / %" PRIu64 "]%s[%s %% %" PRIu64 "]%s;\n", &sc->sharedStride, &sc->gl_LocalInvocationID_y, (i + k * used_registers) * &sc->localSize[1], &sc->gl_LocalInvocationID_x, convTypeLeft, &sc->inoutID, &sc->inputBufferBlockSize, &inputsStruct, &sc->inoutID, &sc->inputBufferBlockSize, convTypeRight);
				PfAdd(sc, &sc->sdataID, &sc->gl_LocalInvocationID_y, &temp_int);
				if (((type/10) == 140) && (!sc->readToRegisters) && (readWrite == 0)) {
					temp_int1.data.i = 2;
					PfDiv(sc, &sc->sdataID, &sc->sdataID, &temp_int1);
				}
				PfMul(sc, &sc->sdataID, &sc->sharedStride, &sc->sdataID, 0);
				PfAdd(sc, &sc->sdataID, &sc->sdataID, &sc->gl_LocalInvocationID_x);
				if ((sc->performDST == 1) && (!sc->forceCallbackVersionRealTransforms))
					PfAdd(sc, &sc->sdataID, &sc->sdataID, &sc->sharedStride);
			}

			temp_int1.data.i = k * used_registers.data.i + i;

			if (readWrite == 0) {
				if ((sc->inputMemoryCode % 10) == 2) {
					if (((type/10) == 70) || ((type/10) == 110) || ((type/10) == 111) || ((type/10) == 120) || ((type/10) == 121) || ((type/10) == 130) || ((type/10) == 131) || ((type/10) == 142) || ((type/10) == 143)) {
						if (sc->readToRegisters) {
							if (recalculateAtEveryStep_inoutID)
								checkZeropadStart_currentFFTAxis(sc, readWrite, type, local_inoutID);

							appendGlobalToRegisters_x(sc, &sc->regIDs[temp_int1.data.i], &sc->inputsStruct, &sc->inoutID);
							if (recalculateAtEveryStep_inoutID)
								checkZeropadEnd_currentFFTAxis(sc, readWrite, type);

							if (((((sc->performDCT == 3) || (sc->performDST == 3)) && (sc->actualInverse == 0)) || (((sc->performDCT == 2) || (sc->performDST == 2)) && (sc->actualInverse == 1))) && ((type / 10) == 131)) {
								if (recalculateAtEveryStep_inoutID) {
									checkZeropadStart_currentFFTAxis(sc, readWrite, type, &sc->inoutID2);
									if (sc->axis_id > 0) {
										PfMul(sc, &sc->inoutID2, &sc->inoutID2, &bufferStride[1], 0);
										PfAdd(sc, &sc->inoutID2, &sc->inoutID2, &sc->inoutID_x);
										PfAdd(sc, &sc->inoutID2, &sc->inoutID2, &sc->shiftZ);
									}
									else {
										PfAdd(sc, &sc->inoutID2, &sc->inoutID2, &sc->shiftZ);
									}
								}
								appendGlobalToRegisters_y(sc, &sc->regIDs[temp_int1.data.i], &sc->inputsStruct, &sc->inoutID2);
								if (recalculateAtEveryStep_inoutID)
									checkZeropadEnd_currentFFTAxis(sc, readWrite, type);
							}
							append_processing_multiupload_R2R(sc, local_inoutID, &sc->regIDs[temp_int1.data.i], readWrite, type, 0, 0);
							
						}
						else {
							PfSetToZero(sc, &sc->temp);
							if (recalculateAtEveryStep_inoutID)
								checkZeropadStart_currentFFTAxis(sc, readWrite, type, local_inoutID);
							appendGlobalToRegisters_x(sc, &sc->temp, &sc->inputsStruct, &sc->inoutID);
							if (recalculateAtEveryStep_inoutID)
								checkZeropadEnd_currentFFTAxis(sc, readWrite, type);
							if (((((sc->performDCT == 3) || (sc->performDST == 3)) && (sc->actualInverse == 0)) || (((sc->performDCT == 2) || (sc->performDST == 2)) && (sc->actualInverse == 1))) && ((type / 10) == 131)) {
								if (recalculateAtEveryStep_inoutID) {
									checkZeropadStart_currentFFTAxis(sc, readWrite, type, &sc->inoutID2);
									if (sc->axis_id > 0) {
										PfMul(sc, &sc->inoutID2, &sc->inoutID2, &bufferStride[1], 0);
										PfAdd(sc, &sc->inoutID2, &sc->inoutID2, &sc->inoutID_x);
										PfAdd(sc, &sc->inoutID2, &sc->inoutID2, &sc->shiftZ);
									}
									else {
										PfAdd(sc, &sc->inoutID2, &sc->inoutID2, &sc->shiftZ);
									}
								}
								appendGlobalToRegisters_y(sc, &sc->temp, &sc->inputsStruct, &sc->inoutID2);
								if (recalculateAtEveryStep_inoutID)
									checkZeropadEnd_currentFFTAxis(sc, readWrite, type);
							}
							append_processing_multiupload_R2R(sc, local_inoutID, &sc->temp, readWrite, type, 0, 0);
							appendRegistersToShared(sc, &sc->sdataID, &sc->temp);
						}
					}
					else  if ((type/10) == 140) {
						if (recalculateAtEveryStep_inoutID)
							checkZeropadStart_currentFFTAxis(sc, readWrite, type, local_inoutID);
						if (sc->readToRegisters) {
							if (i < used_registers.data.i / 2) {
								appendGlobalToRegisters_x(sc, &sc->regIDs[k * sc->registers_per_thread + i], &sc->inputsStruct, &sc->inoutID);
							}
							else {
								appendGlobalToRegisters_y(sc, &sc->regIDs[k * sc->registers_per_thread + i - used_registers.data.i / 2], &sc->inputsStruct, &sc->inoutID);
							}
						}
						else {
							PfAdd(sc, &sc->combinedID, &sc->gl_LocalInvocationID_y, &temp_int);
							temp_int.data.i = 2;
							PfMod(sc, &sc->tempInt, &sc->combinedID, &temp_int);

							appendGlobalToRegisters_x(sc, &sc->temp, &sc->inputsStruct, &sc->inoutID);

							temp_int.data.i = 0;
							PfIf_eq_start(sc, &sc->tempInt, &temp_int);
							appendRegistersToShared_x_x(sc, &sc->sdataID, &sc->temp);
							PfIf_else(sc);
							appendRegistersToShared_y_x(sc, &sc->sdataID, &sc->temp);
							PfIf_end(sc);
						}
						if (recalculateAtEveryStep_inoutID)
							checkZeropadEnd_currentFFTAxis(sc, readWrite, type);
					}
					else  if ((type / 10) == 141) {
						if (recalculateAtEveryStep_inoutID)
							checkZeropadStart_currentFFTAxis(sc, readWrite, type, local_inoutID);

						appendGlobalToRegisters_x(sc, &sc->regIDs[k * sc->registers_per_thread + i], &sc->inputsStruct, &sc->inoutID);
								
						if (recalculateAtEveryStep_inoutID)
							checkZeropadEnd_currentFFTAxis(sc, readWrite, type);

						if (recalculateAtEveryStep_inoutID) {
							checkZeropadStart_currentFFTAxis(sc, readWrite, type, &sc->inoutID2);
									
							if (sc->axis_id > 0) {
								PfMul(sc, &sc->inoutID2, &sc->inoutID2, &bufferStride[1], 0);
								PfAdd(sc, &sc->inoutID2, &sc->inoutID2, &sc->inoutID_x);
								PfAdd(sc, &sc->inoutID2, &sc->inoutID2, &sc->shiftZ);
							}
							else {
								PfAdd(sc, &sc->inoutID2, &sc->inoutID2, &sc->shiftZ);
							}
						}
						appendGlobalToRegisters_y(sc, &sc->regIDs[k * sc->registers_per_thread + i], &sc->inputsStruct, &sc->inoutID2);

						if (recalculateAtEveryStep_inoutID)
							checkZeropadEnd_currentFFTAxis(sc, readWrite, type);
						append_processing_multiupload_R2R(sc, local_inoutID, &sc->regIDs[k * sc->registers_per_thread + i], readWrite, type, 0, 0);
						if (!sc->readToRegisters) {
							appendRegistersToShared(sc, &sc->sdataID, &sc->regIDs[k * sc->registers_per_thread + i]);
						}
					}
				}
				else {
					if (recalculateAtEveryStep_inoutID)
						checkZeropadStart_currentFFTAxis(sc, readWrite, type, local_inoutID);
						
					if (sc->readToRegisters) {
						appendGlobalToRegisters(sc, &sc->regIDs[temp_int1.data.i], &sc->inputsStruct, &sc->inoutID);
						append_inoutID_processing_multiupload_R2C(sc, &sc->inoutID_x, &sc->regIDs[temp_int1.data.i], readWrite, type, &sc->stageInvocationID, 0);				
					}
					else {
						if (sc->performR2C) {
							appendGlobalToRegisters(sc, &sc->temp, &sc->inputsStruct, &sc->inoutID);
							append_inoutID_processing_multiupload_R2C(sc, &sc->inoutID_x, &sc->temp, readWrite, type, &sc->stageInvocationID, 0);	
							appendRegistersToShared(sc, &sc->sdataID, &sc->temp);
						}
						else
							appendGlobalToShared(sc, &sc->sdataID, &sc->inputsStruct, &sc->inoutID);
					}

					if (recalculateAtEveryStep_inoutID)
						checkZeropadEnd_currentFFTAxis(sc, readWrite, type);
				}
				if (recalculateAtEveryStep_inoutID) {
					append_inoutID_postprocessing_multiupload_R2R(sc, local_inoutID, readWrite, type, &sc->stageInvocationID);
					append_inoutID_postprocessing_multiupload_R2C(sc, local_inoutID, readWrite, type, &sc->stageInvocationID);
				}
			}
			else {
				if ((sc->outputMemoryCode % 10) == 2) {
					if ((type / 10) == 111) {
						if (sc->zeropadBluestein[readWrite]) {
							if (readWrite) {
								temp_int.data.i = sc->fft_zeropad_Bluestein_left_write[sc->axis_id].data.i;
							}
							else {
								temp_int.data.i = sc->fft_zeropad_Bluestein_left_read[sc->axis_id].data.i;
							}
						}
						else {
							temp_int.data.i = sc->fft_dim_full.data.i;
							if(sc->performDCT)
								temp_int.data.i = (temp_int.data.i+2)/2;
							if(sc->performDST)
								temp_int.data.i = (temp_int.data.i-2)/2;
						}
						PfIf_lt_start(sc, local_inoutID, &temp_int);
					}
					if (((type/10) == 80) || ((type/10) == 110) || ((type/10) == 111) || ((type/10) == 120) || ((type/10) == 121) || ((type/10) == 130) || ((type/10) == 131) || ((type/10) == 142) || ((type/10) == 143)) {
						if (sc->writeFromRegisters) {
							append_processing_multiupload_R2R(sc, local_inoutID, &sc->regIDs[temp_int1.data.i], readWrite, type, 0, 0);
							if (recalculateAtEveryStep_inoutID)
								checkZeropadStart_currentFFTAxis(sc, readWrite, type, local_inoutID);
					
							if (sc->performDST == 1) {
								PfMovNeg(sc, &sc->regIDs[temp_int1.data.i].data.c[1], &sc->regIDs[temp_int1.data.i].data.c[1]);
								appendRegistersToGlobal_y(sc, &sc->outputsStruct, &sc->inoutID, &sc->regIDs[temp_int1.data.i]);
							}
							else
								appendRegistersToGlobal_x(sc, &sc->outputsStruct, &sc->inoutID, &sc->regIDs[temp_int1.data.i]);

							if (recalculateAtEveryStep_inoutID)
								checkZeropadEnd_currentFFTAxis(sc, readWrite, type);
						}
						else {
							appendSharedToRegisters(sc, &sc->temp, &sc->sdataID);
							append_processing_multiupload_R2R(sc, local_inoutID, &sc->temp, readWrite, type, 0, 0);
							if (recalculateAtEveryStep_inoutID)
								checkZeropadStart_currentFFTAxis(sc, readWrite, type, local_inoutID);
					
							if (sc->performDST == 1) {
								PfMovNeg(sc, &sc->temp.data.c[1], &sc->temp.data.c[1]);
								appendRegistersToGlobal_y(sc, &sc->outputsStruct, &sc->inoutID, &sc->temp);
							}
							else
								appendRegistersToGlobal_x(sc, &sc->outputsStruct, &sc->inoutID, &sc->temp);

							if (recalculateAtEveryStep_inoutID)
								checkZeropadEnd_currentFFTAxis(sc, readWrite, type);
						}
					}
					else  if (((type/10) == 140) || ((type/10) == 141)) {
						if (!sc->writeFromRegisters) {
							appendSharedToRegisters(sc, &sc->regIDs[k * sc->registers_per_thread + i], &sc->sdataID);
						}
						if (recalculateAtEveryStep_inoutID)
							checkZeropadStart_currentFFTAxis(sc, readWrite, type, local_inoutID);
						append_processing_multiupload_R2R(sc, local_inoutID, &sc->regIDs[k * sc->registers_per_thread + i], readWrite, type, 0, 0);

						if ((sc->fftDim.data.i == 1) && (sc->normalize) && (sc->actualInverse)){ //workaround for DCT/DST-IV of size 2 that has no FFT stages (where normalization typically happens). 
							PfContainer temp_double = VKFFT_ZERO_INIT;
							temp_double.type = 22;
							temp_double.data.d = pfFPinit("0.25");
							PfMul(sc, &sc->regIDs[k * sc->registers_per_thread + i], &sc->regIDs[k * sc->registers_per_thread + i], &temp_double, 0);
						}

						appendRegistersToGlobal_x(sc, &sc->outputsStruct, &sc->inoutID, &sc->regIDs[k * sc->registers_per_thread + i]);
						if (recalculateAtEveryStep_inoutID)
							checkZeropadEnd_currentFFTAxis(sc, readWrite, type);

						if (sc->zeropadBluestein[readWrite]) {
							if (readWrite) {
								temp_int.data.i = sc->fft_zeropad_Bluestein_left_write[sc->axis_id].data.i;
							}
							else {
								//appendSetSMToZero(sc);
								temp_int.data.i = sc->fft_zeropad_Bluestein_left_read[sc->axis_id].data.i;
							}
							if ((type / 10) == 141) temp_int.data.i /= 2;
						}
						else {
							temp_int.data.i = sc->fft_dim_full.data.i;
						}

						if (recalculateAtEveryStep_inoutID) {
							if ((type / 10) == 141) {
								temp_int.data.i = 2 * temp_int.data.i - 1;
								PfSub(sc, &sc->sdataID, &temp_int, local_inoutID);
							}
							else
								PfAdd(sc, &sc->sdataID, local_inoutID, &temp_int);
							checkZeropadStart_currentFFTAxis(sc, readWrite, type, &sc->sdataID);
							if (sc->axis_id > 0) {
								PfMul(sc, &sc->tempInt, &sc->sdataID, &bufferStride[1], 0);
								PfAdd(sc, &sc->tempInt, &sc->tempInt, &sc->inoutID_x);

								PfAdd(sc, &sc->tempInt, &sc->tempInt, &sc->shiftZ);
							}
							else {
								PfAdd(sc, &sc->tempInt, &sc->sdataID, &sc->shiftZ);
							}
						}
						else {
							temp_int.data.i = fftDim.data.i * bufferStride[1].data.i;
							PfAdd(sc, &sc->tempInt, &sc->inoutID, &temp_int);
						}
						appendRegistersToGlobal_y(sc, &sc->outputsStruct, &sc->tempInt, &sc->regIDs[k * sc->registers_per_thread + i]);
						if (recalculateAtEveryStep_inoutID)
							checkZeropadEnd_currentFFTAxis(sc, readWrite, type);
					}
					if ((type / 10) == 111) {
						PfIf_end(sc);
					}
				}
				else {
					if (recalculateAtEveryStep_inoutID)
						checkZeropadStart_currentFFTAxis(sc, readWrite, type, local_inoutID);

					if (sc->writeFromRegisters) {
						appendRegistersToGlobal(sc, &sc->outputsStruct, &sc->inoutID, &sc->regIDs[temp_int1.data.i]);
					}
					else {
						appendSharedToGlobal(sc, &sc->outputsStruct, &sc->inoutID, &sc->sdataID);
					}

					if (recalculateAtEveryStep_inoutID)
						checkZeropadEnd_currentFFTAxis(sc, readWrite, type);
				}
				if (recalculateAtEveryStep_inoutID) {
					append_inoutID_postprocessing_multiupload_R2R(sc, local_inoutID, readWrite, type, &sc->stageInvocationID);
					append_inoutID_postprocessing_multiupload_R2C(sc, local_inoutID, readWrite, type, &sc->stageInvocationID);
				}
			}
			
			temp_int1.data.i = (k * used_registers.data.i + i + 1) * sc->localSize[1].data.i;
			if (temp_int1.data.i > fftDim.data.i) {
				//&sc->tempIntLen = sprintf(&sc->tempIntStr, "		}\n");
				PfIf_end(sc);
			}
		}
	}
	//&sc->tempIntLen = sprintf(&sc->tempIntStr, "	}\n");

	if (sc->useDisableThreads) {
		temp_int.data.i = 0;
		PfIf_end(sc);
	}

	PfIf_end(sc);
	return;
}

static inline void appendReadDataVkFFT(VkFFTSpecializationConstantsLayout* sc, int type) {
	if (sc->res != VKFFT_SUCCESS) return;
	switch (type % 10) {
	case 0: 
		appendReadWriteDataVkFFT_nonstrided(sc, 0, type);
		break;
	case 1: case 2: //grouped_c2c + single_c2c_strided
		appendReadWriteDataVkFFT_strided(sc, 0, type);
		break;
	}
	return;
}
static inline void appendWriteDataVkFFT(VkFFTSpecializationConstantsLayout* sc, int type) {
	if (sc->res != VKFFT_SUCCESS) return;
	switch (type % 10) {
	case 0: 
		appendReadWriteDataVkFFT_nonstrided(sc, 1, type);
		break;
	case 1: case 2: //grouped_c2c + single_c2c_strided
		appendReadWriteDataVkFFT_strided(sc, 1, type);
		break;
	}
	return;
}
#endif
