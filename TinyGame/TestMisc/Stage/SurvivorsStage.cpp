
#include "StageBase.h"
#include "GameRenderSetup.h"
#include "RHI/RHIGraphics2D.h"
#include "RandomUtility.h"
#include "ProfileSystem.h"
#include "ParallelCollision/ParallelCollision.h"
#include "Async/AsyncWork.h"
#include "Widget/WidgetUtility.h"
#include "RenderUtility.h"
#include "SystemPlatform.h"
#include "InputManager.h" // Added for InputManager

#include <vector>
#include <memory>
#include <algorithm>
#include <cmath>
#include "RHI/ShaderProgram.h"
#include "RHI/GlobalShader.h"
#include "RHI/ShaderManager.h"
#include "RHI/RHIGlobalResource.h"
#include "RHI/RHIUtility.h"
#include "Image/ImageData.h"


namespace Survivors
{
	using namespace ParallelCollision;
	using namespace Math;
	using namespace Render;

	enum CollisionCategory
	{
		Category_Monster = 1,
		Category_Hero = 2,
		Category_Bullet = 4,
		Category_MonsterBullet = 8,
		Category_HeroAbility = 16,
		Category_Fence = 32,
	};

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
	IMPLEMENT_SHADER_PROGRAM(MonsterShaderProgram);

	enum class VisualType
	{
		Slash,
		Shockwave,
		HitSpark,
	};

	struct VisualEffect
	{
		Vector2 pos;
		float rotation;
		float radius;
		float angle;
		float timer;
		float duration;
		VisualType type;
	};

	class SurvivorsStage;

	enum class MonsterType
	{
		Golem,
		Slime,
		Count
	};

	class Monster : public IPhysicsEntity
	{
	public:
		MonsterType mType = MonsterType::Golem;
		Vector2 mPos;
		Vector2 mVel = Vector2::Zero();
		Vector2 mKnockbackVel = Vector2::Zero();
		float mStunTimer = 0;
		float mRadius = 10.0f;
		float mScale = 1.0f;
		float mSpeed = 80.0f;
		int mHP = 50;
		bool mIsDead = false;
		SurvivorsStage* mStage = nullptr;

		Monster(Vector2 const& pos, SurvivorsStage* stage, MonsterType type = MonsterType::Golem) 
			: mPos(pos), mStage(stage), mType(type) 
		{
			if (type == MonsterType::Golem) { mSpeed = 50.0f; mHP = 500; mRadius = 30.0f; }
			else if (type == MonsterType::Slime) { mSpeed = 130.0f; mHP = 20; mRadius = 10.0f; }
		}

		void handleCollisionEnter(IPhysicsEntity* other, int category) override;
		void handleCollisionStay(IPhysicsEntity* other, int category) override; 
		void handleCollisionExit(IPhysicsEntity* other, int category) override {}

		void handleCollisionEnter(IPhysicsBullet* bullet, int category) override;
		void handleCollisionStay(IPhysicsBullet* bullet, int category) override {}
		void handleCollisionExit(IPhysicsBullet* bullet, int category) override {}

		void applyKnockback(float force, float duration);

		void update(Vector2 const& target, float dt)
		{
			if (mStunTimer > 0)
			{
				mStunTimer -= dt;
				mVel = mKnockbackVel;
				mKnockbackVel *= std::max(0.0f, 1.0f - 4.0f * dt);
			}
			else
			{
				Vector2 dir = target - mPos;
				float dist = dir.length();
				if (dist > 1.0f) mVel = (dir / dist) * mSpeed;
				else mVel = Vector2::Zero();
			}
		}
	};

	class GameBullet : public IPhysicsBullet
	{
	public:
		Vector2 mPos;
		Vector2 mVel;
		float mRadius = 12.0f;
		float mLifeTime = 5.0f;
		bool mIsDead = false;
		int mCategory = Category_Bullet;

		GameBullet(Vector2 const& pos, Vector2 const& vel, int category = Category_Bullet) : mPos(pos), mVel(vel), mCategory(category) {}

		void handleCollisionEnter(IPhysicsEntity* other, int category) override 
		{
			// Pierce through monsters
			// if (mCategory == Category_Bullet && category == Category_Monster) mIsDead = true;
			if (mCategory == Category_MonsterBullet && category == Category_Hero) mIsDead = true;
		}
		void handleCollisionStay(IPhysicsEntity* other, int category) override {}
		void handleCollisionExit(IPhysicsEntity* other, int category) override {}

		void handleCollisionEnter(IPhysicsBullet* other, int category) override 
		{
			if (mCategory == Category_Bullet && category == Category_MonsterBullet) mIsDead = true;
			else if (mCategory == Category_MonsterBullet && category == Category_Bullet) mIsDead = true;
		}
		void handleCollisionStay(IPhysicsBullet* other, int category) override {}
		void handleCollisionExit(IPhysicsBullet* other, int category) override {}

		void update(float dt)
		{
			mPos += mVel * dt;
			mLifeTime -= dt;
			if (mLifeTime <= 0) mIsDead = true;
		}
	};

	struct Gem
	{
		Vector2 pos;
		int     value;
		bool    isDead = false;
	};

	class Hero : public IPhysicsEntity
	{
	public:
		Vector2 mPos;
		Vector2 mMoveTarget;
		float mRadius = 15.0f;
		int mMaxHP = 1000;
		int mHP = 1000;
		int mLevel = 1;
		int mXP = 0;
		int mNextLevelXP = 100;
		Vector2 mFacing = Vector2(1, 0);
		Vector2 mJoystickDir = Vector2::Zero();
		SurvivorsStage* mStage = nullptr;

		// Skill States
		float mBulletRate = 6.0f; // Actions per sec
		float mBulletTimer = 0;
		int   mOrbCount = 4;
		bool  mEnableOrbs = false;
		float mOrbRotation = 0;
		std::vector<std::unique_ptr<GameBullet>> mOrbBullets;

		float mDamageFlashTimer = 0;
		float mDamageTimer = 0;
		bool  mbInvincible = false;

		Hero(Vector2 const& pos, SurvivorsStage* stage) : mPos(pos), mMoveTarget(pos), mStage(stage) {}

		void takeDamage(int amount)
		{
			if (mbInvincible) return;
			if (mDamageTimer > 0) return;
			mHP -= amount;
			if (mHP < 0) mHP = 0;
			mDamageFlashTimer = 0.2f;
			mDamageTimer = 0.2f; // Reduced invincibility for better feedback
		}

		int getLevel() { return mLevel; }

		void handleCollisionEnter(IPhysicsEntity* other, int category) override 
		{
			if (category == Category_Monster) takeDamage(20);
		}
		void handleCollisionStay(IPhysicsEntity* other, int category) override 
		{ 
			if (category == Category_Monster) takeDamage(20);
		}
		void handleCollisionExit(IPhysicsEntity* other, int category) override {}

		void handleCollisionEnter(IPhysicsBullet* bullet, int category) override 
		{
			if (category == Category_MonsterBullet)
			{
				takeDamage(10);
			}
		}
		void handleCollisionStay(IPhysicsBullet* bullet, int category) override {}
		void handleCollisionExit(IPhysicsBullet* bullet, int category) override {}

		Render::RHITexture2DRef mTexture;
		Vector2 mTextureSize;

