#ifndef CometEmiter_h__
#define CometEmiter_h__

#include "CometBase.h"

namespace Comet 
{
	class Particle;

	class ParticleStorage : public Object
	{
	public:
		virtual ~ParticleStorage(){}
	};


	class ParticleVisitor
	{
	public:
		virtual void visit( Particle& p ) = 0;
	};

	class ParticleFactory : public ManagedObject
	{
	public:
		virtual ~ParticleFactory(){}
		virtual ParticleStorage* createStroge() = 0;
		virtual void destroyStroge( ParticleStorage* storage ) = 0;
		virtual void update( Emitter& e , ParticleStorage& storage , TimeType time ) = 0;
		virtual void visit ( Emitter& e , ParticleStorage& storage , ParticleVisitor& visitor ) = 0;
	};


	template< class T >
	class ArrayParticleFactory : public ParticleFactory
	{
		ArrayParticleFactory( int maxNum )
			:maxNum( maxNum ){}
		typedef T PType;

		class Storage : public ParticleStorage
		{
		private:
			int mNum;
			T*  mData;
			friend class ArrayParticleFactory;
		};
		int maxNum;
	public:
		virtual ParticleStorage* createStroge()
		{
			return new Storage( maxNum );
		}

		virtual void destroyStroge( ParticleStorage* storage )
		{
			delete storage;
		}

		virtual void update( Emitter& e , ParticleStorage& storage , TimeType time )
		{
			Storage& myStorge = static_cast< ParticleStorage& >( storage );
			for ( int i = 0 ; i < myStorge.mNum ; ++i )
			{
				Particle& p = myStorge.mData[i];
				e.doUpdate( p , time );
			}
		}

		virtual void visit( Emitter& e ,  ParticleStorage& storage , ParticleVisitor& visitor )
		{
			Storage& myStorge = static_cast< ParticleStorage& >( storage );
			for ( int i = 0 ; i < myStorge.mNum ; ++i )
			{
				Particle& p = myStorge.mData[i];
				if ( !p.isDead() )
					visitor.visit( p );
			}
		}

	};



	class ParticleEventListener
	{
	public:
		virtual void  preUpdate( TimeType time ) = 0;
		virtual void  updateParticle ( Particle& p , TimeType time ){}
		virtual void  respawnParticle( Particle& p ){}
		virtual void  killParticle   ( Particle& p ){}
	};


	class Emitter : public ActionCollection 
		          , public InitializerCollection
	{
	public:
		Emitter( ParticleFactory* factory , Clock* clock )
			:mFactory( factory )
			,mClock( clock )
		{
			mStorage = factory->createStroge();
		}

		void       run( TimeType time );
		bool       isEnable() const { return mEnable; }
		void       enable(bool val) { mEnable = val; }
		void       visitParticle( ParticleVisitor& visitor ){  mFactory->visit( *this , *mStorage , visitor ); }
		void       doUpdate( Particle& p , TimeType time );

	protected:
		int                         mNumRespawn;

		ParticleStorage*            mStorage;
		SharePtr< Clock >           mClock;
		SharePtr< ParticleFactory > mFactory;
		ParticleEventListener*      mEventListener;
		bool                        mEnable;
		
	};


}//namespace Comet


#endif // CometEmiter_h__
