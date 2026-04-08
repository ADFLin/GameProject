#include "SpriteRender.h"
#include "Json.h"
#include "FileSystem.h"
#include "RHI/RHIGraphics2D.h"
#include "RHI/RHICommand.h"
#include "RHI/RHIUtility.h"
#include "RHI/RHIGlobalResource.h"
#include "RHI/ShaderManager.h"

#define SPRITE_USE_GEOMERTY_SHADER 0

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


constexpr int MaxSpriteRenderCount = 60000;

// -------------------------------------------------------------

SpriteRendererRHICommand::SpriteRendererRHICommand(Render::RHICommandList& inCmdList)
	: cmdList(inCmdList)
{
}

void SpriteRendererRHICommand::beginDraw(Math::Matrix4 const& transform)
{
	currentTransform = transform;
	currentTexture = nullptr;
	instanceBuffer.clear();
}

void SpriteRendererRHICommand::drawSprite(SpriteSheet const* sheet, int32 regionIndex, Render::RenderTransform2D const& transform, Color4f const& color)
{
	if (!sheet || !sheet->texture || regionIndex < 0 || regionIndex >= sheet->regions.size())
		return;

	if (currentTexture != sheet->texture.get() || instanceBuffer.size() > MaxSpriteRenderCount)
	{
		if (currentTexture)
			flushBatch();
		currentTexture = const_cast<Render::RHITexture2D*>(sheet->texture.get());
	}

	SpriteRegion const& region = sheet->getRegion(regionIndex);
	Math::Vector2 localCenter = Math::Vector2((0.5f - region.pivot.x) * region.size.x, (0.5f - region.pivot.y) * region.size.y);

	Render::RenderTransform2D finalTransform = transform;
	finalTransform.translateLocal(localCenter);

	Render::SpriteInstanceData data;
	data.color = color;
	data.size = region.size;
	data.uvPos = region.uvPos;
	data.uvSize = region.uvSize;
	data.transform = finalTransform;
	instanceBuffer.push_back(data);
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
				if (instanceBuffer.size() > MaxSpriteRenderCount)
				{
					flushBatch();
				}

				SpriteRegion const& region = sheet->getRegion(index);
				
				Render::SpriteInstanceData data;
				data.color = color;
				data.size = region.size;
				data.uvPos = region.uvPos;
				data.uvSize = region.uvSize;
				data.transform.M.setIdentity();
				data.transform.P = pos + Math::Vector2(x * map.tileSize.x + 0.5f * region.size.x, y * map.tileSize.y + 0.5f * region.size.y);
				instanceBuffer.push_back(data);
			}
		}
	}
}

void SpriteRendererRHICommand::flushBatch()
{
	if (instanceBuffer.empty())
		return;

	CHECK(currentTexture);
	using namespace Render;
	FSprite::Render(cmdList, currentTransform, *currentTexture, instanceBuffer);
	instanceBuffer.clear();
}

void SpriteRendererRHICommand::endDraw()
{
	if (currentTexture && !instanceBuffer.empty())
	{
		flushBatch();
	}
}


namespace Render
{
	namespace
	{
		class SpriteInstanceDynamicBufferResource : public IGlobalRenderResource
		{
		public:
			RHIBufferRef buffer;
			uint32 capacity = 0;

			virtual void restoreRHI() override
			{
			}

			virtual void releaseRHI() override
			{
				buffer.release();
				capacity = 0;
			}

			bool ensureCapacity(uint32 requiredCount)
			{
				if (requiredCount == 0)
					return false;

				if (!buffer || capacity < requiredCount)
				{
					uint32 newCapacity = capacity > 0 ? capacity : 1024;
					while (newCapacity < requiredCount)
						newCapacity = newCapacity * 2;

					buffer = RHICreateBuffer(sizeof(SpriteInstanceData), newCapacity, BCF_CpuAccessWrite | BCF_UsageVertex);
					if (!buffer)
						return false;

					capacity = newCapacity;
				}
				return true;
			}

			bool upload(TArrayView<SpriteInstanceData const> data)
			{
				if (!ensureCapacity(uint32(data.size())))
					return false;

				void* ptr = RHILockBuffer(buffer, ELockAccess::WriteDiscard);
				if (!ptr)
					return false;

				FMemory::Copy(ptr, data.data(), data.size() * sizeof(SpriteInstanceData));
				RHIUnlockBuffer(buffer);
				return true;
			}
		};

		SpriteInstanceDynamicBufferResource& GetSpriteInstanceDynamicBuffer()
		{
			static SpriteInstanceDynamicBufferResource StaticInstance;
			return StaticInstance;
		}
	}

