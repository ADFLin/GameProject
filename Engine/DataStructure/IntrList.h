#ifndef IntrList_h__
#define IntrList_h__

#include "TypeCast.h"

#include <cassert>
#include <algorithm>

class ListNode
{
public:
	ListNode* getPrev() const { return mPrev; }
	ListNode* getNext() const { return mNext; }
protected:
	ListNode* mPrev;
	ListNode* mNext;
	friend struct ListNodeTraits;
};

struct NodeTraits
{
	class NodePtr;
	static NodePtr getPrev( NodePtr const& n );
	static NodePtr getNext( NodePtr const& n );
	static void setNext( NodePtr const& n , NodePtr const& next );
	static void setPrev( NodePtr const& n , NodePtr const& prev );
	static bool isEmpty( NodePtr const& n );
};

struct ListNodeTraits
{
	typedef ListNode  NodeType;
	typedef ListNode* NodePtr;
	typedef ListNode const* ConstNodePtr;

	static NodePtr getPrev( NodePtr const& n ){ return n->mPrev; }
	static NodePtr getNext( NodePtr const& n ){ return n->mNext; }
	static ConstNodePtr getPrev( ConstNodePtr const& n ){ return n->mPrev; }
	static ConstNodePtr getNext( ConstNodePtr const& n ){ return n->mNext; }
	static void setNext( NodePtr const& n , NodePtr const& next ){ n->mNext = next; }
	static void setPrev( NodePtr const& n , NodePtr const& prev ){ n->mPrev = prev; }
	static bool isEmpty( ConstNodePtr const& n ){ return n == 0; }
	static bool isEmpty( NodePtr const& n ){ return n == 0; }

};

template< class NT >
class CycleListAlgorithm
{
	using NodeTraits = NT;
	typedef typename NodeTraits::NodeType NodeType;
	typedef typename NodeTraits::NodePtr NodePtr;
	typedef typename NodeTraits::ConstNodePtr ConstNodePtr;

public:
	static void initHeader( NodePtr const& n )
	{
		NodeTraits::setNext( n , n );
		NodeTraits::setPrev( n , n );
	}

	static void swapHeader(NodePtr const& n1, NodePtr const& n2)
	{
		NodeType tempNode;
		linkBefore(&tempNode , n1);
		unlink(n1);
		linkBefore(n1, n2);
		unlink(n2);
		linkAfter(n2, &tempNode);
		unlink(&tempNode);
	}

	static void initNode( NodePtr const& n )
	{
		NodeTraits::setNext( n , NodePtr(0) );
		NodeTraits::setPrev( n , NodePtr(0) );
	}
	static bool isLinked( ConstNodePtr const& n )
	{ 
		return NodeTraits::isEmpty( NodeTraits::getNext( n ) ) == false;
	}

	static void linkBefore( NodePtr const& node , NodePtr const& where )
	{
		NodePtr prev = NodeTraits::getPrev( where );

		NodeTraits::setPrev( node , prev );
		NodeTraits::setNext( node , where );

		NodeTraits::setPrev( where , node );
		NodeTraits::setNext( prev , node );
	}

	static void linkAfter( NodePtr const& node , NodePtr const& where )
	{
		NodePtr next = NodeTraits::getNext( where );

		NodeTraits::setPrev( node , where );
		NodeTraits::setNext( node , next );

		NodeTraits::setNext( where , node );
		NodeTraits::setPrev( next  , node );
	}

	static void linkBefore( NodePtr const& from , NodePtr const& to , NodePtr const& where )
	{
		NodePtr prev = NodeTraits::getPrev( where );

		NodeTraits::setPrev( from , prev );
		NodeTraits::setNext( prev , from );
		NodeTraits::setPrev( where , to );
		NodeTraits::setNext( to   , where );
	}

	static void linkAfter( NodePtr const& from , NodePtr const& to , NodePtr const& where )
	{
		NodePtr next = NodeTraits::getNext( where );

		NodeTraits::setPrev( from , where );
		NodeTraits::setNext( where , from );
		NodeTraits::setPrev( next , to );
		NodeTraits::setNext( to , next );
	}

	static size_t count( ConstNodePtr const& from , ConstNodePtr const& end )
	{
		ConstNodePtr node = from;
		size_t result = 0;
		while ( node != end )
		{
			++result;
			node = NodeTraits::getNext( node );
		}
		return result;
	}

	static void unlink( NodePtr const& node )
	{
		NodePtr prev = NodeTraits::getPrev( node );
		NodePtr next = NodeTraits::getNext( node );
		NodeTraits::setNext( prev , next );
		NodeTraits::setPrev( next , prev );
		initNode( node );
	}
};



