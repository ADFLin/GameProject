#ifndef TTable_h__
#define TTable_h__

#include <list>
#include <vector>
#include <new>

template < class T , class K = unsigned >
class TTable
{
public:
	typedef std::list< T > CStorage;
	typedef typename CStorage::iterator       iterator;
	typedef typename CStorage::const_iterator const_iterator;
	typedef K        KeyType;

	TTable()
	{
		mFreeID = sErrorID;
	}
	
	void      reserve( size_t size ){  mMap.reserve( size );  }
	void      clear();
	bool      replace( KeyType id , T const& t );
	KeyType   insert( T const& t );
	bool      remove( KeyType id );
	T*        getItem( KeyType id );
	size_t    size() const { return mStorage.size(); }

	iterator       begin()       {  return mStorage.begin();  }
	iterator       end()         {  return mStorage.end();  }
	const_iterator begin() const {  return mStorage.begin();  }
	const_iterator end()   const {  return mStorage.end();  }

	iterator  find( T const& t ){  return std::find( begin() , end() , t );  }

private:

	struct MapData
	{
		iterator getData(){  return iter;  }
		void     setData( iterator _iter )
		{ 
			index = 0;
			iter  = _iter;  
		}
		iterator iter;
		unsigned index;
	};
	typedef std::vector< MapData > CMap;

	static const KeyType sErrorID = ( KeyType ) -1;

	KeyType   mFreeID;
	CStorage  mStorage;
	CMap      mMap;
};

template < class T , class K>
void TTable< T , K >::clear()
{
	mFreeID = sErrorID;
	mStorage.clear();
	mMap.clear();
}

template < class T , class K>
bool  TTable< T , K >::replace( KeyType id , T const& t )
{
	CStorage::iterator iter = mMap[id].getData();
	if ( iter == mStorage.end() )
		return false;

	mStorage.erase( iter );
	mStorage.push_front( t );

	mMap[id].setData( mStorage.begin() );

	return true;
}

template < class T , class K>
K  TTable< T , K >::insert( T const& t )
{
	mStorage.push_front( t );

	if ( mFreeID != sErrorID )
	{
		KeyType idx = mFreeID - 1;

		MapData& data = mMap[ idx ];
		mFreeID = data.index;
		data.setData( mStorage.begin() );
		return idx;
	}

	MapData data;
	data.setData( mStorage.begin() );
	mMap.push_back( data );
	return KeyType(mMap.size() - 1);
}

template < class T , class K >
bool  TTable< T , K >::remove( KeyType id )
{
	if ( mMap[id].index )
		return false;

	mStorage.erase( mMap[id].getData() );
	mMap[id].index = mFreeID;
	mFreeID = id + 1;
	return true;
}

template < class T , class K >
T*   TTable< T , K >::getItem( KeyType id )
{
	if ( id >= mMap.size() )
		return NULL;

	CStorage::iterator iter = mMap[id].getData();

	if ( !mMap[id].index )
		return &( *iter );

	return NULL;
}

#if 0

template < class T , class K = unsigned >
class TTableNew
{
public:

	typedef K        KeyType;

	TTable()
	{
		mFreeId = 0;
		mNumUsed = 0;
		mLists   = 0;
	}

	void      reserve( size_t size )
	{
		mMap.reserve( size );  
	}
	void      clear();
	bool      replace( KeyType key , T const& val )
	{
		if ( key >= mMap.size() )
			return false;

		Node* node = mMap[ key ];
		if ( node->)

	}
	KeyType   insert( T const& val )
	{
		KeyType key = fetchNode();
		Node* node = mMap[ key ];
		buildNode( node , val );
		return key;
	}
	bool      remove( KeyType id )
	{
		if ( id >= mMap.size() )
			return false;

		Node* node = mMap[ id ];
		if ( !node )
			return false;

		freeNode( node );
		return true;
	}
	T*        getItem( KeyType id )
	{
		if ( id < mMap.size() )
		{		
			Node* node = mMap[ id ];
			if ( node )
				return reinterpret_cast< T* >( node->data );
		}
		return 0;
	}
	size_t    size() const { return mNumUsed; }

	class Iterator
	{




	};
	iterator       begin()       {  return mStorage.begin();  }
	iterator       end()         {  return mStorage.end();  }
	const_iterator begin() const {  return mStorage.begin();  }
	const_iterator end()   const {  return mStorage.end();  }

	iterator  find( T const& t ){  return std::find( begin() , end() , t );  }

private:

	struct Node
	{
		char   data[ sizeof( T ) ];
		Node*  next;
		union
		{
			Node**  prev;
		};
	};

	KeyType  fetchNode()
	{
		KeyType key;
		if ( mFreeId )
		{
			key = mFreeId - 1;
			mFreeId = mMap[ key ];
		}
		else
		{
			key = mMap.size();
			mMap.resize( mMap.size() + 1 , new Node );
		}
		++mNumUsed;
		return key;
	}
	void   freeNode( Node* node )
	{
		reinterpret_cast< T* >( node->data )->~T();
		if ( node->prev )
			*( node->prev ) = node->next; 
		node->next->prev = node->prev;
		--mNumUsed;
	}

	void  buildNode( Node* node , T const& val )
	{
		new ( node->data ) T( val );
		node->next = mLists;
		node->prev = 0;
		if ( mLists )
			mLists->prev = &mLists;
		mLists = node;
	}


	typedef std::vector< Node* > MapType;
	MapType mMap;
	Node*   mLists;
	size_t  mNumUsed;
	KeyType mFreeId;
};
#endif

#endif // TTable_h__