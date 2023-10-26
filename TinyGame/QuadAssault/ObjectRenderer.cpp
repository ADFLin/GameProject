#include "ObjectRenderer.h"

#include "RenderSystem.h"
#include "TextureManager.h"
#include "RenderUtility.h"
#include "Texture.h"

#include "Player.h"
#include "Mob.h"
#include "LaserMob.h"
#include "MinigunMob.h"
#include "PlasmaMob.h"

#include "LaserBullet.h"
#include "MinigunBullet.h"
#include "PlasmaBullet.h"

#include "SmokeParticle.h"
#include "DebrisParticle.h"

#include "KeyPickup.h"
#include "WeaponPickup.h"
#include "DebrisPickup.h"


IObjectRenderer::IObjectRenderer()
{
	mRenderOrder = 0;
}

void IObjectRenderer::renderGroup(PrimitiveDrawer& drawer, RenderPass pass , int numObj, LevelObject* object)
{
	for( ; object ; object = NextObject( object ) )
	{
		render(drawer, pass , object);
	} 
}

class PlayerRenderer : public IObjectRenderer
{
public:
	PlayerRenderer()
	{
		mRenderOrder = 2;
	}
	virtual void init()
	{
		TextureManager* texMgr = getRenderSystem()->getTextureMgr();
		textura = texMgr->getTexture("tenkTorzoDiffuse.tga");	
		texturaN = texMgr->getTexture("tenkTorzoNormal.tga");

		podloga_tex = texMgr->getTexture("tenkPodlogaDiffuse.tga");
		podloga_normal = texMgr->getTexture("tenkPodlogaNormal.tga");

		tracnica_tex = texMgr->getTexture("tracnicaDiffuse.tga");	
		tracnica_normal = texMgr->getTexture("tracnicaNormal.tga");
	}

	virtual void render(PrimitiveDrawer& drawer, RenderPass pass , LevelObject* object)
	{
		Player* player = object->cast< Player >();

		if( player->mIsDead )
			return;

		if (pass == RP_BASE_PASS)
		{
			{
				//Shadow
				drawer.beginTranslucent(0.5);
				drawer.getStack().push();
				drawer.getStack().translate(Vec2f(4, 4));

				Color3f color = Color3f(0, 0, 0);
				RenderPodlogu(drawer, color, player);
				renderTrack(drawer, color, player);
				renderTorso(drawer, color, player);

				drawer.getStack().pop();
				drawer.endTranslucent();
			}
			{
				Color3f color = Color3f(1.0, 1.0, 1.0);
				RenderPodlogu(drawer, color, player);
				renderTrack(drawer, color, player);
				renderTorso(drawer, color, player);
			}
		}


		for(int i=0; i<4; ++i)
		{
			Weapon* weapon = player->mWeaponSlot[i];
			if( weapon )
			{
				Vec2f centerPos = player->getPos();
				drawer.getStack().push();
				drawer.getStack().translate(centerPos);
				drawer.getStack().rotate(player->rotationAim + Math::DegToRad(90));
				weapon->render( pass, drawer );
				drawer.getStack().pop();
			}
		}

	}

	void RenderPodlogu(PrimitiveDrawer& drawer, Color3f const& color, Player* player)
	{
		PrimitiveMat mat;
		mat.baseTex = podloga_tex;
		mat.normalTex = podloga_normal;
		mat.color = color;
		drawer.setMaterial(mat);
		drawer.drawRect( player->getRenderPos() , player->getSize() , player->getRotation() );	
	}

	void renderTorso(PrimitiveDrawer& drawer, Color3f const& color, Player* player)
	{
		PrimitiveMat mat;
		mat.baseTex = textura;
		mat.normalTex = texturaN;
		mat.color = color;
		drawer.setMaterial(mat);
		drawer.drawRect( player->getRenderPos() , player->getSize(), player->rotationAim + Math::DegToRad(90) );
	}