	class SpriteInputLayout : public StaticRHIResourceT<SpriteInputLayout, RHIInputLayout>
	{
	public:
		static RHIInputLayoutRef CreateRHI()
		{
			return RHICreateInputLayout(GetSetupValue());
		}
		static InputLayoutDesc GetSetupValue()
		{
			InputLayoutDesc desc;
#if SPRITE_USE_GEOMERTY_SHADER
			constexpr bool bInstanceData = false;
#else
			constexpr bool bInstanceData = true;
#endif
			constexpr int instanceStepRate = bInstanceData ? 1 : 0;
			desc.addElement(0, EVertex::ATTRIBUTE0, EVertex::Float2, false, bInstanceData, instanceStepRate); // InSize
			desc.addElement(0, EVertex::ATTRIBUTE1, EVertex::Float2, false, bInstanceData, instanceStepRate); // InUVPos
			desc.addElement(0, EVertex::ATTRIBUTE2, EVertex::Float2, false, bInstanceData, instanceStepRate); // InUVSize
			desc.addElement(0, EVertex::ATTRIBUTE3, EVertex::Float4, false, bInstanceData, instanceStepRate); // InTransformM
			desc.addElement(0, EVertex::ATTRIBUTE4, EVertex::Float2, false, bInstanceData, instanceStepRate); // InTransformP
			desc.addElement(0, EVertex::ATTRIBUTE5, EVertex::Float4, false, bInstanceData, instanceStepRate); // InColor
			return desc;
		}

		static uint32 GetHashKey()
		{
			return GetSetupValue().getTypeHash();
		}
	};

	class SpriteShaderProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(SpriteShaderProgram, Global);
	public:
		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			option.addDefine(SHADER_PARAM(USE_GEOMERTY_SHADER), SPRITE_USE_GEOMERTY_SHADER);
		}
		static char const* GetShaderFileName() { return "Shader/SpriteRender"; }
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
	#if SPRITE_USE_GEOMERTY_SHADER
				{ EShader::Geometry , SHADER_ENTRY(MainGS) },
	#endif
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, WorldToClip);
			BIND_TEXTURE_PARAM(parameterMap, Texture);
		}

		void setParameters(RHICommandList& commandList, Matrix4 const& worldToClip, RHITexture2D& texture, RHISamplerState& sampler = TStaticSamplerState<>::GetRHI())
		{
			SET_SHADER_PARAM(commandList, *this, WorldToClip, worldToClip);
			SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *this, Texture, texture, sampler);
		}

		DEFINE_SHADER_PARAM(WorldToClip);
		DEFINE_TEXTURE_PARAM(Texture);
	};

	IMPLEMENT_SHADER_PROGRAM(SpriteShaderProgram);

	void FSprite::Render(RHICommandList& commandList, Matrix4 const& xForm, RHITexture2D& texture, TArrayView< SpriteInstanceData const> data)
	{
		FSprite::Render(commandList, xForm, texture, nullptr, data);
	}

	void FSprite::Render(RHICommandList& commandList, Matrix4 const& xForm, RHITexture2D& texture, RHISamplerState* sampler, TArrayView< SpriteInstanceData const> data)
	{
		if (data.empty())
			return;

		auto& dynamicBuffer = GetSpriteInstanceDynamicBuffer();
		if (!dynamicBuffer.upload(data))
			return;

		FSprite::Render(commandList, xForm, texture, sampler, *dynamicBuffer.buffer, int(data.size()), 0);
	}

	void FSprite::Render(RHICommandList& commandList, Matrix4 const& xForm, RHITexture2D& texture, RHIBuffer& instanceBuffer, int numInstance, uint32 baseInstance)
	{
		FSprite::Render(commandList, xForm, texture, nullptr, instanceBuffer, numInstance, baseInstance);
	}

	void FSprite::Render(RHICommandList& commandList, Matrix4 const& xForm, RHITexture2D& texture, RHISamplerState* sampler, RHIBuffer& instanceBuffer, int numInstance, uint32 baseInstance)
	{
		if (numInstance <= 0)
			return;
		CHECK(instanceBuffer.getElementSize() == sizeof(SpriteInstanceData));
		CHECK(baseInstance + uint32(numInstance) <= instanceBuffer.getNumElements());

		SpriteShaderProgram* shader = ShaderManager::Get().getGlobalShaderT<SpriteShaderProgram>();
		RHISetShaderProgram(commandList, shader->getRHI());
		shader->setParameters(commandList, xForm, texture, sampler ? *sampler : TStaticSamplerState<>::GetRHI());

		InputStreamInfo inputStream;
		inputStream.buffer = &instanceBuffer;
		inputStream.offset = 0;
		inputStream.stride = sizeof(SpriteInstanceData);
		RHISetInputStream(commandList, &SpriteInputLayout::GetRHI(), &inputStream, 1);
#if SPRITE_USE_GEOMERTY_SHADER
		RHIDrawPrimitive(commandList, EPrimitive::Points, int(baseInstance), numInstance);
#else
		RHIDrawPrimitiveInstanced(commandList, EPrimitive::TriangleStrip, 0, 4, uint32(numInstance), baseInstance);
#endif
	}


}
