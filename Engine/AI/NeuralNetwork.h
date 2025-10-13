#pragma once
#ifndef NeuralNetwork_H_9D9E20F4_5B5B_45BE_A95C_5A2CEB755C3F
#define NeuralNetwork_H_9D9E20F4_5B5B_45BE_A95C_5A2CEB755C3F

#include "Math/Base.h"
#include "DataStructure/Array.h"

#include "Meta/MetaBase.h"
#include "Math/SIMD.h"
#include <cassert>
#include "Template/ArrayView.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"

using IntVector2 = TVector2<int>;
using IntVector3 = TVector3<int>;


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

	struct WeakReLU
	{
		static NNScalar constexpr NSlope = 1e-4;
		static NNScalar Value(NNScalar value)
		{
			return (value > 0) ? value : NSlope * value;
		}

		static NNScalar Derivative(NNScalar value)
		{
			return (value > 0) ? NNScalar(1) : NSlope;
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


struct NNTransformLayer
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

struct NeuralLayer
{
	int numNode;
	int weightOffset;
	int biasOffset;

	NeuralLayer()
	{
		numNode = 0;
		weightOffset = 0; 
		biasOffset = 0;
	}
};

struct NNLinearLayer : NeuralLayer
{

	void init(int inInputLength, int inNumNode)
	{
		inputLength = inInputLength;
		numNode = inNumNode;
	}

	int inputLength;

	int getParameterLength() const
	{
		return numNode * (inputLength + 1);
	}

	int getWeightLength()
	{
		return numNode * (inputLength);
	}


	int getPassOutputNum() const
	{
		return numNode;
	}

	int getOutputLength() const
	{
		return numNode;
	}
};

struct NNConv2DLayer : NeuralLayer
{
	NNConv2DLayer() = default;
	NNConv2DLayer(int inConvSize, int inNumNode)
	{
		convSize = inConvSize;
		numNode = inNumNode;
	}

	enum FastMethod
	{
		eNone,
		eF23,
		eF43,
	};



	void init(IntVector3 const& inInputSize, int inConvSize, int inNumNode)
	{
		inputSize = inInputSize;
		convSize = inConvSize;
		numNode = inNumNode;

		dataSize[0] = inputSize[0] - convSize + 1;
		dataSize[1] = inputSize[1] - convSize + 1;
	}

	FastMethod fastMethod = FastMethod::eNone;
	IntVector3 inputSize;
	IntVector2 dataSize;
	int convSize;


	int getParameterLength() const
	{
		return numNode * (inputSize.z * convSize * convSize + 1);
	}

	int getWeightLength() const
	{
		return numNode * (inputSize.z * convSize * convSize );
	}

	int getPassOutputLength() const
	{
		return dataSize[0] * dataSize[1] * numNode;
	}

	int getOutputLength() const
	{
		return dataSize[0] * dataSize[1] * numNode;
	}

	IntVector3 getOutputSize() const
	{
		return IntVector3(dataSize.x, dataSize.y, numNode);
	}
};

struct NNConv2DPaddingLayer : NeuralLayer
{
	NNConv2DPaddingLayer() = default;
	NNConv2DPaddingLayer(int inConvSizeX, int inConvSizeY, int inNumNode, int inPadding)
	{
		convSize[0] = inConvSizeX;
		convSize[1] = inConvSizeY;
		numNode = inNumNode;
		padding = inPadding;
	}

	int convSize[2];
	int dataSize[2];
	int padding = 0;

	int getParametersNum(int sliceInputNum)
	{
		return numNode * (sliceInputNum * convSize[0] * convSize[1] + 1);
	}

	int getOutputLength() const
	{
		return dataSize[0] * dataSize[1] * numNode;
	}
};

struct NNMaxPooling2DLayer
{
	IntVector2 inputSize;
	int numNode;
	IntVector2 dataSize;
	int poolSize;

	void init(IntVector3 const& inInputSize, int inPoolSize)
	{
		inputSize.x = inInputSize.x;
		inputSize.y = inInputSize.y;

		poolSize = inPoolSize;
		numNode = inInputSize.z;

		dataSize[0] = inputSize[0] / poolSize;
		dataSize[1] = inputSize[1] / poolSize;
	}

	int getOutputLength()
	{
		return dataSize[0] * dataSize[1] * numNode;
	}
};

struct NNAveragePooling2DLayer
{
	IntVector2 inputSize;
	int numNode;
	IntVector2 dataSize;
	int poolSize;

	void init(IntVector3 const& inInputSize, int inPoolSize)
	{
		inputSize.x = inInputSize.x;
		inputSize.y = inInputSize.y;

		poolSize = inPoolSize;
		numNode = inInputSize.z;

		dataSize[0] = inputSize[0] / poolSize;
		dataSize[1] = inputSize[1] / poolSize;
	}

	int getOutputLength()
	{
		return dataSize[0] * dataSize[1] * numNode;
	}
};


class FNNMath
{
public:
	static bool IsValid(NNScalar value)
	{
		if (Math::Abs(value) > 1e20 || isnan(value) || isinf(value))
			return false;


		return true;
	}


	static void VectorAdd(int dim, NNScalar* RESTRICT a, NNScalar const* RESTRICT b);
	static void VectorMul(int dim, NNScalar* RESTRICT a, NNScalar const* RESTRICT b);
	static void VectorAdd(int dim, NNScalar const* RESTRICT a, NNScalar const* RESTRICT b, NNScalar* RESTRICT out);
	static void VectorSub(int dim, NNScalar const* RESTRICT a, NNScalar const* RESTRICT b, NNScalar* RESTRICT out);
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

	FORCEINLINE static void Fill(int dim, NNScalar* RESTRICT v, NNScalar value)
	{
		for (int i = 0; i < dim; ++i)
		{
			v[i] = value;
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
			out[col] = VectorDot(dimRow, v, m, dimCol);
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
	static int SoftMax(int dim, NNScalar* RESTRICT inoutValues);

	static int Max(int dim, NNScalar const* inputs);

	static NNScalar Sum(int dim, NNScalar const* inputs);

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

		if (isinf(total))
		{
			NNScalar maxValue = 0.0;
			for (int i = 0; i < dim; ++i)
			{
				maxValue = Math::Max(maxValue, Math::Abs(inoutValues[i]));
			}

			total = 0.0;
			for (int i = 0; i < dim; ++i)
			{
				inoutValues[i] /= maxValue;
				total += Math::Square(inoutValues[i]);
			}
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

	static FORCEINLINE NNScalar AreaDot(int stride, NNScalar const* RESTRICT area, int dimX, int dimY, NNScalar const* RESTRICT weight)
	{
		NNScalar result = 0;
		for (int i = 0; i < dimY; ++i)
		{
			result += VectorDot(dimX, area, weight);
			area += stride;
			weight += dimX;
		}
		return result;
	}

	static FORCEINLINE NNScalar AreaDot(int stride, NNScalar const* RESTRICT area, int dimX, int dimY, int strideW, NNScalar const* RESTRICT weight)
	{
		NNScalar result = 0;
		for (int i = 0; i < dimY; ++i)
		{
			result += VectorDot(dimX, area, weight);
			area += stride;
			weight += strideW;
		}
		return result;
	}

	static FORCEINLINE NNScalar AreaDot(int stride, NNScalar const* RESTRICT area, int dim, NNScalar const* RESTRICT weight)
	{
		return AreaDot(stride, area, dim, dim, weight);
	}

	static FORCEINLINE void Conv(int dimX, int dimY, NNScalar const* RESTRICT m, int dimXW, int dimYW, NNScalar const* RESTRICT weight, NNScalar* RESTRICT inoutOutput)
	{
		CHECK(dimX >= dimXW && dimY >= dimYW);
		int dimXOutput = dimX - dimXW + 1;
		int dimYOutput = dimY - dimYW + 1;

		for (int oy = 0; oy < dimYOutput; ++oy)
		{
			NNScalar* pOutput = inoutOutput + oy * dimXOutput;
			NNScalar const* pArea = m + oy * dimX;
			for (int ox = 0; ox < dimXOutput; ++ox)
			{
				*pOutput += AreaDot(dimX, pArea, dimXW, dimYW, weight);
				++pArea;
				++pOutput;
			}
		}
	}

	static FORCEINLINE void FullConv(int dimX, int dimY, NNScalar const* RESTRICT m, int dimXW, int dimYW, NNScalar const* RESTRICT weight, NNScalar* RESTRICT inoutOutput)
	{
		CHECK(dimX >= dimXW);
		CHECK(dimY >= dimYW);

		//int dimXOutput = dimX + dimXW - 1;
		//int dimYOutput = dimY + dimYW - 1;
		int paddingX = dimXW - 1;
		int paddingY = dimYW - 1;
		Conv(dimX, dimY, m, dimXW, dimYW, weight, paddingX, paddingY, inoutOutput);
	}

	static void ClipToAreaRange(int offset, int size, int wSize, int padding, int& outOffset, int& outSize)
	{
		outSize = wSize;
		outOffset = 0;

		int input = offset - padding;
		if (input < 0)
		{
			outOffset = -input;
			outSize += input;
			input = 0;
		}
		if (input + outSize >= size)
		{
			outSize = size - input;
		}
	}

	static FORCEINLINE void Rotate180(int dimX, int dimY, NNScalar const inWeights[], NNScalar outWeights[])
	{
		NNScalar* pOut = outWeights;
		int len = dimX * dimY;
		NNScalar const* pIn = inWeights + (len - 1);

		for (int i = len; i; --i)
		{
			*pOut = *pIn;

			++pOut;
			--pIn;
		}
	}

	static FORCEINLINE void Conv(int dimX, int dimY, NNScalar const* RESTRICT m, int dimXW, int dimYW, NNScalar const* RESTRICT weight, int paddingX, int paddingY, NNScalar* RESTRICT inoutOutput)
	{
		CHECK(dimX >= dimXW && dimY >= dimYW);
		CHECK(paddingX > 0 && paddingY > 0);
		int dimXOutput = dimX - dimXW + 2 * paddingX + 1;
		int dimYOutput = dimY - dimYW + 2 * paddingY + 1;

		for (int oy = 0; oy < dimYOutput; ++oy)
		{
			int offY, sizeY;
			ClipToAreaRange(oy, dimY, dimYW, paddingY, offY, sizeY);
			if (sizeY <= 0)
				continue;

			NNScalar* pOutput = inoutOutput + oy * dimXOutput;
			NNScalar const* pArea = m + ((oy + offY - paddingY) * dimX - paddingX);
			NNScalar const* pWeight = weight + offY * dimXW;

			for (int ox = 0; ox < dimXOutput; ++ox)
			{
				int offX, sizeX;
				ClipToAreaRange(ox, dimX, dimXW, paddingX, offX, sizeX);
				if (sizeX <= 0)
					continue;

				*pOutput += AreaDot(dimX, pArea + offX, sizeX, sizeY, dimXW, pWeight + offX);
				++pArea;
				++pOutput;
			}
		}
	}

	static FORCEINLINE void FullConvRotate180(int dimX, int dimY, NNScalar const* RESTRICT m, int dimXW, int dimYW, NNScalar const* RESTRICT weightR, NNScalar* RESTRICT inoutOutput)
	{
		//CHECK(dimX >= dimXW);
		//CHECK(dimY >= dimYW);

		//int dimXOutput = dimX + dimXW - 1;
		//int dimYOutput = dimY + dimYW - 1;
		int paddingX = dimXW - 1;
		int paddingY = dimYW - 1;
		ConvRotate180(dimX, dimY, m, dimXW, dimYW, weightR, paddingX, paddingY, inoutOutput);
	}

	static NNScalar VectorDotInverse(int dim, NNScalar const* RESTRICT a, NNScalar const* RESTRICT bInverse)
	{
		NNScalar result = 0;
		NNScalar const* RESTRICT pA = a;
		NNScalar const* RESTRICT pBInv = bInverse;
		for (; dim; --dim)
		{
			result += (*pA) * (*pBInv);
			++pA;
			--pBInv;
		}
		return result;
	}

	static FORCEINLINE NNScalar AreaDotRotate180(int stride, NNScalar const* RESTRICT area, int dimX, int dimY, int strideW, NNScalar const* RESTRICT weightR)
	{
		NNScalar result = 0;
		for (int i = 0; i < dimY; ++i)
		{
			result += VectorDotInverse(dimX, area, weightR);
			area += stride;
			weightR -= strideW;
		}
		return result;
	}

	static FORCEINLINE void ConvRotate180(int dimX, int dimY, NNScalar const* RESTRICT m, int dimXW, int dimYW, NNScalar const* RESTRICT weightR, int paddingX, int paddingY, NNScalar* RESTRICT inoutOutput)
	{
		//CHECK(dimX >= dimXW && dimY >= dimYW);
		CHECK(paddingX > 0 && paddingY > 0);
		int dimXOutput = dimX - dimXW + 2 * paddingX + 1;
		int dimYOutput = dimY - dimYW + 2 * paddingY + 1;

		for (int oy = 0; oy < dimYOutput; ++oy)
		{
			int offY, sizeY;
			ClipToAreaRange(oy, dimY, dimYW, paddingY, offY, sizeY);
			if (sizeY <= 0)
				continue;

			NNScalar* pOutput = inoutOutput + oy * dimXOutput;
			NNScalar const* pArea = m + ((oy + offY - paddingY) * dimX - paddingX);
			NNScalar const* pWeight = weightR + (dimY - offY - 2) * dimXW;

			for (int ox = 0; ox < dimXOutput; ++ox)
			{
				int offX, sizeX;
				ClipToAreaRange(ox, dimX, dimXW, paddingX, offX, sizeX);
				if (sizeX <= 0)
					continue;

				*pOutput += AreaDotRotate180(dimX, pArea + offX, sizeX, sizeY, dimXW, pWeight + (dimX - offX - 2));
				++pArea;
				++pOutput;
			}
		}
	}

	static void TranformAreaF23(int rowStride, int numSlice, int sliceStride, NNScalar const* RESTRICT area, NNScalar* RESTRICT outArea);
	static void AreaConvF23(int stride, NNScalar inoutV[], int numSlice, NNScalar const* RESTRICT area, NNScalar const* RESTRICT weight);
	static void TranformAreaF43(int rowStride, int numSlice, int sliceStride, NNScalar const* RESTRICT area, NNScalar* RESTRICT outArea);
	static void AreaConvF43(int stride, NNScalar inoutV[], int numSlice, NNScalar const* RESTRICT area, NNScalar const* RESTRICT weight);

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
	static void AreaConv(int stride, NNScalar inoutV[], int numSlice, NNScalar const* RESTRICT area, NNScalar const* RESTRICT weight)
	{
		FNNMath::AreaConvF23(stride, inoutV, numSlice, area, weight);
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
	static void AreaConv(int stride, NNScalar inoutV[], int numSlice, NNScalar const* RESTRICT area, NNScalar const* RESTRICT weight)
	{
		FNNMath::AreaConvF43(stride, inoutV, numSlice, area, weight);
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

class NNFullConLayout
{
public:
	void init(int parameterOffset, TArrayView<uint32 const> topology);
	void init(TArrayView<uint32 const> topology) { init(0, topology); }
	void init(uint32 numInput, uint32 numOutput, uint32 numHiddenLayer, uint32 const hiddenlayerNodeNum[], int parameterOffset = 0);

	NNLinearLayer& getLayer(int idx) { return mLayers[idx]; }
	NNLinearLayer const& getLayer(int idx) const { return mLayers[idx]; }
	void getTopology(TArray<uint32>& outTopology) const;


	int getLayerInputNum(int idxLayer) const;

	int getInputSignalOffset(int idxLayer, bool bHadActiveInput) const;
	int getOutputSignalOffset(int idxLayer, bool bHadActiveInput) const;

	int getHiddenLayerNum() const { return (int)mLayers.size() - 1; }
	int getInputNum()  const { return mLayers[0].inputLength; }

	int getTotalNodeNum() const { return mTotalNodeNum; }


	int getSignalNum() const { return getInputNum() + mTotalNodeNum; }

	int getHiddenNodeNum() const;

	int getMaxLayerNodeNum() const { return mMaxLayerNodeNum; }


	template< typename FuncType >
	void setActivationFunction(int idxLayer)
	{
		mLayers[idxLayer].setFuncionT<FuncType>();
	}


	template< typename FuncType >
	void setHiddenLayerTransform()
	{
		mHiddenActiveLayer.setFuncionT< FuncType >();
	}

	template< typename FuncType >
	void setOutputLayerTransform()
	{
		mOutputActiveLayer.setFuncionT< FuncType >();
	}

	NNScalar* forwardSignal(
		NNScalar const* parameters,
		NNScalar const inputs[],
		NNScalar outputs[]) const;

	int getParameterLength() const;

	int getOutputLength() const
	{
		return mLayers.back().numNode;
	}
	int getPassOutputLength() const
	{
		return 2 * getTotalNodeNum();
	}
	int getTempLossGradLength() const
	{
		return 2 * getMaxLayerNodeNum();
	}

	void forwardFeedback(
		NNScalar const parameters[],
		NNScalar const inputs[],
		NNScalar outputs[]) const;

	NNScalar* forward(
		NNScalar const parameters[],
		NNScalar const inputs[],
		NNScalar outputs[]) const;

	void backward(
		NNScalar const parameters[],
		NNScalar const inInputs[],
		NNScalar const inOutputs[],
		NNScalar const inOutputLossGrads[],
		TArrayView<NNScalar> tempLossGrads,
		NNScalar inoutParameterGrads[],
		NNScalar* outLossGrads = nullptr) const;


	NNTransformLayer mHiddenActiveLayer;
	NNTransformLayer mOutputActiveLayer;
	int mMaxLayerNodeNum;
	int mTotalNodeNum;
	TArray< NNLinearLayer > mLayers;
};

class FNNAlgo
{
public:
	static NNScalar* Forward(
		NNLinearLayer const& layer, NNScalar const* parameters,
		NNScalar const inputs[],
		NNScalar outputs[]);

	static void BackwardLoss(
		NNLinearLayer const& layer, NNScalar const* parameters, 
		NNScalar const inOutputLossGrads[],
		NNScalar outLossGrads[]);

	static void BackwardWeight(
		NNLinearLayer const& layer,
		NNScalar const inInput[], 
		NNScalar const inOutputLossGrads[],
		NNScalar inoutParameterGrads[]);

	static NNScalar* Forward(
		NNConv2DLayer const& layer, NNScalar const* parameters,
		NNScalar const inputs[],
		NNScalar outputs[]);

	static void BackwardLoss(
		NNConv2DLayer const& layer, NNScalar const* parameters,
		NNScalar const inOutputLossGrads[],
		NNScalar outLossGrads[]);

	static void BackwardWeight(
		NNConv2DLayer const& layer,
		NNScalar const inInputs[],
		NNScalar const inOutputs[],
		NNScalar const inOutputLossGrads[],
		NNScalar inoutParameterGrads[]);

	static NNScalar* Forward(
		NNConv2DPaddingLayer const& layer, NNScalar const* parameters,
		int numSliceInput, int const inputSize[],
		NNScalar const inputs[],
		NNScalar outputs[]);

	static NNScalar* Forward(
		NNMaxPooling2DLayer const& layer,
		NNScalar const inputs[],
		NNScalar outputs[]);

	static void Backward(
		NNMaxPooling2DLayer const& layer,
		NNScalar const inInputs[],
		NNScalar const inOutputs[],
		NNScalar const inOutputLossGrads[],
		NNScalar outLossGrads[]);

	static NNScalar* Forward(
		NNAveragePooling2DLayer const& layer,
		NNScalar const inputs[],
		NNScalar outputs[]);

	static void Backward(
		NNAveragePooling2DLayer const& layer,
		NNScalar const inOutputLossGrads[],
		NNScalar outLossGrads[]);

	static NNScalar* Forward(
		NNTransformLayer const& layer,
		int inputLength,
		NNScalar const inInputs[],
		NNScalar outOutputs[]);

	static NNScalar* Forward(
		NNTransformLayer const& layer,
		int inputLength,
		NNScalar inoutValues[]);


	static void Backward(
		NNTransformLayer const& layer, 
		int inputLength,
		NNScalar const inInputs[], 
		NNScalar inoutLossDerivatives[]);

	static void Backward(
		NNTransformLayer const& layer,
		int inputLength,
		NNScalar const inInputs[],
		NNScalar const inOutputLossGrads[],
		NNScalar outLossGrads[]);


	static void SoftMaxBackward(
		int inputLength,
		NNScalar const inInputs[],
		NNScalar const inOutputs[],
		NNScalar const inOutputLossGrads[],
		NNScalar outLossGrads[]);
};

class FCNeuralNetwork
{
public:
	void init(NNFullConLayout const& inLayout)
	{
		mLayout = &inLayout;
	}
	NNScalar* getWeights(int idxLayer, int idxNode);
	NNFullConLayout const& getLayout() const { return *mLayout; }


	void setParamsters(TArray< NNScalar >& weights)
	{
		assert(mLayout->getParameterLength() <= weights.size());
		mParameters = &weights[0];
	}
	
	void forwardFeedback(NNScalar const inputs[], NNScalar outputs[])
	{
		getLayout().forwardFeedback(mParameters, inputs, outputs);
	}
	void forwardSignal(NNScalar const inInputs[], NNScalar outSignals[])
	{
		FNNMath::VectorCopy(getLayout().getInputNum(), inInputs, outSignals);
		NNScalar* outActivations = outSignals + getLayout().getInputNum();
		getLayout().forwardSignal(mParameters, inInputs, outActivations);
	}

	NNScalar* forward(NNScalar const inInputs[], NNScalar outSignals[]) const
	{
		FNNMath::VectorCopy(getLayout().getInputNum(), inInputs, outSignals);
		NNScalar* outOutputs = outSignals + getLayout().getInputNum();
		return getLayout().forward(mParameters, inInputs, outOutputs);
	}

	void backward(NNScalar const inOutputLossGrads[], NNScalar const inSignals[], TArrayView<NNScalar> tempLossGrads, NNScalar inoutParameterGrads[]) const
	{
		getLayout().backward(mParameters, inSignals, inSignals + getLayout().getInputNum(), inOutputLossGrads, tempLossGrads, inoutParameterGrads);
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
	
	NNFullConLayout const* mLayout = nullptr;
	NNScalar* mParameters;	
};

#endif // NeuralNetwork_H_9D9E20F4_5B5B_45BE_A95C_5A2CEB755C3F
