#ifndef ZParticle_h__
#define ZParticle_h__

#include "ZBase.h"
#include "Comet.h"
#include "IRenderSystem.h"

namespace Zuma
{

	class IParticleSystem
	{
	public:
		virtual void update( long time ) = 0;
		virtual void render() = 0;
	};

#if 0
using Comet::Particle;
using Comet::TimeType;

class ZPSystemBase : public Comet::ParticleEventListener
{
public:
	void render();

protected:
	virtual void preRender(){}
	virtual void renderParticle( Particle& p ) = 0;
	virtual void postRender(){}

	IRenderSystem& getRenderSystem();
	Comet::Emitter mEmitter;
};

class ZTexturePSBase : public ZPSystemBase
{
public:
	class MyParticle : public Comet::Particle
	{
	public:
		ColorKey color;
	};
	ZTexturePSBase( int num , Emitter* emitter , unsigned lifeTime , float spawnSpeed)
		:Emitter( num , emitter , lifeTime , spawnSpeed )
	{

	}

	void        updateParticle( Particle& p ,  TimeType time );
	float       fadeInFactor;
	float       fadeOutFactor;
	ITexture2D* tex;
	ColorKey    baseColor;
};

class ZStarPS : public ZTexturePSBase
{
public:
	ZStarPS( IRenderSystem& RDSystem );
	~ZStarPS();

	class MyParticle : public ZTexturePSBase::MyParticle
	{
	public:

	};



	float speed;


	Particle* createParticle( int num ){  return new MyParticle[ num ];  }
	void      destroyParticle( Particle* p , int num ){ delete [] static_cast< MyParticle* >( p ); }

	virtual  void  respawnParticle( Particle& p );
	virtual  void  updateParticle( Particle& p , long time );
	virtual  void  killParticle( Particle& p ){}
	virtual  void  onPrevRender( IRenderSystem& RDSystem );
	virtual  void  onPostRender( IRenderSystem& RDSystem )
	{

	}
	virtual  void  onRender( Particle& p , IRenderSystem& RDSystem );

};

//
//class BallComponent
//{
//	enum
//	{
//		DATA_FRAME ,
//		DATA_ANGLE ,
//		DATA_SCALE ,
//	};
//	void registerData( ParticleRegister& reg )
//	{
//		ParticleParamInfo info[] = 
//		{
//			{ DATA_FRAME , PT_INT } ,
//			{ DATA_ANGLE , PT_REAL } ,
//			{ DATA_SCALE , PT_REAL } ,
//		}
//	}
//	void update( Particle& p , DataAccess& compData , TimeType time )
//	{
//		compData.getRealData( DATA_ANGLE ) += 0.1f * time ;
//	}
//};



class BallPS : public Comet::Emitter
{
public:
	BallPS();
	~BallPS();

	Comet::SingleZone mPosZone;
	MyInitializer     mInitData;

	Vec2D    emitDir;
	ColorKey baseColor;
	float    angle;
	int      frame;
	float    fadeInTime;
	float    fadeOutTime;

	ITexture2D*  tex;

	unsigned alpha;

	class MyParticle : public Comet::Particle
	{
	public:
		int      frame;
		float    angle;
		float    scale;
		ColorKey color;
	};

	typedef Comet::ArrayParticleFactory< MyParticle > MyParticleFactoy;

	class MyInitializer : public Comet::Initializer
	{
	public:

		void initialize( Particle& p )
		{
			MyParticle& mp = static_cast< MyParticle >( p );

			angle += 0.1;

			MyParticle& mp = p.cast< MyParticle >();

			float factor = Random( 1.5f , 2.0f ); 

			mp.color.r = 255 - baseColor.r * factor ;
			mp.color.g = 255 - baseColor.g * factor ;
			mp.color.b = 255 - baseColor.b * factor ;

			mp.color.a = 255;

			mp.angle = Random( 0 , 360 );

			mp.scale = 0.3f;
			mp.frame = ( rand() )% 13;

			float theta = Random( 0 , 2 * PI );

			Vec2D dir = p.getPos() - pos;
			float len = sqrt( dir.length2() );

			p.setVelocity(  0.0001  * dir  );
		}

		Vec2D    emitDir;
		ColorKey baseColor;
		float    angle;
		int      frame;
		Vec2D    pos;
	};


	virtual  void  onUpdate( TimeType time );

	virtual  void  respawnParticle( Particle& p );
	virtual  void  updateParticle( Particle& p , TimeType time );
	virtual  void  killParticle( Particle& p ){}

	virtual  void  prevRender();
	virtual  void  postRender();
	virtual  void  renderParticle( Particle& p );
};


class SamplePS : public ZPSystemBase
{
public:

	class MyParticle : public Comet::Particle
	{
	public:
		float    rotateSpeed;
		float    angle;
		ColorKey color;
	};

	SamplePS();

	float    angle;
	Vec2D    emitDir;
	unsigned alpha;

	float     fadeInTime;
	float     fadeOutTime;



	virtual    void  doUpdateSystem( long time )
	{
		angle += 0.1;
	}
	virtual   void  respawnParticle( Particle& p );
	virtual   void  updateParticle( Particle& p , long time );
	virtual   void  killParticle( Particle& p ){}


	virtual  void  onPrevRender(IRenderSystem& RDSystem );
	virtual  void  onPostRender(IRenderSystem& RDSystem );

	void   onRender( Particle& p , IRenderSystem& RDSystem );
};
#endif

}//namespace Zuma

#endif // ZParticle_h__
