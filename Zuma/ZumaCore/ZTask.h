#ifndef _ZTask_H_
#define _ZTask_H_

#include "ZBase.h"

#include "TaskBase.h"
#include "IRenderSystem.h"

#include "ZParticle.h"

namespace Zuma
{
	class ZLevel;
	class ZPath;
	class ZFont;
	enum  ResID;


	class PSRunTask : public LifeTimeTask
		            , public IRenderable
	{
		typedef LifeTimeTask BaseClass;
	public:
		PSRunTask( IRenderSystem& renderSys , IParticleSystem* ps , long time )
			:BaseClass( time ) , mRenderSys( renderSys ), mParticleSys( ps )
		{

		}
		~PSRunTask(){ delete mParticleSys; }
		void   release(){ delete this; }
		virtual void onStart()
		{
			setOrder( 3 );
			mRenderSys.addRenderable( this );
		}
		virtual bool onUpdate( long time )
		{
			mParticleSys->update( time );
			return true;
		}
		virtual void onEnd( bool beComplete )
		{
			mRenderSys.removeObj( this );
		}

		virtual void onRender( IRenderSystem& RDSystem )
		{
			mParticleSys->render();
		}

		IRenderSystem&   mRenderSys;
		IParticleSystem* mParticleSys;
	};




	class ShowTextTask : public LifeTimeTask
		               , public IRenderable
	{
		typedef LifeTimeTask BaseClass;
	public:
		ShowTextTask( std::string const& _str , ResID _fontType , unsigned time );
		void   release(){ delete this; }
		void  onStart();
		void  onRender( IRenderSystem& RDSystem );
		bool  onUpdate( long time );
		void  onEnd( bool beComplete );


		ColorKey       color;
		Vector2          pos;
		Vector2          speed;
		std::string    str;
		ResID          fontType;

	};

}//namespace Zuma

#endif
