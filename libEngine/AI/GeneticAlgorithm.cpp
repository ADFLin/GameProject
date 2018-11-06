#include "GeneticAlgorithm.h"

#include "Math/Base.h"

#include <cassert>
#include <algorithm>

void GeneticAlgorithm::select(int numSelection, std::vector< GenotypePtr > const& candidates, std::vector< GenotypePtr >& outSelections)
{
	int size = candidates.size();
	assert(numSelection <= size);

	struct GeneSelectFactor
	{
		int   index;
		float factor;
	};

	std::vector< GeneSelectFactor > selectFactors;
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

void GeneticAlgorithm::crossover(int numCrossover, std::vector< GenotypePtr > const& bases, std::vector< GenotypePtr >& outOffsprings)
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
		if( totalFitness > 1e-5 )
			powerA = (bases[idx0]->fitness / totalFitness);
#endif

		crossoverData(powerA, bases[idx0]->data, bases[idx1]->data, outData);
		GenotypePtr ptr{ new Genotype(std::move(outData)) };
		outOffsprings.push_back(ptr);
	}
}

void GeneticAlgorithm::mutate(std::vector< GenotypePtr >& populations, float geneMutationProb, float valueMutationProb, float valueDelta)
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

void GeneticAlgorithm::mutateData(DataType& data, float valueMutationProb, NNScale valueDelta)
{
	for( int i = 0; i < data.size(); ++i )
	{
		if( mRand.nextFloat() < valueMutationProb )
		{
			data[i] += data[i] * (  mRand.nextScale() * (2 * valueDelta) - valueDelta );
		}
	}
}

void Genotype::randomizeData(NNRand& random, NNScale valueMin, NNScale valueMax)
{
	NNScale valueDelta = valueMax - valueMin;
	for( int i = 0; i < data.size(); ++i )
	{
		data[i] = random.nextScale() * valueDelta + valueMin;
	}
}

bool Genotype::isDataEquivalent(DataType const& otherData, NNScale maxError /*= 1e-10*/)
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

int GenePool::appendWithCopy(GenePool& other, float fitnessError, NNScale dataError)
{
	int result = 0;
	for( int i = 0; i < other.mStorage.size(); ++i )
	{
		GenotypePtr pGT = other[i];
		if( haveEqualGenotype(pGT, fitnessError, dataError) )
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

void GenePool::removeEqual(float fitnessError, NNScale dataError)
{
	RemoveEqualGenotype(mStorage, fitnessError, dataError);
}

bool GenePool::haveEqualGenotype(GenotypePtr gt, float fitnessError, NNScale dataError, int startIndex /*= 0*/)
{
	for( int i = startIndex; i < (int)mStorage.size(); ++i )
	{
		GenotypePtr gtTest = mStorage[i];
		if( gtTest->fitness > gt->fitness + fitnessError )
			break;

		if( Math::Abs(gtTest->fitness - gt->fitness) > fitnessError )
			continue;

		if( !gt->isDataEquivalent(gtTest->data, dataError) )
			continue;

		return true;
	}

	return false;
}

void GenePool::RemoveEqualGenotype(std::vector< GenotypePtr >& sortedGenotypes, float fitnessError, NNScale dataError)
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
				if( !gt1->isDataEquivalent(gt2->data, dataError) )
					continue;
			}

			sortedGenotypes.erase(sortedGenotypes.begin() + j);
			--j;
		}
	}
}
