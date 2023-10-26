#include "WeaponRenderer.h"

#include "RenderSystem.h"
#include "TextureManager.h"
#include "Texture.h"

#include "Minigun.h"
#include "Laser.h"
#include "Plasma.h"

#include <algorithm>
#include "RenderUtility.h"

void WeaponRenderer::render( RenderPass pass ,Weapon* weapon, PrimitiveDrawer& drawer)
{
	float factor = std::min( 1.0f , weapon->mFireTimer / weapon->mCDTime );

	Vec2f rPos = weapon->getPos() - weapon->mSize / 2;

	float off;
	float len = 20;
	if ( factor < 0.5 )
	{
		off = len * factor;
	}
	else
	{
		off = len * ( 1 - factor );
	}

	if (pass == RP_BASE_PASS)
	{
		PrimitiveMat mat;
		mat.baseTex = mTextues[TG_DIFFUSE];
		mat.normalTex = mTextues[TG_NORMAL];
		drawer.setMaterial(mat);

		drawer.getStack().push();
		drawer.getStack().translate(rPos + Vec2f(0, off));
		drawer.drawRect(Vec2f::Zero(), weapon->mSize);
		drawer.getStack().pop();
	}
	else
	{
		if (mTextues[TG_GLOW])
		{
			drawer.setGlow(mTextues[TG_GLOW]);
			drawer.getStack().push();
			drawer.getStack().translate(rPos + Vec2f(0, off));
			drawer.drawRect(Vec2f::Zero(), weapon->mSize);
			drawer.getStack().pop();
		}
	}
}


class LaserRenderer : public WeaponRenderer
{
public:
	void init()
	{
		TextureManager* texMgr = getRenderSystem()->getTextureMgr();
		mTextues[ TG_DIFFUSE ] = texMgr->getTexture("weapon1.tga");
		mTextues[ TG_NORMAL ]  = texMgr->getTexture("weapon1Normal.tga");
		mTextues[ TG_GLOW ]    = texMgr->getTexture("oruzje1Glow.tga");
	}
};


class MinigunRenderer : public WeaponRenderer
{
public:
	void init()
	{
		TextureManager* texMgr = getRenderSystem()->getTextureMgr();
		mTextues[ TG_DIFFUSE ] = texMgr->getTexture("weapon1.tga");
		mTextues[ TG_NORMAL ]  = texMgr->getTexture("weapon1Normal.tga");
		mTextues[ TG_GLOW ]    = texMgr->getTexture("oruzje3Glow.tga");
	}
};

class PlasmaRenderer : public WeaponRenderer
{
public:
	void init()
	{
		TextureManager* texMgr = getRenderSystem()->getTextureMgr();
		mTextues[ TG_DIFFUSE ] = texMgr->getTexture("weapon1.tga");
		mTextues[ TG_NORMAL ]  = texMgr->getTexture("weapon1Normal.tga");
		mTextues[ TG_GLOW ]    = texMgr->getTexture("oruzje2Glow.tga");
	}
};


DEF_WEAPON_RENDERER( Laser , LaserRenderer )
DEF_WEAPON_RENDERER( Minigun , MinigunRenderer )
DEF_WEAPON_RENDERER( Plasma , PlasmaRenderer )