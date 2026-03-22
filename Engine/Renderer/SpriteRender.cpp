#include "SpriteRender.h"
#include "Json.h"
#include "FileSystem.h"
#include "RHI/RHIGraphics2D.h"
#include "RHI/RHICommand.h"
#include "RHI/RHIUtility.h"

// Specialization for HashString to work with JsonSerializer map support
namespace JsonDetail
{
	template<>
	inline HashString FromString<HashString>(std::string const& str) { return HashString(str.c_str()); }
	template<>
	inline std::string ToString<HashString>(HashString const& key) { return key.c_str(); }
}

bool FSpriteUtility::load(char const* path, SpriteSheet& sheet)
{
	JsonFile* file = JsonFile::Load(path);
	if (!file)
		return false;

	JsonObject obj = file->getObject();
	if (obj.isVaild())
	{
		JsonSerializer serializer(obj, true);

		std::string texturePath;
		if (obj.tryGet("texturePath", texturePath))
		{
			sheet.texture = Render::RHIUtility::LoadTexture2DFromFile(texturePath.c_str());
		}

		TJsonSerializeCollector<SpriteSheet> collector(serializer, sheet);
		SpriteSheet::CollectReflection(collector);
	}

	file->release();
	return true;
}

bool FSpriteUtility::save(char const* path, SpriteSheet& sheet, char const* texturePath)
{
	JsonFile* file = JsonFile::Create();
	if (!file)
		return false;

	JsonObject obj = file->getObject();

	if (texturePath)
	{
		obj.set("texturePath", texturePath);
	}

	JsonSerializer serializer(obj, false);
	TJsonSerializeCollector<SpriteSheet> collector(serializer, sheet);
	SpriteSheet::CollectReflection(collector);

	std::string str = file->toString();
	bool bSuccess = FFileUtility::SaveFromBuffer(path, (uint8 const*)str.c_str(), (uint32)str.length());

	file->release();
	return bSuccess;
}

int32 SpriteAnimSequence::getFrameIndex(float playTime) const
{
	if (frames.empty())
		return -1;

	float totalDuration = 0.0f;
	for (const auto& frame : frames)
	{
		totalDuration += frame.duration;
	}

	if (totalDuration <= 0.0f)
		return frames[0].regionIndex;

	if (bLoop)
	{
		playTime = fmod(playTime, totalDuration);
	}
	else if (playTime >= totalDuration)
	{
		return frames.back().regionIndex;
	}

	float currentTime = 0.0f;
	for (const auto& frame : frames)
	{
		currentTime += frame.duration;
		if (playTime <= currentTime)
		{
			return frame.regionIndex;
		}
	}
	return frames.back().regionIndex;
}

// -------------------------------------------------------------

void SpriteRendererG2D::beginDraw(Math::Matrix4 const& transform)
{

}

void SpriteRendererG2D::drawSprite(SpriteSheet const* sheet, int32 regionIndex, Render::RenderTransform2D const& transform, Color4f const& color)
{
	if (!sheet || regionIndex < 0 || regionIndex >= sheet->regions.size() || !sheet->texture)
		return;

	const SpriteRegion& region = sheet->getRegion(regionIndex);
	mGraphics.pushXForm();
	mGraphics.transformXForm(transform, false);
	Math::Vector2 drawPos = Math::Vector2(-region.size.x * region.pivot.x, -region.size.y * region.pivot.y);
	mGraphics.setTexture(const_cast<Render::RHITexture2D&>(*sheet->texture));
	mGraphics.drawTexture(drawPos, region.size, region.uvPos, region.uvSize, color);
	mGraphics.popXForm();
}

void SpriteRendererG2D::drawTileMap(SpriteSheet const* sheet, TileMapData const& map, Math::Vector2 const& pos, Color4f const& color)
{
	if (!sheet || !sheet->texture)
		return;

	mGraphics.setTexture(const_cast<Render::RHITexture2D&>(*sheet->texture));
	for (int y = 0; y < map.height; ++y)
	{
		for (int x = 0; x < map.width; ++x)
		{
			int32 index = map.tileIndices[y * map.width + x];
			if (index >= 0 && index < sheet->regions.size())
			{
				const SpriteRegion& region = sheet->getRegion(index);
				Math::Vector2 drawPos = pos + Math::Vector2(x * map.tileSize.x, y * map.tileSize.y);
				mGraphics.drawTexture(drawPos, region.size, region.uvPos, region.uvSize, color);
			}
		}
	}
}

void SpriteRendererG2D::endDraw()
{
}

// -------------------------------------------------------------

SpriteRendererRHICommand::SpriteRendererRHICommand(Render::RHICommandList& inCmdList)
	: cmdList(inCmdList)
{
}

void SpriteRendererRHICommand::beginDraw(Math::Matrix4 const& transform)
{
	currentTransform = transform;
	currentTexture = nullptr;
	vertexBuffer.clear();
	indexBuffer.clear();
}

