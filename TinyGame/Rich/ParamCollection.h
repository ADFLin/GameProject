#pragma once
#ifndef ParamCollection_H_5A9DFA41_157D_41B2_99FF_EF0DCB652E10
#define ParamCollection_H_5A9DFA41_157D_41B2_99FF_EF0DCB652E10


struct AnyParam
{
	template< class T >
	T  get() const { return static_cast<T>(ptrValue); }

	template<>
	int get<int>() const { return intValue; }

	template<>
	float get<float>() const { return floatValue; }

	template< class T , TEnableIf_Type< std::is_pointer_v<T> , bool > = true >
	void setValue(T value)
	{
		ptrValue = value;
	}

	void setValue(int value)
	{
		intValue = value;
	}
	void setValue(size_t value)
	{
		intValue = (int)value;
	}
	void setValue(float value)
	{
		floatValue = value;
	}

	union
	{
		int   intValue;
		float floatValue;
		void* ptrValue;
	};
};

template< int N >
struct ParamCollection
{
	ParamCollection()
	{
		numParam = 0;
	}
	static int const MaxParamNum = N;
	AnyParam params[MaxParamNum];
	int      numParam;

	template< class T >
	ParamCollection& addParam(T const& value)
	{
		params[numParam++].setValue(value);
		return *this;
	}
	template< class T >
	ParamCollection& addParam(T value[])
	{
		params[numParam++].setValue(static_cast<T*>(value));
		return *this;
	}

	template< class T >
	T getParam(int idx = 0) const
	{
		return params[idx].get<T>();
	}
};



#endif // ParamCollection_H_5A9DFA41_157D_41B2_99FF_EF0DCB652E10
