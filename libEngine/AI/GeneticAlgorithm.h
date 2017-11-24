#pragma once
#ifndef GeneticAlgorithm_H_F5535AE5_6B55_41B1_9BBF_A509410271E4
#define GeneticAlgorithm_H_F5535AE5_6B55_41B1_9BBF_A509410271E4

#include "Random.h"

#include <vector>
#include <memory>

typedef double NNScale;

class NNRand
{
public:

	uint32 CPUSeed()
	{
		uint64 result;
#if CPP_COMPILER_MSVC
		__asm
		{
			rdtsc;
			mov dword ptr result, eax;
			mov dword ptr result + 4, edx;
		}
#else

#endif
		return result;
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
	NNScale nextScale()
	{
		return double(nextInt()) / 0xffffffff;
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

	typedef std::vector< NNScale > DataType;

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

	void randomizeData(NNRand& random, NNScale valueMin, NNScale valueMax);
	bool isDataEquivalent(DataType const& otherData, NNScale maxError = 1e-10);

	float    fitness;
	DataType data;

};

typedef std::shared_ptr< Genotype > GenotypePtr;

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


	int appendWithCopy(GenePool& other, float fitnessError, NNScale dataError);

	GenotypePtr cloneGenotype(int index);

	bool add(GenotypePtr pGenotype);
	void clear();

	void removeEqual(float fitnessError, NNScale dataError);
	bool haveEqualGenotype(GenotypePtr gt, float fitnessError, NNScale dataError, int startIndex = 0);

	static void RemoveEqualGenotype(std::vector< GenotypePtr >& sortedGenotypes, float fitnessError, NNScale dataError);

	std::vector< GenotypePtr > const& getDataSet() { return mStorage; }

	GenotypePtr& operator[](int idx) { return mStorage[idx]; }
	std::vector< GenotypePtr > mStorage;
};

class GeneticAlgorithm
{
public:
	typedef std::vector< NNScale > DataType;
	float selectPowerFactor = 5;

	void select(int numSelection, std::vector< GenotypePtr > const& candidates, std::vector< GenotypePtr >& outSelections);
	void crossover(int numCrossover, std::vector< GenotypePtr > const& bases, std::vector< GenotypePtr >& outOffsprings);
	void mutate(std::vector< GenotypePtr >& populations, float geneMutationProb, float valueMutationProb, float valueDelta);

	void crossoverData(float powerA, DataType const& dataA, DataType const& dataB, DataType& outData);
	void mutateData(DataType& data, float valueMutationProb, NNScale valueDelta);

	NNRand mRand;
};

#endif // GeneticAlgorithm_H_F5535AE5_6B55_41B1_9BBF_A509410271E4
