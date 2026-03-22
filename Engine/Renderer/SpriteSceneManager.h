#pragma once
#ifndef SpriteSceneManager_H_
#define SpriteSceneManager_H_

#include "SpriteRender.h"
#include <algorithm>

using SpriteEntityID = uint32;

struct SpriteTransform : public Render::RenderTransform2D
{
    int32 zOrder = 0;
};

struct SpriteRenderData
{
    SpriteSheet const* sheet = nullptr;
    int32 regionIndex = -1;
    Color4f color = Color4f::White();
    bool bVisible = true;
};

struct SpriteAnimData
{
    SpriteEntityID entityID = 0;
    SpriteAnimSequence const* animSequence = nullptr;
    float playTime = 0.0f;
};

struct TileMapDataComponent
{
    SpriteEntityID entityID = 0;
    SpriteSheet const* sheet = nullptr;
    TileMapData mapData;
    Color4f color = Color4f::White();
    int32 zOrder = 0;
};


class SpriteSceneManager
{
public:
    SpriteEntityID createSprite(SpriteSheet const* sheet, int32 defaultRegion);
    
    void addAnimation(SpriteEntityID id, SpriteAnimSequence const* anim);
    
    SpriteTransform& getTransform(SpriteEntityID id);

    void addTileMap(SpriteSheet const* sheet, TileMapData const& mapData, SpriteEntityID entityID, int32 zOrder = 0);
    
    void updateAnimations(float deltaTime);

    void render(ISpriteRenderer* renderer, Math::Matrix4 const& viewTransform);

private:
    SpriteEntityID nextEntityID = 0;

    TArray<SpriteTransform>  transformComponents;
    TArray<SpriteRenderData> renderComponents;
    
    TArray<SpriteAnimData>       animComponents;
    TArray<TileMapDataComponent> tileMapComponents;
};

#endif // SpriteSceneManager_H_
