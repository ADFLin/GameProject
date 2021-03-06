#include "NeuralNetwork.h"

#include "Math/Base.h"
#include "CompilerConfig.h"


NNScale* FCNeuralNetwork::getWeights(int idxLayer, int idxNode)
{
	FCNNLayout const& NNLayout = getLayout();

	NeuralFullConLayer const& layer = NNLayout.mLayers[idxLayer];
	NNScale* result = &mWeights[layer.weightOffset];
	result += idxNode * (NNLayout.getPrevLayerNodeNum(idxLayer) + 1);
	return result;
}


void FCNeuralNetwork::calcForwardFeedbackSignal(NNScale inputs[], NNScale outSingnals[])
{
	FCNNLayout const& NNLayout = getLayout();

	NNScale* tempInputs = inputs;
	NNScale* tempOutputs = &outSingnals[0];
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

void FCNeuralNetwork::calcForwardFeedback(NNScale inputs[], NNScale outputs[])
{
	FCNNLayout const& NNLayout = getLayout();

	NNScale* tempInputs = (NNScale*)_alloca( 2 * NNLayout.getMaxLayerNodeNum() * sizeof(NNScale));
	NNScale* tempOutputs = tempInputs + NNLayout.getMaxLayerNodeNum();

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


void FCNeuralNetwork::calcForwardFeedbackSignal(NNScale inputs[], NNScale outSingnals[], NNScale outNetworkInputs[])
{
	FCNNLayout const& NNLayout = getLayout();

	NNScale* tempInputs = inputs;
	NNScale* tempOutputs = &outSingnals[0];
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


void FNNCalc::LayerFrontFeedback(NeuralFullConLayer const& layer, NNScale* RESTRICT weightData, int numInput, NNScale* RESTRICT inputs, NNScale* RESTRICT outputs, NNScale* RESTRICT outNetworkInputs )
{
	NNScale* pWeight = weightData + layer.weightOffset;
	NNScale* pInput = inputs;
	NNScale* pOutput = outputs;
	NNScale* pNetworkInput = outNetworkInputs;

	for( int i = 0; i < layer.numNode; ++i )
	{
		NNScale value = *pWeight;
		++pWeight;
		value += VectorDot(numInput, pWeight, pInput);
		pWeight += numInput;

		*pNetworkInput = value;
		++pNetworkInput;
		*pOutput = value;
		++pOutput;
	}

	if( layer.transformFunc )
	{
		(*layer.transformFunc)(outputs, layer.numNode);
	}
}

void FNNCalc::LayerFrontFeedback(NeuralFullConLayer const& layer, NNScale* RESTRICT weightData, int numInput, NNScale* RESTRICT inputs, NNScale* RESTRICT outputs)
{
	NNScale* pWeight = weightData + layer.weightOffset;
	NNScale* pInput = inputs;
	NNScale* pOutput = outputs;
	
	for( int i = 0; i < layer.numNode; ++i )
	{
		NNScale value = *pWeight;
		++pWeight;
		value += VectorDot(numInput, pWeight, pInput);
		pWeight += numInput;

		*pOutput = value;
		++pOutput;
	}

	if( layer.transformFunc )
	{
		(*layer.transformFunc)(outputs, layer.numNode);
	}
}

void FNNCalc::LayerFrontFeedback(NeuralConv2DLayer const& layer, NNScale* RESTRICT weightData, int numSliceInput, int inputSize[], NNScale* RESTRICT inputs, NNScale* RESTRICT outputs)
{

	int const nx = inputSize[0] - layer.convSize + 1;
	int const ny = inputSize[1] - layer.convSize + 1;

	int const sliceInputSize = inputSize[0] * inputSize[1];
	int const convLen = layer.convSize * layer.convSize;
	int const sliceOutputSize = nx * ny;
	int const inputStride = inputSize[0];

	NNScale* pWeight = weightData + layer.weightOffset;
	NNScale* pInput = inputs;
	NNScale* pOutput = outputs;

	for( int idxNode = 0; idxNode < layer.numNode; ++idxNode )
	{
#if 0	
		NNScale* pSliceWeight = pWeight;
		NNScale* pSliceInput = pInput;

		for( int idxSlice = 0; idxSlice < numSliceInput; ++idxSlice )
		{
			NNScale* pSliceOutput = pOutput;

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
		NNScale* pSliceOutput = outputs;
		for( int j = 0; j < ny; ++j )
		{
			for( int i = 0; i < nx; ++i )
			{
				int idx = i + inputSize[0] * j;

				NNScale* pSliceWeight = pWeight;
				NNScale* pSliceInput = pInput + idx;

				NNScale value = *pSliceWeight;
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


	if( layer.transformFunc )
	{
		int num = sliceOutputSize * layer.numNode;
		(*layer.transformFunc)(outputs, num);
	}
}

void FCNNLayout::init(int const topology[], int dimNum)
{
	assert(dimNum >= 2);
	int numInput = topology[0];
	int numOutput = topology[dimNum - 1];
	init(numInput, numOutput, dimNum - 2, topology + 1);
}

void FCNNLayout::init(int numInput, int numOutput, int numHiddenLayer, int const hiddenlayerNodeNum[])
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

void FCNNLayout::getTopology(std::vector<int>& outTopology)
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