		void update(float dt);
		void updateOrbs(float dt);
		void fireBullet();
		void performSlash(Vector2 const& mousePos);
		void performShockwave();
		void reset();
	};

	class SurvivorsStage : public StageBase, public IGameRenderSetup
	{
		using BaseClass = StageBase;
	public:
		SurvivorsStage() : mParallelManager(mThreadPool) {}

		Vector2 getHeroPos() const { return mHero->mPos; }
		ParallelCollisionManager& getParallelManager() { return mParallelManager; }
		void addMonster(std::unique_ptr<Monster> m) { mMonsters.push_back(std::move(m)); }
		void addBullet(std::unique_ptr<GameBullet> b) { mBullets.push_back(std::move(b)); }
		void addVisual(VisualEffect const& ve) { mVisuals.push_back(ve); }
		const std::vector<std::unique_ptr<Monster>>& getMonsters() const { return mMonsters; }

		bool onInit() override
		{
			if (!BaseClass::onInit()) return false;

			mScreenSize = ::Global::GetScreenSize();
			mHero = std::make_unique<Hero>(Vector2(mScreenSize.x / 2.0f, mScreenSize.y / 2.0f), this);
			mCameraPos = mHero->mPos;

			// Load Hero Texture
			mHero->mTexture = RHIUtility::LoadTexture2DFromFile("Survivor/Hero.png", TextureLoadOption().AutoMipMap());
			if (mHero->mTexture.isValid())
			{
				mHero->mTextureSize = Vector2(mHero->mTexture->getSizeX(), mHero->mTexture->getSizeY());
			}

			mParallelManager.init(Vector2(-2000, -2000), Vector2(mScreenSize.x + 2000, mScreenSize.y + 2000), 32.0f);
			
			// Load Map Texture
			mMapTexture = RHIUtility::LoadTexture2DFromFile("Survivor/Map.png", TextureLoadOption().AutoMipMap());

			// Load Monster Textures
			mMonsterTextures[(int)MonsterType::Golem] = RHIUtility::LoadTexture2DFromFile("Survivor/Golem.png");
			mMonsterTextures[(int)MonsterType::Slime] = RHIUtility::LoadTexture2DFromFile("Survivor/Slime.png");

			// Process monster textures with Color Key if they were loaded manually (or fallback to procedural)
			auto processTexture = [&](Render::RHITexture2DRef& tex, char const* path, Color4ub fallbackColor)
			{
				ImageData imageData;
				if (imageData.load(path, ImageLoadOption().UpThreeComponentToFour()))
				{
					Color4ub* pPixel = (Color4ub*)imageData.data;
					for (int i = 0; i < imageData.width * imageData.height; ++i)
					{
						if (pPixel[i].r > 240 && pPixel[i].g > 240 && pPixel[i].b > 240) pPixel[i].a = 0;
					}
					int numMip = RHIUtility::CalcMipmapLevel(std::max(imageData.width, imageData.height));
					tex = RHICreateTexture2D(ETexture::RGBA8, imageData.width, imageData.height, numMip, 1, TCF_DefalutValue | TCF_GenerateMips, imageData.data);
				}
				else if (!tex.isValid())
				{
					// Simple procedural monster fallback
					int w = 32, h = 32;
					TArray<Color4ub> pixels; pixels.resize(w * h);
					for (int i = 0; i < w * h; ++i) pixels[i] = fallbackColor;
					tex = RHICreateTexture2D(ETexture::RGBA8, w, h, 0, 1, TCF_DefalutValue, pixels.data());
				}
			};

			processTexture(mMonsterTextures[(int)MonsterType::Golem], "Survivor/Golem.png", Color4ub(150, 100, 50, 255));
			processTexture(mMonsterTextures[(int)MonsterType::Slime], "Survivor/Slime.png", Color4ub(100, 255, 100, 255));
			processTexture(mHeroBulletTexture, "Survivor/Knife.png", Color4ub(255, 255, 255, 255));
			processTexture(mMonsterBulletTexture, "Survivor/MagicOrb.png", Color4ub(200, 100, 255, 255));
			processTexture(mOrbBulletTexture, "Survivor/Fireball.png", Color4ub(255, 150, 50, 255));
			
			{
				CollisionEntityData data;
				data.position = mHero->mPos;
				data.radius = mHero->mRadius;
				data.velocity = Vector2::Zero();
				data.mass = 10000.0f;
				data.isStatic = false;
				data.bQueryCollision = true;
				data.categoryId = Category_Hero;
				data.targetMask = Category_Monster | Category_MonsterBullet;
				mParallelManager.registerEntity(mHero.get(), data);
			}

			mParallelManager.addWall(Vector2(-500, -500), Vector2(mScreenSize.x + 500, -400));
			mParallelManager.addWall(Vector2(-500, mScreenSize.y + 400), Vector2(mScreenSize.x + 500, mScreenSize.y + 500));
			mParallelManager.addWall(Vector2(-500, -500), Vector2(-400, mScreenSize.y + 500));
			mParallelManager.addWall(Vector2(mScreenSize.x + 400, -500), Vector2(mScreenSize.x + 500, mScreenSize.y + 500));

			mThreadPool.init(std::max(1u, (uint32)4));
			mParallelManager.getSolver().settings.frameTargetSubStep = 1.0f / 120.0f;

			::Global::GUI().cleanupWidget();
			DevFrame* frame = WidgetUtility::CreateDevFrame();
			
			// Use FWidgetProperty::Bind or manual binding if WidgetUtility::Bind is unavailable
			// Based on WidgetUtility.h reading, FWidgetProperty::Bind exists, but maybe not WidgetUtility::Bind?
			// Checking WidgetUtility.h again...
			// Oops, they are static members of FWidgetProperty. 
			
			FWidgetProperty::Bind(frame->addSlider("Spawn Rate"), mSpawnRate, 0.5f, 100.0f);
			FWidgetProperty::Bind(frame->addSlider("Bullet Rate"), mHero->mBulletRate, 1.0f, 20.0f);
			FWidgetProperty::Bind(frame->addSlider("Monster Fire Rate"), mMonsterFireRate, 0.1f, 10.0f);
			
			frame->addCheckBox("Enable Orbs (Q)", mHero->mEnableOrbs);
			
			FWidgetProperty::Bind(frame->addSlider("Iterations"), mParallelManager.getSolver().settings.iterations, 1, 20);

			frame->addCheckBox("Show Grid", mShowCells);
			frame->addCheckBox("Show Entities", mShowEntities);
			frame->addCheckBox("Show Bullets", mShowBullets);
			frame->addCheckBox("Show Events", mShowEvents);
			frame->addCheckBox("Show Walls", mShowWalls);
			FWidgetProperty::Bind(frame->addSlider("Golem Draw Scale"), mMonsterDrawScales[(int)MonsterType::Golem], 1.0f, 10.0f);
			FWidgetProperty::Bind(frame->addSlider("Slime Draw Scale"), mMonsterDrawScales[(int)MonsterType::Slime], 1.0f, 10.0f);


			return true;
		}

		void onEnd() override
		{
			if (mHero) mHero->mOrbBullets.clear();
			mParallelManager.cleanup();
			mMonsters.clear();
			mBullets.clear();
			mThreadPool.waitAllWorkComplete();
			BaseClass::onEnd();
		}