	void renderTrack(PrimitiveDrawer& drawer, Color3f const& color, Player* player)
	{
		float razmak_tracnica=8;
		float odmak=24;

		PrimitiveMat mat;
		mat.baseTex = tracnica_tex;
		mat.normalTex = tracnica_normal;
		mat.color = color;
		drawer.setMaterial(mat);

		Vec2f centerPos = player->getPos();
		float shift = player->shiftTrack;
		Vec2f size = player->getSize();

		drawer.getStack().push();
		drawer.getStack().translate(centerPos);
		drawer.getStack().rotate(player->getRotation());
		drawer.getStack().translate(Vec2f(-odmak - razmak_tracnica, -size.y / 2));
		drawer.drawRect(Vec2f(0, 0), Vec2f(size.x / 4, size.y), Vec2f(0.0, shift), Vec2f(1.0, shift + 1.0));
		drawer.getStack().pop();

		drawer.getStack().push();
		drawer.getStack().translate(centerPos);
		drawer.getStack().rotate(player->getRotation());
		drawer.getStack().translate(Vec2f(odmak - razmak_tracnica, -size.y / 2));
		drawer.drawRect(Vec2f(0, 0), Vec2f(size.x / 4, size.y), Vec2f(0.0, shift), Vec2f(1.0, shift + 1.0));
		drawer.getStack().pop();
	}

	Texture* textura;
	Texture* texturaN;
	Texture* tracnica_tex;
	Texture* tracnica_normal;
	Texture* podloga_tex;
	Texture* podloga_normal;

};

class MobRenderer : public IObjectRenderer
{
public:
	MobRenderer()
	{
		mRenderOrder = 1;
	}
	virtual void render(PrimitiveDrawer& drawer, RenderPass pass , LevelObject* object);
	virtual void renderGroup(PrimitiveDrawer& drawer, RenderPass pass , int numObj, LevelObject* object);

protected:
	Texture* mTextures[ TextureGroupCount ];
};


void MobRenderer::render(PrimitiveDrawer& drawer, RenderPass pass , LevelObject* object)
{
	Mob* mob = static_cast< Mob* >( object );

	if (pass == RP_BASE_PASS)
	{
		PrimitiveMat mat;
		mat.color = Color3f(0,0,0);
		mat.baseTex = mTextures[TG_DIFFUSE];
		mat.normalTex = mTextures[TG_NORMAL];
		drawer.setMaterial(mat);
		drawer.beginTranslucent(0.6);
		drawer.drawRect(mob->getRenderPos() + Vec2f(5, 5), mob->getSize(), mob->getRotation());
		drawer.endTranslucent();

		mat.color = Color3f(1, 1, 1);
		drawer.setMaterial(mat);
		drawer.drawRect(mob->getRenderPos(), mob->getSize(), mob->getRotation());
	}
	else
	{
		drawer.setGlow(mTextures[TG_GLOW]);
		drawer.drawRect(mob->getRenderPos(), mob->getSize(), mob->getRotation());
	}
}

void MobRenderer::renderGroup(PrimitiveDrawer& drawer, RenderPass pass , int numObj, LevelObject* object)
{
	if (pass == RP_BASE_PASS)
	{
		PrimitiveMat mat;
		mat.color = Color3f(0, 0, 0);
		mat.baseTex = mTextures[TG_DIFFUSE];
		mat.normalTex = mTextures[TG_NORMAL];
		drawer.setMaterial(mat);
		drawer.beginTranslucent(0.6);
		for (LevelObject* cur = object; cur; cur = NextObject(cur))
		{
			Mob* mob = static_cast<Mob*>(cur);
			drawer.drawRect(mob->getRenderPos() + Vec2f(5, 5), mob->getSize(), mob->getRotation());
		}
		drawer.endTranslucent();

		mat.color = Color3f(1, 1, 1);
		drawer.setMaterial(mat);
		for (LevelObject* cur = object; cur; cur = NextObject(cur))
		{
			Mob* mob = static_cast<Mob*>(cur);
			drawer.drawRect(mob->getRenderPos(), mob->getSize(), mob->getRotation());
		}
	}
	else
	{
		drawer.setGlow(mTextures[TG_GLOW]);		
		for (LevelObject* cur = object; cur; cur = NextObject(cur))
		{
			Mob* mob = static_cast<Mob*>(cur);
			drawer.drawRect(mob->getRenderPos(), mob->getSize(), mob->getRotation());
		}
	}
}

class PlasmaMobRenderer : public MobRenderer
{
public:
	virtual void init()
	{
		TextureManager* texMgr = getRenderSystem()->getTextureMgr();
		mTextures[ TG_DIFFUSE ] = texMgr->getTexture("mob1Diffuse.tga");
		mTextures[ TG_NORMAL  ] = texMgr->getTexture("mob1Normal.tga");
		mTextures[ TG_GLOW    ] = texMgr->getTexture("mob2Glow.tga");
	}
};

