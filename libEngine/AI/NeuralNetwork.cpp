#include "NeuralNetwork.h"

#include "Math/Base.h"
#include "CompilerConfig.h"


NNScalar* FCNeuralNetwork::getWeights(int idxLayer, int idxNode)
{
	FCNNLayout const& NNLayout = getLayout();

	NeuralFullConLayer const& layer = NNLayout.mLayers[idxLayer];
	NNScalar* result = &mWeights[layer.weightOffset];
	result += idxNode * (NNLayout.getPrevLayerNodeNum(idxLayer) + 1);
	return result;
}


void FCNeuralNetwork::calcForwardFeedbackSignal(NNScalar inputs[], NNScalar outSingnals[])
{
	FCNNLayout const& NNLayout = getLayout();

	NNScalar* tempInputs = inputs;
	NNScalar* tempOutputs = &outSingnals[0];
	int curInputNum = NNLayout.getInputNum();
	for( int i = 0; i < NNLayout.mLayers.size(); ++i )
	{
		auto const& layer = NNLayout.mLayers[i];
		FNNCalc::LayerFrontFeedback(layer, mWeights, curInputNum, tempInputs, tempOutputs);
		curInputNum = layer.numNode;
		tempInputs = tempOutputs;
		tempOutputs += curInputNum;
	}
}

template< typename T >
T  AlignUp(T  value, T  align)
{
	return (value + align - 1) & ~(align - 1);
}

void FCNeuralNetwork::calcForwardFeedback(NNScalar inputs[], NNScalar outputs[])
{
	FCNNLayout const& NNLayout = getLayout();

	NNScalar* tempInputs = (NNScalar*)(_alloca((2 * NNLayout.getMaxLayerNodeNum() + 3) * sizeof(NNScalar)));
	NNScalar* tempOutputs = tempInputs + NNLayout.getMaxLayerNodeNum();

	FNNCalc::LayerFrontFeedback(NNLayout.mLayers[0], mWeights , NNLayout.getInputNum(), inputs, tempInputs);
	int curInputNum = NNLayout.mLayers[0].numNode;
	for( int i = 1; i < NNLayout.mLayers.size() - 1; ++i )
	{
		auto& layer = NNLayout.mLayers[i];
		FNNCalc::LayerFrontFeedback(layer, mWeights, curInputNum, tempInputs, tempOutputs);
		curInputNum = layer.numNode;
		std::swap(tempInputs, tempOutputs);
	}
	FNNCalc::LayerFrontFeedback(NNLayout.mLayers.back(), mWeights, curInputNum, tempInputs, outputs);
	//_freea(tempInputs);
}


void FCNeuralNetwork::calcForwardFeedbackSignal(NNScalar inputs[], NNScalar outSingnals[], NNScalar outNetworkInputs[])
{
	FCNNLayout const& NNLayout = getLayout();

	NNScalar* tempInputs = inputs;
	NNScalar* tempOutputs = &outSingnals[0];
	int curInputNum = NNLayout.getInputNum();
	for(const NeuralFullConLayer& layer : NNLayout.mLayers)
	{
		FNNCalc::LayerFrontFeedback(layer, mWeights, curInputNum, tempInputs, tempOutputs , outNetworkInputs );

		tempInputs = tempOutputs;
		tempOutputs += layer.numNode;
		outNetworkInputs += layer.numNode;
		curInputNum = layer.numNode;
	}
}

void FNNCalc::LayerFrontFeedback(NeuralFullConLayer const& layer, NNScalar* RESTRICT weightData, int numInput, NNScalar* RESTRICT inputs, NNScalar* RESTRICT outputs, NNScalar* RESTRICT outNetworkInputs)
{
	NNScalar* pWeight = weightData + layer.weightOffset;
	NNScalar* pInput = inputs;
	NNScalar* pOutput = outputs;
	NNScalar* pNetworkInput = outNetworkInputs;

	for (int i = 0; i < layer.numNode; ++i)
	{
		NNScalar value = *pWeight;
		++pWeight;
		value += VectorDotNOP(numInput, pWeight, pInput);
		pWeight += numInput;

		*pNetworkInput = value;
		++pNetworkInput;
		*pOutput = value;
		++pOutput;
	}

	if (layer.funcTransform)
	{
		(*layer.funcTransform)(outputs, layer.numNode);
	}
}

