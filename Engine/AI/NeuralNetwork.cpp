#include "NeuralNetwork.h"

#include "Math/Base.h"
#include "CompilerConfig.h"
#include "MarcoCommon.h"

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

#if CPP_COMPILER_MSVC
#define ALLOCA _alloca
#else
#define ALLOCA alloca
#endif
void FCNeuralNetwork::calcForwardFeedback(NNScalar const inputs[], NNScalar outputs[])
{
	FCNNLayout const& NNLayout = getLayout();

	if (NNLayout.mLayers.size() == 1)
	{
		NNLayout.mLayers[0].frontFeedback(mParameters, NNLayout.getInputNum(), inputs, outputs);
	}
	else
	{
		NNScalar* tempInputs = (NNScalar*)(ALLOCA((2 * NNLayout.getMaxLayerNodeNum()) * sizeof(NNScalar)));
		NNScalar* tempOutputs = tempInputs + NNLayout.getMaxLayerNodeNum();
		NNLayout.mLayers[0].frontFeedback(mParameters, NNLayout.getInputNum(), inputs, tempInputs);
		int curInputNum = NNLayout.mLayers[0].numNode;
		for (int i = 1; i < NNLayout.mLayers.size() - 1; ++i)
		{
			auto& layer = NNLayout.mLayers[i];
			layer.frontFeedback(mParameters, curInputNum, tempInputs, tempOutputs);
			curInputNum = layer.numNode;
			std::swap(tempInputs, tempOutputs);
		}
		NNLayout.mLayers.back().frontFeedback(mParameters, curInputNum, tempInputs, outputs);
	}

	//_freea(tempInputs);
}

void FCNeuralNetwork::calcForwardFeedbackSignal(NNScalar const inInputs[], NNScalar outActivations[])
{
	FCNNLayout const& NNLayout = getLayout();


	int curInputNum = NNLayout.getInputNum();
	NNScalar const* inputs = inInputs;
	NNScalar* outputs = outActivations;
	for (const NeuralFullConLayer& layer : NNLayout.mLayers)
	{
		//NNMatrixView weights = getWeights(layer);
		layer.frontFeedback(mParameters, curInputNum, inputs, outputs);
		curInputNum = layer.numNode;
		inputs = outputs;
		outputs += curInputNum;
	}
}

void FCNeuralNetwork::calcForwardPassBatch(int batchSize, NNScalar const inInputs[], NNScalar outActivations[], NNScalar outNetInputs[]) const
{
	FCNNLayout const& NNLayout = getLayout();

	int curInputNum = batchSize * NNLayout.getInputNum();
	NNScalar const* inputs = inInputs;
	NNScalar* outputs = outActivations;
	for (const NeuralFullConLayer& layer : NNLayout.mLayers)
	{
		layer.frontFeedbackBatch(batchSize, mParameters, curInputNum, inputs, outputs, outNetInputs);

		curInputNum = batchSize * layer.numNode;
		inputs = outputs;
		outputs += curInputNum;
		outNetInputs += curInputNum;
	}
}
void FCNeuralNetwork::calcForwardPass(NNScalar const inInputs[], NNScalar outActivations[], NNScalar outNetInputs[]) const
{
	FCNNLayout const& NNLayout = getLayout();

	int curInputNum = NNLayout.getInputNum();
	NNScalar const* inputs = inInputs;
	NNScalar* outputs = outActivations;
	for (const NeuralFullConLayer& layer : NNLayout.mLayers)
	{
		layer.frontFeedback(mParameters, curInputNum, inputs, outputs, outNetInputs);

		curInputNum = layer.numNode;
		inputs = outputs;
		outputs += curInputNum;
		outNetInputs += curInputNum;
	}
}