class LaserMobRenderer : public MobRenderer
{
public:
	virtual void init()
	{
		TextureManager* texMgr = getRenderSystem()->getTextureMgr();
		mTextures[ TG_DIFFUSE ] = texMgr->getTexture("mob1Diffuse.tga");
		mTextures[ TG_NORMAL  ] = texMgr->getTexture("mob1Normal.tga");
		mTextures[ TG_GLOW    ] = texMgr->getTexture("mob1Glow.tga");
	}
};

class MinigunMobRenderer : public MobRenderer
{
public:
	virtual void init()
	{
		TextureManager* texMgr = getRenderSystem()->getTextureMgr();
		mTextures[ TG_DIFFUSE ] = texMgr->getTexture("mob1Diffuse.tga");
		mTextures[ TG_NORMAL  ] = texMgr->getTexture("mob1Normal.tga");
		mTextures[ TG_GLOW    ] = texMgr->getTexture("mob3Glow.tga");
	}
};

class BulletRenderer : public IObjectRenderer
{
public:
	BulletRenderer()
	{
		mRenderOrder = 3;
	}
};

class LaserBulletRenderer : public BulletRenderer
{
public:
	virtual void init()
	{
		texG = getRenderSystem()->getTextureMgr()->getTexture("laser1Glow.tga");
	}

	virtual void render(PrimitiveDrawer& drawer, RenderPass pass , LevelObject* object)
	{
		if (pass != RP_GLOW)
			return;

		LaserBullet* bullet = object->cast< LaserBullet >();
		Vec2f size = Vec2f(16, 32);
		float rot = Math::ATan2(bullet->mDir.y, bullet->mDir.x) + Math::DegToRad(90);
		drawer.setGlow(texG);
		drawer.drawRect(bullet->getPos() - size / 2, size, rot);
	}
	Texture* texG;
};


class MinigunBulletRenderer : public BulletRenderer
{
public:
	virtual void init()
	{
		texG = getRenderSystem()->getTextureMgr()->getTexture("minigun1Glow.tga");
	}

	virtual void render(PrimitiveDrawer& drawer, RenderPass pass , LevelObject* object)
	{
		
		if (pass != RP_GLOW)
			return;
	
		MinigunBullet* bullet = object->cast< MinigunBullet >();
		Vec2f size = Vec2f(16, 32);
		float rot = Math::ATan2(bullet->mDir.y, bullet->mDir.x) + Math::DegToRad(90);
		drawer.setGlow(texG);
		drawer.drawRect(bullet->getPos() - size / 2, size, rot);
	}
	Texture* texG;
};

class PlasmaBulletRenderer : public BulletRenderer
{
public:
	virtual void init()
	{
		tex = getRenderSystem()->getTextureMgr()->getTexture("granata1.tga");
	}

	virtual void render(PrimitiveDrawer& drawer, RenderPass pass , LevelObject* object)
	{
		if( pass != RP_BASE_PASS )
			return;
		
		PrimitiveMat mat;
		mat.baseTex = tex;
		mat.normalTex = nullptr;
		drawer.beginTranslucent(1.0);
		drawer.setMaterial(mat);
		drawer.drawRect(object->getRenderPos(), object->getSize());
		drawer.endTranslucent();
	}
	Texture* tex;
};

class SmokeRenderer : public IObjectRenderer
{
public:
	virtual void init()
	{
		tex = getRenderSystem()->getTextureMgr()->getTexture("Dim1Diffuse.tga");
		texN= getRenderSystem()->getTextureMgr()->getTexture("Dim1Normal.tga");
	}

