#ifndef dataFileIO_h__
#define dataFileIO_h__

#include "common.h"
#include "TFileIO.h"

class dataFileIO : public TFileIO
{
public:
	template < class T >
	void write(T* p){  setVal( *p );  }
	template < class T >
	void read(T* p){   getVal(*p);  }

	template < class T > 
	void write( T* p , size_t num )
	{
		for (size_t i = 0; i < num ; ++i )
			setVal( *(p + i) );
	}

	template < class T > 
	void read( T* p , size_t num )
	{
		for (size_t i = 0; i < num ; ++i )
			getVal( *(p + i) );
	}
};

#endif // dataFileIO_h__
