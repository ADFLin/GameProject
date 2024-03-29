#ifndef Message_h__
#define Message_h__

#include "Base.h"
#include "RenderSystem.h"

class Level;
class Sound;
class Texture;
class IText;
class RHIGraphics2D;

class Message
{

public:
	~Message();
	void init( String const& sender, String const& content, float durstion, String const& soundName );
	void nodifyShow();
	void tick();
	void updateRender( float dt );


	void renderFrame(RHIGraphics2D& g);
	void render(RHIGraphics2D& g);
	bool needDestroy;
private:	
	FObjectPtr< IText >  p_text;
	FObjectPtr< IText >  text;
	Vec2f   mPos;
	float   mDuration;
	float    timer;	
	Texture* portrait;
	String   mSoundName;	
	SoundPtr  sound;

};


#endif // Message_h__