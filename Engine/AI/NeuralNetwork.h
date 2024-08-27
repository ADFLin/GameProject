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


namespace NNFunc
{

	struct Linear
	{
		static NNScalar Value(NNScalar value)
		{
			return value;
		}
		static NNScalar Derivative(NNScalar value)
		{
			return 1.0f;
		}
	};

	struct Sigmoid
	{
		static NNScalar Value(NNScalar value)
		{
#if 0
			if (value < -10)
				return 0;
			if (value > 10)
				return 1;
#endif
			return NNScalar(1.0) / (NNScalar(1.0) + exp(-value));
		}

		static NNScalar Derivative(NNScalar value)
		{
#if 0
			if (value < -10)
				return 0;
			if (value > 10)
				return 0;
#endif
			NNScalar v = NNScalar(1.0) / (NNScalar(1.0) + exp(-value));
			return v * (NNScalar(1) - v);
		}
	};

	struct Tanh
	{
		static NNScalar Value(NNScalar value)
		{
#if 0
			if (value < -10)
				return 0;
			if (value > 10)
				return 1;
#endif

			NNScalar expV = exp(value);
			NNScalar expVInv = 1 / expV;

			return (expV - expVInv) / (expV + expVInv);
		}

		static NNScalar Derivative(NNScalar value)
		{
#if 0
			if (value < -10)
				return 0;
			if (value > 10)
				return 0;
#endif

			NNScalar expV = exp(value);
			NNScalar expVInv = 1 / expV;
			return (expV - expVInv) / (expV + expVInv);
		}
	};

	struct ReLU
	{
		static NNScalar Value(NNScalar value)
		{
			return (value > 0) ? value : 0;
		}

		static NNScalar Derivative(NNScalar value)
		{
			return (value > 0) ? NNScalar(1) : 0;
		}
	};



	template< NNActivationFunc Func >
	static void Trasnform(NNScalar* inoutValues, int numValues)
	{
		for (; numValues; --numValues)
		{
			*inoutValues = Func(*inoutValues);
			++inoutValues;
		}
	}
}

enum class ENNFunc : uint8
{
	Linear,
	Sigmoid,
	Tanh,
	ReLU,
};

template< ENNFunc >
struct NNFuncTraits {};

#define NN_FUNC_TRAITS( NAME , FUNC )\
	template<>\
	struct NNFuncTraits< NAME >\
	{\
		using FuncType = FUNC;\
	};

NN_FUNC_TRAITS(ENNFunc::ReLU, NNFunc::ReLU);
NN_FUNC_TRAITS(ENNFunc::Tanh, NNFunc::Tanh);
NN_FUNC_TRAITS(ENNFunc::Sigmoid, NNFunc::Sigmoid);
NN_FUNC_TRAITS(ENNFunc::Linear, NNFunc::Linear);



struct ActiveLayer
{
	NNActivationFunc funcDerivative;
	NNActivationTrasnformFunc funcTransform;

	template< ENNFunc Name >
	void setFuncionT()
	{
		using FuncType = NNFuncTraits<Name>::FuncType;
		funcDerivative = FuncType::Derivative;
		funcTransform = NNFunc::Trasnform< FuncType::Value >;
	}

};

struct NeuralLayer : ActiveLayer
{
	int numNode;
	int weightOffset;
	int biasOffset;

	NeuralLayer()
	{
		numNode = 0;
		weightOffset = 0; 
		biasOffset = 0;
		setFuncionT< ENNFunc::Sigmoid >();
	}
};

struct NeuralFullConLayer : NeuralLayer
{
	NeuralFullConLayer()
	{

	}

	void frontFeedback(
		NNScalar const* RESTRICT parameterData,
		int numInput, NNScalar const* RESTRICT inputs,
		NNScalar* RESTRICT outputs) const;

	void frontFeedback(
		NNScalar const* RESTRICT parameterData, 
		int numInput, NNScalar const* RESTRICT inputs,
		NNScalar* RESTRICT outputs, 
		NNScalar* RESTRICT outNetInputs) const;

