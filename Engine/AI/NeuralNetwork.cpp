#include "NeuralNetwork.h"

#include "Math/Base.h"
#include "CompilerConfig.h"
#include "MarcoCommon.h"


#define REORDER_WEIGHT 1

#if CPP_COMPILER_MSVC
#define ALLOCA _alloca
#else
#define ALLOCA alloca
#endif

#define USE_DUFF_DEVICE 0

#if USE_DUFF_DEVICE
#include "Misc/DuffDevice.h"
#endif

void FNNMath::VectorAdd(int dim, NNScalar* RESTRICT a, NNScalar const* RESTRICT b)
{
#if USE_DUFF_DEVICE
#define OP	*a += *b; ++a; ++b;
	DUFF_DEVICE_8(dim, OP);
#undef OP
#else
	for (int i = 0; i < dim; ++i)
	{
		a[i] += b[i];
	}
#endif
}

void FNNMath::VectorAdd(int dim, NNScalar const* RESTRICT a, NNScalar const* RESTRICT b, NNScalar* RESTRICT out)
{
#if USE_DUFF_DEVICE
#define OP	*out = *a + *b; ++out; ++a; ++b;
	DUFF_DEVICE_8(dim, OP);
#undef OP
#else
	for (int i = 0; i < dim; ++i)
	{
		out[i] = a[i] + b[i];
	}
#endif
}

void FNNMath::VectorSub(int dim, NNScalar const* RESTRICT a, NNScalar const* RESTRICT b, NNScalar* RESTRICT out)
{
	for (int i = 0; i < dim; ++i)
	{
		out[i] = a[i] - b[i];
	}
}

void FNNMath::VectorMul(int dim, NNScalar* RESTRICT a, NNScalar const* RESTRICT b)
{
	for (int i = 0; i < dim; ++i)
	{
		a[i] *= b[i];
	}
}

NNScalar FNNMath::VectorDot(int dim, NNScalar const* RESTRICT a, NNScalar const* RESTRICT b)
{
#if USE_MATH_SIMD  && 0
	if constexpr (Meta::IsSameType< NNScalar, float >::Value)
	{
		NNScalar result = 0;
		using namespace SIMD;
		int numPackedDim = dim / 4;

		SScalar vr = SScalar(0.0f);
		for (; numPackedDim; --numPackedDim)
		{
			SVector4 va = SVector4(a);
			SVector4 vb = SVector4(b);
			vr = vr + va.dot(vb);
			a += 4;
			b += 4;
		}

		switch (dim % 4)
		{
		case 1: vr = vr + SScalar(a[0]) * SScalar(b[0]); break;
		case 2: 
			{
				SVector2 va = SVector2(a);
				SVector2 vb = SVector2(b);
				vr = vr + va.dot(vb);
			}
			break;
		case 3:
			{
				SVector3 va = SVector3(a);
				SVector3 vb = SVector3(b);
				vr = vr + va.dot(vb);
			}
			break;
		}
		result = vr;

		return result;
	}
	else
#endif
	{
		return VectorDotNOP(dim, a, b);
	}
}


NNScalar FNNMath::VectorDot(int dim, NNScalar const* RESTRICT a, NNScalar const* RESTRICT b, int bStride)
{
	NNScalar result = 0;
#if USE_DUFF_DEVICE
#define OP	result += *a * *b; ++a; b += bStride;
	DUFF_DEVICE_8(dim, OP);
#undef OP
#else
	NNScalar const* RESTRICT pA = a;
	NNScalar const* RESTRICT pB = b;
	for (; dim; --dim)
	{
		result += (*pA) * (*pB);
		++pA;
		pB += bStride;
	}
#endif
	return result;
}

NNScalar FNNMath::VectorDot(int dim, NNScalar const* RESTRICT a, int aStride, NNScalar const* RESTRICT b, int bStride)
{
	NNScalar result = 0;
	for (; dim; --dim)
	{
		result += (*a) * (*b);
		a += aStride;
		b += bStride;
	}
	return result;
}

NNScalar FNNMath::VectorDotNOP(int dim, NNScalar const* RESTRICT a, NNScalar const* RESTRICT b)
{
	NNScalar result = 0;
#if USE_DUFF_DEVICE
#define OP	result += (*a++) * (*b++);
	DUFF_DEVICE_8(dim, OP);
#undef OP
#else
	for (; dim; --dim)
	{
		result += (*a++) * (*b++);
	}
#endif
	return result;
}

void FNNMath::MatrixMulAddVector(int dimRow, int dimCol, NNScalar const* RESTRICT m, NNScalar const* RESTRICT v, NNScalar const* RESTRICT b, NNScalar* RESTRICT out)
{
#if USE_DUFF_DEVICE
#define OP	*out = *b + VectorDot(dimCol, m, v); ++out; ++b; m += dimCol;
	DUFF_DEVICE_8(dimRow, OP);
#undef OP
#else
	for (int row = 0; row < dimRow; ++row)
	{
		out[row] = VectorDot(dimCol, m, v) + b[row];
		m += dimCol;
	}
#endif
}

void FNNMath::VectorMulMatrixAdd(int dimRow, int dimCol, NNScalar const* RESTRICT m, NNScalar const* RESTRICT v, NNScalar const* RESTRICT b, NNScalar* RESTRICT out)
{
	for (int col = 0; col < dimCol; ++col)
	{
		out[col] = VectorDot(dimRow, v, m, dimCol) + b[col];
		m += 1;
	}
}

int FNNMath::SoftMax(int dim, NNScalar const* RESTRICT inputs, NNScalar* outputs)
{
	NNScalar sum = 0.0;
	for (int i = 0; i < dim; ++i)
	{
		outputs[i] = exp(inputs[i]);
		sum += outputs[i];
	}
	int index = INDEX_NONE;
	NNScalar maxValue = 0.0f;
	for (int i = 0; i < dim; ++i)
	{
		outputs[i] /= sum;
		if (maxValue < outputs[i])
		{
			index = i;
			maxValue = outputs[i];
		}
	}
	return index;
}

int FNNMath::SoftMax(int dim, NNScalar* RESTRICT inoutValues)
{
	NNScalar sum = 0.0;
	for (int i = 0; i < dim; ++i)
	{
		inoutValues[i] = exp(inoutValues[i]);
		sum += inoutValues[i];
	}
	int index = INDEX_NONE;
	NNScalar maxValue = 0.0f;
	for (int i = 0; i < dim; ++i)
	{
		inoutValues[i] /= sum;
		if (maxValue < inoutValues[i])
		{
			index = i;
			maxValue = inoutValues[i];
		}
	}
	return index;
}


int FNNMath::Max(int dim, NNScalar const* inputs)
{
	int index = 0;
	NNScalar maxValue = inputs[0];
	for (int i = 1; i < dim; ++i)
	{
		if (maxValue < inputs[i])
		{
			index = i;
			maxValue = inputs[i];
		}
	}
	return index;
}

NNScalar FNNMath::Sum(int dim, NNScalar const* inputs)
{
	NNScalar result = 0;
	for (int i = 0; i < dim; ++i)
	{
		result += inputs[i];
	}
	return result;
}


