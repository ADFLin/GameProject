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
		return parent.template getColumn<I>()[index];
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

	TSoAVectorWithAllocator(const TSoAVectorWithAllocator& rhs) : mNum(0)
	{
		copyConstructFrom(rhs);
	}

	TSoAVectorWithAllocator(TSoAVectorWithAllocator&& rhs) : mNum(0)
	{
		moveConstructFrom(rhs);
	}

	~TSoAVectorWithAllocator()
	{
		clear();
	}

	TSoAVectorWithAllocator& operator = (const TSoAVectorWithAllocator& rhs)
	{
		CHECK(this != &rhs);
		clear();
		copyConstructFrom(rhs);
		return *this;
	}

	TSoAVectorWithAllocator& operator = (TSoAVectorWithAllocator&& rhs)
	{
		CHECK(this != &rhs);
		clear();
		moveConstructFrom(rhs);
		return *this;
	}

	void clear()
	{
		std::apply([this](auto&... cols) { (destructColumn(cols, mNum), ...); }, mColumns);
		mNum = 0;
	}

	void reserve(size_t cap)
	{
		std::apply([this, cap](auto&... cols) { (reserveColumn(cols, mNum, cap), ...); }, mColumns);
	}

	void resize(size_t sz)
	{
		if (sz == mNum)
			return;

		if (sz < mNum)
		{
			size_t numRemove = mNum - sz;
			std::apply([sz, numRemove](auto&... cols) { (destructColumn(cols, sz, numRemove), ...); }, mColumns);
		}
		else
		{
			size_t numAdd = sz - mNum;
			std::apply([this, numAdd](auto&... cols) { (addDefaultColumn(cols, mNum, numAdd), ...); }, mColumns);
		}
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

	template<size_t I> auto* getColumn()
	{
		return getColumnData<I + 1>();
	}

	template<size_t I> const auto* getColumn() const
	{
		return getColumnData<I + 1>();
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
		std::apply([this](auto&... cols) { (reserveColumn(cols, mNum, mNum + 1), ...); }, mColumns);
		FTypeMemoryOp::Construct(getColumnData<0>() + mNum);
		push_back_impl(std::make_index_sequence<sizeof...(Ts)>{}, values...);
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
	template<typename, typename, typename...>
	friend class TSoAVectorWithAllocator;

	template<typename OtherAlloc, size_t... Is>
	void copyFrom_impl(size_t dst, const TSoAVectorWithAllocator<OtherAlloc, Dummy, Ts...>& src, size_t srcIdx, std::index_sequence<Is...>)
	{
		((getColumnData<Is>()[dst] = src.template getColumnData<Is>()[srcIdx]), ...);
	}

	template<size_t... Is>
	void push_back_impl(std::index_sequence<Is...>, const Ts&... values)
	{
		(pushBackColumn(std::get<Is + 1>(mColumns), values), ...);
	}

	void copyConstructFrom(const TSoAVectorWithAllocator& rhs)
	{
		std::apply([this, &rhs](auto&... cols) { (reserveColumn(cols, 0, rhs.mNum), ...); }, mColumns);
		copyConstructFrom_impl(rhs, std::make_index_sequence<NumTotalColumns>{});
		mNum = rhs.mNum;
	}

	template<size_t... Is>
	void copyConstructFrom_impl(const TSoAVectorWithAllocator& rhs, std::index_sequence<Is...>)
	{
		(copyConstructColumn(std::get<Is>(mColumns), std::get<Is>(rhs.mColumns), rhs.mNum), ...);
	}

	void moveConstructFrom(TSoAVectorWithAllocator& rhs)
	{
		std::apply([this, &rhs](auto&... cols) { (reserveColumn(cols, 0, rhs.mNum), ...); }, mColumns);
		moveConstructFrom_impl(rhs, std::make_index_sequence<NumTotalColumns>{});
		mNum = rhs.mNum;
		rhs.mNum = 0;
	}

	template<size_t... Is>
	void moveConstructFrom_impl(TSoAVectorWithAllocator& rhs, std::index_sequence<Is...>)
	{
		(moveConstructColumn(std::get<Is>(mColumns), std::get<Is>(rhs.mColumns), rhs.mNum), ...);
	}

	template<typename T>
	using AllocatorData = typename Allocator::template TArrayData<T>;

	template<typename TColumn>
	static auto* getColumnData(TColumn& col)
	{
		return col.getAllocation();
	}

	template<typename TColumn>
	static const auto* getColumnData(const TColumn& col)
	{
		return col.getAllocation();
	}

	template<size_t I>
	auto* getColumnData()
	{
		return getColumnData(std::get<I>(mColumns));
	}

	template<size_t I>
	const auto* getColumnData() const
	{
		return getColumnData(std::get<I>(mColumns));
	}

	template<typename TColumn>
	static void reserveColumn(TColumn& col, size_t oldSize, size_t newSize)
	{
		if (newSize > oldSize && col.needAlloc(oldSize, newSize - oldSize))
		{
			col.alloc(oldSize, newSize - oldSize);
		}
	}

	template<typename TColumn>
	static void destructColumn(TColumn& col, size_t num)
	{
		destructColumn(col, 0, num);
	}

	template<typename TColumn>
	static void destructColumn(TColumn& col, size_t index, size_t num)
	{
		FTypeMemoryOp::DestructSequence(getColumnData(col) + index, num);
	}

	template<typename TColumn>
	static void addDefaultColumn(TColumn& col, size_t oldSize, size_t numAdd)
	{
		reserveColumn(col, oldSize, oldSize + numAdd);
		FTypeMemoryOp::ConstructSequence(getColumnData(col) + oldSize, numAdd);
	}

	template<typename T, typename TColumn>
	void pushBackColumn(TColumn& col, const T& value)
	{
		FTypeMemoryOp::Construct(getColumnData(col) + mNum, value);
	}

	template<typename TColumn>
	static void copyConstructColumn(TColumn& dst, const TColumn& src, size_t num)
	{
		FTypeMemoryOp::ConstructSequence(getColumnData(dst), num, getColumnData(src));
	}

	template<typename TColumn>
	static void moveConstructColumn(TColumn& dst, TColumn& src, size_t num)
	{
		FTypeMemoryOp::MoveSequence(getColumnData(dst), num, getColumnData(src));
	}

	std::tuple<AllocatorData<Dummy>, AllocatorData<Ts>...> mColumns;
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
#define SOA_EXTRACT_VALUE(type, name) name,

#define DECLARE_SOA_LAYOUT(Name, Fields) \
	struct Name { Fields(SOA_EXTRACT_MEM) }; \
	struct Name##Ref { Fields(SOA_EXTRACT_REF_MEM) operator Name() const { return Name{ Fields(SOA_EXTRACT_VALUE) }; } }; \
	struct Name##ConstRef { Fields(SOA_EXTRACT_CONST_REF_MEM) operator Name() const { return Name{ Fields(SOA_EXTRACT_VALUE) }; } }; \
	template<typename Alloc = DefaultAllocator> \
	using Name##SoAWithAllocator = TSoAVectorWithAllocator<Alloc, int Fields(SOA_EXTRACT_TYPE) >; \
	using Name##SoA = Name##SoAWithAllocator<DefaultAllocator>;
