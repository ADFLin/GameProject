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
		if constexpr (Meta::IsSameType<FuncType, NNFunc::Linear>::Value)
		{
			funcTransform = nullptr;
		}
		else
		{
			funcTransform = NNFunc::Trasnform< FuncType::Value >;
		}
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
	enum FastMethod
	{
		eNone,
		eF23,
		eF43,
	};

	FastMethod fastMethod = FastMethod::eNone;
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
	static void VectorMul(int dim, NNScalar* RESTRICT a, NNScalar const* RESTRICT b);
	static void VectorAdd(int dim, NNScalar const* RESTRICT a, NNScalar const* RESTRICT b, NNScalar* RESTRICT out);
	// out = a * b + c
	FORCEINLINE static void VectorMulAdd(int dim, NNScalar const* RESTRICT a, NNScalar const* RESTRICT b, NNScalar const* RESTRICT c, NNScalar* RESTRICT out)
	{
		for (int i = 0; i < dim; ++i)
		{
			out[i] = a[i] * b[i] + c[i];
		}
	}

	FORCEINLINE static void VectorMulAdd(int dim, NNScalar const* RESTRICT a, NNScalar const* RESTRICT b, NNScalar* RESTRICT inout)
	{
		for (int i = 0; i < dim; ++i)
		{
			inout[i] += a[i] * b[i];
		}
	}

	static NNScalar VectorDot(int dim, NNScalar const* RESTRICT a, NNScalar const* RESTRICT b);
	static NNScalar VectorDot(int dim, NNScalar const* RESTRICT a, NNScalar const* RESTRICT b, int bStride);
	static NNScalar VectorDot(int dim, NNScalar const* RESTRICT a, int aStride, NNScalar const* RESTRICT b, int bStride);
	static NNScalar VectorDotNOP(int dim, NNScalar const* RESTRICT a, NNScalar const* RESTRICT b);

	FORCEINLINE static void VectorCopy(int dim, NNScalar const* RESTRICT v, NNScalar* RESTRICT out)
	{
		for (int i = 0; i < dim; ++i)
		{
			out[i] = v[i];
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
			out[col] = VectorDot(dimRow, v, m, dimRow);
			m += 1;
		}
	}

	static void MatrixMulMatrix(int dimRow, int dimCol, NNScalar const* RESTRICT m, int dimCol2, NNScalar const* RESTRICT m2, NNScalar* RESTRICT out)
	{
		NNScalar* RESTRICT pOut = out;
		for (int row = 0; row < dimRow; ++row)
		{
			for (int col = 0; col < dimCol2; ++col)
			{
				*pOut = VectorDot(dimCol, m, m2 + col, dimCol);
				++pOut;
			}

			m += dimCol;
		}
	}

	static void MatrixMulMatrix(int dimRow, int dimCol, NNScalar const* RESTRICT m, int dimCol2, NNScalar const* RESTRICT m2, int rowStride2, NNScalar* RESTRICT out)
	{
		NNScalar* RESTRICT pOut = out;
		for (int row = 0; row < dimRow; ++row)
		{
			for (int col = 0; col < dimCol2; ++col)
			{
				*pOut = VectorDot(dimCol, m, m2 + col, rowStride2);
				++pOut;
			}

			m += dimCol;
		}
	}

	static void MatrixMulMatrixT(int dimRow, int dimCol, NNScalar const* RESTRICT m, int dimCol2, NNScalar const* RESTRICT m2, NNScalar* RESTRICT out)
	{
		NNScalar* RESTRICT pOut = out;
		for (int row = 0; row < dimRow; ++row)
		{
			NNScalar const* RESTRICT pM2 = m2;
			for (int col = 0; col < dimCol2; ++col)
			{
				*pOut = VectorDot(dimCol, m, pM2);
				++pOut;
				pM2 += dimCol;
			}
			m += dimCol;
		}
	}

	static int SoftMax(int dim, NNScalar const* RESTRICT inputs, NNScalar* outputs);

	static int Max(int dim, NNScalar const* inputs);

	static void GetNormalizeParams(int dim, NNScalar const* RESTRICT inputs, NNScalar& outMean, NNScalar& outVariance)
	{
		NNScalar mean = 0.0;
		for( int i = 0; i < dim; ++i)
		{
			mean += inputs[i];
		}
		mean /= dim;

		NNScalar variance = 0;
		for (int i = 0; i < dim; ++i)
		{
			variance += Math::Square(inputs[i] - mean);
		}
		variance /= dim;
		variance = Math::Sqrt( variance + 1e-5 );

		outMean = mean;
		outVariance = variance;
	}

	static void Normalize(int dim, NNScalar const* RESTRICT inputs, NNScalar mean, NNScalar variance, NNScalar* output)
	{
		for (int i = 0; i < dim; ++i)
		{
			output[i] = (inputs[i] - mean) / variance;
		}
	}

	static void Normalize(int dim, NNScalar const* RESTRICT inputs, NNScalar* output)
	{
		NNScalar mean, variance;
		GetNormalizeParams(dim, inputs, mean, variance);
		Normalize(dim, inputs, mean, variance, output);
	}


	static void ClipNormalize(int dim, NNScalar* RESTRICT inoutValues, NNScalar maxValue)
	{
		NNScalar total = 0.0;
		for (int i = 0; i < dim; ++i)
		{
			total += Math::Square(inoutValues[i]);
		}

		NNScalar clipCoef = maxValue / (Math::Sqrt(total) + 1e-6);
		if (clipCoef < 1)
		{
			for (int i = 0; i < dim; ++i)
			{
				inoutValues[i] *= clipCoef;
			}
		}
	}

	static FORCEINLINE NNScalar AreaConv(int dim, int stride, NNScalar const* RESTRICT area, NNScalar const* RESTRICT weight)
	{
		NNScalar result = 0;
		for( int i = 0; i < dim; ++i )
		{
			result += VectorDot(dim, area, weight);
			area += stride;
			weight += dim;
		}
		return result;
	}

	static void TranformAreaF23(int rowStride, int numSlice, int sliceStride, NNScalar const* RESTRICT area, NNScalar* RESTRICT outArea);
	static void AreaConvF23(NNScalar inoutV[], int numSlice, NNScalar const* RESTRICT area, NNScalar const* RESTRICT weight);
	static void TranformAreaF43(int rowStride, int numSlice, int sliceStride, NNScalar const* RESTRICT area, NNScalar* RESTRICT outArea);
	static void AreaConvF43(NNScalar inoutV[], int numSlice, NNScalar const* RESTRICT area, NNScalar const* RESTRICT weight);

};