	virtual void render(PrimitiveDrawer& drawer, RenderPass pass , LevelObject* object)
	{
		return;
		SmokeParticle* smoke = object->cast< SmokeParticle >();

		if(pass==TG_DIFFUSE)// || pass==NORMAL)
		{
			Texture* t;
			if(pass==TG_DIFFUSE)
				t=tex;
			if(pass==TG_NORMAL)
				t=texN;	

			float faktorSkaliranja = 0.5+ 0.5 * ( smoke->maxLife / smoke->life );
			if( faktorSkaliranja > 2 )
				faktorSkaliranja = 2;

			float faktorBoje=smoke->life/smoke->maxLife;

			Vec2f center = smoke->getPos();
			glPushMatrix();
			glTranslatef( center.x , center.y , 0);	
			glScalef(faktorSkaliranja,faktorSkaliranja,0);
			glTranslatef( -smoke->getSize().x ,-smoke->getSize().y ,0 );	

			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
			glColor3f(faktorBoje,faktorBoje,faktorBoje);

			glEnable(GL_TEXTURE_2D);
			t->bind();
			glBegin(GL_QUADS);
			glTexCoord2f(0.0, 0.0); glVertex2f(0.0, 0.0);
			glTexCoord2f(1.0, 0.0); glVertex2f(smoke->getSize().x, 0.0);
			glTexCoord2f(1.0, 1.0); glVertex2f(smoke->getSize().x, smoke->getSize().y);
			glTexCoord2f(0.0, 1.0); glVertex2f(0.0, smoke->getSize().y);
			glEnd();
			glBindTexture(GL_TEXTURE_2D,0);
			glDisable(GL_TEXTURE_2D);	

			glDisable(GL_BLEND);
			glPopMatrix();	

			glColor3f(1.0, 1.0, 1.0);	
		}
	}

	Texture* tex;
	Texture* texN;
};


class DebrisParticleRenderer : public IObjectRenderer
{
public:

	virtual void init()
	{
		tex = getRenderSystem()->getTextureMgr()->getTexture("Dim1Diffuse.tga");
		texN= getRenderSystem()->getTextureMgr()->getTexture("Dim1Normal.tga");
	}

	virtual void render(PrimitiveDrawer& drawer, RenderPass pass , LevelObject* object)
	{
		return;
		DebrisParticle* particle = object->cast< DebrisParticle >();
		Texture* t;
		if(pass==TG_DIFFUSE)
			t=tex;
		if(pass==TG_NORMAL)
			t=texN;
		if(pass==TG_DIFFUSE)// || pass==NORMAL)
		{
			float factor = particle->life / particle->maxLife;
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
			glColor3f( factor , factor , factor );
			drawSprite( particle->getRenderPos() , particle->getSize() , 0 ,t );
			glColor3f(1.0, 1.0, 1.0);
			glDisable(GL_BLEND);
		}
	}

	virtual void renderGroup(PrimitiveDrawer& drawer, RenderPass pass , int numObj , LevelObject* object)
	{
		return;
		Texture* t;
		if(pass==TG_DIFFUSE)
			t=tex;
		if(pass==TG_NORMAL )
			t=texN;
		if(pass==TG_DIFFUSE)// || pass==NORMAL)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
			for( ; object ; object = NextObject( object ) )
			{
				DebrisParticle* particle = object->cast< DebrisParticle >();
				float factor = particle->life / particle->maxLife;
				glColor3f( factor , factor , factor );
				drawSprite( particle->getRenderPos() , particle->getSize() , 0 ,t );
			}
			glColor3f(1.0, 1.0, 1.0);
			glDisable(GL_BLEND);
		}

	}

	Texture* tex;
	Texture* texN;
};


class DebrisPickupRenderer : public IObjectRenderer
{
public:
	virtual void init()
	{
		tex = getRenderSystem()->getTextureMgr()->getTexture("SmeceDiffuse.tga");
		texN= getRenderSystem()->getTextureMgr()->getTexture("SmeceNormal.tga");
	}

	virtual void render(PrimitiveDrawer& drawer, RenderPass pass , LevelObject* object)
	{
		if( pass == RP_GLOW )
			return;

		PrimitiveMat mat;
		mat.baseTex = tex;
		mat.normalTex = texN;
		drawer.setMaterial(mat);
		drawer.drawRect( object->getRenderPos() , object->getSize() );
	}

	Texture* tex;
	Texture* texN;
};