void SpriteRendererRHICommand::drawSprite(SpriteSheet const* sheet, int32 regionIndex, Render::RenderTransform2D const& transform, Color4f const& color)
{
	if (!sheet || !sheet->texture || regionIndex < 0 || regionIndex >= sheet->regions.size())
		return;

	if (currentTexture != sheet->texture.get() || vertexBuffer.size() > 60000)
	{
		if (currentTexture)
			flushBatch();
		currentTexture = const_cast<Render::RHITexture2D*>(sheet->texture.get());
	}

	const SpriteRegion& region = sheet->getRegion(regionIndex);
	Math::Vector2 basePos = Math::Vector2(-region.size.x * region.pivot.x, -region.size.y * region.pivot.y);
	Math::Vector2 p1 = basePos;
	Math::Vector2 p2 = basePos + Math::Vector2(region.size.x, 0);
	Math::Vector2 p3 = basePos + Math::Vector2(region.size.x, region.size.y);
	Math::Vector2 p4 = basePos + Math::Vector2(0, region.size.y);

	Math::Vector2 trP1 = transform.transformPosition(p1);
	Math::Vector2 trP2 = transform.transformPosition(p2);
	Math::Vector2 trP3 = transform.transformPosition(p3);
	Math::Vector2 trP4 = transform.transformPosition(p4);

	Math::Vector2 uv1 = region.uvPos;
	Math::Vector2 uv2 = region.uvPos + Math::Vector2(region.uvSize.x, 0);
	Math::Vector2 uv3 = region.uvPos + region.uvSize;
	Math::Vector2 uv4 = region.uvPos + Math::Vector2(0, region.uvSize.y);

	Color4ub quadColor(color.r * 255.f, color.g * 255.f, color.b * 255.f, color.a * 255.f);
	uint16 startIndex = (uint16)vertexBuffer.size();
	vertexBuffer.push_back({trP1, uv1, quadColor});
	vertexBuffer.push_back({trP2, uv2, quadColor});
	vertexBuffer.push_back({trP3, uv3, quadColor});
	vertexBuffer.push_back({trP4, uv4, quadColor});
	indexBuffer.push_back(startIndex + 0);
	indexBuffer.push_back(startIndex + 1);
	indexBuffer.push_back(startIndex + 2);
	indexBuffer.push_back(startIndex + 0);
	indexBuffer.push_back(startIndex + 2);
	indexBuffer.push_back(startIndex + 3);
}

void SpriteRendererRHICommand::drawTileMap(SpriteSheet const* sheet, TileMapData const& map, Math::Vector2 const& pos, Color4f const& color)
{
	if (!sheet || !sheet->texture)
		return;

	if (currentTexture != sheet->texture.get())
	{
		if (currentTexture)
			flushBatch();
		currentTexture = const_cast<Render::RHITexture2D*>(sheet->texture.get());
	}

	Color4ub quadColor(color.r * 255.f, color.g * 255.f, color.b * 255.f, color.a * 255.f);
	for (int y = 0; y < map.height; ++y)
	{
		for (int x = 0; x < map.width; ++x)
		{
			int32 index = map.tileIndices[y * map.width + x];
			if (index >= 0 && index < sheet->regions.size())
			{
				if (vertexBuffer.size() > 60000)
				{
					flushBatch();
				}
				const SpriteRegion& region = sheet->getRegion(index);
				Math::Vector2 tl = pos + Math::Vector2(x * map.tileSize.x, y * map.tileSize.y);
				Math::Vector2 tr = tl + Math::Vector2(region.size.x, 0);
				Math::Vector2 bl = tl + Math::Vector2(0, region.size.y);
				Math::Vector2 br = tl + region.size;
				Math::Vector2 uvTL = region.uvPos;
				Math::Vector2 uvTR = region.uvPos + Math::Vector2(region.uvSize.x, 0);
				Math::Vector2 uvBL = region.uvPos + Math::Vector2(0, region.uvSize.y);
				Math::Vector2 uvBR = region.uvPos + region.uvSize;
				uint16 startIndex = (uint16)vertexBuffer.size();
				vertexBuffer.push_back({tl, uvTL, quadColor});
				vertexBuffer.push_back({tr, uvTR, quadColor});
				vertexBuffer.push_back({br, uvBR, quadColor});
				vertexBuffer.push_back({bl, uvBL, quadColor});
				indexBuffer.push_back(startIndex + 0);
				indexBuffer.push_back(startIndex + 1);
				indexBuffer.push_back(startIndex + 2);
				indexBuffer.push_back(startIndex + 0);
				indexBuffer.push_back(startIndex + 2);
				indexBuffer.push_back(startIndex + 3);
			}
		}
	}
}

void SpriteRendererRHICommand::flushBatch()
{
	if (indexBuffer.empty() || !currentTexture)
		return;
	vertexBuffer.clear();
	indexBuffer.clear();
}

void SpriteRendererRHICommand::endDraw()
{
	if (currentTexture && !indexBuffer.empty())
	{
		flushBatch();
	}
}
