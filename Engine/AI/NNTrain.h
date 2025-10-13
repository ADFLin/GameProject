#pragma once
#ifndef NNTrain_H_E17423EA_82CC_446C_8A7F_2CAAB54D7F6D
#define NNTrain_H_E17423EA_82CC_446C_8A7F_2CAAB54D7F6D

struct AdamOptimizer
{
	void init(int numParaneters)
	{
		m.resize(numParaneters, 0.0);
		v.resize(numParaneters, 0.0);
		beta1Decayed = 1.0f;
		beta2Decayed = 1.0f;
	}

	TArray<NNScalar> m;
	TArray<NNScalar> v;

	float beta1Decayed = 1.0f;
	float beta2Decayed = 1.0f;

	void update(TArrayView<NNScalar> inoutParameters, TArrayView<NNScalar> parametersGrads, float learnRate)
	{
		CHECK(inoutParameters.size() == parametersGrads.size());
		CHECK(inoutParameters.size() == m.size());
		constexpr NNScalar Beta1 = 0.9;
		constexpr NNScalar Beta2 = 0.999;
		constexpr NNScalar Epsilon = 1e-8;

		// bias correction
		beta1Decayed *= Beta1;
		beta2Decayed *= Beta2;

		for (int i = 0; i < inoutParameters.size(); ++i)
		{
			m[i] = Beta1 * m[i] + (1.0f - Beta1) * parametersGrads[i];
			v[i] = Beta2 * v[i] + (1.0f - Beta2) * parametersGrads[i] * parametersGrads[i];

			float mhat = m[i] / (1.0f - beta1Decayed);
			float vhat = v[i] / (1.0f - beta2Decayed);

			inoutParameters[i] -= learnRate * mhat / (std::sqrt(vhat) + Epsilon);
		}
	}
};

struct RMSProp
{

	void init(int numParaneters)
	{
		mRMSPropSquare.resize(numParaneters, 0.0);
	}

	void update(TArrayView<NNScalar> inoutParameters, TArrayView<NNScalar> parametersGrads, float learnRate)
	{
		CHECK(inoutParameters.size() == parametersGrads.size());
		CHECK(inoutParameters.size() == mRMSPropSquare.size());
		constexpr NNScalar Alpha = 0.9;
		constexpr NNScalar Epsilon = 1e-8;

		for (int i = 0; i < inoutParameters.size(); ++i)
		{
			NNScalar g = parametersGrads[i];
			NNScalar thiSquare = /*mRMSPropSquare[i] == 0 ? g : */Alpha * mRMSPropSquare[i] + (1 - Alpha) * Math::Square(g);
			inoutParameters[i] -= learnRate * g / (Math::Sqrt(thiSquare) + Epsilon);
			mRMSPropSquare[i] = thiSquare;
		}
	}

	TArray<NNScalar> mRMSPropSquare;
};


class FRMSELoss
{
public:
	//MSE £U(r-s)^2
	static NNScalar CalcDevivative(NNScalar prediction, NNScalar label)
	{
		NNScalar delta = label - prediction;
		return -delta;
	}

	static NNScalar Calc(NNScalar prediction, NNScalar label)
	{
		NNScalar delta = label - prediction;
		return  0.5 * delta * delta;;
	}
};


class FCrossEntropyLoss
{
public:
	//Cross-Entropy -£Urln(s)
	static NNScalar CalcDevivative(NNScalar prediction, NNScalar label)
	{
		return  -label / prediction;
	}

	static NNScalar CalcSoftMaxDevivative(NNScalar prediction, NNScalar label)
	{
		return  prediction - label;
	}

	static NNScalar Calc(NNScalar prediction, NNScalar label)
	{
		return -label * log(prediction);
	}

	static NNScalar Calc(int dim , NNScalar* distribution, NNScalar* labels)
	{
		NNScalar result = 0.0;
		for (int i = 0; i < dim; ++i)
		{
			result += -labels[i] * log(distribution[i]);
		}
		return result;
	}
};

#endif // NNTrain_H_E17423EA_82CC_446C_8A7F_2CAAB54D7F6D