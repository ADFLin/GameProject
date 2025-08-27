#include "NeuralNetwork.h"

#include "Math/Base.h"
#include "CompilerConfig.h"
#include "MarcoCommon.h"


#if CPP_COMPILER_MSVC
#define ALLOCA _alloca
#else
#define ALLOCA alloca
#endif

#if USE_DUFF_DEVICE
#include "Misc/DuffDevice.h"
#endif

NNScalar* FCNeuralNetwork::getWeights(int idxLayer, int idxNode)
{
	FCNNLayout const& NNLayout = getLayout();

	NeuralFullConLayer const& layer = NNLayout.mLayers[idxLayer];
	NNScalar* result = mParameters + layer.weightOffset;
	result += idxNode * NNLayout.getLayerNodeWeigetNum(idxLayer);
	return result;
}

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
	for (; dim; --dim)
	{
		result += (*a) * (*b);
		++a;
		b += bStride;
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
		out[row] = b[row] + VectorDot(dimCol, m, v);
		m += dimCol;
	}
#endif
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
void AreaConvT(NNScalar inoutV[], int rowStride, int numSlice, int sliceStride, NNScalar const* RESTRICT area, NNScalar const* RESTRICT weight)
{
	NNScalar temp2[TKernel::WeightSize * TKernel::WeightSize];
	std::fill_n(temp2, ARRAY_SIZE(temp2), 0);

	for (int indexSlice = 0; indexSlice < numSlice; ++indexSlice)
	{
		FNNMath::VectorMulAdd(TKernel::WeightSize * TKernel::WeightSize, area, weight, temp2);
		area += TKernel::WeightSize * TKernel::WeightSize;
		weight += TKernel::WeightSize * TKernel::WeightSize;
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


void FNNMath::AreaConvF23(NNScalar inoutV[], int numSlice, NNScalar const* RESTRICT area, NNScalar const* RESTRICT weight)
{
	NNScalar temp2[4 * 4];
	std::fill_n(temp2, ARRAY_SIZE(temp2) , 0);

	int constexpr WeightLen = WinogradKernel23::WeightSize * WinogradKernel23::WeightSize;
	for (int indexSlice = 0; indexSlice < numSlice; ++indexSlice)
	{
		FNNMath::VectorMulAdd(WeightLen, area, weight, temp2);
		area += WeightLen;
		weight += WeightLen;
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

			v += 2;
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

void FNNMath::AreaConvF43(NNScalar inoutV[], int numSlice, NNScalar const* RESTRICT area, NNScalar const* RESTRICT weight)
{
	NNScalar temp2[6 * 6];
	std::fill_n(temp2, ARRAY_SIZE(temp2), 0);

	int constexpr WeightLen = WinogradKernel43::WeightSize * WinogradKernel43::WeightSize;
	for (int indexSlice = 0; indexSlice < numSlice; ++indexSlice)
	{
		FNNMath::VectorMulAdd(WeightLen, area, weight, temp2);
		area += WeightLen;
		weight += WeightLen;
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

			v += 4;
			m += 6;
		}
	}
}

void FCNNLayout::init(uint32 const topology[], uint32 dimNum)
{
	assert(dimNum >= 2);
	uint32 numInput = topology[0];
	uint32 numOutput = topology[dimNum - 1];
	init(numInput, numOutput, dimNum - 2, topology + 1);
}

void FCNNLayout::init(uint32 numInput, uint32 numOutput, uint32 numHiddenLayer, uint32 const hiddenlayerNodeNum[])
{
	mNumInput = numInput;
	mLayers.resize(numHiddenLayer + 1);
	mTotalNodeNum = 0;
	int offset = 0;
	int curInputNum = mNumInput;
	mMaxLayerNodeNum = -1;
	for( int i = 0; i < numHiddenLayer; ++i )
	{
		auto& layer = mLayers[i];
		layer.numNode = hiddenlayerNodeNum[i];
		layer.weightOffset = offset;
		if( mMaxLayerNodeNum < layer.numNode )
			mMaxLayerNodeNum = layer.numNode;
		offset += curInputNum * layer.numNode;
		layer.biasOffset = offset;
		offset += layer.numNode;

		curInputNum = layer.numNode;
		mTotalNodeNum += hiddenlayerNodeNum[i];
	}

	{
		auto& layer = mLayers[numHiddenLayer];
		layer.numNode = numOutput;
		layer.weightOffset = offset;
		if( mMaxLayerNodeNum < layer.numNode )
			mMaxLayerNodeNum = layer.numNode;
		offset += curInputNum * layer.numNode;
		layer.biasOffset = offset;
		offset += layer.numNode;

		mTotalNodeNum += numOutput;
	}
}

void FCNNLayout::getTopology(TArray<uint32>& outTopology) const
{
	outTopology.resize(mLayers.size() + 1);
	outTopology[0] = mNumInput;
	for( int i = 0; i < mLayers.size(); ++i )
	{
		outTopology[i + 1] = mLayers[i].numNode;
	}
}

int FCNNLayout::getLayerNodeWeigetNum(int idxLayer) const
{
	return (idxLayer == 0) ? mNumInput : mLayers[idxLayer - 1].numNode;
}

int FCNNLayout::getNetInputOffset(int idxLayer) const
{
	int result = 0;
	for( int i = 0; i < idxLayer; ++i )
	{
		result += mLayers[i].numNode;
	}
	return result;
}

int FCNNLayout::getInputSignalOffset(int idxLayer) const
{
	if (idxLayer == 0)
		return 0;

	int result = mNumInput;
	for (int i = 0; i < idxLayer - 1; ++i)
	{
		result += mLayers[i].numNode;
	}
	return result;
}

int FCNNLayout::getOutputSignalOffset(int idxLayer) const
{
	int result = mNumInput;
	for (int i = 0; i < idxLayer; ++i)
	{
		result += mLayers[i].numNode;
	}
	return result;
}

int FCNNLayout::getHiddenNodeNum() const
{
	return mTotalNodeNum - getOutputNum();
}

int FCNNLayout::getParameterNum() const
{
	int result = 0;
	int curInputNum = mNumInput;
	for( auto const& layer : mLayers )
	{
		result += (curInputNum + 1)* layer.numNode;
		curInputNum = layer.numNode;
	}

	return result;
}


void FNNAlgo::ForwardFeedback(
	NeuralFullConLayer const& layer, NNScalar const* RESTRICT parameters,
	int numInput, NNScalar const* RESTRICT inputs,
	NNScalar* RESTRICT outputs, bool bOutputActiveInput)
{
	NNScalar const* pWeight = parameters + layer.weightOffset;
	NNScalar const* pBias = parameters + layer.biasOffset;
	FNNMath::MatrixMulAddVector(layer.numNode, numInput, pWeight, inputs, pBias, outputs);

	int const outputLen = layer.numNode;
	if (bOutputActiveInput)
	{
		FNNMath::VectorCopy(outputLen, outputs, outputs + outputLen);
		outputs += outputLen;
	}

	if (layer.funcTransform)
	{
		layer.funcTransform(outputs, outputLen);
	}
}

void FNNAlgo::ForwardFeedback(
	NeuralFullConLayer const& layer, NNScalar const* RESTRICT parameters, 
	int numInput, NNScalar const* RESTRICT inputs, 
	NNScalar* RESTRICT outputs, 
	NNScalar* RESTRICT outNetInputs)
{
	NNScalar const* pWeight = parameters + layer.weightOffset;
	NNScalar const* pBias = parameters + layer.biasOffset;

	FNNMath::MatrixMulAddVector(layer.numNode, numInput, pWeight, inputs, pBias, outputs);
	FNNMath::VectorCopy(layer.numNode, outputs, outNetInputs);
	if (layer.funcTransform)
	{
		layer.funcTransform(outputs, layer.numNode);
	}
}

void FNNAlgo::ForwardFeedbackBatch(
	NeuralFullConLayer const& layer, NNScalar const* RESTRICT parameters,
	int numInput, NNScalar const* RESTRICT inputs,
	int batchSize,
	NNScalar* RESTRICT outputs, 
	NNScalar* RESTRICT outNetInputs)
{
	NNScalar const* pWeight = parameters + layer.weightOffset;
	NNScalar const* pBias = parameters + layer.biasOffset;

	NNScalar const* pInput = inputs;
	NNScalar* pOutput = outputs;
	for (int i = 0; i < batchSize; ++i)
	{
		FNNMath::MatrixMulAddVector(layer.numNode, numInput, pWeight, pInput, pBias, pOutput);
		pOutput += layer.numNode;
		pInput += numInput;
	}
	FNNMath::VectorCopy(batchSize * layer.numNode, outputs, outNetInputs);

	if (layer.funcTransform)
	{
		layer.funcTransform(outputs, batchSize * layer.numNode);
	}
}

void FNNAlgo::ForwardPassBatch(
	FCNNLayout const& layout, NNScalar const* parameters,
	NNScalar const inInputs[], int batchSize,
	NNScalar outActivations[],
	NNScalar outNetInputs[])
{

	int curInputNum = batchSize * layout.getInputNum();
	NNScalar const* inputs = inInputs;
	NNScalar* outputs = outActivations;
	for (const NeuralFullConLayer& layer : layout.mLayers)
	{
		ForwardFeedbackBatch(layer, parameters, curInputNum, inputs, batchSize, outputs, outNetInputs);

		curInputNum = batchSize * layer.numNode;
		inputs = outputs;
		outputs += curInputNum;
		outNetInputs += curInputNum;
	}
}

void FNNAlgo::ForwardFeedback(
	FCNNLayout const& layout, NNScalar const* parameters, 
	NNScalar const inputs[], 
	NNScalar outputs[])
{
	if (layout.mLayers.size() == 1)
	{
		ForwardFeedback(layout.mLayers[0], parameters, layout.getInputNum(), inputs, outputs, false);
	}
	else
	{
		NNScalar* tempInputs = (NNScalar*)(ALLOCA((2 * layout.getMaxLayerNodeNum()) * sizeof(NNScalar)));
		NNScalar* tempOutputs = tempInputs + layout.getMaxLayerNodeNum();
		ForwardFeedback(layout.mLayers[0], parameters, layout.getInputNum(), inputs, tempInputs, false);
		int curInputNum = layout.mLayers[0].numNode;
		for (int i = 1; i < layout.mLayers.size() - 1; ++i)
		{
			auto& layer = layout.mLayers[i];
			ForwardFeedback(layer, parameters, curInputNum, tempInputs, tempOutputs, false);
			curInputNum = layer.numNode;
			std::swap(tempInputs, tempOutputs);
		}
		ForwardFeedback(layout.mLayers.back(), parameters, curInputNum, tempInputs, outputs, false);
	}

	//_freea(tempInputs);
}

void FNNAlgo::ForwardFeedbackSignal(
	FCNNLayout const& layout, NNScalar const* parameters,
	NNScalar const inInputs[],
	NNScalar outActivations[])
{
	int curInputNum = layout.getInputNum();
	NNScalar const* inputs = inInputs;
	NNScalar* outputs = outActivations;
	for (const NeuralFullConLayer& layer : layout.mLayers)
	{
		//NNMatrixView weights = getWeights(layer);
		ForwardFeedback(layer, parameters, curInputNum, inputs, outputs, false);
		curInputNum = layer.numNode;
		inputs = outputs;
		outputs += curInputNum;
	}
}

void FNNAlgo::ForwardPass(
	FCNNLayout const& layout, NNScalar const* parameters, 
	NNScalar const inInputs[], 
	NNScalar outActivations[], 
	NNScalar outNetInputs[])
{
	int curInputNum = layout.getInputNum();
	NNScalar const* inputs = inInputs;
	NNScalar* outputs = outActivations;
	for (const NeuralFullConLayer& layer : layout.mLayers)
	{
		ForwardFeedback(layer, parameters, curInputNum, inputs, outputs, outNetInputs);

		curInputNum = layer.numNode;
		inputs = outputs;
		outputs += curInputNum;
		outNetInputs += curInputNum;
	}
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

void FNNAlgo::BackwardPassWeight(
	NeuralFullConLayer const& layer,
	int numNodeWeiget,
	NNScalar const inInput[],
	NNScalar const inLossDerivatives[],
	NNScalar outDeltaWeights[])
{
	NNScalar* pDeltaWeightLayer = outDeltaWeights + layer.weightOffset;
	NNScalar* pDeltaBiasLayer = outDeltaWeights + layer.biasOffset;
	for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
	{
		NNScalar dCdz = inLossDerivatives[idxNode];
		NNScalar dCdb = dCdz;
		*pDeltaBiasLayer += dCdb;
		++pDeltaBiasLayer;
		for (int k = 0; k < numNodeWeiget; ++k)
		{
			NNScalar dCdw = dCdz * inInput[k];
			*pDeltaWeightLayer += dCdw;
			++pDeltaWeightLayer;
		}
	}
}

void FNNAlgo::BackwardPass(
	FCNNLayout const& layout, NNScalar* parameters,
	NNScalar const inSignals[],
	NNScalar const inNetInputs[],
	NNScalar const inLossDerivatives[],
	NNScalar outLossGrads[],
	NNScalar outDeltaWeights[])
{

	int totalNodeCount = layout.getHiddenNodeNum() + layout.getOutputNum();

	NNScalar* pLossGrad = outLossGrads + totalNodeCount;
	NNScalar const* pNetworkInputs = inNetInputs + totalNodeCount;
	NNScalar const* pInputSignals = inSignals + (layout.getInputNum() + layout.getHiddenNodeNum());

	{
		int idxLayer = layout.getHiddenLayerNum();
		auto const& layer = layout.getLayer(idxLayer);

		pNetworkInputs -= layer.numNode;
		CHECK(pNetworkInputs = inNetInputs + layout.getNetInputOffset(idxLayer));

		pLossGrad -= layer.numNode;
		CHECK(pLossGrad = outLossGrads + layout.getNetInputOffset(idxLayer));

		int numNodeWeiget = layout.getLayerNodeWeigetNum(idxLayer);
		pInputSignals -= numNodeWeiget;
		CHECK(pInputSignals = inSignals + layout.getInputSignalOffset(idxLayer));

		FNNMath::VectorCopy(layer.numNode, inLossDerivatives, pLossGrad);
		BackwardPass(static_cast<ActiveLayer const&>(layer), layer.numNode, pNetworkInputs, pLossGrad);
		BackwardPassWeight(layer, numNodeWeiget, pInputSignals, pLossGrad, outDeltaWeights);
	}

	for (int idxLayer = layout.getHiddenLayerNum() - 1; idxLayer >= 0; --idxLayer)
	{
		auto const& layer = layout.getLayer(idxLayer);
		auto const& nextLayer = layout.getLayer(idxLayer + 1);

		pNetworkInputs -= layer.numNode;
		CHECK(pNetworkInputs = inNetInputs + layout.getNetInputOffset(idxLayer));
		NNScalar* pSensivityNextLayer = pLossGrad;
		pLossGrad -= layer.numNode;
		CHECK(pLossGrad = outLossGrads + layout.getNetInputOffset(idxLayer));

		int numNodeWeiget = layout.getLayerNodeWeigetNum(idxLayer);
		pInputSignals -= numNodeWeiget;
		CHECK(pInputSignals = inSignals + layout.getInputSignalOffset(idxLayer));

		NNScalar* pWeightNext = parameters + nextLayer.weightOffset;

		FNNMath::VectorMulMatrix(nextLayer.numNode, layer.numNode, pWeightNext, pSensivityNextLayer, pLossGrad);
		BackwardPass(static_cast<ActiveLayer const&>(layer), layer.numNode, pNetworkInputs, pLossGrad);
		BackwardPassWeight(layer, numNodeWeiget, pInputSignals, pLossGrad, outDeltaWeights);

	}
}

template< typename TKernel >
void ForwardFeedbackT(
	NeuralConv2DLayer const& layer, NNScalar const* RESTRICT parameters,
	int numSliceInput, int const inputSize[], NNScalar const* RESTRICT inputs,
	NNScalar* RESTRICT outputs, bool bOutputActiveInput)
{
	int const nx = layer.dataSize[0];
	int const ny = layer.dataSize[1];

	int const sliceInputSize = inputSize[0] * inputSize[1];
	int const nodeOutputLength = nx * ny;
	int const inputStride = inputSize[0];

	NNScalar const* pWeight = parameters + layer.weightOffset;
	NNScalar const* pBias = parameters + layer.biasOffset;

	int const nodeWeightLength = numSliceInput * TKernel::WeightSize * TKernel::WeightSize;

#if 0
	TArray<NNScalar> inputTransformed;
	inputTransformed.resize(nodeWeightSize);
#else
	NNScalar* inputTransformed = (NNScalar*)ALLOCA(nodeWeightLength * sizeof(NNScalar));
#endif

	for (int oy = 0; oy < ny; oy += TKernel::O)
	{
		for (int ox = 0; ox < nx; ox += TKernel::O)
		{
			int idxInput = ox + inputSize[0] * oy;
			int idxOutput = ox + nx * oy;
			NNScalar const* pSliceInput = inputs + idxInput;

			TKernel::TranformArea(inputStride, numSliceInput, sliceInputSize, pSliceInput, inputTransformed);

			NNScalar* pNodeOutput = outputs;
			NNScalar const* pNodeWeight = pWeight;

			for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
			{
				NNScalar values[TKernel::O * TKernel::O];
				std::fill_n(values, ARRAY_SIZE(values), pBias[idxNode]);
				
				TKernel::AreaConv(values, numSliceInput, inputTransformed, pNodeWeight);

				NNScalar* pValue = values;
				NNScalar* pOutput = &pNodeOutput[idxOutput];
				for (int n = 0; n < TKernel::O; ++n)
				{
					FNNMath::VectorCopy(TKernel::O, pValue, pOutput);
					pOutput += nx;
					pValue += TKernel::O;
				}

				pNodeOutput += nodeOutputLength;
				pNodeWeight += nodeWeightLength;
			}
		}
	}

	int const outputLen = nodeOutputLength * layer.numNode;
	if (bOutputActiveInput)
	{
		FNNMath::VectorCopy(outputLen, outputs, outputs + outputLen);
		outputs += outputLen;
	}

	if (layer.funcTransform)
	{
		layer.funcTransform(outputs, outputLen);
	}
}

void FNNAlgo::ForwardFeedback(
	NeuralConv2DLayer const& layer, NNScalar const* RESTRICT parameters,
	int numSliceInput, int const inputSize[], 
	NNScalar const* RESTRICT inputs,
	NNScalar* RESTRICT outputs, bool bOutputActiveInput)
{
	CHECK(layer.dataSize[0] == (inputSize[0] - layer.convSize + 1));
	CHECK(layer.dataSize[1] == (inputSize[1] - layer.convSize + 1));

	if (layer.fastMethod != NeuralConv2DLayer::eNone)
	{
		switch (layer.fastMethod)
		{
		case NeuralConv2DLayer::eF23:
			ForwardFeedbackT<WinogradKernel23>(layer, parameters, numSliceInput, inputSize, inputs, outputs, bOutputActiveInput);
			break;;
		case NeuralConv2DLayer::eF43:
			ForwardFeedbackT<WinogradKernel43>(layer, parameters, numSliceInput, inputSize, inputs, outputs, bOutputActiveInput);
			break;
		}

		return;
	}

	int const nx = layer.dataSize[0];
	int const ny = layer.dataSize[1];

	int const sliceInputLength = inputSize[0] * inputSize[1];
	int const convLength = layer.convSize * layer.convSize;
	int const nodeOutputLength = nx * ny;
	int const inputStride = inputSize[0];

	NNScalar const* pWeight = parameters + layer.weightOffset;
	NNScalar const* pBias = parameters + layer.biasOffset;

	int const nodeWeightSize = numSliceInput * convLength;

	for (int oy = 0; oy < ny; ++oy)
	{
		for (int ox = 0; ox < nx; ++ox)
		{
			NNScalar* pNodeOutput = outputs;
			NNScalar const* pNodeWeight = pWeight;

			for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
			{
				int idxInput = ox + inputSize[0] * oy;
				int idxOutput = ox + nx * oy;

				NNScalar const* pSliceInput = inputs + idxInput;
				NNScalar const* pSliceWeight = pNodeWeight;
				NNScalar value = pBias[idxNode];
				for (int idxSlice = 0; idxSlice < numSliceInput; ++idxSlice)
				{
					value += FNNMath::AreaConv(layer.convSize, inputStride, pSliceInput, pSliceWeight);
					pSliceWeight += convLength;
					pSliceInput += sliceInputLength;
				}

				pNodeOutput[idxOutput] = value;

				pNodeOutput += nodeOutputLength;
				pNodeWeight += nodeWeightSize;
			}
		}
	}
	
	int const outputLen = nodeOutputLength * layer.numNode;
	if (bOutputActiveInput)
	{
		FNNMath::VectorCopy(outputLen, outputs, outputs + outputLen);
		outputs += outputLen;
	}

	if (layer.funcTransform)
	{
		layer.funcTransform(outputs, outputLen);
	}
}

void FNNAlgo::BackwardPassLoss(
	NeuralConv2DLayer const& layer, NNScalar const* RESTRICT parameters,
	int numSliceInput, int const inputSize[],
	NNScalar const inLossDerivatives[],
	NNScalar outLossGrads[])
{
	int const nx = layer.dataSize[0];
	int const ny = layer.dataSize[1];
	int const convLength = layer.convSize * layer.convSize;
	int const nodeOutputLength = nx * ny;
	NNScalar const* pWeight = parameters + layer.weightOffset;

	for (int idxSlice = 0; idxSlice < numSliceInput; ++idxSlice)
	{
		for (int iy = 0; iy < inputSize[1]; ++iy)
		{
			for (int ix = 0; ix < inputSize[0]; ++ix)
			{
				NNScalar loseGrad = 0.0;
				for (int dy = 0; dy < layer.convSize; ++dy)
				{
					int oy = iy - dy;
					if ( oy < 0 || oy >= layer.dataSize[1] )
						continue;

					for (int dx = 0; dx < layer.convSize; ++dx)
					{
						int ox = ix - dx;
						if (ox < 0 || ox >= layer.dataSize[0])
							continue;

						int weightOffset = idxSlice * convLength + layer.convSize * dy + dx;
						int const nodeWeightStride = numSliceInput * convLength;
						NNScalar const* pNodeWeight = pWeight + weightOffset;
						NNScalar const* pNodeLossDerivative = inLossDerivatives + (nx * oy + ox);

						loseGrad += FNNMath::VectorDot(layer.numNode, pNodeWeight, nodeWeightStride, pNodeLossDerivative, nodeOutputLength);
					}
				}
			}
		}
	}
}


//dC/dWi = dC/dOn * dOn/dWi = dC/dOn * d( Wnk * Ik )/dWi

void FNNAlgo::BackwardPassWeight(
	NeuralConv2DLayer const& layer, 
	int numSliceInput, int const inputSize[], 
	NNScalar const inSignals[], 
	NNScalar const inNetInputs[], 
	NNScalar inoutLossDerivatives[],
	NNScalar outDeltaWeights[])
{

	int const nx = layer.dataSize[0];
	int const ny = layer.dataSize[1];
	int const convLength = layer.convSize * layer.convSize;
	int const nodeOutputLength = nx * ny;
	int const sliceInputLength = inputSize[0] * inputSize[1];

	BackwardPass(static_cast<ActiveLayer const&>(layer), nodeOutputLength * layer.numNode, inNetInputs, inoutLossDerivatives); 

	NNScalar* pNodeDeltaWeight = outDeltaWeights + layer.weightOffset;
	NNScalar* pNodeDeltaBias = outDeltaWeights + layer.biasOffset;
	NNScalar* pNodeLossDerivative = inoutLossDerivatives;


	for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
	{
		NNScalar dCdb = 0;
		for (int i = 0; i < nodeOutputLength; ++i)
		{
			dCdb += pNodeLossDerivative[i];
		}
		pNodeDeltaBias[idxNode] = dCdb;
		NNScalar const* pSliceSignals = inSignals;
		for (int idxSlice = 0; idxSlice < numSliceInput ;++idxSlice)
		{
			NNScalar* pSliceDeltaWeight = pNodeDeltaWeight + idxSlice * convLength;
			for (int dy = 0; dy < layer.convSize; ++dy)
			{
				for (int dx = 0; dx < layer.convSize; ++dx)
				{
					NNScalar dCdW = 0;
					for (int iy = 0; iy < inputSize[1]; ++iy)
					{
						int oy = iy - dy;
						if (oy < 0 || oy >= layer.dataSize[1])
							continue;

						for (int ix = 0; ix < inputSize[0]; ++ix)
						{
							int ox = ix - dx;
							if (ox < 0 || ox >= layer.dataSize[0])
								continue;


							dCdW += pNodeLossDerivative[ nx * oy + ox ] * pSliceSignals[layer.dataSize[0] * iy + ix];
						}
					}

					pSliceDeltaWeight[ layer.convSize * dy + dx ] = dCdW;
				}
			}

			pSliceSignals += sliceInputLength;
		}

		pNodeDeltaWeight += numSliceInput * convLength;
	}

}

void FNNAlgo::ForwardFeedback(
	NeuralMaxPooling2DLayer const& layer, 
	int const inputSize[],
	NNScalar const* RESTRICT inputs,
	NNScalar* RESTRICT outputs)
{
	CHECK(layer.dataSize[0] * layer.poolSize == inputSize[0]);
	CHECK(layer.dataSize[1] * layer.poolSize == inputSize[1]);

	int const spliceInputLen = inputSize[0] * inputSize[1];
	int const spliceOutputLen = layer.dataSize[0] * layer.dataSize[1];

	NNScalar const* RESTRICT pNodeInput = inputs;
	NNScalar* RESTRICT pNodeOutput = outputs;
	for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
	{
		for (int j = 0; j < layer.dataSize[1]; ++j)
		{
			for (int i = 0; i < layer.dataSize[0]; ++i)
			{
				int indexInput = layer.poolSize * ( i + j * inputSize[0] );
				int indexOutput = i + j * layer.dataSize[1];
				NNScalar value = std::numeric_limits<NNScalar>::lowest();
				for (int oy = 0; oy < layer.poolSize; ++oy)
				{
					for (int ox = 0; ox < layer.poolSize; ++ox)
					{
						int indexOffset = ox + oy * inputSize[0];
						value = Math::Max(value, pNodeInput[indexInput + indexOffset]);
					}
				}
				pNodeOutput[indexOutput] = value;
			}
		}

		pNodeInput += spliceInputLen;
		pNodeOutput += spliceOutputLen;
	}
}


void FNNAlgo::BackwardPass(
	NeuralMaxPooling2DLayer const& layer, 
	int const inputSize[],
	NNScalar const inInputs[],
	NNScalar const inOutputs[],
	NNScalar const inLossDerivatives[],
	NNScalar outLossGrads[])
{

	CHECK(layer.dataSize[0] * layer.poolSize == inputSize[0]);
	CHECK(layer.dataSize[1] * layer.poolSize == inputSize[1]);

	int const spliceInputLen = inputSize[0] * inputSize[1];
	int const spliceOutputLen = layer.dataSize[0] * layer.dataSize[1];

	NNScalar const* RESTRICT pNodeInput = inInputs;
	NNScalar const* RESTRICT pNodeOutput = inOutputs;
	for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
	{
		for (int j = 0; j < layer.dataSize[1]; ++j)
		{
			for (int i = 0; i < layer.dataSize[0]; ++i)
			{
				int indexInput = layer.poolSize * (i + j * inputSize[0]);
				int indexOutput = i + j * layer.dataSize[1];
				for (int oy = 0; oy < layer.poolSize; ++oy)
				{
					for (int ox = 0; ox < layer.poolSize; ++ox)
					{
						int indexOffset = ox + oy * inputSize[0];
						if (pNodeInput[indexInput + indexOffset] == pNodeOutput[indexOutput])
						{
							outLossGrads[indexInput + indexOffset] = inLossDerivatives[indexOutput];
						}
						else
						{
							outLossGrads[indexInput + indexOffset] = 0;
						}
					}
				}
			}
		}

		pNodeInput += spliceInputLen;
		pNodeOutput += spliceOutputLen;
	}
}

void FNNAlgo::ForwardPass(ActiveLayer const& layer, int numNode, NNScalar const inInputs[], NNScalar outOutputs[])
{
	FNNMath::VectorCopy(numNode, inInputs, outOutputs);
	if (layer.funcTransform)
	{
		layer.funcTransform(outOutputs, numNode);
	}
}

void FNNAlgo::BackwardPass(ActiveLayer const& layer, int numNode, NNScalar const inInputs[], NNScalar inoutLossDerivatives[])
{
	if (layer.funcTransform)
	{
		for (int i = 0; i < numNode; ++i)
		{
			inoutLossDerivatives[i] *= layer.funcDerivative(inInputs[i]);
		}
	}
}

void FNNAlgo::BackwardPass(ActiveLayer const& layer, int numNode, NNScalar const inInputs[], NNScalar const inLossDerivatives[], NNScalar outLossGrads[])
{
	if (layer.funcTransform)
	{
		for (int i = 0; i < numNode; ++i)
		{
			outLossGrads[i] = inLossDerivatives[i] * layer.funcDerivative(inInputs[i]);
		}
	}
	else
	{
		FNNMath::VectorCopy(numNode, inLossDerivatives, outLossGrads);
	}
}