class LinkHook : public ListNode
{
public:
	typedef ListNodeTraits NodeTraits;
	typedef CycleListAlgorithm< NodeTraits > Algorithm;

	LinkHook() {  Algorithm::initNode( this );  }
	~LinkHook()
	{ 
		if ( isLinked() ) 
			unlink();  
	}
	bool      isLinked() const { return Algorithm::isLinked( this );  }
	void      unlink(){  Algorithm::unlink( this );  }

	template< class T , LinkHook T::*Member >
	static T& cast( LinkHook& node ){ return *FTypeCast::MemberToClass( &node , Member ); }
};

struct HookTraits
{
	class NodeType;
	class NodeTraits;
};

template< class Base , LinkHook Base::*Member >
struct MemberHook
{
	typedef ListNode NodeType;
	typedef LinkHook::NodeTraits NodeTraits;
	
	template< class T >
	static T& castValue( NodeType& node )
	{ 
		return static_cast< T& >( 
			LinkHook::cast< Base , Member >( static_cast< LinkHook& >( node ) ) 
		); 
	}
	template< class T >
	static NodeType& castNode( T& value ){ return  value.*Member; }
};

template< class T >
struct DefaultType
{
	typedef T  Type;
	typedef T& InType;
	typedef T& RType;

	static T&    fix( InType value ){ return value; }
	static RType fixRT( T& value )  { return value; }
};

template< class T >
struct PointerType
{
	typedef T* Type;
	typedef T* InType;
	typedef T* RType;
	static T&    fix( InType value ){ return *value; }
	static RType fixRT( T& value )  { return &value; }
};


template< class T , class HookTraits , template< class > class TypePolicy = DefaultType >
class TIntrList
{
	typedef typename HookTraits::NodeType   NodeType;
	typedef typename HookTraits::NodeTraits NodeTraits;
	typedef CycleListAlgorithm< NodeTraits > Algorithm;
	typedef typename NodeTraits::NodePtr NodePtr;
	typedef TypePolicy< T > TP;
	typedef typename TP::RType  RType;
	typedef typename TP::InType InType;
	typedef typename TP::Type   BaseType;
public:
	TIntrList()
	{
		Algorithm::initHeader( &mHeader );
	}

	~TIntrList()
	{
		clear();
	}

	RType  front(){ return TP::fixRT( HookTraits::template castValue< T >( *NodeTraits::getNext( &mHeader ) ) ); }
	RType  back() { return TP::fixRT( HookTraits::template castValue< T >( *NodeTraits::getPrev( &mHeader ) ) ); }

	bool   empty() const { return NodeTraits::getNext( &mHeader ) == &mHeader; }
	size_t size() const {  return Algorithm::count( NodeTraits::getNext( &mHeader ) , &mHeader );  }


	void  clear()
	{
		NodePtr node = NodeTraits::getNext( &mHeader );
		while( node != &mHeader )
		{
			NodePtr next = NodeTraits::getNext( node );
			Algorithm::initNode( node );
			node = next;
		}
		Algorithm::initHeader( &mHeader );
	}


	void   remove( InType value )
	{
		assert( haveLink( value ) );
		NodePtr node = &HookTraits::castNode( TP::fix( value ) );
		Algorithm::unlink( node );
	}


	void   push_front( InType value ){ insertAfter( TP::fix( value ) , mHeader ); }
	void   push_back( InType value ) { insertBefore( TP::fix( value ) , mHeader ); }

	void   insertBefore( InType value , InType where ){ insertBefore( TP::fix( value ) , HookTraits::castNode( TP::fix( where ) ) ); }
	void   insertAfter( InType value , InType where ) { insertAfter( TP::fix( value ) , HookTraits::castNode( TP::fix( where ) ) ); }
	void   moveBefore( InType where )
	{
		if ( empty() )
			return;

		NodePtr node = &HookTraits::castNode( TP::fix( where ) );
		assert( Algorithm::isLinked( node ) );
		Algorithm::linkBefore( 
			&HookTraits::castNode( TP::fix( front() ) ) ,
			&HookTraits::castNode( TP::fix( back() ) ) ,
			node );
		Algorithm::initHeader( &mHeader );
	}

	void   moveAfter( InType where )
	{
		if ( empty() )
			return;

		NodePtr node = &HookTraits::castNode( TP::fix( where ) );
		assert( Algorithm::isLinked( node ) );
		Algorithm::linkAfter( 
			&HookTraits::castNode( TP::fix( front() ) ) ,
			&HookTraits::castNode( TP::fix( back() ) ) ,
			node );
		Algorithm::initHeader( &mHeader );
	}

