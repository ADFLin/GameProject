#pragma once
#ifndef SpriteRender_H_
#define SpriteRender_H_

#include "Core/IntegerType.h"
#include "Core/Color.h"
#include "Math/Vector2.h"
#include "Math/Matrix4.h"
#include "DataStructure/Array.h"
#include "Renderer/RenderTransform2D.h"
#include <map>
#include <unordered_map>
#include <string>

#include "RHI/RHICommon.h"
#include "ReflectionCollect.h"

class RHIGraphics2D;
class JsonSerializer;
namespace Render
{
    class RHITexture2D;
    class RHICommandList;
}

struct SpriteRegion
{
    Math::Vector2 uvPos;
    Math::Vector2 uvSize;
    Math::Vector2 size;
    Math::Vector2 pivot;

    SpriteRegion() 
        : uvPos(0, 0)
        , uvSize(1, 1)
        , size(1, 1)
        , pivot(0.5f, 0.5f)
    {}

    REFLECT_STRUCT_BEGIN(SpriteRegion)
        REF_PROPERTY(uvPos)
        REF_PROPERTY(uvSize)
        REF_PROPERTY(size)
        REF_PROPERTY(pivot)
    REFLECT_STRUCT_END()
};

struct SpriteAnimFrame
{
    int32 regionIndex;
    float duration;

    REFLECT_STRUCT_BEGIN(SpriteAnimFrame)
        REF_PROPERTY(regionIndex)
        REF_PROPERTY(duration)
    REFLECT_STRUCT_END()
};

class SpriteAnimSequence
{
public:
	TArray<SpriteAnimFrame> frames;
	bool bLoop = true;

	int32 getFrameIndex(float playTime) const;

	REFLECT_STRUCT_BEGIN(SpriteAnimSequence)
		REF_PROPERTY(frames)
		REF_PROPERTY(bLoop)
	REFLECT_STRUCT_END()
};

class SpriteSheet
{
public:
	Render::RHITexture2DRef texture;
	TArray<SpriteRegion> regions;
	std::unordered_map<HashString, SpriteAnimSequence> animations;

	const SpriteRegion& getRegion(int32 index) const { return regions[index]; }

	REFLECT_STRUCT_BEGIN(SpriteSheet)
		REF_PROPERTY(regions)
		REF_PROPERTY(animations)
	REFLECT_STRUCT_END()
};

struct TileMapData
{
    int32 width = 0;
    int32 height = 0;
    Math::Vector2 tileSize;
    TArray<int32> tileIndices;
};

class ISpriteRenderer
{
public:
    virtual ~ISpriteRenderer() = default;

    virtual void beginDraw(Math::Matrix4 const& transform) = 0;
    
    virtual void drawSprite(SpriteSheet const* sheet, int32 regionIndex, Render::RenderTransform2D const& transform, Color4f const& color = Color4f::White()) = 0;

    virtual void drawTileMap(SpriteSheet const* sheet, TileMapData const& tileMap, Math::Vector2 const& pos, Color4f const& color = Color4f::White()) = 0;

    virtual void endDraw() = 0;
};

class SpriteRendererG2D : public ISpriteRenderer
{
public:
    SpriteRendererG2D(RHIGraphics2D& inG2D) : mGraphics(inG2D) {}

    virtual void beginDraw(Math::Matrix4 const& transform) override;
    virtual void drawSprite(SpriteSheet const* sheet, int32 regionIndex, Render::RenderTransform2D const& transform, Color4f const& color) override;
    virtual void drawTileMap(SpriteSheet const* sheet, TileMapData const& map, Math::Vector2 const& pos, Color4f const& color) override;
    virtual void endDraw() override;

private:
    RHIGraphics2D& mGraphics;
};

struct SpriteVertex
{
    Math::Vector2 position;
    Math::Vector2 uv;
    Color4ub color;
};

class SpriteRendererRHICommand : public ISpriteRenderer
{
public:
    SpriteRendererRHICommand(Render::RHICommandList& inCmdList);

    virtual void beginDraw(Math::Matrix4 const& transform) override;
    virtual void drawSprite(SpriteSheet const* sheet, int32 regionIndex, Render::RenderTransform2D const& transform, Color4f const& color) override;
    virtual void drawTileMap(SpriteSheet const* sheet, TileMapData const& map, Math::Vector2 const& pos, Color4f const& color) override;
    virtual void endDraw() override;

private:
    void flushBatch();

    Render::RHICommandList& cmdList;
    Render::RHITexture2D* currentTexture = nullptr;
    
    TArray<SpriteVertex> vertexBuffer;
    TArray<uint16> indexBuffer;
    Math::Matrix4 currentTransform;
};

class FSpriteUtility
{
public:
	static bool load(char const* path, SpriteSheet& sheet);
	static bool save(char const* path, SpriteSheet& sheet, char const* texturePath);
};

#endif // SpriteRender_H_
