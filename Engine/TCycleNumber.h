#ifndef TCycleNumber_h__
#define TCycleNumber_h__

#include <cassert>

template < int N , class T = int >
class TCycleNumber
{
	enum EValueChecked
	{
		AsserChecked,
	};

public:
	static const T RestNumber = T( N );
	
	TCycleNumber():mValue(0){}
	explicit TCycleNumber( T val ) { setValue( val ); }
	//explicit TCycleNumber( EForceInit ) :mValue(0) {}


	static TCycleNumber ValueChecked( T val ){ return TCycleNumber( val , AsserChecked ); }
	
	void setValue( T val ){ mValue = val; validate();  }
	T    getValue() const { return mValue; }

	operator T() const { return mValue; }
	TCycleNumber inverse() const { return TCycleNumber( mValue + N / 2 ); }

	TCycleNumber const operator + ( T val ) const {	return TCycleNumber( mValue + val );  }
	TCycleNumber const operator - ( T val ) const {	return TCycleNumber( mValue - val );  }
	TCycleNumber const operator + ( TCycleNumber const& rhs) const
	{	
		T val = mValue + rhs.mValue;
		if ( val >= RestNumber )
			val -= RestNumber;
		return TCycleNumber( val , AsserChecked);
	}
	TCycleNumber const operator - ( TCycleNumber const& rhs) const
	{	
		T val = mValue - rhs.mValue;
		if ( val < 0 )
			val += RestNumber;
		return TCycleNumber( val , AsserChecked);
	}

	TCycleNumber& operator +=( T val ){	mValue += val; validate(); return *this;  }
	TCycleNumber& operator -=( T val ){	mValue -= val; validate(); return *this;  }

	TCycleNumber& operator +=( TCycleNumber const& rhs)
	{	
		mValue += rhs.mValue;
		if ( mValue >= RestNumber ) 
			mValue-= RestNumber; 
		return *this;
	}

	TCycleNumber& operator -=( TCycleNumber const& rhs)
	{	
		mValue -= rhs.mValue;
		if ( mValue < 0 ) 
			mValue += RestNumber; 
		return *this;
	}
	bool operator == ( TCycleNumber const& rhs ) const {   return mValue == rhs.mValue;   }
	bool operator != ( TCycleNumber const& rhs) const {   return !( this->operator == (rhs) );  }

	bool operator == ( T val ) const { CheckRange( val ); return mValue == val; }
	bool operator != ( T val ) const { CheckRange( val ); return mValue != val; }
	bool operator >  ( T val ) const { CheckRange( val ); return mValue > val;  }
	bool operator <  ( T val ) const { CheckRange( val ); return mValue < val;  }
	bool operator >= ( T val ) const { CheckRange( val ); return mValue >= val; }
	bool operator <= ( T val ) const { CheckRange( val ); return mValue <= val; }


	static bool IsValidRange(T a) { return ( 0 <= a && a < RestNumber ); }
private:

	
	TCycleNumber(T val, EValueChecked) { CheckRange(val);  mValue = val; }
	static void CheckRange( T a ){ assert(IsValidRange(a)); }
	void validate()
	{   
		mValue %= RestNumber;
		if ( mValue < 0 ) mValue += RestNumber;
	}
	T mValue;
};

#endif // TCycleNumber_h__