void FNNMath::DeConv(int dimX, int dimY, NNScalar const* RESTRICT m, int dimXW, int dimYW, NNScalar const* RESTRICT weight, int stride, int padding, NNScalar* RESTRICT inoutOutput)
{
	//CHECK(dimX >= dimXW);
	//CHECK(dimY >= dimYW);

	int paddingX = (dimXW - 1) - padding;
	int paddingY = (dimXW - 1) - padding;

	int areaDimX = (dimX - 1) * stride + 1;
	int areaDimY = (dimY - 1) * stride + 1;

	int dimXOutput = areaDimX + 2 * paddingX - dimXW + 1;
	int dimYOutput = areaDimY + 2 * paddingY - dimYW + 1;


	int weightOffset = dimXW * dimYW - 1;

#if 0
	for (int mj = 0; mj < dimY; ++mj)
	{
		for (int mi = 0; mi < dimX; ++mi)
		{
			for (int wj = 0; wj < dimYW; ++wj)
			{
				for (int wi = 0; wi < dimXW; ++wi)
				{
					float w = weight[weightOffset - (wj * dimXW + wi)];
					float a = m[mj * dimX + mi];

					int oi = ;
					int oj = ;
					inoutOutput[ oj * dimXOutput + oi ] += w * a;
				}
			}

		}
	}

#else
	for (int oy = 0; oy < dimYOutput; ++oy)
	{
		int offY, sizeY;
		ClipToAreaRange(oy, areaDimY, dimYW, paddingY, offY, sizeY);
		if (sizeY <= 0)
			continue;

		NNScalar* pOutput = inoutOutput + oy * dimXOutput;

		int offAreaY = oy + offY - paddingY;

		for (int ox = 0; ox < dimXOutput; ++ox)
		{
			int offX, sizeX;
			ClipToAreaRange(ox, areaDimX, dimXW, paddingX, offX, sizeX);
			if (sizeX > 0)
			{
				int offAreaX = ox + offX - paddingX;
				int indexWOffset = offY * dimXW + offX;

				for (int j = 0; j < sizeY; ++j)
				{
					if (((offAreaY + j) % stride) != 0)
						continue;

					int jM = (offAreaY + j) / stride;

					for (int i = 0; i < sizeX; ++i)
					{
						if (((offAreaX + i) % stride) != 0)
							continue;

						int iM = (offAreaX + i) / stride;
						int indexW = indexWOffset + j * dimXW + i;
						NNScalar w = weight[weightOffset - indexW];
						NNScalar a = m[jM * dimX + iM];
						*pOutput += w * a;
					}
				}
			}

			++pOutput;
		}
	}
#endif
}

// Y = At[ (G g Gt) * (Bt d B) ] A
template< typename TKernel >
void TranformArea(int rowStride, int numSlice, int sliceStride, NNScalar const* RESTRICT area, NNScalar* RESTRICT outArea)
{
	NNScalar* RESTRICT pSliceArea = outArea;
	for (int indexSlice = 0; indexSlice < numSlice; ++indexSlice)
	{
		NNScalar temp[TKernel::WeightSize * TKernel::WeightSize];
		FNNMath::MatrixMulMatrix(TKernel::WeightSize, TKernel::WeightSize, TKernel::Bt, TKernel::WeightSize, area, rowStride, temp);
		FNNMath::MatrixMulMatrixT(TKernel::WeightSize, TKernel::WeightSize, temp, TKernel::WeightSize, TKernel::Bt, pSliceArea);

		area += sliceStride;
		pSliceArea += TKernel::WeightSize * TKernel::WeightSize;
	}
}


template< typename TKernel >
void AreaConvT(NNScalar inoutV[], int rowStride, int numSlice, int numNode, int sliceStride, NNScalar const* RESTRICT area, NNScalar const* RESTRICT weight)
{
	NNScalar temp2[TKernel::WeightSize * TKernel::WeightSize];
	FNNMath::Fill(ARRAY_SIZE(temp2), temp2, 0);

	for (int indexSlice = 0; indexSlice < numSlice; ++indexSlice)
	{
		FNNMath::VectorMulAdd(TKernel::WeightSize * TKernel::WeightSize, area, weight, temp2);
		area += TKernel::WeightSize * TKernel::WeightSize;
#if REORDER_WEIGHT
		weight += numNode * TKernel::WeightSize * TKernel::WeightSize;
#else
		weight += TKernel::WeightSize * TKernel::WeightSize;
#endif
	}

	NNScalar temp3[TKernel::O * TKernel::WeightSize];
	FNNMath::MatrixMulMatrix(TKernel::O, TKernel::WeightSize, TKernel::At, TKernel::WeightSize, temp2, temp3);
	NNScalar temp4[TKernel::O * TKernel::O];
	FNNMath::MatrixMulMatrixT(TKernel::O, TKernel::WeightSize, temp3, TKernel::O, TKernel::At, temp4);
	FNNMath::VectorAdd(TKernel::O * TKernel::O, inoutV, temp4);
}

// F(2,3)
// Bt = [ 1  0 -1  0 ] G = [   1    0   0 ]  At = [ 1 1  1  0 ] 
//      [ 0  1  1  0 ]     [ 1/2  1/2 1/2 ]       [ 0 1 -1 -1 ]
//      [ 0 -1  1  0 ]     [ 1/2 -1/2 1/2 ]
//      [ 0  1  0 -1 ]     [   0    0   1 ]
void FNNMath::TranformAreaF23(int rowStride, int numSlice, int sliceStride, NNScalar const* RESTRICT area, NNScalar* RESTRICT outArea)
{
	NNScalar* RESTRICT pSliceArea = outArea;
	for (int indexSlice = 0; indexSlice < numSlice; ++indexSlice)
	{
		NNScalar temp[4 * 4];
		{
			NNScalar const* RESTRICT a = area;
			NNScalar* RESTRICT t = temp;
			for (int i = 0; i < 4; ++i)
			{
				t[0] = (a[0] - a[2]);
				t[1] = (a[1] + a[2]);
				t[2] = (a[2] - a[1]);
				t[3] = (a[1] - a[3]);

				a += rowStride;
				t += 4;
			}
		}
		{
			NNScalar const* RESTRICT a = temp;
			NNScalar* RESTRICT t = pSliceArea;
			for (int i = 0; i < 4; ++i)
			{
				t[4*0] = (a[4*0] - a[4*2]);
				t[4*1] = (a[4*1] + a[4*2]);
				t[4*2] = (a[4*2] - a[4*1]);
				t[4*3] = (a[4*1] - a[4*3]);

				a += 1;
				t += 1;
			}
		}

		area += sliceStride;
		pSliceArea += 4 * 4;
	}
}


