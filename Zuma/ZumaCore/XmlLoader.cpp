#include "ZumaPCH.h"
#include "XmlLoader.h"

#include "DebugSystem.h"

#if USE_NEW_XML

#define XML_ATTR_PATH "<xmlattr>"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

using boost::property_tree::ptree;
using boost::optional;

class CXmlStaticNode;
class CXmlNodeIter;
class CXmlGroupIter;
class CXmlNode;
class CXmlDocument;

enum CXmlType
{
	CXML_NODE ,
	CXML_NODE_ITER ,
	CXML_GROUP_ITER ,
	CXML_DOC ,
};

class CXmlNodeBase : public IXmlNode
{
public:
	bool            tryGetProperty( char const* key , int& value )        {  return tryGetPropertyT( key , value );  }
	bool            tryGetProperty( char const* key , float& value )      {  return tryGetPropertyT( key , value );  }
	bool            tryGetProperty( char const* key , char const*& value );
	int             getProperty( char const* key , int value )        { return getPropertyT( key , value ); }
	float           getProperty( char const* key , float value )      { return getPropertyT( key , value ); }
	char const*     getProperty( char const* key , char const* value );

	char const*     getStringProperty( char const* key );
	int             getIntProperty( char const* key )   { return getPropertyThrowT< int >( key ); }
	float           getFloatProperty( char const* key ) { return getPropertyThrowT< float >( key ); }

	IXmlNodePtr     getAttriute(){ return getChild( XML_ATTR_PATH ); }
	IXmlNodePtr     getChild( char const* name );
	IXmlNodeIterPtr getChildren();
	IXmlNodeIterPtr getChildGroup( char const* name );

	virtual int    countChildGroup( char const* name );
	virtual void   release() = 0;

	int getType(){ return CXML_NODE; }

protected:

	template< class T >
	T getPropertyThrowT( char const* key )
	{
		try
		{
			return mNodeImpl->get< T >( key );
		}
		catch( boost::property_tree::ptree_bad_data &)
		{
			throw XmlException();
		}

	}

	template< class T >
	T getPropertyT( char const* key , T val )
	{
		return mNodeImpl->get< T >( key , val );
	}
	template< class T >
	bool tryGetPropertyT(  char const* key , T& val )
	{
		optional<T> const& gV = mNodeImpl->get_optional<T>( key );
		if ( gV )
		{
			val = gV.get();
			return true;
		}
		return false;
	}
	
	friend class CXmlDocument;
	friend class CXmlNodeIter;
	friend class CXmlGroupIter;
	ptree*        mNodeImpl;
	CXmlDocument* mDoc;
};

class CXmlStaticNode : public CXmlNodeBase
{
public:
	void release(){}
};

class CXmlNode : public CXmlNodeBase
{
public:
	void release();
};

class CXmlNodeIter : public IXmlNodeIter
{
public:
	CXmlNodeIter(){}
	~CXmlNodeIter(){}
	void        init( CXmlNodeBase* parent );
	//IXmlNodeIterator
	void        release();
	bool        haveMore();
	void        goNext();
	char const* getName();
	IXmlNodePtr getNode();

	int         getType(){ return CXML_NODE_ITER;  }
	ptree::iterator mIterImpl;
	CXmlStaticNode  mNode;
	CXmlNodeBase*   mParent;
};


class CXmlGroupIter : public IXmlNodeIter
{
public:
	CXmlGroupIter(){}
	~CXmlGroupIter(){}

	void        init( CXmlNodeBase* parent , char const* name );
	//IXmlNodeIterator
	void        release();
	bool        haveMore(){ return mIterImpl.first != mIterImpl.second; }
	void        goNext();
	char const* getName(){ return mIterImpl.first->first.c_str(); }
	IXmlNodePtr getNode(){ return &mNode; }

	int         getType(){ return CXML_GROUP_ITER; }
	typedef std::pair< ptree::assoc_iterator, ptree::assoc_iterator > IterPair;
	IterPair       mIterImpl;
	CXmlStaticNode mNode;
	CXmlNodeBase*  mParent;
};

