#ifndef IndexList_h__
#define IndexList_h__

template <size_t... Is>
struct TIndexList {};

// Collects internal details for generating index ranges [MIN, MAX)
namespace Private
{
	// Declare primary template for index range builder
	template <size_t MIN, size_t N, size_t... Is>
	struct TRangeBuilder;

	// Base step
	template <size_t MIN, size_t... Is>
	struct TRangeBuilder<MIN, MIN, Is...>
	{
		typedef TIndexList<Is...> type;
	};

	// Induction step
	template <size_t MIN, size_t N, size_t... Is>
	struct TRangeBuilder : public TRangeBuilder<MIN, N - 1, N - 1, Is...>
	{
	};
}

// Meta-function that returns a [MIN, MAX) index range
template<size_t MIN, size_t MAX>
using TIndexRange = typename Private::TRangeBuilder<MIN, MAX>::type;

#endif // IndexList_h__