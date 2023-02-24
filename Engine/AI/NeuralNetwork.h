#pragma once
#ifndef NeuralNetwork_H_9D9E20F4_5B5B_45BE_A95C_5A2CEB755C3F
#define NeuralNetwork_H_9D9E20F4_5B5B_45BE_A95C_5A2CEB755C3F

#include "Math/Base.h"
#include "DataStructure/Array.h"

#include "Meta/MetaBase.h"
#include "Math/SIMD.h"
#include <cassert>



typedef float NNScalar;
typedef NNScalar (*NNActivationFunc)(NNScalar);
typedef void    (*NNActivationTrasnformFunc)(NNScalar* inoutValues, int numValues);
template< class T , class Q , class TFunc >
void Transform(T begin, T end, T dest, TFunc func = TFunc())
{
	T cur = begin;
	while( cur != end )
	{
		dest = func(*cur);
		++cur;
		++dest;
	}
}

class NNFunc
{
public:
	enum EName
	{
		Sigmoid ,
		Tanh ,
		ReLU ,
	};

	static NNScalar SigmoidFunc(NNScalar value)
	{
		if( value < -10 )
			return 0;
		if( value > 10 )
			return 1;
		return 1.0 / (1.0 + exp(-value));
	}

	static NNScalar SigmoidDif(NNScalar value)
	{
		if( value < -10 )
			return 0;
		if( value > 10 )
			return 0;
		NNScalar v = 1.0 / (1.0 + exp(-value));
		return v * (1 - v);
	}
	static NNScalar TanhFunc(NNScalar value)
	{
		if( value < -10 )
			return 0;
		if( value > 10 )
			return 1;

		NNScalar expV = exp(value);
		NNScalar expVInv = 1 / expV;

		return (expV - expVInv) / (expV + expVInv);
	}

	static NNScalar TanhDif(NNScalar value)
	{
		if( value < -10 )
			return 0;
		if( value > 10 )
			return 0;

		NNScalar expV = exp(value);
		NNScalar expVInv = 1 / expV;
		return (expV - expVInv) / (expV + expVInv);
	}

	static NNScalar ReLUFunc(NNScalar value)
	{
		return (value > 0) ? value : 0;
	}

	static NNScalar ReLUDif(NNScalar value)
	{
		return (value > 0) ? 1 : 0;
	}

};


template< NNFunc::EName >
struct NNFuncTraits {};

#define NN_FUNC_TRAITS( NAME , FUNC , FUNC_DIF )\
	template<>\
	struct NNFuncTraits< NAME >\
	{\
		constexpr static NNActivationFunc Func = FUNC;\
		constexpr static NNActivationFunc FuncDif = FUNC_DIF;\
	};

NN_FUNC_TRAITS(NNFunc::ReLU, NNFunc::ReLUFunc, NNFunc::ReLUDif);
NN_FUNC_TRAITS(NNFunc::Tanh, NNFunc::TanhFunc, NNFunc::TanhDif);
NN_FUNC_TRAITS(NNFunc::Sigmoid, NNFunc::SigmoidFunc, NNFunc::SigmoidDif);

struct NeuralLayer
{
	int weightOffset;
	NNActivationFunc func;
	NNActivationFunc funcDif;
	NNActivationTrasnformFunc funcTransform;
	int numNode;
	NeuralLayer()
	{
		numNode = 0;
		weightOffset = 0;
		setFuncionT< NNFunc::ReLU >();
	}

	template< NNFunc::EName Name >
	void setFuncionT()
	{
		func = NNFuncTraits<Name>::Func;
		funcDif = NNFuncTraits<Name>::FuncDif;
		funcTransform = Trasnform< NNFuncTraits<Name>::Func >;
	}

	template< NNActivationFunc Func >
	static void Trasnform(NNScalar* inoutValues, int numValues)
	{
		for (; numValues; --numValues)
		{
			*inoutValues = Func(*inoutValues);
			++inoutValues;
		}
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

	}
};

struct NerualPool2DLayer : NeuralLayer
{
	int PoolSize;


};

class FNNCalc
{
public:
	static void LayerFrontFeedback(
		NeuralFullConLayer const& layer, NNScalar* RESTRICT weightData, 
		int numInput, NNScalar* RESTRICT inputs, 
		NNScalar* RESTRICT outputs);

	static void LayerFrontFeedback(
		NeuralConv2DLayer const& layer, NNScalar* RESTRICT weightData, 
		int numSliceInput, int  inputSize[],
		NNScalar* RESTRICT inputs, NNScalar* RESTRICT outputs);

	static void LayerFrontFeedback(
		NeuralFullConLayer const& layer, NNScalar* RESTRICT weightData, 
		int numInput, NNScalar* RESTRICT inputs, 
		NNScalar* RESTRICT outputs, NNScalar* RESTRICT outNetworkInputs);


	static NNScalar VectorDot(int dim, NNScalar* RESTRICT a, NNScalar* RESTRICT b);
	static NNScalar VectorDotNOP(int dim, NNScalar* RESTRICT a, NNScalar* RESTRICT b);

	static FORCEINLINE NNScalar AreaConv(int dim, int stride, NNScalar* RESTRICT area, NNScalar* RESTRICT v)
	{
		NNScalar result = 0;
		for( int i = 0; i < dim; ++i )
		{
			result += VectorDot(dim, area, v);
			area += stride;
			v += dim;
		}
		return result;
	}
};

class FCNNLayout
{
public:
	void init(uint32 const topology[], uint32 dimNum);
	void init(uint32 numInput, uint32 numOutput, uint32 numHiddenLayer, uint32 const hiddenlayerNodeNum[]);

	NeuralFullConLayer& getLayer(int idx) { return mLayers[idx]; }
	NeuralFullConLayer const& getLayer(int idx) const { return mLayers[idx]; }
	void getTopology(TArray<uint32>& outTopology) const;


	int getPrevLayerNodeNum(int idxLayer) const;

	int getNetworkInputOffset(int idxLayer);

	int getHiddenLayerNum() const { return mLayers.size() - 1; }
	int getInputNum()  const { return mNumInput; }
	int getOutputNum() const { return mLayers.back().numNode; }
	void setActivationFunction(int idxLayer, NNActivationFunc func)
	{
		assert(func);
		mLayers[idxLayer].func = func;
	}

	int getHiddenNodeNum() const;

	int getMaxLayerNodeNum() const { return mMaxLayerNodeNum; }
	int getWeightNum() const;

	int mMaxLayerNodeNum;
	int mNumInput;
	TArray< NeuralFullConLayer > mLayers;
};

class FCNeuralNetwork
{
public:
	void init(FCNNLayout const& inLayout)
	{
		mLayout = &inLayout;
	}
	NNScalar* getWeights(int idxLayer, int idxNode);

	void setWeights(TArray< NNScalar >& weights)
	{
		assert(mLayout->getWeightNum() <= weights.size());
		mWeights = &weights[0];
	}
	
	void calcForwardFeedback(NNScalar inputs[], NNScalar outputs[]);
	void calcForwardFeedbackSignal(NNScalar inputs[], NNScalar outSingnals[]);
	void calcForwardFeedbackSignal(NNScalar inputs[], NNScalar outSingnals[], NNScalar outNetworkInputs[]);
	FCNNLayout const& getLayout() { return *mLayout; }
private:
	
	FCNNLayout const* mLayout = nullptr;
	NNScalar* mWeights;	
};

class Conv2DNeuralNetwork
{

};


class NeuralPiplineNode
{


};

#endif // NeuralNetwork_H_9D9E20F4_5B5B_45BE_A95C_5A2CEB755C3F
