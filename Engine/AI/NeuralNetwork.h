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


struct ActiveLayer
{
	NNActivationFunc funcDerivative;
	NNActivationTrasnformFunc funcTransform;

	template< typename FuncType >
	void setFuncionT()
	{
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
		setFuncionT< NNFunc::Sigmoid >();
	}
};

struct NeuralFullConLayer : NeuralLayer
{
	int getOutputLength() const
	{
		return numNode;
	}
};

struct NeuralConv2DLayer : NeuralLayer
{
	int convSize;
	int dataSize[2];

	int getOutputLength() const
	{
		return dataSize[0] * dataSize[1] * numNode;
	}

	NeuralConv2DLayer()
	{

	}
};

struct NeuralMaxPooling2DLayer
{
	int numNode;
	int dataSize[2];
	int poolSize;

	int getOutputLength()
	{
		return dataSize[0] * dataSize[1] * numNode;
	}
};


class FNNMath
{
public:



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

	static int SoftMax(int dim, NNScalar const* RESTRICT inputs, NNScalar* outputs);

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
		FNNMath::MatrixMulVector(mRows, mCols, mData, rhs.mData, out.mData);
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
		FNNMath::VectorMulMatrix(mRows, mCols, mData, rhs.mData, out.mData);
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

	template< typename FuncType >
	void setActivationFunction(int idxLayer)
	{
		assert(func);
		mLayers[idxLayer].setFuncionT<FuncType>();
	}


	template< typename FuncType >
	void setHiddenLayerFunction()
	{
		for( int i = 0 ; i < mLayers.size() - 1; ++i)
		{
			mLayers[i].setFuncionT<FuncType>();
		}
	}

	template< typename FuncType >
	void setAllActivationFunction()
	{
		for (auto& layer : mLayers)
		{
			layer.setFuncionT<FuncType>();
		}
	}

	int mMaxLayerNodeNum;
	int mNumInput;
	int mTotalNodeNum;
	TArray< NeuralFullConLayer > mLayers;
};

class FNNAlgo
{
public:

	static void ForwardFeedback(
		NeuralFullConLayer const& layer, NNScalar const* RESTRICT parameters,
		int numInput, NNScalar const* RESTRICT inputs,
		NNScalar* RESTRICT outputs);

	static void ForwardFeedback(
		FCNNLayout const& layout, NNScalar const* parameters, 
		NNScalar const inputs[], 
		NNScalar outputs[]);

	static void ForwardFeedback(
		NeuralFullConLayer const& layer, NNScalar const* RESTRICT parameters, 
		int numInput, NNScalar const* RESTRICT inputs, 
		NNScalar* RESTRICT outputs, 
		NNScalar* RESTRICT outNetInputs);

	static void ForwardFeedbackBatch(
		NeuralFullConLayer const& layer, NNScalar const* RESTRICT parameters, 
		int numInput, NNScalar const* RESTRICT inputs, 
		int batchSize, 
		NNScalar* RESTRICT outputs,
		NNScalar* RESTRICT outNetInputs);

	static void ForwardPassBatch(
		FCNNLayout const& layout, NNScalar const* parameters, 
		NNScalar const inInputs[], int batchSize, 
		NNScalar outActivations[], 
		NNScalar outNetInputs[]);

	static void ForwardFeedbackSignal(
		FCNNLayout const& layout, NNScalar const* parameters,
		NNScalar const inInputs[],
		NNScalar outActivations[]);

	static void ForwardPass(
		FCNNLayout const& layout, NNScalar const* parameters, 
		NNScalar const inInputs[], 
		NNScalar outActivations[], 
		NNScalar outNetInputs[]);

	static void BackwardPass(
		FCNNLayout const& layout, NNScalar* parameters, 
		NNScalar const inLossDerivatives[], 
		NNScalar const inSignals[], 
		NNScalar const inNetInputs[], 
		NNScalar outLossGrads[], 
		NNScalar outDeltaWeights[]);

	static void ForwardFeedback(
		NeuralConv2DLayer const& layer, NNScalar const* RESTRICT parameters,
		int numSliceInput, int const inputSize[],
		NNScalar const* RESTRICT inputs,
		NNScalar* RESTRICT outputs);

	static void ForwardFeedback(
		NeuralMaxPooling2DLayer const& layer,
		int const inputSize[],
		NNScalar const* RESTRICT inputs,
		NNScalar* RESTRICT outputs);
};

class FCNeuralNetwork
{
public:
	void init(FCNNLayout const& inLayout)
	{
		mLayout = &inLayout;
	}
	NNScalar* getWeights(int idxLayer, int idxNode);
	FCNNLayout const& getLayout() const { return *mLayout; }


	void setParamsters(TArray< NNScalar >& weights)
	{
		assert(mLayout->getParameterNum() <= weights.size());
		mParameters = &weights[0];
	}
	
	void calcForwardFeedback(NNScalar const inputs[], NNScalar outputs[])
	{
		FNNAlgo::ForwardFeedback(getLayout(), mParameters, inputs, outputs);
	}
	void calcForwardFeedbackSignal(NNScalar const inInputs[], NNScalar outActivations[])
	{
		FNNAlgo::ForwardFeedbackSignal(getLayout(), mParameters, inInputs, outActivations);
	}
	void calcForwardPassBatch(NNScalar const inInputs[], int batchSize, NNScalar outActivations[], NNScalar outNetInputs[]) const
	{
		FNNAlgo::ForwardPassBatch(getLayout(), mParameters, inInputs, batchSize, outActivations, outNetInputs);
	}
	void calcForwardPass(NNScalar const inInputs[], NNScalar outActivations[], NNScalar outNetInputs[]) const
	{
		FNNAlgo::ForwardPass(getLayout(), mParameters, inInputs, outActivations, outNetInputs);
	}
	void calcBackwardPass(NNScalar const inLossDerivatives[], NNScalar const inSignals[], NNScalar const inNetInputs[], NNScalar outLossGrads[], NNScalar outDeltaWeights[]) const
	{
		FNNAlgo::BackwardPass(getLayout(), mParameters, inLossDerivatives, inSignals, inNetInputs, outLossGrads, outDeltaWeights);
	}

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