class CXmlDocument : public IXmlDocument
{
public:
	CXmlDocument( char const* path );
	~CXmlDocument();
	IXmlNodePtr        getRoot(){ return &mRootNode; }
	void               release();
	CXmlNode*          fetchNode();
	CXmlNodeIter*      fetchNodeIter();
	CXmlGroupIter*     fetchGroupIter();
	void               freeNode( CXmlNode* node );
	void               freeNodeIter( CXmlNodeIter* iter );
	void               freeGroupIter( CXmlGroupIter* iter );
	int                getType(){ return CXML_DOC; }
	void               cleanupCacheObject();
private:
	IXmlObject*        fetchFreeObj( CXmlType type );

	std::vector< IXmlObject* >   mFreeObj;
	std::vector< IXmlObject* >   mAllocObj;
	ptree          mRootImpl;
	CXmlStaticNode mRootNode;
};


void CXmlNodeIter::release()
{
	mParent->mDoc->freeNodeIter( this );
}

bool CXmlNodeIter::haveMore()
{
	return mIterImpl != mParent->mNodeImpl->end();
}

void CXmlNodeIter::init( CXmlNodeBase* parent )
{
	mParent    = parent;
	mIterImpl  = parent->mNodeImpl->begin();
	mNode.mDoc = parent->mDoc;
	if ( mIterImpl != parent->mNodeImpl->end() )
	{
		mNode.mNodeImpl = &mIterImpl->second;
	}
	
}

void CXmlNodeIter::goNext()
{
	++mIterImpl; 
	mNode.mNodeImpl = &mIterImpl->second;
}

IXmlNodePtr CXmlNodeIter::getNode()
{
	return &mNode;
}

char const* CXmlNodeIter::getName()
{
	return mIterImpl->first.c_str();
}

CXmlDocument::CXmlDocument( char const* path )
{
	read_xml( path , mRootImpl );
	mRootNode.mDoc      = this;
	mRootNode.mNodeImpl = &mRootImpl;
}

CXmlDocument::~CXmlDocument()
{
	for( int i = 0 ; i < mAllocObj.size() ; ++i )
	{
		IXmlObject* obj = mAllocObj[i];
		delete obj;
	}
}

void CXmlDocument::release()
{
	delete this;
}

void CXmlDocument::cleanupCacheObject()
{
	//for( int i = 0 ; i < mAllocObj.size() ; ++i )
	//{
	//	IXmlObject* obj = mAllocObj[i];
	//	delete obj;
	//}
	//mAllocObj.clear();
}

IXmlObject* CXmlDocument::fetchFreeObj( CXmlType type )
{
	int size = (int)mFreeObj.size() - 1;
	for ( int i = 0 ; i <= size ; ++i )
	{
		IXmlObject* obj = mFreeObj[i];
		if ( obj->getType() == type )
		{
			if ( i != size )
			{
				mFreeObj[i] = mFreeObj[ size ];
			}
			mFreeObj.pop_back();
			return obj;
		}
	}
	return NULL;
}


CXmlNodeIter* CXmlDocument::fetchNodeIter()
{
	CXmlNodeIter* iter = static_cast< CXmlNodeIter* >( fetchFreeObj( CXML_NODE_ITER ) );
	if ( !iter )
	{
		iter =  new CXmlNodeIter;
		mAllocObj.push_back( iter );
	}
	return iter;
}

CXmlGroupIter* CXmlDocument::fetchGroupIter()
{
	CXmlGroupIter* iter = static_cast< CXmlGroupIter* >( fetchFreeObj( CXML_GROUP_ITER ) );
	if ( !iter )
	{
		iter =  new CXmlGroupIter;
		mAllocObj.push_back( iter );
	}
	return iter;
}

CXmlNode* CXmlDocument::fetchNode()
{
	CXmlNode* node = static_cast< CXmlNode* >( fetchFreeObj( CXML_NODE ) );
	if ( !node )
	{
		node =  new CXmlNode;
		node->mDoc = this;
		mAllocObj.push_back( node );
	}
	return node;
}

void CXmlDocument::freeGroupIter( CXmlGroupIter* iter )
{
	mFreeObj.push_back( iter );
}

