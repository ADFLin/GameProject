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
	NNScalar* RESTRICT outputs)
{
	FNNMath::MatrixMulAddVector(layer.numNode, numInput, parameters + layer.weightOffset, inputs, parameters + layer.biasOffset, outputs);

	if (layer.funcTransform)
	{
		(*layer.funcTransform)(outputs, layer.numNode);
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
	NNScalar const* pInput = inputs;
	NNScalar* pOutput = outputs;
	NNScalar* pNetInput = outNetInputs;

	for (int i = 0; i < layer.numNode; ++i)
	{
		NNScalar value = pBias[0] + FNNMath::VectorDot(numInput, pWeight, pInput);
		pWeight += numInput;
		pBias += 1;

		*pNetInput = value;
		++pNetInput;
		*pOutput = value;
		++pOutput;
	}

	if (layer.funcTransform)
	{
		(*layer.funcTransform)(outputs, layer.numNode);
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
	NNScalar* pNetInput = outNetInputs;

	FNNMath::MatrixMulTMatrixRT(layer.numNode, numInput, parameters + layer.weightOffset, batchSize, inputs, pNetInput);
	FNNMath::VectorCopy(batchSize * layer.numNode, pNetInput, outputs);

	if (layer.funcTransform)
	{
		(*layer.funcTransform)(outputs, batchSize * layer.numNode);
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
		ForwardFeedback(layout.mLayers[0], parameters, layout.getInputNum(), inputs, outputs);
	}
	else
	{
		NNScalar* tempInputs = (NNScalar*)(ALLOCA((2 * layout.getMaxLayerNodeNum()) * sizeof(NNScalar)));
		NNScalar* tempOutputs = tempInputs + layout.getMaxLayerNodeNum();
		ForwardFeedback(layout.mLayers[0], parameters, layout.getInputNum(), inputs, tempInputs);
		int curInputNum = layout.mLayers[0].numNode;
		for (int i = 1; i < layout.mLayers.size() - 1; ++i)
		{
			auto& layer = layout.mLayers[i];
			ForwardFeedback(layer, parameters, curInputNum, tempInputs, tempOutputs);
			curInputNum = layer.numNode;
			std::swap(tempInputs, tempOutputs);
		}
		ForwardFeedback(layout.mLayers.back(), parameters, curInputNum, tempInputs, outputs);
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
		ForwardFeedback(layer, parameters, curInputNum, inputs, outputs);
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

void BackwardFeedback(
	NeuralFullConLayer const& layer,
	int numNodeWeiget,
	NNScalar const* pInputSignals,
	NNScalar const* pNetworkInputs,
	NNScalar inoutLossDerivatives[],
	NNScalar outDeltaWeights[])
{
	NNScalar* pDeltaWeightLayer = outDeltaWeights + layer.weightOffset;
	NNScalar* pDeltaBiasLayer = outDeltaWeights + layer.biasOffset;
	for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
	{
		NNScalar z_ln = pNetworkInputs[idxNode];
		inoutLossDerivatives[idxNode] *= layer.funcDerivative(z_ln);
		NNScalar dCdz = inoutLossDerivatives[idxNode];
		NNScalar dCdb = dCdz;
		*pDeltaBiasLayer += dCdb;
		++pDeltaBiasLayer;
		for (int k = 0; k < numNodeWeiget; ++k)
		{
			NNScalar dCdw = dCdz * pInputSignals[k];
			*pDeltaWeightLayer += dCdw;
			++pDeltaWeightLayer;
		}
	}
}

void FNNAlgo::BackwardPass(
	FCNNLayout const& layout, NNScalar* parameters,
	NNScalar const inLossDerivatives[],
	NNScalar const inSignals[],
	NNScalar const inNetInputs[],
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
		BackwardFeedback(layer, numNodeWeiget, pInputSignals, pNetworkInputs, pLossGrad, outDeltaWeights);
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
		BackwardFeedback(layer, numNodeWeiget, pInputSignals, pNetworkInputs, pLossGrad, outDeltaWeights);

	}
}

void FNNAlgo::ForwardFeedback(
	NeuralConv2DLayer const& layer, NNScalar const* RESTRICT parameters,
	int numSliceInput, int const inputSize[], NNScalar const* RESTRICT inputs,
	NNScalar* RESTRICT outputs)
{
	CHECK(layer.dataSize[0] == (inputSize[0] - layer.convSize + 1));
	CHECK(layer.dataSize[1] == (inputSize[1] - layer.convSize + 1));
	
	int const nx = layer.dataSize[0];
	int const ny = layer.dataSize[1];

	int const sliceInputSize = inputSize[0] * inputSize[1];
	int const convLen = layer.convSize * layer.convSize;
	int const nodeOutputSize = nx * ny;
	int const inputStride = inputSize[0];

	NNScalar const* pWeight = parameters + layer.weightOffset;
	NNScalar const* pBias = parameters + layer.biasOffset;
	NNScalar* pNodeOutput = outputs;

	for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
	{
		NNScalar const* pNodeWeight = pWeight + idxNode * numSliceInput * convLen;
		for (int j = 0; j < ny; ++j)
		{
			for (int i = 0; i < nx; ++i)
			{
				int idxInput = i + inputSize[0] * j;
				int idxOutput = i + nx * j;
				NNScalar const* pSliceInput = inputs + idxInput;
				NNScalar const* pSliceWeight = pNodeWeight;
				NNScalar value = pBias[idxNode];

				for (int idxSlice = 0; idxSlice < numSliceInput; ++idxSlice)
				{
					value += FNNMath::AreaConv(layer.convSize, inputStride, pSliceInput, pSliceWeight);
					pSliceWeight += convLen;
					pSliceInput += sliceInputSize;
				}

				pNodeOutput[idxOutput] = value;
			}
		}

		pNodeOutput += nodeOutputSize;
	}

	if (layer.funcTransform)
	{
		(*layer.funcTransform)(outputs, nodeOutputSize * layer.numNode);
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
