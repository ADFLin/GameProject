#pragma once
#ifndef NeuralNetwork_H_9D9E20F4_5B5B_45BE_A95C_5A2CEB755C3F
#define NeuralNetwork_H_9D9E20F4_5B5B_45BE_A95C_5A2CEB755C3F

#include "Math/Base.h"

#include <cassert>
#include <vector>

typedef double NNScale;
typedef NNScale (*NNActivationFun)(NNScale);

template< class T , class Q , class Fun >
void Transform(T begin, T end, T dest, Fun fun = Fun())
{
	T cur = begin;
	while( cur != end )
	{
		dest = fun(*cur);
		++cur;
		++dest;
	}
}

class NNFun
{
public:
	enum Name
	{
		SigmoidFun ,
		TanhFun ,
		ReLuFun ,
	};

	static NNScale Sigmoid(NNScale value)
	{
		if( value < -10 )
			return 0;
		if( value > 10 )
			return 1;
		return 1.0 / (1.0 + exp(-value));
	}


	static NNScale Tanh(NNScale value)
	{
		if( value < -10 )
			return 0;
		if( value > 10 )
			return 1;

		NNScale expV = exp(value);
		NNScale expVInv = 1 / expV;

		return (expV - expVInv) / (expV + expVInv);
	}

	static NNScale ReLU(NNScale value)
	{
		return (value > 0) ? value : 0;
	}

};


struct NeuralLayer
{
	int weightOffset;
	NNActivationFun fun;
	int numNode;
	NeuralLayer()
	{
		numNode = 0;
		weightOffset = 0;
		fun = NNFun::Sigmoid;
		//fun = NNFun::ReLU;
	}
};

struct NeuralFullConLayer : NeuralLayer
{
	NeuralFullConLayer()
	{

	}
};

struct NeuralConv2DLayer : NeuralLayer
{
	int convSize;
	int dataSize[2];

	NeuralConv2DLayer()
	{
		fun = NNFun::ReLU;
	}
};

struct NerualPool2DLayer : NeuralLayer
{
	int PoolSize;


};

class NNCalc
{
public:
	static void CalcLayerFrontFeedback(NeuralFullConLayer& layer, NNScale weightData[], int numInput, NNScale inputs[], NNScale outputs[]);
	static void CalcLayerFrontFeedback(NeuralConv2DLayer& layer, NNScale weightData[], int numSliceInput, int inputSize[], NNScale inputs[], NNScale outputs[]);
};

class FCNNLayout
{
public:
	void init(int const topology[], int dimNum);
	void init(int numInput, int numOutput, int numHiddenLayer, int const hiddenlayerNodeNum[]);

	NeuralFullConLayer& getLayer(int idx) { return mLayers[idx]; }

	void getTopology(std::vector<int>& outTopology);


	int getPrevLayerNodeNum(int idxLayer) const;

	int getHiddenLayerNum() const { return mLayers.size() - 1; }
	int getInputNum()  const { return mNumInput; }
	int getOutputNum() const { return mLayers.back().numNode; }
	void setActivationFunction(int idxLayer, NNActivationFun fun)
	{
		assert(fun);
		mLayers[idxLayer].fun = fun;
	}

	int getHiddenNodeNum() const;

	int getMaxLayerNodeNum() const { return mMaxLayerNodeNum; }
	int getWeightNum() const;

	int mMaxLayerNodeNum;
	int mNumInput;
	std::vector< NeuralFullConLayer > mLayers;
};

class FCNeuralNetwork : public NNCalc
{
public:


	void init(FCNNLayout& inLayout)
	{
		mLayout = &inLayout;
		mTempBuffer.resize(2 * mLayout->getMaxLayerNodeNum());
	}
	NNScale* getWeights(int idxLayer, int idxNode);

	void setWeights(std::vector< NNScale >& weights)
	{
		mWeights = &weights[0];
	}
	
	void calcForwardFeedbackSingnal(NNScale inputs[], NNScale outSingnals[]);
	void calcForwardFeedback(NNScale inputs[], NNScale outputs[] );

	FCNNLayout& getLayout(){  return *mLayout;  }
private:
	
	FCNNLayout* mLayout = nullptr;

	std::vector< NNScale > mTempBuffer;
	NNScale* mWeights;
	
};

class Conv2DNeuralNetwork
{

};


class NeuralPiplineNode
{


};

#endif // NeuralNetwork_H_9D9E20F4_5B5B_45BE_A95C_5A2CEB755C3F