void CXmlDocument::freeNodeIter( CXmlNodeIter* node )
{
	mFreeObj.push_back( node );
}

void CXmlDocument::freeNode( CXmlNode* node )
{
	mFreeObj.push_back( node );
}

IXmlNodePtr CXmlNodeBase::getChild( char const* name )
{
	try
	{
		ptree& pt = mNodeImpl->get_child( name );
		CXmlNode* node = mDoc->fetchNode();
		node->mNodeImpl = &pt;
		return node;
	}
	catch ( boost::property_tree::ptree_bad_path& )
	{
		//::WarmingMsg( 0 , e.what() );
		return NULL;
	}
}

void CXmlNode::release()
{
	mDoc->freeNode( this );
}

IXmlNodeIterPtr CXmlNodeBase::getChildren()
{
	CXmlNodeIter* iter = mDoc->fetchNodeIter();
	iter->init( this );
	return iter;
}


void CXmlGroupIter::init( CXmlNodeBase* parent , char const* name )
{
	mParent    = parent;
	mIterImpl  = parent->mNodeImpl->equal_range( name );
	mNode.mDoc = parent->mDoc;
	if ( mIterImpl.first != mIterImpl.second )
	{
		ptree::value_type& value = *mIterImpl.first;
		mNode.mNodeImpl = &value.second;
	}
}

void CXmlGroupIter::release()
{
	mParent->mDoc->freeGroupIter( this );
}

void CXmlGroupIter::goNext()
{
	++mIterImpl.first;
	ptree::value_type& value = *mIterImpl.first;
	mNode.mNodeImpl = &value.second;
}


int CXmlNodeBase::countChildGroup( char const* name )
{
	return (int)mNodeImpl->count( name );
}

IXmlNodeIterPtr CXmlNodeBase::getChildGroup( char const* name )
{
	CXmlGroupIter* iter = mDoc->fetchGroupIter();
	iter->init( this , name );
	return iter;
}

char const* CXmlNodeBase::getStringProperty( char const* key )
{
	try
	{
		return mNodeImpl->get_child( key ).data().c_str();
	}
	catch( boost::property_tree::ptree_bad_path&  )
	{
		throw XmlException();
	}
}

bool CXmlNodeBase::tryGetProperty( char const* key , char const*& value )
{
	ptree::assoc_iterator iter = mNodeImpl->find( key );
	if ( mNodeImpl->to_iterator( iter ) == mNodeImpl->end() )
		return false;
	value = iter->second.data().c_str();

	return true;
}

char const* CXmlNodeBase::getProperty( char const* key , char const* value )
{
	ptree::assoc_iterator iter = mNodeImpl->find( key );
	if ( mNodeImpl->to_iterator( iter ) == mNodeImpl->end() )
		return value;
	return iter->second.data().c_str();
}

IXmlDocumentPtr IXmlDocument::createFromFile( char const* path )
{
	try
	{
		 IXmlDocument* doc = new CXmlDocument( path );
		 return doc;
	}
	catch (boost::property_tree::xml_parser_error& e)
	{
		::ErrorMsg( e.what() );
		return NULL;
	}
}

#else //USE_NEW_XML

bool XmlLoader::loadFile( char const* path )
{
	try
	{
		read_xml( path , m_root );
	}
	catch (boost::property_tree::xml_parser_error& e)
	{
		::ErrorMsg( e.what() );
		return false;
	}
	return true;
}


ptree* XmlLoader::findID( ptree& pt , char const* nameGroup , char const* nameID )
{
	typedef std::pair< ptree::assoc_iterator, ptree::assoc_iterator > IterPair;
	IterPair pair = pt.equal_range( nameGroup );

	for( ; pair.first != pair.second; ++pair.first )
	{
		ptree::value_type& ptFind = *pair.first;

		ptree& ptGroup = ptFind.second.get_child( XML_ATTR_PATH );

		optional< std::string > id = ptGroup.get_optional< std::string >("id");

		if ( id && strcmp( id.get().c_str(), nameID ) == 0 )
			return &ptFind.second;
	}
	return NULL;
}

#endif //USE_NEW_XML