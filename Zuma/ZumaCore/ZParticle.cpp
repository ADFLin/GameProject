#include "ZumaPCH.h"
#include "ZParticle.h"

#include "IRenderSystem.h"

namespace Zuma
{

#if 0

	static gBallPSLifeTime;

	BallPS::BallPS()
		:Emitter( new MyParticleFactoy( 400 ) , new Comet::ConstClock( 4 ) )
		,mPosZone( )
	{

		using namespace Comet;

		mPosZone.setPos( CoordType( 200 , 200 ) );

		addInitializer( new Position( &mPosZone )  );
		addInitializer( new Life( gBallPSLifeTime , gBallPSLifeTime ) );
		addInitializer( new Velocity( new SphereZone( 0 , 0.001 ) ) );
		addInitializer( &mInitData );


		alpha = 150;
		angle = 0;

		fadeInTime = 0.1f * gBallPSLifeTime;
		fadeOutTime = 0.7f * gBallPSLifeTime;

		frame = 0;
	}

	BallPS::~BallPS()
	{
		delete mEmitter;
	}

	void BallPS::onUpdate( long time )
	{
		angle += 0.1 * time;
	}

	void BallPS::onRespawn( Particle& p )
	{
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

		Vec2D dir = p.getPos() - mPosZone.center;
		float len = sqrt( dir.length2() );

		p.setVelocity(  0.0001  * dir  );
	}

	void BallPS::updateParticle( Particle& p , long time )
	{
		MyParticle& mp = p.cast< MyParticle >();

		if ( p.getLifeTime() < fadeInTime )
			mp.color.a = alpha * ( 1 - ( fadeInTime - p.getLifeTime() ) / (fadeInTime ) ) ;
		else if ( p.getLifeTime() > fadeOutTime )
			mp.color.a = alpha * ( 1 - ( p.getLifeTime() - fadeOutTime ) / ( mLifeTime - fadeOutTime ) );

		mp.scale =  0.3f + 0.7f * p.getLifeTime()  / getLifeTime();
	}

	void BallPS::prevRender(IRenderSystem& RDSystem )
	{
		//glEnable( GL_BLEND );
	}

	void BallPS::postRender( IRenderSystem& RDSystem )
	{
		//glDisable( GL_BLEND );
	}


	SamplePS::SamplePS() 
		:Emitter( 100 , new RectEmitter( Vec2D(-2,-2) , Vec2D(2,2) ) , 1000 , 30 / 1000.0 )
	{
		mSpawnNum = 0;
		getEmitter().setPos( Vec2D(200,200) );

		addInitializer( new Angle( 0 , 3.1415 ) );
		addInitializer( new Life( 0 , 10 ) );
		addInitializer( new Velocity( new SphereZone( 0 , Real(0.1) ) ) );
		addInitializer( new Omega( new ))

			alpha = 150;
		angle = 0;
	}

	void SamplePS::respawnParticle( Particle& p )
	{
		angle += 0.1f;

		MyParticle& mp = p.cast< MyParticle >();

		mp.color.r = rand() % 256;
		mp.color.g = rand() % 256;
		mp.color.b = rand() % 256;
		mp.color.a = 0;

		mp.angle = Random( 0 , 3.1415 );
		mp.rotateSpeed = Random( 0.05 , 0.2 );
		mp.setVelocity(  0.1 * Vec2D( cos(angle) , sin(angle) )   );
	}

	void SamplePS::updateParticle( Particle& p , long time )
	{
		MyParticle& mp = p.cast< MyParticle >();

		float fadeInTime = 0.2f * mLifeTime;
		float fadeOutTime = 0.8f * mLifeTime;

		if ( p.getLifeTime() < fadeInTime )
			mp.color.a = alpha * ( 1 - ( fadeInTime - p.getLifeTime() ) / (fadeInTime ) ) ;
		else if ( p.getLifeTime() > fadeOutTime )
			mp.color.a = alpha * ( 1 - ( p.getLifeTime() - fadeOutTime ) / ( mLifeTime - fadeOutTime ) );

		mp.angle += mp.rotateSpeed * time;
	}


	void SamplePS::prevRender()
	{
		getRenderSystem().setBlendFun( BLEND_SRCALPHA , BLEND_ONE );
	}

	void SamplePS::postRender()
	{

	}