void FNNMath::AreaConvF23(int stride, NNScalar inoutV[], int numSlice, int numNode, NNScalar const* RESTRICT area, NNScalar const* RESTRICT weight)
{
	NNScalar temp2[4 * 4];
	FNNMath::Fill(ARRAY_SIZE(temp2), temp2, 0);

	int constexpr WeightLen = WinogradKernel23::WeightSize * WinogradKernel23::WeightSize;
#if REORDER_WEIGHT
	int WeightStride = numNode * WinogradKernel23::WeightSize * WinogradKernel23::WeightSize;
#else
	int constexpr WeightStride = WinogradKernel23::WeightSize * WinogradKernel23::WeightSize;
#endif
	for (int indexSlice = 0; indexSlice < numSlice; ++indexSlice)
	{
		FNNMath::VectorMulAdd(WeightLen, area, weight, temp2);
		area += WeightLen;
		weight += WeightStride;
	}

	NNScalar temp3[2 * 4];
	{
		NNScalar const* RESTRICT m = temp2;
		NNScalar* RESTRICT v = temp3;
		for (int i = 0; i < 4; ++i)
		{
			v[4*0] = m[4*0] + m[4*1] + m[4*2];
			v[4*1] = m[4*1] - m[4*2] - m[4*3];

			m += 1;
			v += 1;
		}
	}
	{
		NNScalar const* RESTRICT m = temp3;
		NNScalar* RESTRICT v = inoutV;
		for (int i = 0; i < 2; ++i)
		{
			v[0] += m[0] + m[1] + m[2];
			v[1] += m[1] - m[2] - m[3];

			v += stride;
			m += 4;
		}
	}
}

// Y = At[ (GgGt) * (BtdB) ] A
// F(4,3)
// Bt = [ 4  0 -5  0 1 0 ]  G = [  1/4     0    0 ]  At = [ 1 1  1 1  1 0 ]
//      [ 0 -4 -4  1 1 0 ]      [ -1/6  -1/6 -1/6 ]       [ 0 1 -1 2 -2 0 ]
//      [ 0  4 -4 -1 1 0 ]      [ -1/6   1/6 -1/6 ]       [ 0 1  1 4  4 0 ]
//      [ 0 -2 -1  2 1 0 ]      [ 1/24  1/12  1/6 ]       [ 0 1 -1 8 -8 1 ]
//      [ 0  2 -1 -2 1 0 ]      [ 1/24 -1/12  1/6 ]
//      [ 0  4  0 -5 0 1 ]      [    0     0    1 ]
void FNNMath::TranformAreaF43(int rowStride, int numSlice, int sliceStride, NNScalar const* RESTRICT area, NNScalar* RESTRICT outArea)
{
	NNScalar* RESTRICT pSliceArea = outArea;
	for (int indexSlice = 0; indexSlice < numSlice; ++indexSlice)
	{
		NNScalar temp[6 * 6];
		{
			NNScalar const* RESTRICT a = area;
			NNScalar* RESTRICT t = temp;
			for (int i = 0; i < 6; ++i)
			{
				t[0] = ( 4 * a[0] - 5 * a[2] + a[4]);
				t[1] = (-4 * a[1] - 4 * a[2] + a[3] + a[4]);
				t[2] = ( 4 * a[1] - 4 * a[2] - a[3] + a[4]);
				t[3] = (-2 * a[1] - a[2] + 2 * a[3] + a[4]);
				t[4] = ( 2 * a[1] - a[2] - 2 * a[3] + a[4]);
				t[5] = ( 4 * a[1] - 5 * a[3] + a[5]);

				a += rowStride;
				t += 6;
			}
		}
		{
			NNScalar const* RESTRICT a = temp;
			NNScalar* RESTRICT t = pSliceArea;
			for (int i = 0; i < 6; ++i)
			{
				t[6*0] = ( 4 * a[6*0] - 5 * a[6*2] + a[6*4]);
				t[6*1] = (-4 * a[6*1] - 4 * a[6*2] + a[6*3] + a[6*4]);
				t[6*2] = ( 4 * a[6*1] - 4 * a[6*2] - a[6*3] + a[6*4]);
				t[6*3] = (-2 * a[6*1] - a[6*2] + 2 * a[6*3] + a[6*4]);
				t[6*4] = ( 2 * a[6*1] - a[6*2] - 2 * a[6*3] + a[6*4]);
				t[6*5] = ( 4 * a[6*1] - 5 * a[6*3] + a[6*5]);

				a += 1;
				t += 1;
			}
		}
		area += sliceStride;
		pSliceArea += 6 * 6;
	}
}

void FNNMath::AreaConvF43(int stride, NNScalar inoutV[], int numSlice, int numNode, NNScalar const* RESTRICT area, NNScalar const* RESTRICT weight)
{
	NNScalar temp2[6 * 6];
	FNNMath::Fill(ARRAY_SIZE(temp2), temp2, 0);

	int constexpr WeightLen = WinogradKernel43::WeightSize * WinogradKernel43::WeightSize;
#if REORDER_WEIGHT
	int WeightStride = numNode * WinogradKernel23::WeightSize * WinogradKernel23::WeightSize;
#else
	int constexpr WeightStride = WinogradKernel23::WeightSize * WinogradKernel23::WeightSize;
#endif
	for (int indexSlice = 0; indexSlice < numSlice; ++indexSlice)
	{
		FNNMath::VectorMulAdd(WeightLen, area, weight, temp2);
		area += WeightLen;
		weight += WeightStride;
	}

	NNScalar temp3[4 * 6];
	{
		NNScalar const* RESTRICT m = temp2;
		NNScalar* RESTRICT v = temp3;
		for (int i = 0; i < 6; ++i)
		{
			v[6*0] = m[6*0] + m[6*1] + m[6*2] + m[6*3] + m[6*4];
			v[6*1] = m[6*1] - m[6*2] + 2 * (m[6*3] - m[6*4]);
			v[6*2] = m[6*1] + m[6*2] + 4 * (m[6*3] + m[6*4]);
			v[6*3] = m[6*1] - m[6*2] + 8 * (m[6*3] - m[6*4]) + m[6*5];

			m += 1;
			v += 1;
		}
	}
	{
		NNScalar const* RESTRICT m = temp3;
		NNScalar* RESTRICT v = inoutV;
		for (int i = 0; i < 4; ++i)
		{
			v[0] += m[0] + m[1] + m[2] + m[3] + m[4];
			v[1] += m[1] - m[2] + 2 * (m[3] - m[4]);
			v[2] += m[1] + m[2] + 4 * (m[3] + m[4]);
			v[3] += m[1] - m[2] + 8 * (m[3] - m[4]) + m[5];

			v += stride;
			m += 6;
		}
	}
}

void NNFullConLayout::init(int parameterOffset, TArrayView<uint32 const> topology)
{
	CHECK(topology.size() >= 2);
	uint32 numInput = topology[0];
	uint32 numOutput = topology[topology.size() - 1];
	init(numInput, numOutput, topology.size() - 2, topology + 1, parameterOffset);
}

