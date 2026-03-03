#pragma once

#include "Array.h"
#include <tuple>
#include <type_traits>
#include <utility>

template<typename Allocator, typename Dummy, typename... Ts> class TSoAVectorWithAllocator;

/**
 * TSoAReferenceImpl: SoA reference proxy.
 */
template<typename TSoA, bool IsConst>
struct TSoAReferenceImpl
{
	using ParentType = std::conditional_t<IsConst, const TSoA, TSoA>;
	ParentType& parent;
	size_t index;

	template<size_t I>
	decltype(auto) get() const
	{
		return std::get<I + 1>(parent.getColumnsInternal())[index];
	}

	template<size_t I>
	decltype(auto) get_binding() const
	{
		return get<I>();
	}

	/**
	 * Extacts data into a struct using aggregate initialization.
	 */
	template<typename TStruct>
	TStruct as() const
	{
		return as_impl<TStruct>(std::make_index_sequence<TSoA::NumActualComponents>{});
	}

private:
	template<typename TStruct, size_t... Is>
	TStruct as_impl(std::index_sequence<Is...>) const
	{
		return TStruct{ get<Is>()... };
	}
};

template<typename Allocator, typename Dummy, typename... Ts>
class TSoAVectorWithAllocator
{
public:
	static constexpr size_t NumActualComponents = sizeof...(Ts);
	static constexpr size_t NumTotalColumns = sizeof...(Ts) + 1;

	template<size_t I>
	using ComponentType = typename std::tuple_element<I + 1, std::tuple<Dummy, Ts...>>::type;

	using Reference = TSoAReferenceImpl<TSoAVectorWithAllocator, false>;
	using ConstReference = TSoAReferenceImpl<TSoAVectorWithAllocator, true>;

	TSoAVectorWithAllocator() : mNum(0) {}

	void clear()
	{
		std::apply([](auto&... cols) { (cols.clear(), ...); }, mColumns);
		mNum = 0;
	}

	void reserve(size_t cap)
	{
		std::apply([cap](auto&... cols) { (cols.reserve(cap), ...); }, mColumns);
	}

	void resize(size_t sz)
	{
		std::apply([sz](auto&... cols) { (cols.resize(sz), ...); }, mColumns);
		mNum = sz;
	}

	Reference operator[](size_t i)
	{
		return { *this, i };
	}

	ConstReference operator[](size_t i) const
	{
		return { *this, i };
	}

	template<size_t I> auto& getColumn()
	{
		return std::get<I + 1>(mColumns);
	}

	template<size_t I> const auto& getColumn() const
	{
		return std::get<I + 1>(mColumns);
	}

	auto& getColumnsInternal()
	{
		return mColumns;
	}

	const auto& getColumnsInternal() const
	{
		return mColumns;
	}

	void push_back(const Ts&... values)	
	{
		std::apply([&](auto& dummyCol, auto&... cols) { (cols.push_back(values), ...); }, mColumns);
		mNum++;
	}

	template<typename OtherAlloc>
	void copyFrom(size_t dst, const TSoAVectorWithAllocator<OtherAlloc, Dummy, Ts...>& src, size_t srcIdx)
	{
		copyFrom_impl(dst, src, srcIdx, std::make_index_sequence<NumTotalColumns>{});
	}

	size_t size() const
	{
		return mNum;
	}

private:
	template<typename OtherAlloc, size_t... Is>
	void copyFrom_impl(size_t dst, const TSoAVectorWithAllocator<OtherAlloc, Dummy, Ts...>& src, size_t srcIdx, std::index_sequence<Is...>)
	{
		((std::get<Is>(mColumns)[dst] = std::get<Is>(src.getColumnsInternal())[srcIdx]), ...);
	}

	std::tuple<TArray<Dummy, Allocator>, TArray<Ts, Allocator>...> mColumns;
	size_t mNum = 0;
};

namespace std
{
	template<typename T, bool C> struct tuple_size<TSoAReferenceImpl<T, C>> : std::integral_constant<size_t, T::NumActualComponents> {};
	template<size_t I, typename T, bool C> struct tuple_element<I, TSoAReferenceImpl<T, C>>
	{
		using type = std::conditional_t<C, const typename T::template ComponentType<I>, typename T::template ComponentType<I>>;
	};
}

template<size_t I, typename T, bool C> decltype(auto) get(TSoAReferenceImpl<T, C> r)
{
	return r.template get_binding<I>();
}

#define SOA_EXTRACT_TYPE(type, name) , type
#define SOA_EXTRACT_MEM(type, name) type name;
#define SOA_EXTRACT_REF_MEM(type, name) type& name;
#define SOA_EXTRACT_CONST_REF_MEM(type, name) const type& name;

#define DECLARE_SOA_LAYOUT(Name, Fields) \
	struct Name { Fields(SOA_EXTRACT_MEM) }; \
	struct Name##Ref { Fields(SOA_EXTRACT_REF_MEM) }; \
	struct Name##ConstRef { Fields(SOA_EXTRACT_CONST_REF_MEM) }; \
	template<typename Alloc = DefaultAllocator> \
	using Name##SoAWithAllocator = TSoAVectorWithAllocator<Alloc, int Fields(SOA_EXTRACT_TYPE) >; \
	using Name##SoA = Name##SoAWithAllocator<DefaultAllocator>;
