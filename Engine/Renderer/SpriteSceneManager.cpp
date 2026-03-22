#include "SpriteSceneManager.h"

SpriteEntityID SpriteSceneManager::createSprite(SpriteSheet const* sheet, int32 defaultRegion)
{
    SpriteEntityID id = nextEntityID++;
    
    if (transformComponents.size() <= id)
    {
        transformComponents.resize(id + 64);
        renderComponents.resize(id + 64);
    }

    transformComponents[id].setIdentity();
    transformComponents[id].zOrder = 0;

    renderComponents[id].sheet = sheet;
    renderComponents[id].regionIndex = defaultRegion;
    renderComponents[id].color = Color4f::White();
    renderComponents[id].bVisible = true;

    return id;
}

void SpriteSceneManager::addAnimation(SpriteEntityID id, SpriteAnimSequence const* anim)
{
    // Check if already exists
    for (auto& animData : animComponents)
    {
        if (animData.entityID == id)
        {
            animData.animSequence = anim;
            animData.playTime = 0.0f;
            return;
        }
    }

    SpriteAnimData newAnim;
    newAnim.entityID = id;
    newAnim.animSequence = anim;
    newAnim.playTime = 0.0f;
    animComponents.push_back(newAnim);
}

SpriteTransform& SpriteSceneManager::getTransform(SpriteEntityID id)
{
    return transformComponents[id];
}

void SpriteSceneManager::addTileMap(SpriteSheet const* sheet, TileMapData const& mapData, SpriteEntityID entityID, int32 zOrder)
{
    TileMapDataComponent comp;
    comp.entityID = entityID;
    comp.sheet = sheet;
    comp.mapData = mapData;
    comp.zOrder = zOrder;
    tileMapComponents.push_back(comp);
}

void SpriteSceneManager::updateAnimations(float deltaTime)
{
    for (auto& anim : animComponents)
    {
        anim.playTime += deltaTime;
        
        if (anim.animSequence)
        {
            int32 newRegion = anim.animSequence->getFrameIndex(anim.playTime);
            renderComponents[anim.entityID].regionIndex = newRegion;
        }
    }
}

void SpriteSceneManager::render(ISpriteRenderer* renderer, Math::Matrix4 const& viewTransform)
{
    if (!renderer) return;

    TArray<SpriteEntityID> sortedIndices;
    sortedIndices.reserve(nextEntityID);
    for (SpriteEntityID i = 0; i < nextEntityID; ++i)
    {
        if (renderComponents[i].bVisible && renderComponents[i].sheet && renderComponents[i].regionIndex >= 0) 
        {
            sortedIndices.push_back(i);
        }
    }

    std::sort(sortedIndices.begin(), sortedIndices.end(), [this](SpriteEntityID a, SpriteEntityID b) {
        return transformComponents[a].zOrder < transformComponents[b].zOrder;
    });

    renderer->beginDraw(viewTransform);
    
    for(SpriteEntityID id : sortedIndices)
    {
        const auto& tx = transformComponents[id];
        const auto& rd = renderComponents[id];
        
        renderer->drawSprite(rd.sheet, rd.regionIndex, tx, rd.color);
    }

    for(const auto& map : tileMapComponents)
    {
        renderer->drawTileMap(map.sheet, map.mapData, transformComponents[map.entityID].P, map.color);
    }
    
    renderer->endDraw();
}
