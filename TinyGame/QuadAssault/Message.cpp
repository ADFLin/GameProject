#include "Message.h"

#include "Level.h"
#include "TextureManager.h"
#include "SoundManager.h"

#include "RenderSystem.h"
#include "GameInterface.h"
#include "Texture.h"

#include "RHI/RHIGraphics2D.h"

Message::~Message()
{

}

void Message::init( String const& sender, String const& content, float duration, String const& soundName )
{	
	mPos.x=64;
	mPos.y=64;
	mSoundName = soundName;
	mDuration  = duration;
	timer=0.0;
	needDestroy = false;

	p_text.reset( IText::Create( getGame()->getFont(0) , 24 , Color4ub(25,255,25) ) );
	p_text->setString( sender.c_str() );

	text.reset( IText::Create( getGame()->getFont(0) , 24 , Color4ub(255,255,255) ) );
	text->setString( content.c_str() );

	portrait = getRenderSystem()->getTextureMgr()->getTexture("portrait2.tga");	
}

void Message::nodifyShow()
{
	sound = getGame()->getSoundMgr()->addSound( mSoundName.c_str() );
	if ( sound )
		sound->play();
}

void Message::tick()
{
	if( timer < mDuration )
	{
		timer += TICK_TIME;
	}
	else
	{
		if( sound )
			sound->stop();
		needDestroy=true;		
		sound.release();
	}
}
void Message::updateRender( float dt )
{

}

void Message::renderFrame(RHIGraphics2D& g)
{
	using namespace Render;
	//float width =text.getLocalBounds().width;
	float width = 600;

	g.setBrush(Color3f(0.0, 0.25, 0.0));
	g.enablePen(false);
	g.beginBlend(1, ESimpleBlendMode::Add);
	g.drawRect(mPos + Vec2f(32,0) , Vec2f(width+48 ,64));
	g.endBlend();
	g.enablePen(true);

	g.setBrush(Color3f::White());
	g.drawTexture(*portrait->resource, mPos - Vec2f(32, 0), Vec2f(64, 64));

	g.enableBrush(false);
	g.setPen(Color3f(0.1, 1.0, 0.1));
	g.drawRect(mPos - Vec2f(32, 0), Vec2f(width + 112, 64));
	g.enableBrush(true);

}

void Message::render(RHIGraphics2D& g)
{	
	renderFrame(g);
	getRenderSystem()->drawText(p_text , mPos + Vec2i( 48 , 4 ) , TEXT_SIDE_LEFT | TEXT_SIDE_TOP );
	getRenderSystem()->drawText(text , mPos + Vec2i( 48 , 4 + 24 ) , TEXT_SIDE_LEFT | TEXT_SIDE_TOP );
}
