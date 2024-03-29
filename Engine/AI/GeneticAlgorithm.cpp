#include "GeneticAlgorithm.h"

#include "Math/Base.h"

#include <cassert>
#include <algorithm>

void GeneticAlgorithm::select(int numSelection, TArray< GenotypePtr > const& candidates, TArray< GenotypePtr >& outSelections)
{
	int size = candidates.size();
	assert(numSelection <= size);

	struct GeneSelectFactor
	{
		int   index;
		float factor;
	};

	TArray< GeneSelectFactor > selectFactors;
	for( int i = 0; i < size; ++i )
	{
		float factor = mRand.nextFloat() * Math::Exp(-selectPowerFactor * i / size);
		selectFactors.push_back({ i ,  factor });
	}

	std::sort(selectFactors.begin(), selectFactors.end(),
		[](GeneSelectFactor const& a, GeneSelectFactor const& b) -> bool
		{
			return a.factor > b.factor;
		}
	);

	for( int i = 0; i < numSelection; ++i )
	{
		outSelections.push_back(candidates[selectFactors[i].index]);
	}
}

void GeneticAlgorithm::crossover(int numCrossover, TArray< GenotypePtr > const& bases, TArray< GenotypePtr >& outOffsprings)
{
	DataType outData;
	for( int i = 0; i < numCrossover; ++i )
	{
		int idx0 = mRand.nextInt() % bases.size();
		int idx1;
		do
		{
			idx1 = mRand.nextInt() % bases.size();
		} while( idx0 == idx1 );

		float powerA = 0.5;
#if 0
		float totalFitness = bases[idx0]->fitness + bases[idx1]->fitness;
		if( totalFitness > FLOAT_DIV_ZERO_EPSILON)
			powerA = (bases[idx0]->fitness / totalFitness);
#endif

		crossoverData(powerA, bases[idx0]->data, bases[idx1]->data, outData);
		GenotypePtr ptr{ new Genotype(std::move(outData)) };
		outOffsprings.push_back(ptr);
	}
}

void GeneticAlgorithm::mutate(TArray< GenotypePtr >& populations, float geneMutationProb, float valueMutationProb, float valueDelta)
{
	for( int i = 0; i < populations.size(); ++i )
	{
		if( mRand.nextFloat() < geneMutationProb )
		{
			mutateData(populations[i]->data, valueMutationProb, valueDelta);
		}
	}
}

void GeneticAlgorithm::crossoverData(float powerA, DataType const& dataA, DataType const& dataB, DataType& outData)
{
	assert(dataA.size() == dataB.size());
	outData.resize(dataA.size());
	for( int i = 0; i < dataA.size(); ++i )
	{
		if( mRand.nextFloat() < powerA )
		{
			outData[i] = dataA[i];
		}
		else
		{
			outData[i] = dataB[i];
		}
	}
}

void GeneticAlgorithm::mutateData(DataType& data, float valueMutationProb, NNScalar valueDelta)
{
	for( int i = 0; i < data.size(); ++i )
	{
		if( mRand.nextFloat() < valueMutationProb )
		{
			data[i] += data[i] * (  mRand.nextScale() * (2 * valueDelta) - valueDelta );
		}
	}
}

void Genotype::randomizeData(NNRand& random, NNScalar valueMin, NNScalar valueMax)
{
	NNScalar valueDelta = valueMax - valueMin;
	for( int i = 0; i < data.size(); ++i )
	{
		data[i] = random.nextScale() * valueDelta + valueMin;
	}
}

bool Genotype::isDataIdentical(DataType const& otherData, NNScalar maxError /*= 1e-10*/)
{
	if( data.size() != otherData.size() )
		return false;
	for( int i = 0; i < data.size(); ++i )
	{
		if( Math::Abs(data[i] - otherData[i]) > maxError )
			return false;
	}
	return true;
}

int GenePool::appendWithCopy(GenePool& other, float fitnessError, NNScalar dataError)
{
	int result = 0;
	for( int i = 0; i < other.mStorage.size(); ++i )
	{
		GenotypePtr pGT = other[i];
		if( haveIdenticalGenotype(pGT, fitnessError, dataError) )
			continue;

		if( !add(other.cloneGenotype(i)) )
			break;

		++result;
	}
	return result;
}

GenotypePtr GenePool::cloneGenotype(int index)
{
#if 0
	GenotypePtr result = std::make_shared< Genotype >(*mStorage[index]);
#else
	GenotypePtr result = std::make_shared< Genotype >();
	result->data = mStorage[index]->data;
	result->fitness = mStorage[index]->fitness;
#endif
	return result;
}

bool GenePool::add(GenotypePtr pGenotype)
{
	if( mStorage.size() >= maxPoolNum && FitnessCmp()(mStorage.back(), pGenotype) )
		return false;

	auto iter = std::lower_bound(mStorage.begin(), mStorage.end(), pGenotype, FitnessCmp());
	mStorage.insert(iter, pGenotype);
	if( mStorage.size() > maxPoolNum )
	{
		mStorage.pop_back();
	}
	return true;
}

void GenePool::clear()
{
	mStorage.clear();
}

void GenePool::removeIdentical(float fitnessError, NNScalar dataError)
{
	RemoveIdenticalGenotype(mStorage, fitnessError, dataError);
}

bool GenePool::haveIdenticalGenotype(GenotypePtr gt, float fitnessError, NNScalar dataError, int startIndex /*= 0*/)
{
	for( int i = startIndex; i < (int)mStorage.size(); ++i )
	{
		GenotypePtr& gtTest = mStorage[i];
#if 0
		if( gtTest->fitness > gt->fitness + fitnessError )
			break;
#endif

		if( Math::Abs(gtTest->fitness - gt->fitness) > fitnessError )
			continue;

		if( !gt->isDataIdentical(gtTest->data, dataError) )
			continue;

		return true;
	}

	return false;
}

void GenePool::RemoveIdenticalGenotype(TArray< GenotypePtr >& sortedGenotypes, float fitnessError, NNScalar dataError)
{
	for( int i = 0; i < sortedGenotypes.size(); ++i )
	{
		GenotypePtr gt1 = sortedGenotypes[i];
		for( int j = i + 1; j < sortedGenotypes.size(); ++j )
		{
			GenotypePtr gt2 = sortedGenotypes[j];
			if( gt1.get() != gt2.get() )
			{
				if( Math::Abs(gt1->fitness - gt2->fitness) > fitnessError )
					break;
				if( !gt1->isDataIdentical(gt2->data, dataError) )
					continue;
			}

			sortedGenotypes.erase(sortedGenotypes.begin() + j);
			--j;
		}
	}
}