class WeaponPickupRenderer : public IObjectRenderer
{
public:
	virtual void init()
	{
		TextureManager* texMgr = getRenderSystem()->getTextureMgr();
		mTex[ WEAPON_LASER ][ TG_DIFFUSE ] = texMgr->getTexture("weapon1.tga");
		mTex[ WEAPON_LASER ][ TG_NORMAL  ] = texMgr->getTexture("weapon1Normal.tga");
		mTex[ WEAPON_LASER ][ TG_GLOW    ] = texMgr->getTexture("oruzje1Glow.tga");

		mTex[ WEAPON_PLAZMA ][ TG_DIFFUSE ] = texMgr->getTexture("weapon1.tga");
		mTex[ WEAPON_PLAZMA ][ TG_NORMAL  ] = texMgr->getTexture("weapon1Normal.tga");
		mTex[ WEAPON_PLAZMA ][ TG_GLOW    ] = texMgr->getTexture("oruzje2Glow.tga");

		mTex[ WEAPON_MINIGUN ][ TG_DIFFUSE ] = texMgr->getTexture("weapon1.tga");
		mTex[ WEAPON_MINIGUN ][ TG_NORMAL  ] = texMgr->getTexture("weapon1Normal.tga");
		mTex[ WEAPON_MINIGUN ][ TG_GLOW    ] = texMgr->getTexture("oruzje3Glow.tga");
	}

	virtual void render(PrimitiveDrawer& drawer, RenderPass pass , LevelObject* object)
	{
		WeaponPickup* pickup = static_cast<WeaponPickup*>(object);

		if (pass == RP_BASE_PASS)
		{
			PrimitiveMat mat;
			mat.color = Color3f(1, 1, 1);
			mat.baseTex = mTex[pickup->mId][TG_DIFFUSE];
			mat.normalTex = mTex[pickup->mId][TG_NORMAL];
			drawer.setMaterial(mat);
			drawer.drawRect(pickup->getRenderPos() + Vec2f(pickup->getSize().x / 2 - 8, 0), Vec2f(16, 32), pickup->mRotation);
		}
		else
		{
			drawSprite(pickup->getRenderPos() + Vec2f(pickup->getSize().x / 2 - 8, 0), Vec2f(16, 32), pickup->mRotation, mTex[pickup->mId][TG_GLOW]);
		}

	}
	Texture* mTex[ NUM_WEAPON_ID ][ TextureGroupCount ];
};


class KeyPickupRenderer : public IObjectRenderer
{
public:
	virtual void init()
	{
		TextureManager* texMgr = getRenderSystem()->getTextureMgr();
		mTex[ TG_DIFFUSE ] = texMgr->getTexture("KeyDiffuse.tga");
		mTex[ TG_NORMAL  ] = texMgr->getTexture("KeyNormal.tga");	
		mTex[ TG_GLOW    ] = texMgr->getTexture("KeyGlow.tga");
	}

	virtual void render(PrimitiveDrawer& drawer, RenderPass pass , LevelObject* object)
	{
		KeyPickup* myObj = static_cast<KeyPickup*>(object);
		if (pass == RP_BASE_PASS)
		{
			PrimitiveMat mat;
			mat.baseTex = mTex[TG_DIFFUSE];
			mat.normalTex = mTex[TG_NORMAL];
			drawer.setMaterial(mat);
		}
		else
		{
			drawer.setGlow(mTex[TG_GLOW], GetDoorColor(myObj->mId));
		}

		drawer.drawRect(myObj->getRenderPos(),myObj->getSize(),myObj->mRotation);
	}

	Texture* mTex[ TextureGroupCount ];
};




DEF_OBJ_RENDERER( Player , PlayerRenderer )

DEF_OBJ_RENDERER( PlasmaMob , PlasmaMobRenderer )
DEF_OBJ_RENDERER( LaserMob , LaserMobRenderer )
DEF_OBJ_RENDERER( MinigunMob , MinigunMobRenderer )

DEF_OBJ_RENDERER( LaserBullet , LaserBulletRenderer )
DEF_OBJ_RENDERER( MinigunBullet , MinigunBulletRenderer )
DEF_OBJ_RENDERER( PlasmaBullet , PlasmaBulletRenderer )

DEF_OBJ_RENDERER( SmokeParticle , SmokeRenderer )
DEF_OBJ_RENDERER( DebrisParticle , DebrisParticleRenderer )

DEF_OBJ_RENDERER( DebrisPickup , DebrisPickupRenderer )
DEF_OBJ_RENDERER( WeaponPickup , WeaponPickupRenderer )
DEF_OBJ_RENDERER( KeyPickup , KeyPickupRenderer )