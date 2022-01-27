#pragma once
#ifndef Expected_H_3C879F2F_F969_4EEB_8305_B24196557964
#define Expected_H_3C879F2F_F969_4EEB_8305_B24196557964


template< class ...TypeList >
struct TMaxSizeof
{
};

template< class T, class ...TypeList >
struct TMaxSizeof< T, TypeList... >
{
	static constexpr int RHSResult = TMaxSizeof<TypeList...>::Result;
	static constexpr int Result = sizeof(T) >= RHSResult ? sizeof(T) : RHSResult;
};

template< class T >
struct TMaxSizeof< T >
{
	static constexpr int Result = sizeof(T);
};

template< class ...TypeList >
struct TInlineMemoryStorage
{
	template< class T >
	void construct() 
	{ 
		TypeDataHelper::Construct<T>(mData);
	}
	template< class T >
	void construct(T&& v) 
	{
		TypeDataHelper::Construct<T>(mData, v);
	}

	template< class T >
	void destruct() { get<T>().~T(); }


	template< class T >
	T& get() { return *reinterpret_cast<T*>(mData); }

	uint8 mData[TMaxSizeof< TypeList... >::Result];
};

template< typename VT, typename ET >
class TUnexpected
{

};

template< typename VT , typename ET >
class TExpected
{
public:
	using ValueType = VT;
	using ErrorType = ET;

	TExpected(VT const& rhs)
		:mbHaveValue(true)
	{
		mStroage.construct<ValueType>(rhs);
	}

	TExpected(VT&& rhs)
		:mbHaveValue(true)
	{
		mStroage.construct<ValueType>(std::move(rhs));
	}

	TExpected(TUnexpected<ET> const& rhs)
		:mbHaveValue(false)
	{
		mStroage.construct<ET>(std::move(rhs));
	}

	TExpected(TUnexpected<ET>&& rhs)
		:mbHaveValue(false)
	{
		mStroage.construct<ET>(std::move(rhs));
	}

	~TExpected()
	{
		if (mbHaveValue)
		{
			mStroage.destruct<ValueType>();
		}
		else
		{
			mStroage.destruct<ErrorType>();
		}
	}
	bool haveValue() const { return mbHaveValue; }


private:

	TExpected() = delete;

	TInlineMemoryStorage< ValueType, ErrorType > mStroage;
	bool mbHaveValue;
};


#endif // Expected_H_3C879F2F_F969_4EEB_8305_B24196557964