		void spawnMonster()
		{
			Vector2 pos;
			float side = RandFloat(0, 4);
			float margin = 150.0f;
			if (side < 1) pos = Vector2(RandFloat(0, mScreenSize.x), -margin);
			else if (side < 2) pos = Vector2(RandFloat(0, mScreenSize.x), mScreenSize.y + margin);
			else if (side < 3) pos = Vector2(-margin, RandFloat(0, mScreenSize.y));
			else pos = Vector2(mScreenSize.x + margin, RandFloat(0, mScreenSize.y));

			MonsterType type = MonsterType::Slime;
			if (RandFloat(0, 1) < 0.01f) // 1% chance for Large Monster
			{
				type = MonsterType::Golem;
			}

			auto monster = std::make_unique<Monster>(pos, this, type);
			CollisionEntityData data;
			if (type == MonsterType::Golem)
			{
				monster->mRadius = 30.0f;
				monster->mHP = 500;
				data.mass = 20.0f;
			}
			else
			{
				data.mass = 1.0f;
			}

			data.position = pos;
			data.velocity = Vector2::Zero();
			data.isStatic = false;
			data.bQueryCollision = true;
			data.categoryId = Category_Monster;
			data.targetMask = Category_Hero | Category_HeroAbility | Category_Bullet;

			data.radius = monster->mRadius;
			data.mass = (monster->mType == MonsterType::Golem) ? 20.0f : 1.0f;
			mParallelManager.registerEntity(monster.get(), data);
			mMonsters.push_back(std::move(monster));
		}

		void monsterFire()
		{
			for (auto& m : mMonsters)
			{
				if (RandFloat(0, 1) < 0.02f)
				{
					Vector2 dir = mHero->mPos - m->mPos;
					dir.normalize();
					auto bullet = std::make_unique<GameBullet>(m->mPos, dir * 200.0f, Category_MonsterBullet);
					bullet->mRadius = 6.0f;
					CollisionBulletData bdata;
					bdata.position = bullet->mPos;
					bdata.velocity = bullet->mVel;
					bdata.categoryId = Category_MonsterBullet;
					bdata.targetMask = Category_Hero | Category_Bullet;
					bdata.bQueryBulletCollision = true;
					bdata.shapeType = ShapeType::Circle;
					bdata.shapeParams = Vector3(bullet->mRadius, 0, 0);
					mParallelManager.registerBullet(bullet.get(), bdata);
					mBullets.push_back(std::move(bullet));
				}
			}
		}

		void onUpdate(GameTimeSpan deltaTime) override
		{
			mParallelManager.beginFrame();


			{
				PROFILE_ENTRY("SyncGameFormPhysics");
				if (mHero && mHero->mPhysicsId != -1)
				{
					auto& data = mParallelManager.getEntityData(mHero->mPhysicsId);
					mHero->mPos = data.position;
				}
				for (auto& m : mMonsters)
				{
					auto& data = mParallelManager.getEntityData(m->mPhysicsId);
					m->mPos = data.position;
				}
				for (auto& b : mBullets)
				{
					auto& data = mParallelManager.getBulletData(b->mBulletId);
					b->mPos = data.position;
				}
			}


			PROFILE_ENTRY("Game Tick");
			float dt = deltaTime.value;

			if (mbPaused || (mHero && mHero->mHP <= 0))
			{
				// Keep camera moving even in pause/gameover but stop simulation
			}
			else
			{
				mSpawnTimer += dt;
				if (mSpawnRate > 0 && mSpawnTimer > 1.0f / mSpawnRate) { mSpawnTimer = 0; spawnMonster(); }

				mMonsterFireTimer += dt;
				if (mMonsterFireRate > 0 && mMonsterFireTimer > 1.0f / mMonsterFireRate) { mMonsterFireTimer = 0; monsterFire(); }

				Vector2 heroPos = mHero ? mHero->mPos : Vector2::Zero();

				// Update Logic
				if (mHero) mHero->update(dt);
				for (auto& m : mMonsters) m->update(heroPos, dt);
				for (auto& b : mBullets) b->update(dt);
			}

			// Update Camera (inertia follow) should stay outside of pause logic for smooth zoom/transition if any
			if (mHero)
			{
				float lerpSpeed = 1.0f - std::exp(-mCameraFollowSpeed * deltaTime.value);
				mCameraPos += (mHero->mPos - mCameraPos) * lerpSpeed;
			}
			{
				float zoomLerp = 1.0f - std::exp(-5.0f * deltaTime.value);
				mCameraZoom += (mCameraZoomTarget - mCameraZoom) * zoomLerp;
			}
			mWorldToScreen = RenderTransform2D::LookAt(Vector2(mScreenSize), mCameraPos, 0.0f, mCameraZoom);
			mScreenToWorld = mWorldToScreen.inverse();

			if (!mbPaused)
			{
				for (auto it = mVisuals.begin(); it != mVisuals.end();)
				{
					it->timer -= dt;
					if (it->type == VisualType::Slash && mHero) it->pos = mHero->mPos;
					if (it->timer <= 0) it = mVisuals.erase(it);
					else ++it;
				}

				// Sync FROM Objects TO Physics
				if (mHero && mHero->mPhysicsId != -1)
				{
					auto& data = mParallelManager.getEntityData(mHero->mPhysicsId);
					CHECK(mParallelManager.mReceivers[mHero->mPhysicsId].entity == mHero.get());
					data.position = mHero->mPos;
					data.radius = mHero->mRadius;
				}
				for (auto& m : mMonsters)
				{
					auto& data = mParallelManager.getEntityData(m->mPhysicsId);
					data.position = m->mPos;
					data.velocity = m->mVel;
				}
				for (auto& b : mBullets)
				{
					// Slash follows hero
					if (mHero && b->mCategory == Category_HeroAbility)
					{
						auto& data = mParallelManager.getBulletData(b->mBulletId);
						if (data.shapeType == ShapeType::Arc)
						{
							b->mPos = mHero->mPos;
						}
					}

					auto& data = mParallelManager.getBulletData(b->mBulletId);
					data.position = b->mPos;
					data.velocity = b->mVel;
					if (b->mVel.length2() > 0.001f)
						data.rotation = std::atan2(b->mVel.y, b->mVel.x);
				}


				// Clean up
				mMonsters.erase(std::remove_if(mMonsters.begin(), mMonsters.end(), [this](auto& m) {
					if (m->mIsDead) { 
						Gem gem;
						gem.pos = m->mPos;
						gem.value = (m->mType == MonsterType::Golem) ? 10 : 1;
						mGems.push_back(gem);
						mParallelManager.unregisterEntity(m.get()); 
						return true; 
					}
					return false;
				}), mMonsters.end());

				mBullets.erase(std::remove_if(mBullets.begin(), mBullets.end(), [this](auto& b) {
					if (b->mIsDead) { mParallelManager.unregisterBullet(b.get()); return true; }
					return false;
				}), mBullets.end());

				// Gems Update
				if (mHero)
				{
					for (auto& gem : mGems)
					{
						float distSq = (gem.pos - mHero->mPos).length2();
						if (distSq < Square(200.0f)) // Sucking range
						{
							Vector2 dir = mHero->mPos - gem.pos;
							gem.pos += dir * (8.0f * dt); // Simple magnetic movement
							if (distSq < Square(mHero->mRadius + 10.0f))
							{
								mHero->mXP += gem.value;
								gem.isDead = true;
								while (mHero->mXP >= mHero->mNextLevelXP)
								{
									mHero->mXP -= mHero->mNextLevelXP;
									mHero->mLevel++;
									mHero->mNextLevelXP = (int)(mHero->mNextLevelXP * 1.3f);
									VisualEffect ve;
									ve.pos = mHero->mPos;
									ve.timer = ve.duration = 0.5f;
									ve.type = VisualType::Shockwave;
									ve.radius = 400.0f;
									mVisuals.push_back(ve);
									
									// Level Up Benefits
									mHero->mHP = std::min(mHero->mMaxHP, mHero->mHP + (int)(mHero->mMaxHP * 0.2f)); // Heal 20%
									mHero->mBulletRate *= 1.1f; // Fire restriction lifted a bit
									if (mHero->mLevel % 5 == 0 && mHero->mOrbCount < 24) // Cap at 24
									{
										mHero->mOrbCount++;
										// Properly unregister before clearing to avoid physics ghosts
										for (auto& b : mHero->mOrbBullets) 
											mParallelManager.unregisterBullet(b.get());
										mHero->mOrbBullets.clear(); 
									}
									ve.radius = 400.0f;
									mVisuals.push_back(ve);
								}
							}
						}
					}
					mGems.erase(std::remove_if(mGems.begin(), mGems.end(), [](auto& g) { return g.isDead; }), mGems.end());
				}

				mParallelManager.endFrame(dt);
			}
		}