	void frontFeedbackBatch(
		int batchSize,
		NNScalar const* RESTRICT parameterData,
		int numInput, NNScalar const* RESTRICT inputs,
		NNScalar* RESTRICT outputs,
		NNScalar* RESTRICT outNetInputs) const;
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
		NeuralConv2DLayer const& layer, 
		NNScalar const* RESTRICT weightData,
		int numSliceInput, int  inputSize[],
		NNScalar const* RESTRICT inputs, NNScalar* RESTRICT outputs);

	static void VectorAdd(int dim, NNScalar* RESTRICT a, NNScalar const* RESTRICT b);
	static void VectorAdd(int dim, NNScalar const* RESTRICT a, NNScalar const* RESTRICT b, NNScalar* RESTRICT out);

	static NNScalar VectorDot(int dim, NNScalar const* RESTRICT a, NNScalar const* RESTRICT b);
	static NNScalar VectorDot(int dim, NNScalar const* RESTRICT a, NNScalar const* RESTRICT b, int bStride);
	static NNScalar VectorDotNOP(int dim, NNScalar const* RESTRICT a, NNScalar const* RESTRICT b);

	static void VectorCopy(int dim, NNScalar const* RESTRICT v, NNScalar* RESTRICT out)
	{
		for (int i = 0; i < dim; ++i)
		{
			out[i] = v[i];
		}
	}

	static void MatrixMulTMatrixRT(int dimRow, int dimCol, NNScalar const* RESTRICT m, int dimCol2,  NNScalar const* RESTRICT m2, NNScalar* RESTRICT out)
	{
		for (int i = 0; i < dimCol2; ++i)
		{
			MatrixMulVector(dimRow, dimCol, m, m2, out);
			m2 += dimCol;
			out += dimCol;
		}
	}

	static void MatrixMulAddVector(int dimRow, int dimCol, NNScalar const* RESTRICT m, NNScalar const* RESTRICT v, NNScalar const* RESTRICT b, NNScalar* RESTRICT out);

	static void MatrixMulVector(int dimRow, int dimCol,  NNScalar const* RESTRICT m, NNScalar const* RESTRICT v, NNScalar* RESTRICT out)
	{
		for (int row = 0; row < dimRow; ++row)
		{
			out[row] = VectorDot(dimCol, m, v);
			m += dimCol;
		}
	}

	static void VectorMulMatrix(int dimRow,  int dimCol,  NNScalar const* RESTRICT m, NNScalar const* RESTRICT v, NNScalar* RESTRICT out)
	{
		for (int col = 0; col < dimCol; ++col)
		{
			out[col] = VectorDot(dimRow, m, v);
			m += dimRow;
		}
	}

	static void SoftMax(int dim, NNScalar const* RESTRICT inputs, NNScalar* outputs);

	static FORCEINLINE NNScalar AreaConv(int dim, int stride, NNScalar const* RESTRICT area, NNScalar const* RESTRICT v)
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



template< typename T >
struct TVectorView
{
	T*    mData;
	int   mSize;
};

template< typename T >
struct TMatrixView
{
	T*    mData;
	int   mRows, mCols;

	TVectorView<T> row(int idxRow)
	{
		TVectorView<T> result;
		result.mData = mData + idxRow * mCols;
		result.mSize = mCols;
		return result;
	}

	void mul(TVectorView<T> const& rhs, TVectorView<T>& out)
	{
		CHECK(mCols == mRows.mSize && mCols == out.mSize);
		FNNCalc::MatrixMulVector(mRows, mCols, mData, rhs.mData, out.mData);
	}
};


template< typename T >
struct TTransposedMatrixView
{
	T*    mData;
	int   mRows, mCols;