void FCNeuralNetwork::calcBackwardPass(
	NNScalar const inLossDerivatives[],
	NNScalar const inSignals[], 
	NNScalar const inNetInputs[], 
	NNScalar outLossGrads[], 
	NNScalar outDeltaWeights[]) const
{
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
	// dC/da[L,n] = -( t[n] - a[L,n]) 

	// dC/dz[L,n] = da/dz * dC/da =  F'(z[L,n]) * dC/da[L,n] = - F'(z[L,n]) * (t[n] - a[L,n]) 
	// dC/dz[l,n] = da/dz * dC/da =  F'(z[l,n]) * £Uk( dC/dz[l+1,k] * w[l+1,k,n] )

	// dC/dw[l,n,k] = dC/dz * dz/dw = dC/dz[l,n] * a[l-1,k]
	// dC/db[l,n] = dC/dz * dz/db = dC/dz[l,n]
	// w'[l,n,k] = w[l,n,k] + learnRate * dC/dw[l,n,k]
	// b'[l,n] = b[l,n] + learnRate * dC/db[l,n] 

	int totalNodeCount = mLayout->getHiddenNodeNum() + mLayout->getOutputNum();

	NNScalar* pLossGrad = outLossGrads + totalNodeCount;
	NNScalar const* pNetworkInputs = inNetInputs + totalNodeCount;
	NNScalar const* pInputSignals = inSignals + (mLayout->getInputNum() + mLayout->getHiddenNodeNum());


	{
		int idxLayer = mLayout->getHiddenLayerNum();
		auto const& layer = mLayout->getLayer(idxLayer);

		pNetworkInputs -= layer.numNode;
		CHECK(pNetworkInputs = inNetInputs + mLayout->getNetInputOffset(idxLayer));

		pLossGrad -= layer.numNode;
		CHECK(pLossGrad = outLossGrads + mLayout->getNetInputOffset(idxLayer));

		int numNodeWeiget = mLayout->getLayerNodeWeigetNum(idxLayer);
		pInputSignals -= numNodeWeiget;
		CHECK(pInputSignals = inSignals + mLayout->getInputSignalOffset(idxLayer));


		NNScalar* pDeltaWeightLayer = outDeltaWeights + layer.weightOffset;
		NNScalar* pDeltaBiasLayer = outDeltaWeights + layer.biasOffset;
		for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
		{
			NNScalar z_ln = pNetworkInputs[idxNode];
			NNScalar dCdz = layer.funcDerivative(z_ln) * inLossDerivatives[idxNode];

			pLossGrad[idxNode] = dCdz;
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

	for (int idxLayer = mLayout->getHiddenLayerNum() - 1; idxLayer >= 0; --idxLayer)
	{
		auto const& layer = mLayout->getLayer(idxLayer);
		auto const& nextLayer = mLayout->getLayer(idxLayer + 1);

		pNetworkInputs -= layer.numNode;
		CHECK(pNetworkInputs = inNetInputs + mLayout->getNetInputOffset(idxLayer));
		NNScalar* pSensivityNextLayer = pLossGrad;
		pLossGrad -= layer.numNode;
		CHECK(pLossGrad = outLossGrads + mLayout->getNetInputOffset(idxLayer));

		int numNodeWeiget = mLayout->getLayerNodeWeigetNum(idxLayer);
		pInputSignals -= numNodeWeiget;
		CHECK(pInputSignals = inSignals + mLayout->getInputSignalOffset(idxLayer));

		NNScalar* pWeightNext = mParameters + nextLayer.weightOffset;
		int weightStride = nextLayer.numNode;

		NNScalar* pDeltaWeightLayer = outDeltaWeights + layer.weightOffset;
		NNScalar* pDeltaBiasLayer = outDeltaWeights + layer.biasOffset;
		for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
		{
			NNScalar z_ln = pNetworkInputs[idxNode];
			NNScalar dCdz = layer.funcDerivative(z_ln) * FNNCalc::VectorDot(nextLayer.numNode, pSensivityNextLayer, pWeightNext + idxNode, weightStride);

			pLossGrad[idxNode] = dCdz;
			
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
}


void FNNCalc::LayerFrontFeedback(NeuralConv2DLayer const& layer, NNScalar const* RESTRICT weightData, int numSliceInput, int inputSize[], NNScalar const* RESTRICT inputs, NNScalar* RESTRICT outputs)
{

	int const nx = inputSize[0] - layer.convSize + 1;
	int const ny = inputSize[1] - layer.convSize + 1;

	int const sliceInputSize = inputSize[0] * inputSize[1];
	int const convLen = layer.convSize * layer.convSize;
	int const sliceOutputSize = nx * ny;
	int const inputStride = inputSize[0];

	NNScalar const* pWeight = weightData + layer.weightOffset;
	NNScalar const* pInput = inputs;
	NNScalar* pOutput = outputs;

	for( int idxNode = 0; idxNode < layer.numNode; ++idxNode )
	{
#if 0	
		NNScalar const* pSliceWeight = pWeight;
		NNScalar const* pSliceInput = pInput;

		for( int idxSlice = 0; idxSlice < numSliceInput; ++idxSlice )
		{
			NNScalar* pSliceOutput = pOutput;

			std::fill_n(pSliceOutput, sliceOutputSize, *pSliceWeight);
			++pSliceWeight;

			for( int j = 0; j < ny; ++j )
			{
				for( int i = 0; i < nx; ++i )
				{
					int idx = i + inputSize[0] * j;
					*pSliceOutput += AreaConv(layer.convSize, inputStride, pSliceInput + idx , pSliceWeight);
					++pSliceOutput;
				}
			}

			pSliceWeight += convLen;
			pSliceInput += sliceInputSize;
			
		}
#else
		NNScalar* pSliceOutput = outputs;
		for( int j = 0; j < ny; ++j )
		{
			for( int i = 0; i < nx; ++i )
			{
				int idx = i + inputSize[0] * j;

				NNScalar const* pSliceWeight = pWeight;
				NNScalar const* pSliceInput = pInput + idx;

				NNScalar value = *pSliceWeight;
				++pSliceWeight;
				for( int idxSlice = 0; idxSlice < numSliceInput; ++idxSlice )
				{
					value += AreaConv(layer.convSize, inputStride, pSliceInput, pSliceWeight);
					pSliceWeight += convLen;
					pSliceInput += sliceInputSize;
				}
				*pSliceOutput = value;
			}
		}
#endif

		pOutput += sliceOutputSize;
	}


	if( layer.funcTransform )
	{
		int num = sliceOutputSize * layer.numNode;
		(*layer.funcTransform)(outputs, num);
	}
}

void FNNCalc::VectorAdd(int dim, NNScalar* RESTRICT a, NNScalar const* RESTRICT b)
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

void FNNCalc::VectorAdd(int dim, NNScalar const* RESTRICT a, NNScalar const* RESTRICT b, NNScalar* RESTRICT out)
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

NNScalar FNNCalc::VectorDot(int dim, NNScalar const* RESTRICT a, NNScalar const* RESTRICT b)
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


NNScalar FNNCalc::VectorDot(int dim, NNScalar const* RESTRICT a, NNScalar const* RESTRICT b, int bStride)
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

NNScalar FNNCalc::VectorDotNOP(int dim, NNScalar const* RESTRICT a, NNScalar const* RESTRICT b)
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

void FNNCalc::MatrixMulAddVector(int dimRow, int dimCol, NNScalar const* RESTRICT m, NNScalar const* RESTRICT v, NNScalar const* RESTRICT b, NNScalar* RESTRICT out)
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

void FNNCalc::SoftMax(int dim, NNScalar const* RESTRICT inputs, NNScalar* outputs)
{
	NNScalar sum = 0.0;
	for (int i = 0; i < dim; ++i)
	{
		outputs[i] = exp(inputs[i]);
		sum += outputs[i];
	}

	for (int i = 0; i < dim; ++i)
	{
		outputs[i] /= sum;
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

void NeuralFullConLayer::frontFeedback(NNScalar const* RESTRICT parameterData, int numInput, NNScalar const* RESTRICT inputs, NNScalar* RESTRICT outputs) const
{
	FNNCalc::MatrixMulAddVector(numNode, numInput,  parameterData + weightOffset, inputs, parameterData + biasOffset, outputs);

	if (funcTransform)
	{
		(*funcTransform)(outputs, numNode);
	}
}

void NeuralFullConLayer::frontFeedback(NNScalar const* RESTRICT parameterData, int numInput, NNScalar const* RESTRICT inputs, NNScalar* RESTRICT outputs, NNScalar* RESTRICT outNetInputs) const
{
	NNScalar const* pWeight = parameterData + weightOffset;
	NNScalar const* pBias = parameterData + biasOffset;
	NNScalar const* pInput = inputs;
	NNScalar* pOutput = outputs;
	NNScalar* pNetInput = outNetInputs;

	for (int i = 0; i < numNode; ++i)
	{
		NNScalar value = pBias[0] + FNNCalc::VectorDot(numInput, pWeight, pInput);
		pWeight += numInput;
		pBias += 1;

		*pNetInput = value;
		++pNetInput;
		*pOutput = value;
		++pOutput;
	}

	if (funcTransform)
	{
		(*funcTransform)(outputs, numNode);
	}
}

void NeuralFullConLayer::frontFeedbackBatch(
	int batchSize, NNScalar const* RESTRICT parameterData, 
	int numInput, NNScalar const* RESTRICT inputs, 
	NNScalar* RESTRICT outputs, NNScalar* RESTRICT outNetInputs) const
{
	NNScalar const* pWeight = parameterData + weightOffset;
	NNScalar const* pBias = parameterData + biasOffset;
	NNScalar const* pInput = inputs;
	NNScalar* pOutput = outputs;
	NNScalar* pNetInput = outNetInputs;

	FNNCalc::MatrixMulTMatrixRT(numNode, numInput, parameterData + weightOffset, batchSize, inputs, pNetInput);

	FNNCalc::VectorCopy(batchSize * numNode, pNetInput, outputs);

	if (funcTransform)
	{
		(*funcTransform)(outputs, batchSize * numNode);
	}
}
