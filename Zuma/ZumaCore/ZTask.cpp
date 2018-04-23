#include "ZumaPCH.h"
#include "ZTask.h"

#include "ZLevel.h"
#include "ZBallConGroup.h"
#include "ZPath.h"

#include "ResID.h"
#include "AudioPlayer.h"
#include "ZStage.h"
#include "ZLevelManager.h"

#include <functional>

namespace Zuma
{
	ShowTextTask::ShowTextTask( std::string const& _str , ResID _fontType , unsigned time )
		:BaseClass( time )
		,str(_str )
		,fontType(_fontType )
	{
		setRenderOrder( RO_UI );
	}

	void ShowTextTask::onStart()
	{
		Global::getRenderSystem().addRenderable( this );
	}

	bool ShowTextTask::onUpdate( long time )
	{
		pos += speed * time;
		return BaseClass::onUpdate( time );
	}

	void ShowTextTask::onEnd( bool beComplete )
	{
		Global::getRenderSystem().removeObj( this );
	}


}//namespace Zuma