	void mul(TVectorView<T> const& rhs, TVectorView<T>& out)
	{
		CHECK(mRows == mRows.mSize && mRows == out.mSize);
		FNNCalc::VectorMulMatrix(mRows, mCols, mData, rhs.mData, out.mData);
	}
};

using NNMatrixView = TMatrixView<NNScalar>;
using NNVectorView = TVectorView<NNScalar>;



class FCNNLayout
{
public:
	void init(uint32 const topology[], uint32 dimNum);
	void init(uint32 numInput, uint32 numOutput, uint32 numHiddenLayer, uint32 const hiddenlayerNodeNum[]);

	NeuralFullConLayer& getLayer(int idx) { return mLayers[idx]; }
	NeuralFullConLayer const& getLayer(int idx) const { return mLayers[idx]; }
	void getTopology(TArray<uint32>& outTopology) const;


	int getLayerNodeWeigetNum(int idxLayer) const;

	int getNetInputOffset(int idxLayer) const;
	int getInputSignalOffset(int idxLayer) const;
	int getOutputSignalOffset(int idxLayer) const;

	int getHiddenLayerNum() const { return (int)mLayers.size() - 1; }
	int getInputNum()  const { return mNumInput; }
	int getOutputNum() const { return mLayers.back().numNode; }
	int getSignalNum() const { return getInputNum() + mTotalNodeNum; }


	int getHiddenNodeNum() const;

	int getMaxLayerNodeNum() const { return mMaxLayerNodeNum; }
	int getParameterNum() const;

	template< ENNFunc Name >
	void setActivationFunction(int idxLayer)
	{
		assert(func);
		mLayers[idxLayer].setFuncionT<Name>();
	}


	template< ENNFunc Name >
	void setHiddenLayerFunction()
	{
		for( int i = 0 ; i < mLayers.size() - 1; ++i)
		{
			mLayers[i].setFuncionT<Name>();
		}
	}

	template< ENNFunc Name >
	void setAllActivationFunction()
	{
		for (auto& layer : mLayers)
		{
			layer.setFuncionT<Name>();
		}
	}

	int mMaxLayerNodeNum;
	int mNumInput;
	int mTotalNodeNum;
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

	void setParamsters(TArray< NNScalar >& weights)
	{
		assert(mLayout->getParameterNum() <= weights.size());
		mParameters = &weights[0];
	}
	
	void calcForwardFeedback(NNScalar const inputs[], NNScalar outputs[]);
	void calcForwardFeedbackSignal(NNScalar const inInputs[], NNScalar outActivations[]);
	void calcForwardPassBatch(int batchSize, NNScalar const inInputs[], NNScalar outActivations[], NNScalar outNetInputs[]) const;
	void calcForwardPass(NNScalar const inInputs[], NNScalar outActivations[], NNScalar outNetInputs[]) const;
	void calcBackwardPass(NNScalar const inLossDerivatives[], NNScalar const inSignals[], NNScalar const inNetInputs[], NNScalar outLossGrads[], NNScalar outDeltaWeights[]) const;
	FCNNLayout const& getLayout() const { return *mLayout; }


	NNMatrixView getLayerWeight(int idxLayer) const
	{
		auto const& layout = mLayout->getLayer(idxLayer);
		NNMatrixView result;
		result.mData = mParameters + layout.weightOffset;
		result.mRows = layout.numNode;
		result.mCols = mLayout->getLayerNodeWeigetNum(idxLayer);
		return result;
	}
	NNVectorView getLayerBias(int idxLayer) const
	{
		auto const& layout = mLayout->getLayer(idxLayer);
		NNVectorView result;
		result.mData = mParameters + layout.biasOffset;
		result.mSize = layout.numNode;
		return result;
	}
private:
	
	FCNNLayout const* mLayout = nullptr;
	NNScalar* mParameters;	
};

class Conv2DNeuralNetwork
{

};


class NeuralPiplineNode
{


};

#endif // NeuralNetwork_H_9D9E20F4_5B5B_45BE_A95C_5A2CEB755C3F
