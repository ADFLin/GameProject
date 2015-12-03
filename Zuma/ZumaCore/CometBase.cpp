#include "CometBase.h"

namespace Comet
{	
	void* ManagedObject::sLastAllocPtr = NULL;

	ManagedObject::ManagedObject()
	{
		mRefCount = ( sLastAllocPtr == this ) ? NonNewCount : 0;
	}

	void InitializerCollection::clearInitializer()
	{
		mInitializers.clear();
	}

	void InitializerCollection::initialize( Particle& particle )
	{
		for( InitializerList::iterator iter = mInitializers.begin();
			iter != mInitializers.end() ; ++iter )
		{
			(*iter)->initialize( particle );
		}
	}

	void InitializerCollection::addInitializerInternal( Initializer* initializer )
	{
		assert( initializer );
		mInitializers.push_back( initializer );
	}


	void ActionCollection::preUpdate( Emitter& e , TimeType time )
	{
		mActiveActions.clear();
		mActiveActions.reserve( mActions.size() );

		int num = 0;
		for( ActionList::iterator iter = mActions.begin();
			iter != mActions.end() ; ++iter )
		{
			Action* action = *iter;
			if ( action->isActive() )
			{
				mActiveActions[ num ] = action;
				++num;
			}
		}

		for( int i = 0 ; i < num ; ++i )
		{
			mActiveActions[i]->preUpdate( e , time );
		}
	}

	void ActionCollection::update( Emitter& e , Particle& p , TimeType time )
	{
		int num = mActiveActions.size();
		for( int i = 0 ; i < num ; ++i )
		{
			mActions[i]->update( e , p , time );
		}
	}

	void ActionCollection::postUpdate( Emitter& e , TimeType time )
	{
		int num = mActiveActions.size();
		for( int i = 0 ; i < num ; ++i )
		{
			mActiveActions[i]->postUpdate( e , time );
		}
	}

}
