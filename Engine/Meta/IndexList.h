#ifndef IndexList_h__
#define IndexList_h__

template <size_t... Is>
struct TIndexList {};

// Collects internal details for generating index ranges [MIN, MAX)
namespace Private
{
#if 1
	template <size_t Offset, size_t... LIs, size_t ...RIs>
	TIndexList< LIs..., (Offset + RIs)... > AppendWithOffset(TIndexList<LIs...>, TIndexList<RIs...>);

	template <size_t Off, size_t... Is>
	TIndexList< (Off + Is)... > Offset(TIndexList<Is...>);

	template < size_t N >
	struct TSequenceBuilder
	{
		using Type = decltype(AppendWithOffset<N / 2>(TSequenceBuilder<N / 2>::Type(), TSequenceBuilder<N - N / 2>::Type()));
	};

	template<>
	struct TSequenceBuilder< 0 >
	{
		using Type = TIndexList<>;
	};

	template<>
	struct TSequenceBuilder< 1 >
	{
		using Type = TIndexList<0>;
	};

	template<>
	struct TSequenceBuilder< 2 >
	{
		using Type = TIndexList<0, 1>;
	};

	template<>
	struct TSequenceBuilder< 3 >
	{
		using Type = TIndexList<0, 1, 2>;
	};

	template<>
	struct TSequenceBuilder< 4 >
	{
		using Type = TIndexList<0, 1, 2, 3>;
	};

	template<>
	struct TSequenceBuilder< 5 >
	{
		using Type = TIndexList<0, 1, 2, 3, 4>;
	};

	template<>
	struct TSequenceBuilder< 6 >
	{
		using Type = TIndexList<0, 1, 2, 3, 4, 5>;
	};

	template<>
	struct TSequenceBuilder< 7 >
	{
		using Type = TIndexList<0, 1, 2, 3, 4, 5, 6>;
	};

	template<>
	struct TSequenceBuilder< 8 >
	{
		using Type = TIndexList<0, 1, 2, 3, 4, 5, 6, 7>;
	};

	template<size_t MIN, size_t MAX>
	struct TRangeBuilder 
	{
		using Type = decltype(Offset<MIN>(TSequenceBuilder<MAX-MIN>::Type()));
	};
#else
	// Declare primary template for index range builder
	template <size_t MIN, size_t N, size_t... Is>
	struct TRangeBuilder;

	// Base step
	template <size_t MIN, size_t... Is>
	struct TRangeBuilder<MIN, MIN, Is...>
	{
		using Type = TIndexList<Is...>;
	};

	// Induction step
	template <size_t MIN, size_t N, size_t... Is>
	struct TRangeBuilder : public TRangeBuilder<MIN, N - 1, N - 1, Is...>
	{
	};
#endif
}

// Meta-function that returns a [MIN, MAX) index range
template<size_t MIN, size_t MAX>
using TIndexRange = typename Private::TRangeBuilder<MIN, MAX>::Type;

template<size_t N>
using TIndexSequence = typename Private::TSequenceBuilder<N>::Type;
#endif // IndexList_h__