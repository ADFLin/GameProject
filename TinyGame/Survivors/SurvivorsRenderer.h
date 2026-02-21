#ifndef SurvivorsRenderer_h__
#define SurvivorsRenderer_h__

#include "SurvivorsCommon.h"
#include "RHI/RHICommon.h"
#include "RHI/ShaderProgram.h"
#include "RHI/GlobalShader.h"
#include "RHI/RHIGlobalResource.h"
#include "Renderer/RenderTransform2D.h"

#include <vector>

namespace Survivors
{
	// -- Instance rendering data structures --
	using namespace Render;

	struct MonsterInstanceData
	{
		Vector2 pos;
		Vector2 size;
		Vector2 uvPos;
		Vector2 uvSize;
		float scale;
		LinearColor color;
	};

	class MonsterInputLayout : public StaticRHIResourceT<MonsterInputLayout, RHIInputLayout>
	{
	public:
		static RHIInputLayoutRef CreateRHI()
		{
			return RHICreateInputLayout(GetSetupValue());
		}
		static InputLayoutDesc GetSetupValue()
		{
			InputLayoutDesc desc;
			desc.addElement(0, EVertex::ATTRIBUTE0, EVertex::Float2); // InPos
			desc.addElement(0, EVertex::ATTRIBUTE1, EVertex::Float2); // InSize
			desc.addElement(0, EVertex::ATTRIBUTE2, EVertex::Float2); // InUVPos
			desc.addElement(0, EVertex::ATTRIBUTE3, EVertex::Float2); // InUVSize
			desc.addElement(0, EVertex::ATTRIBUTE4, EVertex::Float1); // InScale
			desc.addElement(0, EVertex::ATTRIBUTE5, EVertex::Float4); // InColor
			return desc;
		}
		static uint32 GetHashKey()
		{
			return GetSetupValue().getTypeHash();
		}
	};

	class MonsterShaderProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(MonsterShaderProgram, Global);
	public:
		static void SetupShaderCompileOption(ShaderCompileOption&) {}
		static char const* GetShaderFileName() { return "Shader/Game/SurvivorsMonster"; }
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Geometry , SHADER_ENTRY(MainGS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}

		void bindMonsterTexture(RHICommandList& commandList, RHITexture2D& texture)
		{
			setTexture(commandList, SHADER_PARAM(MonsterTexture), texture, SHADER_SAMPLER(MonsterTexture), TStaticSamplerState<ESampler::Trilinear>::GetRHI());
		}
		void setWorldToClip(RHICommandList& commandList, Matrix4 const& matrix)
		{
			setParam(commandList, SHADER_PARAM(WorldToClip), matrix);
		}
	};

	// -- Scene snapshot structs for render-thread separation --

	struct RenderMonsterData
	{
		Vector2 pos;
		float   radius;
		float   scale;
		float   stunTimer;
		MonsterType type;
	};

	struct RenderGemData
	{
		Vector2 pos;
	};

	struct RenderBulletData
	{
		Vector2 pos;
		Vector2 vel;
		float   radius;
		int     category;
	};

	struct RenderVisualData
	{
		Vector2 pos;
		VisualType type;
		float timer;
		float duration;
		float radius;
		float angle;
		float rotation;
	};

	struct RenderHeroData
	{
		Vector2 pos;
		float   radius;
		float   hpParams;
		float   damageFlashTimer;
		Vector2 facing;
		TArray<RenderBulletData> orbs;
	};

	struct SceneSnapshot
	{
		RenderTransform2D worldToScreen;
		RenderTransform2D screenToWorld;
		Vector2 screenSize;

		TArray<RenderMonsterData> monsters;
		TArray<RenderGemData> gems;
		TArray<RenderBulletData> bullets;
		TArray<RenderVisualData> visuals;

		RenderHeroData hero;
	};

	// -- Renderer class: owns all rendering resources --

	class SurvivorsRenderer
	{
	public:
		void init();
		void renderScene(SceneSnapshot const& scene);

		// --- Texture accessors ---
		RHITexture2DRef const& getMapTexture() const { return mMapTexture; }
		RHITexture2DRef const& getMonsterTexture(int typeIndex) const { return mMonsterTextures[typeIndex]; }
		RHITexture2DRef const& getHeroTexture() const { return mHeroTexture; }
		RHITexture2DRef const& getHeroBulletTexture() const { return mHeroBulletTexture; }
		RHITexture2DRef const& getMonsterBulletTexture() const { return mMonsterBulletTexture; }
		RHITexture2DRef const& getOrbBulletTexture() const { return mOrbBulletTexture; }

		float getMonsterDrawScale(int typeIndex) const { return mMonsterDrawScales[typeIndex]; }
		float& monsterDrawScale(int typeIndex) { return mMonsterDrawScales[typeIndex]; }

	private:
		RHITexture2DRef mMapTexture;
		RHITexture2DRef mHeroTexture;
		RHITexture2DRef mHeroBulletTexture;
		RHITexture2DRef mMonsterBulletTexture;
		RHITexture2DRef mOrbBulletTexture;
		RHITexture2DRef mMonsterTextures[(int)MonsterType::Count];

		float mMonsterDrawScales[(int)MonsterType::Count] = { 1.5f , 2.2f };
	};

}//namespace Survivors

#endif // SurvivorsRenderer_h__