void NNFullConLayout::init(uint32 numInput, uint32 numOutput, uint32 numHiddenLayer, uint32 const hiddenlayerNodeNum[], int parameterOffset)
{
	mLayers.resize(numHiddenLayer + 1);
	mTotalNodeNum = 0;
	int offset = parameterOffset;
	mMaxLayerNodeNum = -1;
	int inputLength = numInput;
	for( int i = 0; i < numHiddenLayer; ++i )
	{
		auto& layer = mLayers[i];

		layer.inputLength = inputLength;
		layer.numNode = hiddenlayerNodeNum[i];
		layer.weightOffset = offset;
		if( mMaxLayerNodeNum < layer.numNode )
			mMaxLayerNodeNum = layer.numNode;
		offset += inputLength * layer.numNode;
		layer.biasOffset = offset;
		offset += layer.numNode;

		mTotalNodeNum += hiddenlayerNodeNum[i];
		inputLength = layer.numNode;
	}

	{
		auto& layer = mLayers[numHiddenLayer];
		layer.inputLength = inputLength;
		layer.numNode = numOutput;
		layer.weightOffset = offset;
		if( mMaxLayerNodeNum < layer.numNode )
			mMaxLayerNodeNum = layer.numNode;

		offset += inputLength * layer.numNode;
		layer.biasOffset = offset;
		offset += layer.numNode;

		mTotalNodeNum += numOutput;
	}
}

void NNFullConLayout::getTopology(TArray<uint32>& outTopology) const
{
	outTopology.resize(mLayers.size() + 1);
	outTopology[0] = getInputNum();
	for( int i = 0; i < mLayers.size(); ++i )
	{
		outTopology[i + 1] = mLayers[i].numNode;
	}
}

int NNFullConLayout::getLayerInputNum(int idxLayer) const
{
	return mLayers[idxLayer].inputLength;
}

int NNFullConLayout::getInputSignalOffset(int idxLayer, bool bHadActiveInput) const
{
	if (idxLayer == 0)
		return 0;

	int result = getInputNum();
	for (int i = 0; i < idxLayer - 1; ++i)
	{
		int num = mLayers[i].numNode;
		if (bHadActiveInput)
			num *= 2;
		result += num;
	}
	return result;
}

int NNFullConLayout::getOutputSignalOffset(int idxLayer, bool bHadActiveInput) const
{
	int result = getInputNum();
	for (int i = 0; i < idxLayer; ++i)
	{
		int num = mLayers[i].numNode;
		if (bHadActiveInput)
			num *= 2;
		result += num;
	}
	return result;
}

int NNFullConLayout::getHiddenNodeNum() const
{
	return mTotalNodeNum - getOutputLength();
}

int NNFullConLayout::getParameterLength() const
{
	int result = 0;
	int curInputNum = getInputNum();
	for( auto const& layer : mLayers )
	{
		result += (curInputNum + 1) * layer.numNode;
		curInputNum = layer.numNode;
	}

	return result;
}

void NNFullConLayout::inference(
	NNScalar const parameters[], 
	NNScalar const inputs[], 
	NNScalar outputs[])  const
{
	if (mLayers.size() == 1)
	{
		NNScalar const* pInputs = inputs;
		NNScalar* pOutputs = outputs;

		NNLinearLayer const& layer = mLayers.back();

		pInputs = FNNAlgo::Forward(layer, parameters, pInputs, pOutputs);
		FNNAlgo::Forward(mOutputActiveLayer, layer.getOutputLength(), const_cast<NNScalar*>(pInputs));
	}
	else
	{
		NNScalar* pInputs = (NNScalar*)(ALLOCA((2 * getMaxLayerNodeNum()) * sizeof(NNScalar)));
		NNScalar* pOutputs = pInputs + getMaxLayerNodeNum();

		int inputNum = getInputNum();
		{
			auto& layer = mLayers.front();
			FNNAlgo::Forward(layer, parameters, inputs, pInputs);
			FNNAlgo::Forward(mHiddenActiveLayer, layer.getOutputLength(), pInputs);
		}

		for (int i = 1; i < mLayers.size() - 1; ++i)
		{
			auto& layer = mLayers[i];
			FNNAlgo::Forward(layer, parameters, pInputs, pOutputs);
			FNNAlgo::Forward(mHiddenActiveLayer, layer.getOutputLength(), pOutputs);
			std::swap(pInputs, pOutputs);
		}

		{
			auto& layer = mLayers.back();
			FNNAlgo::Forward(layer, parameters, pInputs, outputs);
			FNNAlgo::Forward(mOutputActiveLayer, layer.getOutputLength(), outputs);
		}
	}
}

NNScalar* NNFullConLayout::inferenceSignal(
	NNScalar const parameters[], 
	NNScalar const inputs[], 
	NNScalar outputs[]) const
{
	NNScalar const* pInputs = inputs;
	NNScalar* pOutputs = outputs;
	for (int i = 0; i < mLayers.size() - 1; ++i)
	{
		NNLinearLayer const& layer = mLayers[i];

		pInputs = FNNAlgo::Forward(layer, parameters, pInputs, pOutputs);
		FNNAlgo::Forward(mHiddenActiveLayer, layer.getOutputLength(), const_cast<NNScalar*>(pInputs));

		pOutputs += layer.getPassOutputNum();
	}

	{
		NNLinearLayer const& layer = mLayers.back();

		pInputs = FNNAlgo::Forward(layer, parameters, pInputs, pOutputs);
		FNNAlgo::Forward(mHiddenActiveLayer, layer.getOutputLength(), const_cast<NNScalar*>(pInputs));
	}

	return const_cast<NNScalar*>(pInputs);
}

NNScalar* NNFullConLayout::forward(
	NNScalar const parameters[], 
	NNScalar const inputs[], 
	NNScalar outputs[]) const
{
	NNScalar const* pInputs = inputs;
	NNScalar* pOutputs = outputs;

	for (int i = 0; i < mLayers.size() - 1; ++i)
	{
		NNLinearLayer const& layer = mLayers[i];

		pInputs = FNNAlgo::Forward(layer, parameters, pInputs, pOutputs);
		pOutputs += layer.getPassOutputNum();

		pInputs = FNNAlgo::Forward(mHiddenActiveLayer, layer.getOutputLength(), pInputs, pOutputs);
		pOutputs += layer.getPassOutputNum();
	}

	{
		NNLinearLayer const& layer = mLayers.back();

		pInputs = FNNAlgo::Forward(layer, parameters, pInputs, pOutputs);
		pOutputs += layer.getPassOutputNum();

		FNNAlgo::Forward(mOutputActiveLayer, layer.getOutputLength(), pInputs, pOutputs);
	}

	return pOutputs;
}

