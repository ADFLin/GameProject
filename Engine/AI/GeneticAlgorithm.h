#pragma once
#ifndef GeneticAlgorithm_H_F5535AE5_6B55_41B1_9BBF_A509410271E4
#define GeneticAlgorithm_H_F5535AE5_6B55_41B1_9BBF_A509410271E4

#include "Random.h"
#include "DataStructure/Array.h"

#include <memory>

#define USE_SSE 0

#if CPP_COMPILER_MSVC
#include <intrin.h>
#endif


using NNScalar = float;

class NNRand
{
public:

	uint32 CPUSeed()
	{
#if CPP_COMPILER_MSVC
		uint64 result = __rdtsc();
		return uint32(result);
#else
		//#FIXME
		unsigned dummy;
		//return __rdtscp(&dummy);
		return 0;
#endif
	}
	NNRand()
	{
		uint32 s[4];
		s[0] = CPUSeed() & 0xffff;
		s[1] = CPUSeed() & 0xffff;
		s[2] = CPUSeed() & 0xffff;
		s[3] = CPUSeed() & 0xffff;
		mImpl.init(s);
	}
	NNScalar nextScale()
	{
		return NNScalar(nextInt()) / 0xffffffff;
	}
	float   nextFloat()
	{
		return nextScale();
	}
	uint32     nextInt()
	{
		return mImpl.rand();
	}

	Random::LFSR113 mImpl;
};


class Genotype
{
public:

	using DataType = TArray< NNScalar >;

	Genotype(DataType const& inData)
		:data(inData)
		, fitness(0)
	{

	}

	Genotype()
		:data()
		, fitness(0)
	{
	}

	void randomizeData(NNRand& random, NNScalar valueMin, NNScalar valueMax);
	bool isDataIdentical(DataType const& otherData, NNScalar maxError = 1e-10);

	float    fitness;
	DataType data;

};

using GenotypePtr = std::shared_ptr< Genotype >;

class GenePool
{
public:
	int   maxPoolNum = 20;

	struct FitnessCmp
	{
		bool operator()(GenotypePtr const& lhs, GenotypePtr const& rhs) const
		{
			return lhs->fitness > rhs->fitness;
		}
	};


	int appendWithCopy(GenePool& other, float fitnessError, NNScalar dataError);

	GenotypePtr cloneGenotype(int index);

	bool add(GenotypePtr pGenotype);
	void clear();

	void removeIdentical(float fitnessError, NNScalar dataError);
	bool haveIdenticalGenotype(GenotypePtr gt, float fitnessError, NNScalar dataError, int startIndex = 0);

	static void RemoveIdenticalGenotype(TArray< GenotypePtr >& sortedGenotypes, float fitnessError, NNScalar dataError);

	TArray< GenotypePtr > const& getDataSet() { return mStorage; }

	GenotypePtr&       operator[](int idx) { return mStorage[idx]; }
	GenotypePtr const& operator[](int idx) const { return mStorage[idx]; }
	TArray< GenotypePtr > mStorage;
};

class GeneticAlgorithm
{
public:
	using DataType = TArray< NNScalar >;
	float selectPowerFactor = 5;

	void select(int numSelection, TArray< GenotypePtr > const& candidates, TArray< GenotypePtr >& outSelections);
	void crossover(int numCrossover, TArray< GenotypePtr > const& bases, TArray< GenotypePtr >& outOffsprings);
	void mutate(TArray< GenotypePtr >& populations, float geneMutationProb, float valueMutationProb, float valueDelta);

	void crossoverData(float powerA, DataType const& dataA, DataType const& dataB, DataType& outData);
	void mutateData(DataType& data, float valueMutationProb, NNScalar valueDelta);

	NNRand mRand;
};

#endif // GeneticAlgorithm_H_F5535AE5_6B55_41B1_9BBF_A509410271E4