		void onRender(float dFrame) override
		{
			RHIGraphics2D& g = Global::GetRHIGraphics2D();
			RHICommandList& commandList = g.getCommandList(); // Render namespace needed
			RHISetViewport(commandList, 0, 0, (float)mScreenSize.x, (float)mScreenSize.y);
			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.04f, 0.04f, 0.08f, 1), 1);

			g.beginRender();

			// Apply camera transform
			g.transformXForm(mWorldToScreen, true);

			// Compute visible world-space bounds for culling
			Vector2 viewMin = mScreenToWorld.transformPosition(Vector2::Zero());
			Vector2 viewMax = mScreenToWorld.transformPosition(Vector2(mScreenSize));
			if (viewMin.x > viewMax.x) std::swap(viewMin.x, viewMax.x);
			if (viewMin.y > viewMax.y) std::swap(viewMin.y, viewMax.y);



			// 2. Tiled Map
			if (mMapTexture.isValid())
			{
				g.setBlendState(ESimpleBlendMode::None);
				float tileSize = 256.0f; // World size per tile
				int xStart = (int)std::floor(viewMin.x / tileSize);
				int yStart = (int)std::floor(viewMin.y / tileSize);
				int xEnd = (int)std::ceil(viewMax.x / tileSize);
				int yEnd = (int)std::ceil(viewMax.y / tileSize);

				g.setBrush(Color3f::White());
				g.setTexture(const_cast<RHITexture2D&>(*mMapTexture));
				g.setSampler(TStaticSamplerState<ESampler::Trilinear>::GetRHI());


				Vector2 startPos(xStart * tileSize, yStart * tileSize);
				Vector2 totalSize((xEnd - xStart + 1) * tileSize, (yEnd - yStart + 1) * tileSize);
				Vector2 startUV(xStart, yStart);
				Vector2 sizeUV(xEnd - xStart + 1, yEnd - yStart + 1);

				g.drawTexture(startPos, totalSize, startUV, sizeUV);
			}
			
			// 1. Draw Gems
			g.setBrush(Color3f(0.2f, 1.0f, 0.5f));
			g.enablePen(false);
			for (auto& gem : mGems)
			{
				if (gem.pos.x < viewMin.x - 5 || gem.pos.x > viewMax.x + 5 ||
					gem.pos.y < viewMin.y - 5 || gem.pos.y > viewMax.y + 5)
					continue;
				g.drawCircle(gem.pos, 3.0f);
			}
			g.enablePen(true);

