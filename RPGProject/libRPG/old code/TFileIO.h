#ifndef TFileIO_h__
#define TFileIO_h__

#include <fstream>
#include <string>
#include "common.h"


class TFileIO
{
public:
	bool load(char const* fileName);
	void goToString( char const* s );
	void goNextLine()
	{	fs.getline( buf , sizeof( buf ) );   }


	template < class  T > 
	inline void getVal( T& t ){ fs >> t; }


	void getVal( Vec3D& v )
	{  fs >> v[0]; fs >> v[1]; fs >> v[2];  }
	void getVal( Quat& q )
	{  fs >> q[0]; fs>>q[1] ; fs >>q[2] ;fs >> q[3];  }
	void batchElement( int num );

public:
	bool save( char const* fileName );
	template< class T >
	void setVal( T const& t ){ fs << t <<" "; }


	void setVal( const char* str ){ fs << str << " "; }
	void setVal( Vec3D& v )
	{  fs << v[0] << " " << v[1] << " " << v[2] << " ";  }
	void setVal( Quat& q )
	{  fs << q[0] << " " << q[1] << " " << q[2] << " " << q[3] << " ";  }
	void setNextLine(){ fs << "\n"; }

public:
	std::fstream fs;
	char buf[256];

private:
	std::string  str;


};

#endif // TFileIO_h__