#ifndef TCycleNumber_h__
#define TCycleNumber_h__

template < int N , class T = int >
class TCycleNumber
{
	struct NoCheck {};

public:
	static const T RestNumber = T( N );
	
	TCycleNumber():m_val(0){}
	explicit TCycleNumber( T val ) { setValue( val ); }


	static TCycleNumber ValueNoCheck( T val ){ return TCycleNumber( val , NoCheck()); }
	
	void setValue( T val ){ m_val = val; sensible();  }
	T    getValue() const { return m_val; }

	operator T() const { return m_val; }
	TCycleNumber inverse() const { return TCycleNumber( m_val + N / 2 ); }

	TCycleNumber const operator + ( T val ) const {	return TCycleNumber( m_val + val );  }
	TCycleNumber const operator - ( T val ) const {	return TCycleNumber( m_val - val );  }
	TCycleNumber const operator + ( TCycleNumber const& vd ) const 
	{	
		T val = m_val + vd.m_val;
		if ( val >= RestNumber )
			val -= RestNumber;
		return TCycleNumber( val , NoCheck() );  
	}
	TCycleNumber const operator - ( TCycleNumber const& vd ) const 
	{	
		T val = m_val - vd.m_val;
		if ( val < 0 )
			val += RestNumber;
		return TCycleNumber( val , NoCheck() );   
	}

	TCycleNumber& operator +=( T val ){	m_val += val; sensible(); return *this;  }
	TCycleNumber& operator -=( T val ){	m_val -= val; sensible(); return *this;  }

	TCycleNumber& operator +=( TCycleNumber const& vd )
	{	
		m_val += vd.m_val; 
		if ( m_val >= RestNumber ) 
			m_val-= RestNumber; 
		return *this;
	}

	TCycleNumber& operator -=( TCycleNumber const& vd )
	{	
		m_val -= vd.m_val; 
		if ( m_val < 0 ) 
			m_val += RestNumber; 
		return *this;
	}
	bool operator == ( TCycleNumber const& dir ) const {   return m_val == dir.m_val;   }
	bool operator != ( TCycleNumber const& dir ) const {   return !( this->operator == ( dir ) );  }

	bool operator == ( T val ) const { checkRange( val ); return m_val == val; }
	bool operator != ( T val ) const { checkRange( val ); return m_val != val; }
	bool operator >  ( T val ) const { checkRange( val ); return m_val > val;  }
	bool operator <  ( T val ) const { checkRange( val ); return m_val < val;  }
	bool operator >= ( T val ) const { checkRange( val ); return m_val >= val; }
	bool operator <= ( T val ) const { checkRange( val ); return m_val <= val; }

private:

	
	TCycleNumber( T val , NoCheck ){ assert( 0 <= val && val < RestNumber );  m_val = val; }
	static void checkRange( T a ){ assert( 0 <= a && a < RestNumber ); }
	void sensible()
	{   
		m_val %= RestNumber;
		if ( m_val < 0 ) m_val += RestNumber;
	}
	T m_val;
};

#endif // TCycleNumber_h__