void FNNCalc::LayerFrontFeedback(NeuralFullConLayer const& layer, NNScalar* RESTRICT weightData, int numInput, NNScalar* RESTRICT inputs, NNScalar* RESTRICT outputs)
{
	NNScalar* pWeight = weightData + layer.weightOffset;
	NNScalar* pInput = inputs;
	NNScalar* pOutput = outputs;
	
	for( int i = 0; i < layer.numNode; ++i )
	{
		NNScalar value = *pWeight;
		++pWeight;
		value += VectorDot(numInput, pWeight, pInput);
		pWeight += numInput;

		*pOutput = value;
		++pOutput;
	}

	if( layer.funcTransform )
	{
		(*layer.funcTransform)(outputs, layer.numNode);
	}
}

void FNNCalc::LayerFrontFeedback(NeuralConv2DLayer const& layer, NNScalar* RESTRICT weightData, int numSliceInput, int inputSize[], NNScalar* RESTRICT inputs, NNScalar* RESTRICT outputs)
{

	int const nx = inputSize[0] - layer.convSize + 1;
	int const ny = inputSize[1] - layer.convSize + 1;

	int const sliceInputSize = inputSize[0] * inputSize[1];
	int const convLen = layer.convSize * layer.convSize;
	int const sliceOutputSize = nx * ny;
	int const inputStride = inputSize[0];

	NNScalar* pWeight = weightData + layer.weightOffset;
	NNScalar* pInput = inputs;
	NNScalar* pOutput = outputs;

	for( int idxNode = 0; idxNode < layer.numNode; ++idxNode )
	{
#if 0	
		NNScalar* pSliceWeight = pWeight;
		NNScalar* pSliceInput = pInput;

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

				NNScalar* pSliceWeight = pWeight;
				NNScalar* pSliceInput = pInput + idx;

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

NNScalar FNNCalc::VectorDot(int dim, NNScalar* RESTRICT a, NNScalar* RESTRICT b)
{
	NNScalar result = 0;
#if USE_MATH_SIMD 
	if constexpr (Meta::IsSameType< NNScalar, float >::Value)
	{
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
	}
	else
#endif
	{

		for (; dim; --dim)
		{
			result += (*a++) * (*b++);
		}
	}
	return result;
}


NNScalar FNNCalc::VectorDotNOP(int dim, NNScalar* RESTRICT a, NNScalar* RESTRICT b)
{
	NNScalar result = 0;
	for (; dim; --dim)
	{
		result += (*a++) * (*b++);
	}
	return result;
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
		offset += (curInputNum + 1)* layer.numNode;
		curInputNum = layer.numNode;
	}

	{
		auto& layer = mLayers[numHiddenLayer];
		layer.numNode = numOutput;
		layer.weightOffset = offset;
		if( mMaxLayerNodeNum < layer.numNode )
			mMaxLayerNodeNum = layer.numNode;
	}
}

void FCNNLayout::getTopology(std::vector<uint32>& outTopology) const
{
	outTopology.resize(mLayers.size() + 1);
	outTopology[0] = mNumInput;
	for( int i = 0; i < mLayers.size(); ++i )
	{
		outTopology[i + 1] = mLayers[i].numNode;
	}
}

int FCNNLayout::getPrevLayerNodeNum(int idxLayer) const
{
	return (idxLayer == 0) ? mNumInput : mLayers[idxLayer - 1].numNode;
}

int FCNNLayout::getNetworkInputOffset(int idxLayer)
{
	int result = 0;
	for( int i = 0; i < idxLayer; ++i )
	{
		result += mLayers[i].numNode;
	}
	return result;
}

int FCNNLayout::getHiddenNodeNum() const
{
	int result = 0;
	for( int i = 0; i < mLayers.size() - 1; ++i )
	{
		result += mLayers[i].numNode;
	}
	return result;
}

int FCNNLayout::getWeightNum() const
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