struct WinogradKernel23
{
	static int constexpr WeightSize = 4;
	static int constexpr O = 2;
	static int constexpr ConvSize = 3;

	static NNScalar constexpr G[] = 
	{   
			1,      0,     0,
		1/2.0,  1/2.0, 1/2.0,
		1/2.0, -1/2.0, 1/2.0,
			0,      0,     1,
	};
	static NNScalar constexpr At[] =
	{
		1, 1,  1, 0,
		0, 1, -1, -1,
	};
	static NNScalar constexpr Bt[] =
	{
		1, 0, -1, 0,
		0, 1,  1, 0,
		0, -1, 1, 0,
		0, 1,  0, -1,
	};

	static void TranformArea(int rowStride, int numSlice, int sliceStride, NNScalar const* RESTRICT area, NNScalar* RESTRICT outArea)
	{
		FNNMath::TranformAreaF23(rowStride, numSlice, sliceStride, area, outArea);
	}
	static void AreaConv(NNScalar inoutV[], int numSlice, NNScalar const* RESTRICT area, NNScalar const* RESTRICT weight)
	{
		FNNMath::AreaConvF23(inoutV, numSlice, area, weight);
	}
};

struct WinogradKernel43
{
	static int constexpr WeightSize = 6;
	static int constexpr O = 4;
	static int constexpr ConvSize = 3;

	static NNScalar constexpr G[] =
	{
		 1/4.0,       0,      0,
		-1/6.0,  -1/6.0, -1/6.0,
        -1/6.0,   1/6.0, -1/6.0,
		1/24.0,  1/12.0,  1/6.0,
		1/24.0, -1/12.0,  1/6.0,
		     0,       0,      1,
	};
	static NNScalar constexpr At[] =
	{
		1, 1,  1, 1,  1, 0,
		0, 1, -1, 2, -2, 0,
		0, 1,  1, 4,  4, 0,
		0, 1, -1, 8, -8, 1,
	};
	static NNScalar constexpr Bt[] =
	{
		4,  0, -5,  0, 1, 0,
		0, -4, -4,  1, 1, 0,
		0,  4, -4, -1, 1, 0,
		0, -2, -1,  2, 1, 0,
		0,  2, -1, -2, 1, 0,
		0,  4,  0, -5, 0, 1,
	};