void NNFullConLayout::backward(
	NNScalar const parameters[], 
	NNScalar const inInputs[],
	NNScalar const inOutputs[],
	NNScalar const inLossGrads[],
	TArrayView<NNScalar> tempLossGrads,
	NNScalar inoutParameterGrads[],
	NNScalar* outLossGrads) const
{

	NNScalar* pLossGrad = tempLossGrads.data();
	NNScalar* pOutputLossGrad = tempLossGrads.data() + tempLossGrads.size() / 2;

	NNScalar const* pInput = inOutputs + getPassOutputLength();
	{
		auto const& layer = mLayers.back();

		NNScalar const* pOutput = pInput - layer.getOutputLength();
		pInput = pOutput - layer.getOutputLength();
		FNNAlgo::Backward(mOutputActiveLayer, layer.getOutputLength(), pInput, pOutput, inLossGrads, pLossGrad);
		std::swap(pOutputLossGrad, pLossGrad);
	}

	for (int idxLayer = getHiddenLayerNum(); idxLayer > 0; --idxLayer)
	{
		auto const& layer = getLayer(idxLayer);

		pInput -= layer.inputLength;
		FNNAlgo::BackwardWeight(layer, pInput, pOutputLossGrad, inoutParameterGrads);
		FNNAlgo::BackwardLoss(layer, parameters, pOutputLossGrad, pLossGrad);


		NNScalar const* pOutput = pInput;
		pInput -= layer.inputLength;
		FNNAlgo::Backward(mHiddenActiveLayer, layer.inputLength, pInput, pOutput, pLossGrad);

		std::swap(pOutputLossGrad, pLossGrad);
	}

	CHECK(pInput == inOutputs);

	{
		auto const& layer = getLayer(0);
		FNNAlgo::BackwardWeight(layer, inInputs, pOutputLossGrad, inoutParameterGrads);

		if (outLossGrads)
		{
			FNNAlgo::BackwardLoss(layer, parameters, pOutputLossGrad, outLossGrads);
		}
	}
}

NNScalar* FNNAlgo::Forward(
	NNLinearLayer const& layer, 
	NNScalar const parameters[],
	NNScalar const inputs[],
	NNScalar outputs[])
{
	NNScalar const* pWeight = parameters + layer.weightOffset;
	NNScalar const* pBias = parameters + layer.biasOffset;
#if REORDER_WEIGHT
	FNNMath::VectorMulMatrixAdd(layer.inputLength, layer.numNode, pWeight, inputs, pBias, outputs);
#else
	FNNMath::MatrixMulAddVector(layer.numNode, layer.inputLength, pWeight, inputs, pBias, outputs);
#endif
	return outputs;
}

// l-1          l
//             b[l,n] 
//     w[l,n,k]   z[l,n]          w[l+1,m,n] z[l+1,m]
// a[k] ------  +  -----> [F] a[l,n]----> + -------->
//            /   dc/dz             \ 
//           / w[l,n,k+1]            \ w[l+1,m+1,n]
//          /                         \      z[l+1,m+1]
// a[k+1]  /                           \ + -------->
// tn : output
// L : output Layer index
// z[l,n] : NetworkInput   
// a[l,n] : Signal 
// dC/dz[l,n] : Sensitivity Value

// z[l,n] = £Uk( w[l,n,k]*a[l-1,k] ) + b[l,n]
// C = sum( 0.5 *( t[n] - a[L,n] )^2 )
// dC/da[L,n] = -(t[n] - a[L,n]) 

// dC/dz[L,n] = dC/da * da/dz =  F'(z[L,n]) * dC/da[L,n] = F'(z[L,n]) * -(t[n] - a[L,n]) 
// dC/dz[l,n] = dC/da * da/dz =  F'(z[l,n]) * £Uk( dC/dz[l+1,k] * w[l+1,k,n] )

// dC/dw[l,n,k] = dC/dz * dz/dw = dC/dz[l,n] * a[l-1,k]
// dC/db[l,n] = dC/dz * dz/db = dC/dz[l,n]
// w'[l,n,k] = w[l,n,k] + learnRate * dC/dw[l,n,k]
// b'[l,n] = b[l,n] + learnRate * dC/db[l,n] 


// On = Wnk * Ik
// dC/dIn = dC/dOk * dOk/dIn = dC/dOk * d(Wkp * Ip) /dIn = dC/dOk *Wkn 

void FNNAlgo::BackwardWeight(
	NNLinearLayer const& layer,
	NNScalar const inInputs[],
	NNScalar const inLossGrads[],
	NNScalar inoutParameterGrads[])
{
#if REORDER_WEIGHT
	NNScalar* pBiasGrad = inoutParameterGrads + layer.biasOffset;
	FNNMath::VectorAdd(layer.numNode, pBiasGrad, inLossGrads);

	NNScalar* pWeightGrad = inoutParameterGrads + layer.weightOffset;
	for (int i = 0; i < layer.inputLength; ++i)
	{
		for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
		{
			*pWeightGrad += inLossGrads[idxNode] * inInputs[i];
			++pWeightGrad;
		}
	}
#else
	NNScalar* pLayerWeightGrad = inoutParameterGrads + layer.weightOffset;
	NNScalar* pLayerBiasGrad = inoutParameterGrads + layer.biasOffset;
	for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
	{
		*pLayerBiasGrad += inLossGrads[idxNode];
		++pLayerBiasGrad;
		for (int k = 0; k < layer.inputLength; ++k)
		{
			*pLayerWeightGrad += inLossGrads[idxNode] * inInputs[k];
			++pLayerWeightGrad;
		}
	}
#endif
}


void FNNAlgo::BackwardLoss(
	NNLinearLayer const& layer, 
	NNScalar const parameters[],
	NNScalar const inLossGrads[],
	NNScalar outLossGrads[])
{
	NNScalar const* pWeight = parameters + layer.weightOffset;
#if REORDER_WEIGHT
	FNNMath::MatrixMulVector(layer.inputLength, layer.numNode, pWeight, inLossGrads, outLossGrads);
#else
	FNNMath::VectorMulMatrix(layer.numNode, layer.inputLength, pWeight, inLossGrads, outLossGrads);
#endif

}



template< typename TKernel >
NNScalar*  ForwardT(
	NNConv2DLayer const& layer, 
	NNScalar const parameters[],
	NNScalar const inputs[],
	NNScalar outputs[])
{
	int const nx = layer.dataSize[0];
	int const ny = layer.dataSize[1];

	int const sliceInputLen = layer.inputSize[0] * layer.inputSize[1];
	int const nodeOutputLength = nx * ny;
	int const inputStride = layer.inputSize[0];

	NNScalar const* pWeight = parameters + layer.weightOffset;
	NNScalar const* pBias = parameters + layer.biasOffset;

	int const nodeWeightLength = layer.inputSize.z * TKernel::WeightSize * TKernel::WeightSize;

#if 0
	TArray<NNScalar> inputTransformed;
	inputTransformed.resize(nodeWeightSize);
#else
	NNScalar* inputTransformed = (NNScalar*)ALLOCA(nodeWeightLength * sizeof(NNScalar));
#endif
	NNScalar* pNodeOutput = outputs;
	for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
	{
		FNNMath::Fill(nodeOutputLength, pNodeOutput, pBias[idxNode]);
		pNodeOutput += nodeOutputLength;
	}

	for (int oy = 0; oy < ny; oy += TKernel::O)
	{
		for (int ox = 0; ox < nx; ox += TKernel::O)
		{
			int idxInput = ox + layer.inputSize[0] * oy;
			NNScalar const* pSliceInput = inputs + idxInput;
			TKernel::TranformArea(inputStride, layer.inputSize.z, sliceInputLen, pSliceInput, inputTransformed);

			int idxOutput = ox + nx * oy;
			
			pNodeOutput = outputs + idxOutput;
			NNScalar const* pNodeWeight = pWeight;

			for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
			{
				TKernel::AreaConv(nx, pNodeOutput, layer.inputSize.z, layer.numNode, inputTransformed, pNodeWeight);
				pNodeOutput += nodeOutputLength;
#if REORDER_WEIGHT
				pNodeWeight += TKernel::WeightSize * TKernel::WeightSize;
#else
				pNodeWeight += nodeWeightLength;
#endif
			}
		}
	}

	return outputs;
}