			// 2. Draw Sorted Monsters
			{
				std::vector<Monster*> visibleMonsters;
				for (auto& m : mMonsters)
				{
					if (m->mPos.x < viewMin.x - m->mRadius || m->mPos.x > viewMax.x + m->mRadius ||
						m->mPos.y < viewMin.y - m->mRadius || m->mPos.y > viewMax.y + m->mRadius)
						continue;
					visibleMonsters.push_back(m.get());
				}

				// Optional: Sort by Y for 45-degree depth perception if needed within batches
				std::sort(visibleMonsters.begin(), visibleMonsters.end(), [](Monster* a, Monster* b) {
					return a->mPos.y < b->mPos.y;
				});

				// Group by type for instanced rendering
				std::vector<Monster*> monstersByType[(int)MonsterType::Count];
				for (auto* m : visibleMonsters)
				{
					monstersByType[(int)m->mType].push_back(m);
				}

				for (int i = 0; i < (int)MonsterType::Count; ++i)
				{
					auto& typeMonsters = monstersByType[i];
					if (typeMonsters.empty()) continue;

					auto const& tex = mMonsterTextures[i];
					if (tex.isValid())
					{
						std::vector<MonsterInstanceData> instanceData;
						instanceData.reserve(typeMonsters.size());
						for (auto* m : typeMonsters)
						{
							float drawScale = mMonsterDrawScales[i];
							Vector2 spriteSize = Vector2(m->mRadius * 2, m->mRadius * 2);
							MonsterInstanceData data;
							data.pos = m->mPos;
							data.size = spriteSize;
							data.uvPos = Vector2(0, 0);
							data.uvSize = Vector2(1, 1);
							Vector2 toHero = mHero ? (mHero->mPos - m->mPos) : Vector2::Zero();
							if (toHero.x > 0) { data.uvPos.x = 1; data.uvSize.x = -1; }
							data.scale = m->mScale * drawScale;
							// Simple stun/hit flash color
							data.color = (m->mStunTimer > 0) ? LinearColor(5, 5, 5, 1) : LinearColor(1, 1, 1, 1);
							instanceData.push_back(data);
						}

						g.drawCustomFunc([&tex, data = std::move(instanceData)](RHICommandList& commandList, Matrix4 const& baseTransform, RenderBatchedElement& element)
						{
							MonsterShaderProgram* shader = ShaderManager::Get().getGlobalShaderT<MonsterShaderProgram>();
							if (shader)
							{
								RHISetShaderProgram(commandList, shader->getRHI());
								shader->setWorldToClip(commandList, element.transform.toMatrix4() * baseTransform);
								shader->bindMonsterTexture(commandList, const_cast<RHITexture2D&>(*tex));
								RHISetBlendState(commandList, StaticTranslucentBlendState::GetRHI());
								RHISetDepthStencilState(commandList, TStaticDepthStencilState<true, ECompareFunc::LessEqual>::GetRHI());
								RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());

								VertexDataInfo info;
								info.ptr = data.data();
								info.size = (int)(data.size() * sizeof(MonsterInstanceData));
								info.stride = sizeof(MonsterInstanceData);

								RHISetInputStream(commandList, &MonsterInputLayout::GetRHI(), nullptr, 0);
								RHIDrawPrimitiveUP(commandList, EPrimitive::Points, (int)data.size(), &info, 1);
							}
						});
					}
					else
					{
						for (auto* m : typeMonsters)
						{
							if (m->mStunTimer > 0) RenderUtility::SetBrush(g, EColor::White);
							else RenderUtility::SetBrush(g, EColor::Red);
							g.drawCircle(m->mPos, m->mRadius);
						}
					}
				}
			}

			// 3. Draw Hero (Last to stay on top of monsters as requested)
			if (mHero)
			{
				if (mHero->mTexture.isValid())
				{
					g.setBlendState(ESimpleBlendMode::Translucent);
					Vector2 spriteSize = Vector2(60, 60);
					Vector2 pos = mHero->mPos - 0.5f * spriteSize;
					
					Vector2 uvPos(0, 0); Vector2 uvSize(1, 1);
					if (mHero->mFacing.x > 0) { uvPos.x = 1; uvSize.x = -1; }

					if (mHero->mDamageFlashTimer > 0) g.setTextColor(Color3f(1, 0, 0));
					
					g.setBrush(Color3f::White());
					g.setTexture(const_cast<RHITexture2D&>(*mHero->mTexture));
					g.setSampler(TStaticSamplerState<ESampler::Trilinear>::GetRHI());
					g.drawTexture(pos, spriteSize, uvPos, uvSize);
				}
				else
				{
					if (mHero->mDamageFlashTimer > 0) RenderUtility::SetBrush(g, EColor::Red);
					else RenderUtility::SetBrush(g, EColor::Green);
					g.drawCircle(mHero->mPos, mHero->mRadius);
				}

				// Draw Health Bar
				{
					float barWidth = 60.0f;
					float barHeight = 6.0f;
					Vector2 barPos = mHero->mPos + Vector2(-barWidth * 0.5f, mHero->mRadius + 10.0f);
					g.setPen(Color4f(0,0,0,1), 1);
					g.setBrush(Color4f(0,0,0,1));
					g.drawRect(barPos - Vector2(1, 1), Vector2(barWidth + 2, barHeight + 2));
					g.enablePen(false);
					g.setBrush(Color3f(0.3f, 0.1f, 0.1f));
					g.drawRect(barPos, Vector2(barWidth, barHeight));
					float hpRange = (float)mHero->mHP / mHero->mMaxHP;
					g.setBrush(Color3f(1.0f - hpRange, hpRange, 0.0f));
					g.drawRect(barPos, Vector2(barWidth * hpRange, barHeight));
					g.enablePen(true);
				}

				RenderUtility::SetBrush(g, EColor::Cyan);
				for (auto& b : mHero->mOrbBullets)
				{
					if (mOrbBulletTexture.isValid())
					{
						Vector2 dir = b->mPos - mHero->mPos;
						// Tangent direction for orbital motion (CCW)
						float angle = Math::ATan2(dir.y, dir.x) + 0.5f * Math::PI;
						// Adjust by 90 degrees if the tail was pointing outwards
						float visualAngle = angle - Math::DegToRad(90.0f);

						g.pushXForm();
						g.translateXForm(b->mPos.x, b->mPos.y);
						g.rotateXForm(visualAngle);
						g.setBrush(Color3f::White());
						g.setTexture(const_cast<RHITexture2D&>(*mOrbBulletTexture));
						g.drawTexture(Vector2(-b->mRadius * 1.8f, -b->mRadius * 1.8f), Vector2(b->mRadius * 3.6f, b->mRadius * 3.6f));
						g.popXForm();
					}
					else
					{
						RenderUtility::SetBrush(g, EColor::Cyan);
						g.drawCircle(b->mPos, b->mRadius);
					}
				}
			}

			// 4. Draw Bullets
			for (auto& b : mBullets)
			{
				if (b->mPos.x < viewMin.x - b->mRadius || b->mPos.x > viewMax.x + b->mRadius ||
					b->mPos.y < viewMin.y - b->mRadius || b->mPos.y > viewMax.y + b->mRadius)
					continue;

				RHITexture2D* tex = nullptr;
				float offsetDeg = 0;
				if (b->mCategory == Category_Bullet) { tex = mHeroBulletTexture.get(); offsetDeg = 135.0f; }
				else if (b->mCategory == Category_MonsterBullet) { tex = mMonsterBulletTexture.get(); offsetDeg = 0.0f; }

				if (tex)
				{
					float angle = Math::ATan2(b->mVel.y, b->mVel.x);
					float visualAngle = angle - Math::DegToRad(offsetDeg);

					float visualScale = (b->mCategory == Category_Bullet) ? 0.8f : 2.5f;

					g.pushXForm();
					g.translateXForm(b->mPos.x, b->mPos.y);
					g.rotateXForm(visualAngle);
					g.setBrush(Color3f::White());
					g.setTexture(*tex);
					g.drawTexture(Vector2(-b->mRadius * visualScale, -b->mRadius * visualScale), Vector2(b->mRadius * 2 * visualScale, b->mRadius * 2 * visualScale));
					g.popXForm();
				}
				else
				{
					if (b->mCategory == Category_Bullet) { g.setBrush(Color3f(1, 1, 0)); g.setPen(Color3f(1, 1, 0)); }
					else { g.setBrush(Color3f(1, 0.5f, 0)); g.setPen(Color3f(1, 0.5f, 0)); }
					g.drawCircle(b->mPos, b->mRadius);
				}
			}

            
			for (auto& v : mVisuals)
			{
				float alpha = v.timer / v.duration;
				if (v.type == VisualType::Slash)
				{
					g.setPen(Color3f(1, 0.9f, 0.2f), 4);
					float fov = Math::DegToRad(v.angle);
					g.drawArcLine(v.pos, v.radius, v.rotation - fov * 0.5f, fov);
				}
				else if (v.type == VisualType::Shockwave)
				{
					float curR = v.radius * (1.0f - alpha);
					g.enablePen(false);
					g.enableBrush(true);
					g.beginBlend(alpha * 0.5f);
					g.setBrush(Color3f(0.2f, 0.8f, 1.0f));
					g.drawCircle(v.pos, curR);
					g.endBlend();
					g.enablePen(true);
				}
				else if (v.type == VisualType::HitSpark)
				{
					g.setPen(Color3f(1, 1, 1), 1);
					g.setBrush(Color3f(1, 1, 1));
					g.drawCircle(v.pos, 5.0f * alpha);
				}
			}

			// Debug Drawing
			auto& solver = mParallelManager.getSolver();
			if (mShowCells)
			{
				float cellSize = solver.settings.cellSize;
				Vector2 minBound = solver.grid.minBound;
				int width = solver.grid.gridWidth;
				int height = solver.grid.gridHeight;

				g.setPen(Color3f(0, 1, 0), 1);
				g.enableBrush(false);
				for (int i = 0; i <= width; ++i)
				{
					float x = minBound.x + i * cellSize;
					g.drawLine(Vector2(x, minBound.y), Vector2(x, minBound.y + height * cellSize));
				}
				for (int i = 0; i <= height; ++i)
				{
					float y = minBound.y + i * cellSize;
					g.drawLine(Vector2(minBound.x, y), Vector2(minBound.x + width * cellSize, y));
				}
			}

			if (mShowEntities)
			{
				g.setPen(Color3f(0, 0, 1), 1);
				g.enableBrush(false);
				for (int i = 0; i < solver.mEntityCount; ++i)
				{
					g.drawCircle(solver.positions[i], solver.radii[i]);
				}
			}

			if (mShowBullets)
			{
				g.setPen(Color3f(1, 1, 0), 1);
				g.enableBrush(false);
				for (int i = 0; i < solver.mBulletCount; ++i)
				{
					Vector2 pos = solver.bulletPositions[i];
					ShapeType type = solver.bulletShapeTypes[i];
					Vector3 params = solver.bulletShapeParams[i];
					float rot = solver.bulletRotations[i];

					switch (type)
					{
					case ShapeType::Circle:
						g.drawCircle(pos, params.x);
						break;
					case ShapeType::Rect:
						{
							Vector2 halfSize = Vector2(params.x, params.y) * 0.5f;
							Vector2 corners[4] = {
								Vector2(-halfSize.x, -halfSize.y),
								Vector2(halfSize.x, -halfSize.y),
								Vector2(halfSize.x, halfSize.y),
								Vector2(-halfSize.x, halfSize.y)
							};
							float c = std::cos(rot), s = std::sin(rot);
							for (int k = 0; k < 4; ++k)
							{
								float nx = corners[k].x * c - corners[k].y * s;
								float ny = corners[k].x * s + corners[k].y * c;
								corners[k] = pos + Vector2(nx, ny);
							}
							g.drawPolygon(corners, 4);
						}
						break;
					case ShapeType::Arc:
						{
							g.drawArcLine(pos, params.x, rot - params.y * 0.5f, params.y);
							float startAngle = rot - params.y * 0.5f;
							float endAngle = rot + params.y * 0.5f;
							Vector2 p1(Math::Cos(startAngle), Math::Sin(startAngle));
							Vector2 p2(Math::Cos(endAngle), Math::Sin(endAngle));
							g.drawLine(pos, pos + p1 * params.x);
							g.drawLine(pos, pos + p2 * params.x);
						}
						break;
					}
				}
			}

			if (mShowEvents)
			{
				g.setPen(Color3f(1, 0, 0), 1);
				g.enableBrush(false);
				for (int id : mParallelManager.mLastActiveEntityIndices)
				{
					if (id < 0 || id >= mParallelManager.mEntityCount) continue;
					auto const& rec = mParallelManager.mReceivers[id];
					Vector2 posA = mParallelManager.mEntityDataBuffer[id].position;
					if (rec.eventData)
					{
						for (int otherHandle : rec.eventData->prevEntities)
						{
							IPhysicsEntity* other = mParallelManager.getEntityFromHandle(otherHandle);
							if (other && other->mPhysicsId >= 0 && other->mPhysicsId < mParallelManager.mEntityCount)
							{
								g.drawLine(posA, mParallelManager.mEntityDataBuffer[other->mPhysicsId].position);
							}
						}
						for (int bHandle : rec.eventData->prevBullets)
						{
							IPhysicsBullet* bullet = mParallelManager.getBulletFromHandle(bHandle);
							if (bullet && bullet->mBulletId >= 0 && bullet->mBulletId < mParallelManager.mBulletCount)
							{
								g.drawLine(posA, mParallelManager.mBulletDataBuffer[bullet->mBulletId].position);
							}
						}
					}
				}
				for (int id : mParallelManager.mLastActiveBulletIndices)
				{
					if (id < 0 || id >= mParallelManager.mBulletCount) continue;
					auto const& rec = mParallelManager.mBulletReceivers[id];
					Vector2 posA = mParallelManager.mBulletDataBuffer[id].position;
					if (rec.eventData)
					{
						for (int otherBHandle : rec.eventData->prevBullets)
						{
							IPhysicsBullet* bullet = mParallelManager.getBulletFromHandle(otherBHandle);
							if (bullet && bullet->mBulletId >= 0 && bullet->mBulletId < mParallelManager.mBulletCount)
							{
								g.drawLine(posA, mParallelManager.mBulletDataBuffer[bullet->mBulletId].position);
							}
						}
					}
				}
			}

			if (mShowWalls)
			{
				g.setPen(Color3f(1, 1, 0), 1);
				g.setBrush(Color3f(1, 1, 0));
				for (auto const& wall : solver.walls)
				{
					g.enableBrush(true);
					g.enablePen(false);
					g.beginBlend(0.15f);
					g.drawRect(wall.min, wall.max - wall.min);
					g.endBlend();
					g.enableBrush(false);
					g.enablePen(true);
					g.drawRect(wall.min, wall.max - wall.min);
				}
				g.enableBrush(true);
				g.enablePen(true);
			}
			
			// Switch to screen space for UI overlay
			g.pushXForm();
			g.identityXForm();

			// Draw Virtual Joystick
			if (mVJoystickActive)
			{
				float outerRadius = mVJoystickRadius;
				float innerRadius = 12.0f;

				// Outer ring
				g.enableBrush(false);
				g.setPen(Color4f(1, 1, 1, 0.4f), 2);
				g.drawCircle(mVJoystickCenter, outerRadius);

				// Inner knob
				Vector2 offset = mVJoystickCurrentPos - mVJoystickCenter;
				float dist = offset.length();
				if (dist > outerRadius)
					offset = offset * (outerRadius / dist);

				g.enableBrush(true);
				g.setBrush(Color4f(1, 1, 1, 0.5f));
				g.setPen(Color4f(1, 1, 1, 0.8f), 1);
				g.drawCircle(mVJoystickCenter + offset, innerRadius);
			}

			g.setTextColor(Color3ub(255, 255, 255));
			InlineString<512> info;
			info.format("Hero HP: %d | Monsters: %d | Bullets: %d %s\nCollision: %.3f ms | Iters: %d\n[WASD] Move | [G] GodMode | [Q] Orbs | [P] Pause | [R-Click] Slash | [M-Click] Shockwave\n[R] Reset Simulation", 
				mHero ? mHero->mHP : 0, (int)mMonsters.size(), (int)mBullets.size() + (mHero ? (int)mHero->mOrbBullets.size() : 0),
				(mHero && mHero->mbInvincible) ? "[GOD MODE ON]" : "",
				mParallelManager.getLastSolveTime(), mParallelManager.getSolver().settings.iterations);
			g.drawText(20, 20, info);

			if (mHero && mHero->mHP <= 0)
			{
				g.identityXForm();
				g.setTextColor(Color3ub(255, 50, 50));
				g.drawText(Vector2(mScreenSize.x * 0.5f - 60, mScreenSize.y * 0.5f - 30), "GAME OVER");
				g.setTextColor(Color3ub(255, 255, 255));
				g.drawText(Vector2(mScreenSize.x * 0.5f - 80, mScreenSize.y * 0.5f + 10), "Press 'R' to Restart");
			}
			else if (mbPaused)
			{
				g.identityXForm();
				g.setTextColor(Color3ub(255, 100, 100));
				g.drawText(Vector2(mScreenSize.x * 0.5f - 50, mScreenSize.y * 0.5f - 20), "GAME PAUSED");
			}

			// Draw Level & XP Bar
			if (mHero)
			{
				g.identityXForm();
				g.setTextColor(Color3ub(50, 200, 255));
				InlineString<64> lvStr; lvStr.format("LV. %d", mHero->mLevel);
				g.drawText(20, mScreenSize.y - 55, lvStr);

				float xpW = (float)mHero->mXP / mHero->mNextLevelXP * (mScreenSize.x - 40);
				
				// 1. Background
				g.enablePen(false);
				g.enableBrush(true);
				g.setBrush(Color3f(0.05f, 0.05f, 0.05f));
				g.drawRect(Vector2(20, mScreenSize.y - 30), Vector2(mScreenSize.x - 40, 12));
				
				// 2. XP Fill
				g.setBrush(Color3f(0.2f, 0.7f, 1.0f));
				g.drawRect(Vector2(20, mScreenSize.y - 30), Vector2(xpW, 12));

				// 3. Border
				g.enablePen(true);
				g.enableBrush(false);
				g.setPen(Color4f(1, 1, 1, 0.8f), 1);
				g.drawRect(Vector2(20, mScreenSize.y - 30), Vector2(mScreenSize.x - 40, 12));
				g.enableBrush(true);
			}

			g.popXForm();

			g.endRender();
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			if (mHero)
			{
				if (msg.onLeftDown())
				{
					mVJoystickActive = true;
					mVJoystickCenter = msg.getPos();
					mVJoystickCurrentPos = msg.getPos();
					mHero->mJoystickDir = Vector2::Zero();
				}
				else if (msg.onLeftUp())
				{
					mVJoystickActive = false;
					mHero->mJoystickDir = Vector2::Zero();
				}
				else if (msg.onMoving() && mVJoystickActive && msg.isLeftDown())
				{
					mVJoystickCurrentPos = msg.getPos();
					Vector2 offset = Vector2(msg.getPos()) - mVJoystickCenter;
					float dist = offset.length();
					if (dist > 5.0f)
					{
						float t = std::min(dist / mVJoystickRadius, 1.0f);
						mHero->mJoystickDir = (offset / dist) * t;
					}
					else
					{
						mHero->mJoystickDir = Vector2::Zero();
					}
				}

				if (msg.onWheelFront())
				{
					mCameraZoomTarget *= 1.1f;
					mCameraZoomTarget = std::min(mCameraZoomTarget, 5.0f);
				}
				else if (msg.onWheelBack())
				{
					mCameraZoomTarget *= 0.9f;
					mCameraZoomTarget = std::max(mCameraZoomTarget, 0.2f);
				}

				if (msg.onRightDown())
				{
					Vector2 worldPos = mWorldToScreen.transformInvPosition(Vector2(msg.getPos()));
					mHero->performSlash(worldPos);
				}
				if (msg.onMiddleDown()) mHero->performShockwave();
			}
			return BaseClass::onMouse(msg);
		}
		
		Vector2 getHeroPos() { return mHero ? mHero->mPos : Vector2::Zero(); }
		int getHeroLevel() { return mHero ? mHero->getLevel() : 1; }



		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::Q: if (mHero) mHero->mEnableOrbs = !mHero->mEnableOrbs; break;
				case EKeyCode::G: if (mHero) mHero->mbInvincible = !mHero->mbInvincible; break;
				case EKeyCode::P: mbPaused = !mbPaused; break;
				case EKeyCode::Space:
					for (int i = 0; i < 100; ++i) spawnMonster();
					break;
				case EKeyCode::R:
					mMonsters.clear();
					mBullets.clear();
					mVisuals.clear();
					if (mHero) mHero->reset();
					mParallelManager.cleanup();
					if (mHero)
					{
						CollisionEntityData data;
						data.position = mHero->mPos;
						data.radius = mHero->mRadius;
						data.mass = 10000.0f;
						data.categoryId = Category_Hero;
						data.bQueryCollision = true;
						data.targetMask = Category_Monster | Category_MonsterBullet;
						mParallelManager.registerEntity(mHero.get(), data);
					}
					break;
				}
			}
			return BaseClass::onKey(msg);
		}

		ERenderSystem getDefaultRenderSystem() override { return ERenderSystem::None; }
		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) 
		{ 
			systemConfigs.screenWidth = 1280; systemConfigs.screenHeight = 720; 
		}

	private:
		QueueThreadPool mThreadPool;
		ParallelCollisionManager mParallelManager;
		std::unique_ptr<Hero> mHero;
		std::vector<std::unique_ptr<Monster>> mMonsters;
		std::vector<std::unique_ptr<GameBullet>> mBullets;
		std::vector<Gem> mGems;
		std::vector<VisualEffect> mVisuals;

		RenderTransform2D mWorldToScreen;
		RenderTransform2D mScreenToWorld;

		Vec2i mScreenSize;
		float mSpawnRate = 5.0f; // Actions per sec
		float mSpawnTimer = 0;
		float mMonsterFireRate = 0.5f; // Actions per sec
		float mMonsterFireTimer = 0;

		bool mShowCells = false;
		bool mShowEntities = false;
		bool mShowBullets = false;
		bool mShowEvents = false;
		bool mShowWalls = false;
		bool mbPaused = false;

		// Virtual Joystick
		bool mVJoystickActive = false;
		Vector2 mVJoystickCenter;
		Vector2 mVJoystickCurrentPos;
		float mVJoystickRadius = 60.0f;

		// Camera
		Vector2 mCameraPos = Vector2::Zero();
		float mCameraZoom = 1.0f;
		float mCameraZoomTarget = 1.0f;
		float mCameraFollowSpeed = 4.0f;

		float mMonsterDrawScales[(int)MonsterType::Count] = { 1.5f , 2.2f };

		Render::RHITexture2DRef mMapTexture;
		Render::RHITexture2DRef mHeroBulletTexture;
		Render::RHITexture2DRef mMonsterBulletTexture;
		Render::RHITexture2DRef mOrbBulletTexture;
		Render::RHITexture2DRef mMonsterTextures[(int)MonsterType::Count];
	};

	// Implement Monster methods after SurvivorsStage definition
	void Monster::handleCollisionEnter(IPhysicsEntity* other, int category) 
	{
		if (category == Category_HeroAbility)
		{
			// applyKnockback removed from Enter to reduce sensitivity
		}
		else if (category == Category_Hero)
		{
			static_cast<Hero*>(other)->takeDamage(20);
		}
	}

	void Monster::handleCollisionStay(IPhysicsEntity* other, int category) 
	{
		if (category == Category_Monster)
		{
			Monster* otherMonster = static_cast<Monster*>(other);
			float otherSpeedSq = otherMonster->mKnockbackVel.length2();
			float mySpeedSq = mKnockbackVel.length2();
			if (otherSpeedSq > 500.0f && otherSpeedSq > mySpeedSq)
			{
				mKnockbackVel = otherMonster->mKnockbackVel * 0.8f;
			}
		}
		else if (category == Category_Hero)
		{
			static_cast<Hero*>(other)->takeDamage(20);
		}
	}

	void Monster::handleCollisionEnter(IPhysicsBullet* bullet, int category) 
	{ 
		if (category == Category_Bullet || category == Category_HeroAbility)
		{
			// Damage scaling with level (roughly)
			int baseDmg = (category == Category_HeroAbility) ? 10 : 2;
			int levelBonus = mStage->getHeroLevel() / 2; 
			int dmg = baseDmg + levelBonus;
			mHP -= dmg;
			if (mHP <= 0) mIsDead = true;
			
			if (category == Category_HeroAbility)
			{
				applyKnockback(500.0f, 0.8f);
			}

			VisualEffect ve;
			ve.pos = mPos + Vector2(RandFloat(-5, 5), RandFloat(-5, 5));
			ve.timer = ve.duration = 0.1f;
			ve.type = VisualType::HitSpark;
			mStage->addVisual(ve);
		}
	}

	void Monster::applyKnockback(float force, float duration)
	{
		Vector2 pushDir = mPos - mStage->getHeroPos();
		if (pushDir.normalize() < 0.001f) pushDir = Vector2(1, 0);
		mKnockbackVel = pushDir * force;
		mStunTimer = duration;
	}

	// Hero Implementations
	void Hero::update(float dt)
	{
		// Movement
		Vector2 moveDir = Vector2::Zero();
		if (InputManager::Get().isKeyDown(EKeyCode::W)) moveDir.y -= 1;
		if (InputManager::Get().isKeyDown(EKeyCode::S)) moveDir.y += 1;
		if (InputManager::Get().isKeyDown(EKeyCode::A)) moveDir.x -= 1;
		if (InputManager::Get().isKeyDown(EKeyCode::D)) moveDir.x += 1;

		// Virtual joystick overrides keyboard if active
		if (mJoystickDir.length2() > 0.01f)
		{
			moveDir = mJoystickDir;
		}

		if (moveDir.length2() > 0.1f)
		{
			if (moveDir.length2() > 1.0f)
				moveDir.normalize();
			mFacing = moveDir;
			mPos += moveDir * 300.0f * dt;
			mMoveTarget = mPos;
		}

		// Fire Bullets
		mBulletTimer += dt;
		if (mBulletRate > 0 && mBulletTimer > 1.0f / mBulletRate) { mBulletTimer = 0; fireBullet(); }

		updateOrbs(dt);

		if (mDamageFlashTimer > 0) mDamageFlashTimer -= dt;
		if (mDamageTimer > 0) mDamageTimer -= dt;
	}

	void Hero::updateOrbs(float dt)
	{
		if (!mEnableOrbs)
		{
			for (auto& b : mOrbBullets) mStage->getParallelManager().unregisterBullet(b.get());
			mOrbBullets.clear();
			return;
		}

		if (mOrbBullets.empty())
		{
			for (int i = 0; i < mOrbCount; ++i)
			{
				auto bullet = std::make_unique<GameBullet>(mPos, Vector2::Zero(), Category_HeroAbility);
				bullet->mRadius = 25.0f; // Visual radius matches physics
				CollisionBulletData bdata;
				bdata.position = bullet->mPos;
				bdata.categoryId = Category_HeroAbility;
				bdata.targetMask = Category_Monster;
				bdata.bQueryBulletCollision = false;
				bdata.shapeType = ShapeType::Circle;
				bdata.shapeParams = Vector3(bullet->mRadius, 0, 0);
				mStage->getParallelManager().registerBullet(bullet.get(), bdata);
				mOrbBullets.push_back(std::move(bullet));
			}
		}

		mOrbRotation += 220.0f * dt;
		for (int i = 0; i < mOrbBullets.size(); ++i)
		{
			float angle = mOrbRotation + i * (360.0f / mOrbBullets.size());
			float rad = angle * Math::PI / 180.0f;
			Vector2 offset(Math::Cos(rad), Math::Sin(rad));
			mOrbBullets[i]->mPos = mPos + offset * 100.0f;
			auto& data = mStage->getParallelManager().getBulletData(mOrbBullets[i]->mBulletId);
			data.position = mOrbBullets[i]->mPos;
			data.rotation = rad + Math::PI / 2; // Face tangent
		}
	}

	void Hero::fireBullet()
	{
		const auto& monsters = mStage->getMonsters();
		if (monsters.empty()) return;

		Monster* nearest = nullptr;
		float minDist = 1e10f;
		for (auto& m : monsters)
		{
			float d = (m->mPos - mPos).length2();
			if (d < minDist) { minDist = d; nearest = m.get(); }
		}

		if (nearest)
		{
			Vector2 dir = nearest->mPos - mPos;
			dir.normalize();
			auto bullet = std::make_unique<GameBullet>(mPos, dir * 600.0f, Category_Bullet);
			bullet->mRadius = 18.0f; // Larger visual and collision size
			CollisionBulletData bdata;
			bdata.position = bullet->mPos;
			bdata.velocity = bullet->mVel;
			bdata.rotation = std::atan2(dir.y, dir.x);
			bdata.categoryId = Category_Bullet;
			bdata.targetMask = Category_Monster | Category_MonsterBullet;
			bdata.bQueryBulletCollision = true;
			bdata.shapeType = ShapeType::Rect;
			bdata.shapeParams = Vector3(20, 5, 0); // Half-extent X, Y
			mStage->getParallelManager().registerBullet(bullet.get(), bdata);
			mStage->addBullet(std::move(bullet));
		}
	}

	void Hero::performSlash(Vector2 const& mousePos)
	{
		Vector2 dir = mousePos - mPos;
		float angle;
		if (dir.length2() < 25.0f) // 5px distance threshold
		{
			angle = std::atan2(mFacing.y, mFacing.x);
		}
		else
		{
			dir.normalize();
			mFacing = dir; // Face the slash direction
			angle = std::atan2(dir.y, dir.x);
		}

		auto bullet = std::make_unique<GameBullet>(mPos, Vector2::Zero(), Category_HeroAbility);
		bullet->mLifeTime = 0.12f;
		CollisionBulletData bdata;
		bdata.position = mPos;
		bdata.rotation = angle;
		bdata.categoryId = Category_HeroAbility;
		bdata.targetMask = Category_Monster;
		bdata.bQueryBulletCollision = false;
		bdata.shapeType = ShapeType::Arc;
		bdata.shapeParams = Vector3(120.0f, 60.0f * Math::PI / 180.0f, 0);
		mStage->getParallelManager().registerBullet(bullet.get(), bdata);
		mStage->addBullet(std::move(bullet));

		VisualEffect ve;
		ve.pos = mPos; ve.rotation = angle; ve.radius = 120.0f; ve.angle = 60.0f;
		ve.timer = ve.duration = 0.12f; ve.type = VisualType::Slash;
		mStage->addVisual(ve);
	}

	void Hero::performShockwave()
	{
		auto bullet = std::make_unique<GameBullet>(mPos, Vector2::Zero(), Category_HeroAbility);
		bullet->mLifeTime = 0.2f;
		CollisionBulletData bdata;
		bdata.position = mPos; 
		bdata.categoryId = Category_HeroAbility; bdata.targetMask = Category_Monster;
		bdata.bQueryBulletCollision = false; bdata.shapeType = ShapeType::Circle;
		bdata.shapeParams = Vector3(200.0f, 0, 0);
		mStage->getParallelManager().registerBullet(bullet.get(), bdata);
		mStage->addBullet(std::move(bullet));

		VisualEffect ve;
		ve.pos = mPos; ve.radius = 200.0f;
		ve.timer = ve.duration = 0.3f; ve.type = VisualType::Shockwave;
		mStage->addVisual(ve);
	}

	void Hero::reset()
	{
		mOrbBullets.clear();
		mBulletTimer = 0;
		mOrbRotation = 0;
		mDamageFlashTimer = 0;
		mDamageTimer = 0;
		mHP = 1000;
	}

	REGISTER_STAGE_ENTRY("Survivors Stage", SurvivorsStage, EExecGroup::Test, "Physics|Benchmark");
}
