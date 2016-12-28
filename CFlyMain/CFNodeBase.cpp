#include "CFlyPCH.h"
#include "CFNodeBase.h"

namespace CFly
{

	NodeBase::NodeBase( NodeManager* manager )
	{
		init();
		manager->addNode( this );
	}


	NodeBase::NodeBase( NodeBase const& node ) 
		:mFlag( node.mFlag )
		,mName( node.mName )
		,mManager( node.mManager )
		,mParent( nullptr )
	{
		setParent( node.mParent );
	}

	NodeBase::~NodeBase()
	{
		if ( mManager )
			mManager->removeNode( this );
	}

	bool NodeBase::registerName( char const* name )
	{
		return mManager->registerName( this , name );
	}

	void NodeBase::init()
	{
		mFlag      = 0;
		mParent    = nullptr;
		mManager   = nullptr;
	}

	void NodeBase::setParent( NodeBase* parent )
	{
		if ( mParent )
			mParent->mChildren.remove( this );

		mParent = parent;

		if ( mParent )
			mParent->mChildren.push_back( this );
	}

	void NodeBase::enableFlag( unsigned flag ,bool beE )
	{
		if ( beE )
			addFlag( flag );
		else 
			removeFlag( flag );
	}

	void NodeManager::unregisterName( NodeBase* node )
	{
		if ( node->mName == EName::None )
			return;

		NodeNameMap::iterator iter = mNameMap.find( node->mName );
		if ( iter == mNameMap.end() )
			return;

		mNameMap.erase( iter );
		node->mName = EName::None;
	}

	bool NodeManager::registerName( NodeBase* node , char const* name )
	{
		if ( node->mName != EName::None )
			unregisterName( node );

		node->mName = name;
		std::pair< NodeNameMap::iterator, bool> result = mNameMap.insert( 
			std::make_pair(node->mName, node ) );

		return result.second;
	}

	NodeBase* NodeManager::findNodeByName( char const* name )
	{
		NodeNameMap::iterator iter = mNameMap.find( name );
		if ( iter == mNameMap.end() )
			return nullptr;

		return iter->second;
	}

	void NodeManager::removeNode( NodeBase* node )
	{
		assert( node->mManager == this );
		node->mManager = NULL;
	}

	void NodeManager::addNode( NodeBase* node )
	{
		assert( node->mManager == NULL );
		node->mManager = this;
	}

}//namespace CFly