NNScalar* FNNAlgo::Forward(
	NNConv2DLayer const& layer, 
	NNScalar const parameters[],
	NNScalar const inputs[],
	NNScalar outputs[])
{
	CHECK(layer.dataSize[0] == (layer.inputSize[0] - layer.convSize + 1));
	CHECK(layer.dataSize[1] == (layer.inputSize[1] - layer.convSize + 1));

	int numSliceInput = layer.inputSize[2];

	if (layer.fastMethod != NNConv2DLayer::eNone)
	{
		switch (layer.fastMethod)
		{
		case NNConv2DLayer::eF23:
			return ForwardT<WinogradKernel23>(layer, parameters, inputs, outputs);
			break;
		case NNConv2DLayer::eF43:
			return ForwardT<WinogradKernel43>(layer, parameters, inputs, outputs);
			break;
		}

		return nullptr;
	}

	int const nx = layer.dataSize[0];
	int const ny = layer.dataSize[1];

	NNScalar const* pWeight = parameters + layer.weightOffset;
	NNScalar const* pBias = parameters + layer.biasOffset;

	int const sliceInputLength = layer.inputSize[0] * layer.inputSize[1];
	int const convLength = layer.convSize * layer.convSize;
	int const nodeOutputLength = nx * ny;


	NNScalar* pNodeOutput = outputs;
	NNScalar const* pNodeWeight = pWeight;

#if REORDER_WEIGHT
	for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
	{
		FNNMath::Fill(nodeOutputLength, pNodeOutput, pBias[idxNode]);
		pNodeOutput += nodeOutputLength;
	}

	NNScalar const* pSliceInput = inputs;
	for (int idxSlice = 0; idxSlice < numSliceInput; ++idxSlice)
	{
		pNodeOutput = outputs;
		for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
		{
			FNNMath::Conv(layer.inputSize[0], layer.inputSize[1], pSliceInput, layer.convSize, layer.convSize, pNodeWeight, pNodeOutput);
			pNodeOutput += nodeOutputLength;
			pNodeWeight += convLength;
		}

		pSliceInput += sliceInputLength;
	}
#else

	int const nodeWeightSize = numSliceInput * convLength;

	for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
	{
		NNScalar const* pSliceInput = inputs;
		NNScalar const* pSliceWeight = pNodeWeight;

		FNNMath::Fill(nodeOutputLength, pNodeOutput, pBias[idxNode]);

		for (int idxSlice = 0; idxSlice < numSliceInput; ++idxSlice)
		{
			FNNMath::Conv(layer.inputSize[0], layer.inputSize[1], pSliceInput, layer.convSize, layer.convSize, pSliceWeight, pNodeOutput);
			pSliceWeight += convLength;
			pSliceInput += sliceInputLength;
		}

		pNodeOutput += nodeOutputLength;
		pNodeWeight += nodeWeightSize;
	}
#endif

	return outputs;
}

NNScalar* FNNAlgo::Forward(
	NNConv2DPaddingLayer const& layer, NNScalar const parameters[],
	int numSliceInput, int const inputSize[], 
	NNScalar const inputs[],
	NNScalar outputs[])
{
	CHECK(layer.dataSize[0] == (inputSize[0] - layer.convSize[0] + 1));
	CHECK(layer.dataSize[1] == (inputSize[1] - layer.convSize[1] + 1));

	int const nx = layer.dataSize[0];
	int const ny = layer.dataSize[1];

	int const sliceInputLength = inputSize[0] * inputSize[1];
	int const convLength = layer.convSize[0] * layer.convSize[1];
	int const nodeOutputLength = nx * ny;
	int const inputStride = inputSize[0];

	NNScalar const* pWeight = parameters + layer.weightOffset;
	NNScalar const* pBias = parameters + layer.biasOffset;

	int const nodeWeightSize = numSliceInput * convLength;

	NNScalar* pNodeOutput = outputs;
	NNScalar const* pNodeWeight = pWeight;

	if (layer.padding > 0)
	{
#if REORDER_WEIGHT
		for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
		{
			FNNMath::Fill(nodeOutputLength, pNodeOutput, pBias[idxNode]);
			pNodeOutput += nodeOutputLength;
		}

		NNScalar const* pSliceInput = inputs;
		for (int idxSlice = 0; idxSlice < numSliceInput; ++idxSlice)
		{
			pNodeOutput = outputs;
			for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
			{
				FNNMath::Conv(inputSize[0], inputSize[1], pSliceInput, layer.convSize[0], layer.convSize[1], pNodeWeight, layer.padding, layer.padding, pNodeOutput);
				pNodeOutput += nodeOutputLength;
				pNodeWeight += convLength;
			}

			pSliceInput += sliceInputLength;
		}
#else
		for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
		{
			NNScalar const* pSliceInput = inputs;
			NNScalar const* pSliceWeight = pNodeWeight;

			FNNMath::Fill(nodeOutputLength, pNodeOutput, pBias[idxNode]);

			for (int idxSlice = 0; idxSlice < numSliceInput; ++idxSlice)
			{
				FNNMath::Conv(inputSize[0], inputSize[1], pSliceInput, layer.convSize[0], layer.convSize[1], pSliceWeight, layer.padding, layer.padding, pNodeOutput);
				pSliceWeight += convLength;
				pSliceInput += sliceInputLength;
			}

			pNodeOutput += nodeOutputLength;
			pNodeWeight += nodeWeightSize;
		}
#endif
	}
	else
	{
#if REORDER_WEIGHT
		for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
		{
			FNNMath::Fill(nodeOutputLength, pNodeOutput, pBias[idxNode]);
			pNodeOutput += nodeOutputLength;
		}

		NNScalar const* pSliceInput = inputs;
		for (int idxSlice = 0; idxSlice < numSliceInput; ++idxSlice)
		{
			pNodeOutput = outputs;
			for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
			{
				FNNMath::Conv(inputSize[0], inputSize[1], pSliceInput, layer.convSize[0], layer.convSize[1], pNodeWeight, pNodeOutput);
				pNodeOutput += nodeOutputLength;
				pNodeWeight += convLength;
			}

			pSliceInput += sliceInputLength;
		}
#else
		for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
		{
			NNScalar const* pSliceInput = inputs;
			NNScalar const* pSliceWeight = pNodeWeight;

			FNNMath::Fill(nodeOutputLength, pNodeOutput, pBias[idxNode]);

			for (int idxSlice = 0; idxSlice < numSliceInput; ++idxSlice)
			{
				FNNMath::Conv(inputSize[0], inputSize[1], pSliceInput, layer.convSize[0], layer.convSize[1], pSliceWeight, pNodeOutput);
				pSliceWeight += convLength;
				pSliceInput += sliceInputLength;
			}

			pNodeOutput += nodeOutputLength;
			pNodeWeight += nodeWeightSize;
		}
#endif
	}

	return outputs;
}