	static void TranformArea(int rowStride, int numSlice, int sliceStride, NNScalar const* RESTRICT area, NNScalar* RESTRICT outArea)
	{
		FNNMath::TranformAreaF43(rowStride, numSlice, sliceStride, area, outArea);
	}
	static void AreaConv(NNScalar inoutV[], int numSlice, NNScalar const* RESTRICT area, NNScalar const* RESTRICT weight)
	{
		FNNMath::AreaConvF43(inoutV, numSlice, area, weight);
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


	int getLayerInputNum(int idxLayer) const;

	int getInputSignalOffset(int idxLayer, bool bHadActiveInput) const;
	int getOutputSignalOffset(int idxLayer, bool bHadActiveInput) const;

	int getHiddenLayerNum() const { return (int)mLayers.size() - 1; }
	int getInputNum()  const { return mNumInput; }
	int getOutputNum() const { return mLayers.back().numNode; }
	int getSignalNum() const { return getInputNum() + mTotalNodeNum; }
	int getTotalNodeNum() const { return mTotalNodeNum; }

	int getPassSignalNum() const
	{
		return getSignalNum() + getTotalNodeNum();
	}

	int getHiddenNodeNum() const;

	int getMaxLayerNodeNum() const { return mMaxLayerNodeNum; }
	int getParameterNum() const;

	template< typename FuncType >
	void setActivationFunction(int idxLayer)
	{
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
	static NNScalar* ForwardFeedback(
		NeuralFullConLayer const& layer, NNScalar const* RESTRICT parameters,
		int numInput, NNScalar const* RESTRICT inputs,
		NNScalar* RESTRICT outputs, bool bOutputActiveInput);

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
		NNScalar outOutputs[]);

	static void BackwardPass(
		FCNNLayout const& layout, NNScalar* parameters, 
		NNScalar const inSignals[], 
		NNScalar const inLossDerivatives[],
		NNScalar outLossGrads[], 
		NNScalar outDeltaWeights[]);

	static void BackwardPass(
		NeuralFullConLayer const& layer, NNScalar* parameters,
		int numInput, NNScalar const inInputs[],
		NNScalar const inOutputs[],
		NNScalar  inoutLossGradsInput[],
		NNScalar  outDeltaWeights[],
		NNScalar* outLossGrads);

	static void BackwardPassWeight(
		NeuralFullConLayer const& layer, int numInput,
		NNScalar const inInput[], 
		NNScalar const inLossDerivatives[], 
		NNScalar outDeltaWeights[]);

	static NNScalar* ForwardFeedback(
		NeuralConv2DLayer const& layer, NNScalar const* RESTRICT parameters,
		int numSliceInput, int const inputSize[],
		NNScalar const* RESTRICT inputs,
		NNScalar* RESTRICT outputs, bool bOutputActive = false);

	static void BackwardPassLoss(
		NeuralConv2DLayer const& layer, NNScalar const* RESTRICT parameters,
		int numSliceInput, int const inputSize[],
		NNScalar const inLossDerivatives[],
		NNScalar outLossGrads[]);

	static void BackwardPassWeight(
		NeuralConv2DLayer const& layer,
		int numSliceInput, int const inputSize[],
		NNScalar const inSignals[],
		NNScalar const inNetInputs[],
		NNScalar inoutLossDerivatives[],
		NNScalar outDeltaWeights[]);

	static NNScalar* ForwardFeedback(
		NeuralMaxPooling2DLayer const& layer,
		int const inputSize[],
		NNScalar const* RESTRICT inputs,
		NNScalar* RESTRICT outputs);

	static void BackwardPass(
		NeuralMaxPooling2DLayer const& layer,
		int const inputSize[],
		NNScalar const inInputs[],
		NNScalar const inOutputs[],
		NNScalar const inLossDerivatives[],
		NNScalar outLossGrads[]);


	static void ForwardPass(
		ActiveLayer const& layer,
		int numNode,
		NNScalar const inInputs[],
		NNScalar outOutputs[]);


	static void BackwardPass(
		ActiveLayer const& layer, 
		int numNode, 
		NNScalar const inInputs[], 
		NNScalar inoutLossDerivatives[]);

	static void BackwardPass(
		ActiveLayer const& layer,
		int numNode,
		NNScalar const inInputs[],
		NNScalar const inLossDerivatives[],
		NNScalar outLossGrads[]);


	static void SoftMaxBackwardPass(
		int numNode,
		NNScalar const inInputs[],
		NNScalar const inOutputs[],
		NNScalar const inLossDerivatives[],
		NNScalar outLossGrads[]);
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
	void calcForwardFeedbackSignal(NNScalar const inInputs[], NNScalar outSignals[])
	{
		FNNMath::VectorCopy(mLayout->getInputNum(), inInputs, outSignals);
		NNScalar* outActivations = outSignals + mLayout->getInputNum();
		FNNAlgo::ForwardFeedbackSignal(getLayout(), mParameters, inInputs, outActivations);
	}
	void calcForwardPassBatch(NNScalar const inInputs[], int batchSize, NNScalar outActivations[], NNScalar outNetInputs[]) const
	{
		FNNAlgo::ForwardPassBatch(getLayout(), mParameters, inInputs, batchSize, outActivations, outNetInputs);
	}

	NNScalar* calcForwardPass(NNScalar const inInputs[], NNScalar outSignals[]) const
	{
		FNNMath::VectorCopy(mLayout->getInputNum(), inInputs, outSignals);
		NNScalar* outOutputs = outSignals + mLayout->getInputNum();
		FNNAlgo::ForwardPass(getLayout(), mParameters, inInputs, outOutputs);
		return outOutputs + (2 * mLayout->getHiddenNodeNum() + mLayout->getOutputNum());
	}

	void calcBackwardPass(NNScalar const inLossDerivatives[], NNScalar const inSignals[], NNScalar outLossGrads[], NNScalar outDeltaWeights[]) const
	{
		FNNAlgo::BackwardPass(getLayout(), mParameters, inSignals, inLossDerivatives, outLossGrads, outDeltaWeights);
	}

	NNMatrixView getLayerWeight(int idxLayer) const
	{
		auto const& layout = mLayout->getLayer(idxLayer);
		NNMatrixView result;
		result.mData = mParameters + layout.weightOffset;
		result.mRows = layout.numNode;
		result.mCols = mLayout->getLayerInputNum(idxLayer);
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
