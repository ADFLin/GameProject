#pragma once
#ifndef CycleValue_H_652E0FBB_9A12_4610_A7D0_22724E31D44D
#define CycleValue_H_652E0FBB_9A12_4610_A7D0_22724E31D44D

#include <cassert>

template < int N , class T = int >
class TCycleValue
{
	enum EValueChecked
	{
		AssumeChecked,
	};

public:
	static const T RestValue = T( N );
#if 1
	TCycleValue():mValue(0){}
#else
	explicit TCycleValue( EForceInit ) :mValue(0) {}
#endif
	explicit TCycleValue( T val ) { setValue( val ); }

	static TCycleValue ValueChecked( T val ){ return TCycleValue( val , AssumeChecked ); }
	
	void setValue( T val ){ mValue = val; validate();  }
	T    getValue() const { return mValue; }

	operator T() const { return mValue; }
	TCycleValue inverse() const { return TCycleValue( mValue + N / 2 ); }

	TCycleValue const operator + ( T val ) const {	return TCycleValue( mValue + val );  }
	TCycleValue const operator - ( T val ) const {	return TCycleValue( mValue - val );  }
	TCycleValue const operator + ( TCycleValue const& rhs) const
	{	
		T val = mValue + rhs.mValue;
		if ( val >= RestValue )
			val -= RestValue;
		return TCycleValue( val , AssumeChecked);
	}
	TCycleValue const operator - ( TCycleValue const& rhs) const
	{	
		T val = mValue - rhs.mValue;
		if ( val < 0 )
			val += RestValue;
		return TCycleValue( val , AssumeChecked);
	}

	TCycleValue& operator +=( T val ){	mValue += val; validate(); return *this;  }
	TCycleValue& operator -=( T val ){	mValue -= val; validate(); return *this;  }

	TCycleValue& operator +=( TCycleValue const& rhs)
	{	
		mValue += rhs.mValue;
		if ( mValue >= RestValue ) 
			mValue-= RestValue; 
		return *this;
	}

	TCycleValue& operator -=( TCycleValue const& rhs)
	{	
		mValue -= rhs.mValue;
		if ( mValue < 0 ) 
			mValue += RestValue; 
		return *this;
	}
	bool operator == ( TCycleValue const& rhs ) const {   return mValue == rhs.mValue;   }
	bool operator != ( TCycleValue const& rhs) const {   return !( this->operator == (rhs) );  }

	bool operator == ( T val ) const { CheckRange( val ); return mValue == val; }
	bool operator != ( T val ) const { CheckRange( val ); return mValue != val; }
	bool operator >  ( T val ) const { CheckRange( val ); return mValue > val;  }
	bool operator <  ( T val ) const { CheckRange( val ); return mValue < val;  }
	bool operator >= ( T val ) const { CheckRange( val ); return mValue >= val; }
	bool operator <= ( T val ) const { CheckRange( val ); return mValue <= val; }


	static bool IsValidRange(T a) { return ( 0 <= a && a < RestValue ); }
private:

	
	TCycleValue(T val, EValueChecked) { CheckRange(val);  mValue = val; }
	static void CheckRange( T a ){ assert(IsValidRange(a)); }
	void validate()
	{   
		mValue %= RestValue;
		if ( mValue < 0 ) mValue += RestValue;
	}
	T mValue;
};

#endif // CycleValue_H_652E0FBB_9A12_4610_A7D0_22724E31D44D