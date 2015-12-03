#ifndef CometParticle_h__
#define CometParticle_h__

#include "CometBase.h"

namespace Comet 
{

	enum ParamType
	{
		PT_INT   ,
		PT_REAL  ,
		PT_CHAR  ,
		PT_WORD  ,
		PT_POINTER ,
	};

	struct ParticleParamInfo
	{
		int index;
		int type;
	};


	class DataAccess
	{
	public:
		Real&   getReal( int index ){ return getDataT< Real >( index ); }
		int&    getInt ( int index ){ return getDataT< int >( index ); }
		void*&  getPtr ( int index ){ return getDataT< void* >( index ); }
		uint32& getWord( int index ){ return getDataT< uint32 >( index ); }
		

	private:
		template< class T >
		T&  getDataT( int index )
		{
			return *reinterpret_cast< T* >( mData + mDataOffset[index] );
		}
		char* mData;
		int*  mDataOffset;
	};

	class ParticleRegister;
	class ParticleComponent
	{
	public:
		virtual void update( Particle& p , DataAccess& compData , TimeType time ) = 0;
		virtual void registerData( ParticleRegister& reg ) = 0;

	private:
		friend class ParticleRegister;
	};



	class Particle
	{
	public:
		Particle();
		~Particle();

		CoordType const& getVelocity() const { return mVelocity; }
		void             setVelocity( CoordType const& val) { mVelocity = val; }
		CoordType const& getPos(){ return mPos; }
		void             shiftPos( CoordType const& val ){ mPos += val; }
		void             setPos( CoordType const& val ){ mPos = val; }

		void             setDead( bool beD ){ ( beD ) ? addFlag( eDead ) : removeFlag( eDead ); }
		bool             isDead(){ return checkFlag( eDead ); }

		unsigned         getActMask() const { return mActMask; }
		void             setActMask( unsigned mask ){ mActMask = mask; }

		void             initLifeTime( TimeType val ) { mInitLifeTime = mLifeTime = val;  }
		TimeType         getInitLifeTime() const { return mInitLifeTime;  }

		TimeType         getLifeTime() const { return mLifeTime; }
		void             setLifeTime( TimeType val ) {  mLifeTime = val;  }

		template< class T >
		T&               cast(){ return static_cast< T& >( this ); }

		DataAccess       getDataAccess( ParticleComponent* comp );
		DataAccess       getDataAccess( char const* name );

	private:

		enum
		{
			eDead    = 1 << 0,
			eDestroy = 1 << 1,
		};

		struct ClintDataInfo
		{
			void* data;
			bool  recycle;
		};


		CoordType mPos;
		CoordType mVelocity;
		TimeType  mLifeTime;
		TimeType  mInitLifeTime;
		Real      mAlapha;
		unsigned  mActMask;
		unsigned  mFlag;

		void         addFlag   ( unsigned flag ){ mFlag |= flag; }
		void         removeFlag( unsigned flag ){ mFlag &= ~flag; }
		bool         checkFlag ( unsigned flag ){ return ( mFlag & flag ) != 0; }

		friend class Emitter;
	};


	class ParticleRegister
	{
	public:
		virtual void registerParamData( ParticleComponent* comp , ParticleParamInfo info[] , int num ) = 0;
	};



}//namespace Comet 

#endif // CometParticle_h__
