#ifndef WeaponRenderer_h__
#define WeaponRenderer_h__

#include "Renderer.h"

class Weapon;
class PrimitiveDrawer;

class WeaponRenderer : public IRenderer
{
public:
	virtual void render( RenderPass pass , Weapon* weapon , PrimitiveDrawer& drawer);
	Texture* mTextues[ TextureGroupCount ];
};

#define DEF_WEAPON_RENDERER( CLASS , RENDERER )\
	static RENDERER g##CLASS##Renderer;\
	WeaponRenderer* CLASS::getRenderer(){  return &g##CLASS##Renderer;  }



#endif // WeaponRenderer_h__