void FNNAlgo::BackwardLoss(
	NNConv2DLayer const& layer, 
	NNScalar const parameters[],
	NNScalar const inLossGrads[],
	NNScalar outLossGrads[])
{
	int const nx = layer.dataSize[0];
	int const ny = layer.dataSize[1];
	int const convLength = layer.convSize * layer.convSize;
	int const nodeOutputLength = nx * ny;
	int const sliceInputLength = layer.inputSize[0] * layer.inputSize[1];

	NNScalar const* pWeight = parameters + layer.weightOffset;

#if REORDER_WEIGHT

#define USE_ROTATE_180_FUNC 1


	NNScalar const* pNodeWeight = pWeight;
	FNNMath::Fill(sliceInputLength * layer.inputSize.z, outLossGrads, 0);

#if USE_ROTATE_180_FUNC

#else
	NNScalar* weightRotated = (NNScalar*)ALLOCA(sizeof(NNScalar) * convLength);
#endif
	NNScalar* pSliceLossGrad = outLossGrads;
	for (int idxSlice = 0; idxSlice < layer.inputSize.z; ++idxSlice)
	{
		NNScalar const* pNodeLossGrad = inLossGrads;
		for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
		{
#if USE_ROTATE_180_FUNC
			FNNMath::FullConvRotate180(nx, ny, pNodeLossGrad, layer.convSize, layer.convSize, pNodeWeight, pSliceLossGrad);
#else
			FNNMath::Rotate180(layer.convSize, layer.convSize, pNodeWeight, weightRotated);
			FNNMath::FullConv(nx, ny, pNodeLossGrad, layer.convSize, layer.convSize, weightRotated, pSliceLossGrad);
#endif
			pNodeWeight += convLength;
			pNodeLossGrad += nodeOutputLength;
		}
		pSliceLossGrad += sliceInputLength;
	}

#else

#define USE_ROTATE_180_FUNC 1

	NNScalar const* pNodeLossGrad = inLossGrads;
	NNScalar const* pSliceWeight = pWeight;

	FNNMath::Fill(sliceInputLength * layer.inputSize.z, outLossGrads, 0);

#if USE_ROTATE_180_FUNC

#else
	NNScalar* weightRotated = (NNScalar*)ALLOCA(sizeof(NNScalar) * convLength);
#endif
	for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
	{
		NNScalar* pSliceLossGrad = outLossGrads;
		for (int idxSlice = 0; idxSlice < layer.inputSize.z; ++idxSlice)
		{
#if USE_ROTATE_180_FUNC
			FNNMath::FullConvRotate180(nx, ny, pNodeLossGrad, layer.convSize, layer.convSize, pSliceWeight, pSliceLossGrad);
#else
			FNNMath::Rotate180(layer.convSize, layer.convSize, pSliceWeight, weightRotated);
			FNNMath::FullConv(nx, ny, pNodeLossGrad, layer.convSize, layer.convSize, weightRotated, pSliceLossGrad);
#endif

			pSliceWeight += convLength;
			pSliceLossGrad += sliceInputLength;
		}

		pNodeLossGrad += nodeOutputLength;
	}
#endif
}


//dC/dWi = dC/dOn * dOn/dWi = dC/dOn * d( Wnk * Ik )/dWi

void FNNAlgo::BackwardWeight(
	NNConv2DLayer const& layer, 
	NNScalar const inInputs[], 
	NNScalar const inOutputs[], 
	NNScalar const inLossGrads[],
	NNScalar inoutParameterGrads[])
{

	int const nx = layer.dataSize[0];
	int const ny = layer.dataSize[1];
	int const convLength = layer.convSize * layer.convSize;
	int const nodeOutputLength = nx * ny;
	int const sliceInputLength = layer.inputSize[0] * layer.inputSize[1];

	NNScalar* pNodeWeightGrad = inoutParameterGrads + layer.weightOffset;
	NNScalar* pNodeBiasGrad = inoutParameterGrads + layer.biasOffset;
	NNScalar const* pNodeLossGrad = inLossGrads;


#if REORDER_WEIGHT
	for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
	{
		pNodeBiasGrad[idxNode] = FNNMath::Sum(nodeOutputLength, pNodeLossGrad);
		pNodeLossGrad += nodeOutputLength;
	}

	NNScalar const* pSliceInputs = inInputs;
	for (int idxSlice = 0; idxSlice < layer.inputSize.z; ++idxSlice)
	{
		pNodeLossGrad = inLossGrads;
		for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
		{
			FNNMath::Conv(layer.inputSize[0], layer.inputSize[1], pSliceInputs, nx, ny, pNodeLossGrad, pNodeWeightGrad);

			pNodeWeightGrad += convLength;
			pNodeLossGrad += nodeOutputLength;
		}
		pSliceInputs += sliceInputLength;

	}

#else
	for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
	{
		pNodeBiasGrad[idxNode] = FNNMath::Sum(nodeOutputLength, pNodeLossGrad);

		NNScalar const* pSliceInputs = inInputs;
		for (int idxSlice = 0; idxSlice < layer.inputSize.z; ++idxSlice)
		{
			FNNMath::Conv(layer.inputSize[0], layer.inputSize[1], pSliceInputs, nx, ny, pNodeLossGrad, pNodeWeightGrad);
			pSliceInputs += sliceInputLength;
			pNodeWeightGrad += convLength;
		}

		pNodeLossGrad += nodeOutputLength;
	}
#endif

}

NNScalar* FNNAlgo::Forward(
	NNMaxPooling2DLayer const& layer, 
	NNScalar const inputs[],
	NNScalar outputs[])
{
	CHECK(layer.dataSize[0] * layer.poolSize == layer.inputSize[0]);
	CHECK(layer.dataSize[1] * layer.poolSize == layer.inputSize[1]);

	int const spliceInputLen = layer.inputSize[0] * layer.inputSize[1];
	int const spliceOutputLen = layer.dataSize[0] * layer.dataSize[1];

	NNScalar const* pNodeInput = inputs;
	NNScalar* pNodeOutput = outputs;
	for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
	{
		for (int j = 0; j < layer.dataSize[1]; ++j)
		{
			for (int i = 0; i < layer.dataSize[0]; ++i)
			{
				int indexInput = layer.poolSize * ( i + j * layer.inputSize[0] );
				int indexOutput = i + j * layer.dataSize[0];
				NNScalar value = std::numeric_limits<NNScalar>::lowest();
				for (int oy = 0; oy < layer.poolSize; ++oy)
				{
					for (int ox = 0; ox < layer.poolSize; ++ox)
					{
						int indexOffset = ox + oy * layer.inputSize[0];
						value = Math::Max(value, pNodeInput[indexInput + indexOffset]);
					}
				}
				pNodeOutput[indexOutput] = value;
			}
		}

		pNodeInput += spliceInputLen;
		pNodeOutput += spliceOutputLen;
	}

	return outputs;
}


