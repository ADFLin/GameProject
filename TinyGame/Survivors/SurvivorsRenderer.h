#ifndef SurvivorsRenderer_h__
#define SurvivorsRenderer_h__

#include "SurvivorsCommon.h"
#include "RHI/RHICommon.h"
#include "RHI/ShaderProgram.h"
#include "RHI/GlobalShader.h"
#include "RHI/RHIGlobalResource.h"
#include "Renderer/RenderTransform2D.h"
#include "Renderer/SpriteRender.h"
#include <vector>

namespace Survivors
{
	using namespace Render;


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