	void BallPS::renderParticle( Particle& p )
	{
		IRenderSystem& RDSystem = getRenderSystem();

		MyParticle& mp = p.cast< MyParticle >();

		Vec2D const& pos = p.getPos();

		RDSystem.setWorldIdentity();
		RDSystem.translateWorld( pos );
		RDSystem.rotateWorld( angle );
		RDSystem.scaleWorld( mp.scale , mp.scale );

		RDSystem.setColor( mp.color );

		RDSystem.setBlendFun( BLEND_SRCALPHA , BLEND_ONE );
		RDSystem.drawBitmap( *tex , 
			Vec2D(0 , tex->getWidth() * mp.frame ) , 
			Vec2D(tex->getWidth() , tex->getWidth() ) , 
			TBF_OFFSET_CENTER | TBF_USE_BLEND_PARAM
			);

		RDSystem.setColor( 255 , 255 , 255 , 255 );

		//glBegin( GL_QUADS );
		//	glVertex3f( -1  , -1 , 0 );  
		//	glVertex3f(  1  , -1 , 0 );
		//	glVertex3f(  1 ,   1 , 0 );
		//	glVertex3f( -1 ,   1 , 0 );
		//glEnd();
	}

	void SamplePS::renderParticle( Particle& p )
	{
		Data* data = p.getUserData<Data>();

		Vec2D const& pos = p.getPos();

		RDSystem.setWorldIdentity();
		RDSystem.translateWorld( pos );
		RDSystem.rotateWorld( data->angle );
		RDSystem.scaleWorld( 2 , 2 );

		RDSystem.setColor( data->color );

		//glBegin( GL_QUADS );
		//glVertex3f( -1  , -1 , 0 );  
		//glVertex3f(  1  , -1 , 0 );
		//glVertex3f(  1 ,   1 , 0 );
		//glVertex3f( -1 ,   1 , 0 );
		//glEnd();

		RDSystem.setColor( 255 , 255 , 255 , 255 );
	}

	ZStarPS::ZStarPS( IRenderSystem& RDSystem ) 
		:ZTexturePSBase( 100 , *new SphereEmitter( 0 , 50 ) , 2000 , 100 / 1000.0 )
	{
		tex = RDSystem.getTexture( IMAGE_SPARKLE );

		fadeInFactor  = 0.5f;
		fadeOutFactor = 0.5f;
		baseColor.value = 0xffffffff;

		speed = 0.1f;

	}

	ZStarPS::~ZStarPS()
	{
		delete &getEmitter();
	}

	void ZStarPS::onRender( Particle& p , IRenderSystem& RDSystem )
	{
		MyParticle& mp = p.cast< MyParticle >();

		StarData* data = p.getUserData<StarData>();

		Vec2D const& pos = p.getPos();

		RDSystem.setWorldIdentity();
		RDSystem.translateWorld( pos );

		int frame = tex->col * p.getLifeTime() / mLifeTime;

		RDSystem.setColor( data->color );

		int w = tex->getWidth() / tex->col;
		RDSystem.drawBitmap( *tex , 
			Vec2D( w * frame , 0 ) , 
			Vec2D( w  , tex->getHeight() ) , 
			TBF_ENABLE_ALPHA_BLEND | TBF_OFFSET_CENTER | TBF_USE_BLEND_PARAM
			);
		RDSystem.setColor( 255 , 255 , 255 , 255 );
	}

	void ZStarPS::onPrevRender( IRenderSystem& RDSystem )
	{
		RDSystem.setBlendFun( BLEND_ONE , BLEND_ONE );
	}

	void ZStarPS::respawnParticle( Particle& p )
	{
		MyParticle& mp = p.cast< MyParticle >();

		mp.color.r = 128 + rand() % 128;
		mp.color.g = 128 + rand() % 128;
		mp.color.b = 128 + rand() % 128;
		mp.color.a = baseColor.a;

		Vec2D dir = p.getPos() - getEmitter().getPos();
		p.setVelocity( ( speed / sqrt( dir.length2()) ) * dir );
	}

	void ZStarPS::updateParticle( Particle& p , unsigned time )
	{
		ZTexturePSBase::updateParticle( p , time );
	}

	void ZTexturePSBase::updateParticle( Particle& p , unsigned time )
	{
		MyParticle& mp = p.cast< MyParticle >();

		float factor = float( mp.getLifeTime() ) / mLifeTime; 

		if ( factor < fadeInFactor )
			mp.color.a = baseColor.a * ( factor / fadeInFactor ) ;
		else if ( factor > fadeOutFactor )
			mp.color.a = baseColor.a * ( ( 1.0f - factor ) / ( 1.0f - fadeOutFactor ) );
	}

	void ZPSystemBase::render()
	{
		struct RenderVisitor : public Comet::Visitor
		{
			void visit( Particle& p ){ ps->render( p ); }
			ZPSystemBase* ps;
		} visitor;
		visitor.ps = this;
		preRender();
		visitParticle( visitor );
		postRender();
	}

#endif


}//namespace Zuma