void FNNAlgo::Backward(
	NNMaxPooling2DLayer const& layer, 
	NNScalar const inInputs[],
	NNScalar const inOutputs[],
	NNScalar const inLossGrads[],
	NNScalar outLossGrads[])
{
	CHECK(layer.dataSize[0] * layer.poolSize == layer.inputSize[0]);
	CHECK(layer.dataSize[1] * layer.poolSize == layer.inputSize[1]);

	int const spliceInputLen = layer.inputSize[0] * layer.inputSize[1];
	int const spliceOutputLen = layer.dataSize[0] * layer.dataSize[1];

	NNScalar const* pNodeInput = inInputs;
	NNScalar* pNodeLossGrad = outLossGrads;
	NNScalar const* pNodeOutput = inOutputs;
	NNScalar const* pInNodeLossGrad = inLossGrads;
	for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
	{
		for (int j = 0; j < layer.dataSize[1]; ++j)
		{
			for (int i = 0; i < layer.dataSize[0]; ++i)
			{
				int indexInput = layer.poolSize * (i + j * layer.inputSize[0]);
				int indexOutput = i + j * layer.dataSize[0];
				for (int oy = 0; oy < layer.poolSize; ++oy)
				{
					for (int ox = 0; ox < layer.poolSize; ++ox)
					{
						int index = indexInput + ox + oy * layer.inputSize[0];
						pNodeLossGrad[index] = (pNodeInput[index] == pNodeOutput[indexOutput]) ? pInNodeLossGrad[indexOutput] : 0;
					}
				}
			}
		}

		pNodeInput += spliceInputLen;
		pNodeLossGrad += spliceInputLen;

		pNodeOutput += spliceOutputLen;
		pInNodeLossGrad += spliceOutputLen;
	}
}

NNScalar* FNNAlgo::Forward(
	NNAveragePooling2DLayer const& layer, 
	NNScalar const inputs[], 
	NNScalar outputs[])
{
	CHECK(layer.dataSize[0] * layer.poolSize == layer.inputSize[0]);
	CHECK(layer.dataSize[1] * layer.poolSize == layer.inputSize[1]);

	int const spliceInputLen = layer.inputSize[0] * layer.inputSize[1];
	int const spliceOutputLen = layer.dataSize[0] * layer.dataSize[1];

	NNScalar const* pNodeInput = inputs;
	NNScalar* pNodeOutput = outputs;

	int const poolLength = layer.poolSize * layer.poolSize;
	for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
	{
		for (int j = 0; j < layer.dataSize[1]; ++j)
		{
			for (int i = 0; i < layer.dataSize[0]; ++i)
			{
				int indexInput = layer.poolSize * (i + j * layer.inputSize[0]);
				int indexOutput = i + j * layer.dataSize[0];
				NNScalar value = 0;
				for (int oy = 0; oy < layer.poolSize; ++oy)
				{
					for (int ox = 0; ox < layer.poolSize; ++ox)
					{
						int indexOffset = ox + oy * layer.inputSize[0];
						value += pNodeInput[indexInput + indexOffset];
					}
				}
				pNodeOutput[indexOutput] = value / poolLength;
			}
		}

		pNodeInput += spliceInputLen;
		pNodeOutput += spliceOutputLen;
	}

	return outputs;

}

void FNNAlgo::Backward(
	NNAveragePooling2DLayer const& layer, 
	NNScalar const inLossGrads[], 
	NNScalar outLossGrads[])
{
	CHECK(layer.dataSize[0] * layer.poolSize == layer.inputSize[0]);
	CHECK(layer.dataSize[1] * layer.poolSize == layer.inputSize[1]);

	int const spliceInputLen = layer.inputSize[0] * layer.inputSize[1];
	int const spliceOutputLen = layer.dataSize[0] * layer.dataSize[1];
	int const poolLength = layer.poolSize * layer.poolSize;

	NNScalar* pNodeLossGrad = outLossGrads;
	NNScalar const* pNodeOutputLossGrad = inLossGrads;
	for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
	{
		for (int j = 0; j < layer.dataSize[1]; ++j)
		{
			for (int i = 0; i < layer.dataSize[0]; ++i)
			{
				int indexInput = layer.poolSize * (i + j * layer.inputSize[0]);
				int indexOutput = i + j * layer.dataSize[0];
				for (int oy = 0; oy < layer.poolSize; ++oy)
				{
					for (int ox = 0; ox < layer.poolSize; ++ox)
					{
						int index = indexInput + ox + oy * layer.inputSize[0];
						pNodeLossGrad[index] = pNodeOutputLossGrad[indexOutput] / poolLength;
					}
				}
			}
		}

		pNodeLossGrad += spliceInputLen;
		pNodeOutputLossGrad += spliceOutputLen;
	}
}

NNScalar* FNNAlgo::Forward(NNTransformLayer const& layer, int inputLength, NNScalar const inInputs[], NNScalar outOutputs[])
{
	if (layer.funcTransform)
	{
		layer.funcTransform(inputLength, inInputs, outOutputs);
	}
	else
	{
		FNNMath::VectorCopy(inputLength, inInputs, outOutputs);
	}
	return outOutputs;
}

NNScalar* FNNAlgo::Forward(NNTransformLayer const& layer, int inputLength, NNScalar inoutValues[])
{
	if (layer.funcTransform)
	{
		layer.funcTransform(inputLength, inoutValues, inoutValues);
	}
	return inoutValues;
}


void FNNAlgo::Backward(NNTransformLayer const& layer, int inputLength, NNScalar const inInputs[], NNScalar const inOutputs[], NNScalar inoutLossGrads[])
{
	if (layer.funcLossGrad)
	{
		layer.funcLossGrad(inputLength, inInputs, inOutputs, inoutLossGrads, inoutLossGrads);
	}
}

void FNNAlgo::Backward(NNTransformLayer const& layer, int inputLength, NNScalar const inInputs[], NNScalar const inOutputs[], NNScalar const inLossGrads[], NNScalar outLossGrads[])
{
	if (layer.funcLossGrad)
	{
		layer.funcLossGrad(inputLength, inInputs, inOutputs, inLossGrads, outLossGrads);
	}
	else
	{
		FNNMath::VectorCopy(inputLength, inLossGrads, outLossGrads);
	}
}

void FNNAlgo::SoftMaxBackward(int inputLength, NNScalar const inInputs[], NNScalar const inOutputs[], NNScalar const inLossGrads[], NNScalar outLossGrads[])
{
	for (int i = 0; i < inputLength; ++i)
	{
		NNScalar lossGrad = 0;
		for (int io = 0; io < inputLength; ++io)
		{
			lossGrad += inLossGrads[io] * (((io == i) ? 1 : 0) - inOutputs[io]) * inOutputs[i];
		}

		outLossGrads[i] = lossGrad;
	}
}

