#ifndef CGFContent_h__
#define CGFContent_h__

#include "serialization.h"


class CSceneLevel;
class IDesignObject;
class ICGFBase;

enum CGFType
{
	CGF_SOILD   ,
	CGF_ENTITY  ,
	CGF_BRUSH   ,
	CGF_PREFABS ,
	CGF_TERRAIN ,
	CGF_AREA    ,
	CGF_SCENE   ,
};

struct CGFContructHelper
{
	bool         isLoading;
	CSceneLevel* level;
};



class IDesignObject
{
public:
	virtual ICGFBase* getCGF() = 0;
	virtual void modify() = 0;
	virtual void destroy() = 0;
};


struct CGFParamValue 
{
	union
	{
		int   iValue;
		float fValue;
		void* ptr;
		char const*   strValue;
		struct { float x , y , z; } vec3;
	};

	CGFParamValue( int value ):iValue( value ){}
	CGFParamValue( unsigned value ):iValue( value ){}
	CGFParamValue( float value ):fValue( value ){}
	CGFParamValue( char const* value ):strValue( value ){}
	CGFParamValue( void* value ):ptr( value ){}

};
class ICGFBase
{
public:
	ICGFBase( CGFType type ):mType( type ){}
	CGFType getType(){ return mType;  }
	//virtual IDesignObject* createDesignObject() = 0;
	virtual bool construct( CGFContructHelper const& helper ) = 0;
	//virtual void release()= 0;
	virtual void setupParam( int paramID , CGFParamValue const& value ){}

private:
	CGFType mType;
};

template< class T , CGFType TYPE >
class CGFHelper : public ICGFBase
{
public:
	CGFHelper():ICGFBase( TYPE ){}
	//virtual void setupParam( int paramID , CGFParamValue const& value ) = 0;

	struct SParamDesc
	{
		int id;

	};
	//virtual void 
};


class CBrushCGF : public CGFHelper< CBrushCGF , CGF_BRUSH >
{





	bool      isStatic;
	CFObject* obj;
	String    mPath;
};



class CGFContent
{
public:
	void  add( ICGFBase* cgf ){ mCGFVec.push_back( cgf );  }
	bool  loadFromFile( char const* path );
	typedef std::vector< ICGFBase* > CGFVec;
	CGFVec mCGFVec;
};



#endif // CGFContent_h__