	bool   haveLink( InType value )
	{
		NodePtr node = &HookTraits::castNode( TP::fix( value ) );
		if ( Algorithm::isLinked( node ) == false )
			return false;

		NodePtr cur = NodeTraits::getNext( &mHeader );
		while ( cur != &mHeader )
		{
			if ( cur == node )
				return true;
			cur = NodeTraits::getNext( cur );
		}
		return false;
	}

	void swap(TIntrList& other)
	{
		Algorithm::swapHeader(&mHeader, &other.mHeader);
	}

private:
	

	template< class Node >
	class Iterator
	{
	public:
		typedef RType     reference;
		typedef BaseType* pointer;

		Iterator(NodePtr node ):mNode( node ){}

		Iterator  operator++(){ NodePtr node = mNode; mNode = NodeTraits::getNext( mNode ); return Iterator( node ); }
		Iterator  operator--(){ NodePtr node = mNode; mNode = NodeTraits::getPrev( mNode ); return Iterator( node ); }
		Iterator& operator++( int ){ mNode = NodeTraits::getNext( mNode ); return *this; }
		Iterator& operator--( int ){ mNode = NodeTraits::getPrev( mNode ); return *this; }

		reference operator*(){ return TP::fixRT( HookTraits::template castValue< T >( *mNode ) );  }
		pointer   operator->(){ return &TP::fixRT( HookTraits::template castValue< T >( *mNode ) ); }
		bool operator == ( Iterator other ) const { return mNode == other.mNode; }
		bool operator != ( Iterator other ) const { return mNode != other.mNode; }
	private:
		friend class TIntrList;
		NodePtr mNode;
	};

	template< class Node >
	class ReverseIterator
	{
	public:
		typedef RType     reference;
		typedef BaseType* pointer;

		ReverseIterator(NodePtr node) :mNode(node) {}

		ReverseIterator  operator--() { NodePtr node = mNode; mNode = NodeTraits::getNext(mNode); return ReverseIterator(node); }
		ReverseIterator  operator++() { NodePtr node = mNode; mNode = NodeTraits::getPrev(mNode); return ReverseIterator(node); }
		ReverseIterator& operator--(int) { mNode = NodeTraits::getNext(mNode); return *this; }
		ReverseIterator& operator++(int) { mNode = NodeTraits::getPrev(mNode); return *this; }

		reference operator*() { return TP::fixRT(HookTraits::template castValue< T >(*mNode)); }
		pointer   operator->() { return &TP::fixRT(HookTraits::template castValue< T >(*mNode)); }
		bool operator == (ReverseIterator other) const { return mNode == other.mNode; }
		bool operator != (ReverseIterator other) const { return mNode != other.mNode; }
	private:
		friend class TIntrList;
		NodePtr mNode;
	};
public:

	typedef Iterator< NodeType >       iterator;
	typedef ReverseIterator< NodeType >      reverse_iterator;
	iterator begin() { return iterator(NodeTraits::getNext(&mHeader)); }
	iterator end()    { return iterator(&mHeader); }
	reverse_iterator rbegin() { return reverse_iterator(NodeTraits::getPrev(&mHeader)); }
	reverse_iterator rend()   { return reverse_iterator(&mHeader); }

	iterator beginNode(InType value)
	{
		NodePtr node = &HookTraits::castNode(TP::fix(value));
		return iterator(node);
	}
	
	iterator erase( iterator iter )
	{
		NodePtr next = NodeTraits::getNext( iter.mNode );
		Algorithm::unlink( iter.mNode );
		return iterator( next );
	}

private:

	TIntrList( TIntrList const& );
	TIntrList& operator = ( TIntrList const& other );

	void insertBefore( T& value , NodeType& nodeWhere )
	{
		NodeType& nodeValue = HookTraits::castNode( value );
		assert( !Algorithm::isLinked( &nodeValue ) );
		assert( Algorithm::isLinked( &nodeWhere ) );
		Algorithm::linkBefore( &nodeValue , &nodeWhere );
	}
	void insertAfter( T& value , NodeType& nodeWhere )
	{
		NodeType& nodeValue = HookTraits::castNode( value );
		assert( !Algorithm::isLinked( &nodeValue ) );
		assert( Algorithm::isLinked( &nodeWhere ) );
		Algorithm::linkAfter( &nodeValue , &nodeWhere );
	}

	NodeType mHeader;
};



#endif // IntrList_h__
