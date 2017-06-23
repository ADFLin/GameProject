#ifndef XmlLoader_h__
#define XmlLoader_h__

#define USE_NEW_XML 1

#if USE_NEW_XML 

class IXmlObject
{
public:
	IXmlObject():mRefCount(0){}
	virtual ~IXmlObject(){}
	virtual int   getType() = 0;
	virtual void  release() = 0;

private:
	template< class T >
	friend class IXmlObjectPtr;
	int mRefCount;
};

template< class T >
class IXmlObjectPtr
{
public:
	IXmlObjectPtr():mPtr(0){}
	IXmlObjectPtr( T* ptr ):mPtr( ptr ){ doRef(); }
	IXmlObjectPtr( IXmlObjectPtr& other ):mPtr( other.mPtr ){ doRef(); }
	~IXmlObjectPtr(){ doUnRef(); }

	IXmlObjectPtr& operator = ( IXmlObjectPtr& other )
	{
		doUnRef();
		mPtr = other.mPtr;
		doRef();
		return *this;
	}

	T*       operator -> ()       { return mPtr; }
	T const* operator -> () const { return mPtr; }

	operator void* () const { return mPtr; } 

private:
	void doRef()
	{
		if ( mPtr )
			++mPtr->mRefCount;
	}
	void doUnRef()
	{
		if ( mPtr && ( --mPtr->mRefCount == 0 ) )
		{
			mPtr->release();
			mPtr = NULL;
		}
	}
	T* mPtr;
};

class IXmlNodeIter;
class IXmlNode;
class IXmlDocument;

typedef IXmlObjectPtr< IXmlNode >      IXmlNodePtr;
typedef IXmlObjectPtr< IXmlNodeIter >  IXmlNodeIterPtr;
typedef IXmlObjectPtr< IXmlDocument >  IXmlDocumentPtr;


class XmlException : public std::exception
{

};

class IXmlNode : public IXmlObject
{
public:
	virtual bool            tryGetProperty( char const* key , int& value ) = 0;
	virtual bool            tryGetProperty( char const* key , float& value ) = 0;
	virtual bool            tryGetProperty( char const* key , char const*& value ) = 0;
	virtual int             getProperty( char const* key , int value ) = 0;
	virtual float           getProperty( char const* key , float value ) = 0;
	virtual char const*     getProperty( char const* key , char const* value ) = 0;
	virtual char const*     getStringProperty( char const* key ) /*throw XmlException*/= 0;
	virtual int             getIntProperty( char const* key )   /*throw XmlException*/ = 0;
	virtual float           getFloatProperty( char const* key ) /*throw XmlException*/ = 0;
	virtual int             countChildGroup( char const* name ) = 0;
	virtual IXmlNodePtr     getChild( char const* name ) = 0;
	virtual IXmlNodePtr     getAttriute() = 0;
	virtual IXmlNodeIterPtr getChildren() = 0;
	virtual IXmlNodeIterPtr getChildGroup( char const* name ) = 0;
};

class IXmlNodeIter : public IXmlObject
{
public:
	virtual bool        haveMore() = 0;
	virtual void        goNext() = 0;
	virtual char const* getName() = 0;
	virtual IXmlNodePtr getNode() = 0; 
};

class IXmlDocument : public IXmlObject
{
public:
	virtual IXmlNodePtr     getRoot() = 0;
	virtual void            cleanupCacheObject() = 0;
public:
	static IXmlDocumentPtr CreateFromFile( char const* path );
};


#else  //USE_NEW_XML

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

#define XML_ATTR_PATH "<xmlattr>"

using boost::property_tree::ptree;
using boost::optional;


class XmlLoader
{
public:
	bool   loadFile( char const* path );
	ptree* findID( ptree& pt , char const* nameGroup , char const* nameID );

	template< class T >
	static bool getVal( ptree& pt , T& val , char const* key )
	{
		optional<T> const& gV = pt.get_optional<T>( key );
		if ( gV )
		{
			val = gV.get();
			return true;
		}
		return false;
	}

	ptree& getRoot(){ return m_root; }
private:
	ptree  m_root;
};

#endif //USE_NEW_XML


#endif // XmlLoader_h__
