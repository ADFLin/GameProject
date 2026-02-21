#include "SurvivorsRenderer.h"

#include "RHI/RHIGraphics2D.h"
#include "RHI/ShaderManager.h"
#include "RHI/RHIGlobalResource.h"
#include "RHI/RHIUtility.h"
#include "RenderUtility.h"
#include "Image/ImageData.h"
#include "GameGlobal.h"

namespace Survivors
{
	IMPLEMENT_SHADER_PROGRAM(MonsterShaderProgram);

	static void ProcessTextureWithColorKey(RHITexture2DRef& tex, char const* path, Color4ub fallbackColor)
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
			// Simple procedural fallback
			int w = 32, h = 32;
			TArray<Color4ub> pixels; pixels.resize(w * h);
			for (int i = 0; i < w * h; ++i) pixels[i] = fallbackColor;
			tex = RHICreateTexture2D(ETexture::RGBA8, w, h, 0, 1, TCF_DefalutValue, pixels.data());
		}
	}

	void SurvivorsRenderer::init()
	{
		// Load Map Texture
		mMapTexture = RHIUtility::LoadTexture2DFromFile("Survivor/Map.png", TextureLoadOption().AutoMipMap());

		// Load Hero Texture
		mHeroTexture = RHIUtility::LoadTexture2DFromFile("Survivor/Hero.png", TextureLoadOption().AutoMipMap());

		// Load Monster Textures
		mMonsterTextures[(int)MonsterType::Golem] = RHIUtility::LoadTexture2DFromFile("Survivor/Golem.png");
		mMonsterTextures[(int)MonsterType::Slime] = RHIUtility::LoadTexture2DFromFile("Survivor/Slime.png");

		// Process textures with Color Key
		ProcessTextureWithColorKey(mMonsterTextures[(int)MonsterType::Golem], "Survivor/Golem.png", Color4ub(150, 100, 50, 255));
		ProcessTextureWithColorKey(mMonsterTextures[(int)MonsterType::Slime], "Survivor/Slime.png", Color4ub(100, 255, 100, 255));
		ProcessTextureWithColorKey(mHeroBulletTexture, "Survivor/Knife.png", Color4ub(255, 255, 255, 255));
		ProcessTextureWithColorKey(mMonsterBulletTexture, "Survivor/MagicOrb.png", Color4ub(200, 100, 255, 255));
		ProcessTextureWithColorKey(mOrbBulletTexture, "Survivor/Fireball.png", Color4ub(255, 150, 50, 255));
	}

	void SurvivorsRenderer::renderScene(SceneSnapshot const& scene)
	{
		RHIGraphics2D& g = Global::GetRHIGraphics2D_RenderThread();
		RHICommandList& commandList = g.getCommandList();
		g.setViewportSize(scene.screenSize.x, scene.screenSize.y);
		RHISetViewport(commandList, 0, 0, (float)scene.screenSize.x, (float)scene.screenSize.y);
		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.04f, 0.04f, 0.08f, 1), 1);

		g.beginRender();
		g.setBlendAlpha(1.0f);
		g.transformXForm(scene.worldToScreen, true);

		Vector2 viewMin = scene.screenToWorld.transformPosition(Vector2::Zero());
		Vector2 viewMax = scene.screenToWorld.transformPosition(Vector2(scene.screenSize));
		if (viewMin.x > viewMax.x) std::swap(viewMin.x, viewMax.x);
		if (viewMin.y > viewMax.y) std::swap(viewMin.y, viewMax.y);

		// 1. Tiled Map
		if (mMapTexture.isValid())
		{
			g.setBlendState(ESimpleBlendMode::None);
			float tileSize = 256.0f;
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

		// 2. Gems
		g.setBrush(Color3f(0.2f, 1.0f, 0.5f));
		g.enablePen(false);
		for (auto const& gem : scene.gems)
		{
			if (gem.pos.x < viewMin.x - 5 || gem.pos.x > viewMax.x + 5 ||
				gem.pos.y < viewMin.y - 5 || gem.pos.y > viewMax.y + 5)
				continue;
			g.drawCircle(gem.pos, 3.0f);
		}
		g.enablePen(true);

		// 3. Monsters
		{
			TArray<RenderMonsterData const*> visibleMonsters;
			for (auto const& m : scene.monsters)
			{
				if (m.pos.x < viewMin.x - m.radius || m.pos.x > viewMax.x + m.radius ||
					m.pos.y < viewMin.y - m.radius || m.pos.y > viewMax.y + m.radius)
					continue;
				visibleMonsters.push_back(&m);
			}

			std::sort(visibleMonsters.begin(), visibleMonsters.end(), [](auto* a, auto* b) {
				return a->pos.y < b->pos.y;
			});

			TArray<RenderMonsterData const*> monstersByType[(int)MonsterType::Count];
			for (auto* m : visibleMonsters)
			{
				monstersByType[(int)m->type].push_back(m);
			}

			for (int i = 0; i < (int)MonsterType::Count; ++i)
			{
				auto& typeMonsters = monstersByType[i];
				if (typeMonsters.empty()) continue;

				auto const& tex = mMonsterTextures[i];
				if (tex.isValid())
				{
					TArray<MonsterInstanceData> instanceData;
					instanceData.reserve(typeMonsters.size());
					for (auto* m : typeMonsters)
					{
						float drawScale = mMonsterDrawScales[i];
						MonsterInstanceData data;
						data.pos = m->pos;
						data.size = Vector2(m->radius * 2, m->radius * 2);
						data.uvPos = Vector2(0, 0);
						data.uvSize = Vector2(1, 1);
						if (scene.hero.pos.x > m->pos.x) { data.uvPos.x = 1; data.uvSize.x = -1; }
						data.scale = m->scale * drawScale;
						data.color = (m->stunTimer > 0) ? LinearColor(5, 5, 5, 1) : LinearColor(1, 1, 1, 1);
						instanceData.push_back(data);
					}

					g.drawCustomFunc([tex, data = std::move(instanceData)](RHICommandList& commandList, Matrix4 const& baseTransform, RenderBatchedElement& element, Render::RenderTransform2D const& transform)
					{
						MonsterShaderProgram* shader = ShaderManager::Get().getGlobalShaderT<MonsterShaderProgram>();
						if (shader)
						{
							RHISetShaderProgram(commandList, shader->getRHI());
							shader->setWorldToClip(commandList, transform.toMatrix4() * baseTransform);
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
						if (m->stunTimer > 0) RenderUtility::SetBrush(g, EColor::White);
						else RenderUtility::SetBrush(g, EColor::Red);
						g.drawCircle(m->pos, m->radius);
					}
				}
			}
		}

		// 4. Hero
		{
			auto const& hero = scene.hero;
			if (mHeroTexture.isValid())
			{
				g.setBlendState(ESimpleBlendMode::Translucent);
				Vector2 spriteSize(60, 60);
				Vector2 pos = hero.pos - 0.5f * spriteSize;
				Vector2 uvPos(0, 0); Vector2 uvSize(1, 1);
				if (hero.facing.x > 0) { uvPos.x = 1; uvSize.x = -1; }
				if (hero.damageFlashTimer > 0) g.setTextColor(Color3f(1, 0, 0));
				g.setBrush(Color3f::White());
				g.setTexture(const_cast<RHITexture2D&>(*mHeroTexture));
				g.setSampler(TStaticSamplerState<ESampler::Trilinear>::GetRHI());
				g.drawTexture(pos, spriteSize, uvPos, uvSize);
			}
			else
			{
				if (hero.damageFlashTimer > 0) RenderUtility::SetBrush(g, EColor::Red);
				else RenderUtility::SetBrush(g, EColor::Green);
				g.drawCircle(hero.pos, hero.radius);
			}

			// Health Bar
			float barW = 60.0f; float barH = 6.0f;
			Vector2 barPos = hero.pos + Vector2(-barW * 0.5f, hero.radius + 10.0f);
			g.setPen(Color4f(0, 0, 0, 1), 1); g.setBrush(Color4f(0, 0, 0, 1));
			g.drawRect(barPos - Vector2(1, 1), Vector2(barW + 2, barH + 2));
			g.enablePen(false);
			g.setBrush(Color3f(0.3f, 0.1f, 0.1f)); g.drawRect(barPos, Vector2(barW, barH));
			g.setBrush(Color3f(1.0f - hero.hpParams, hero.hpParams, 0.0f));
			g.drawRect(barPos, Vector2(barW * hero.hpParams, barH));
			g.enablePen(true);

			// Orbs
			for (auto const& b : hero.orbs)
			{
				if (mOrbBulletTexture.isValid())
				{
					Vector2 dir = b.pos - hero.pos;
					float angle = Math::ATan2(dir.y, dir.x) + 0.5f * Math::PI;
					float visualAngle = angle - Math::DegToRad(90.0f);
					g.pushXForm();
					g.translateXForm(b.pos.x, b.pos.y);
					g.rotateXForm(visualAngle);
					g.setBrush(Color3f::White());
					g.setTexture(const_cast<RHITexture2D&>(*mOrbBulletTexture));
					g.drawTexture(Vector2(-b.radius * 1.8f, -b.radius * 1.8f), Vector2(b.radius * 3.6f, b.radius * 3.6f));
					g.popXForm();
				}
				else
				{
					RenderUtility::SetBrush(g, EColor::Cyan);
					g.drawCircle(b.pos, b.radius);
				}
			}
		}

		// 5. Bullets
		for (auto const& b : scene.bullets)
		{
			if (b.pos.x < viewMin.x - b.radius || b.pos.x > viewMax.x + b.radius ||
				b.pos.y < viewMin.y - b.radius || b.pos.y > viewMax.y + b.radius)
				continue;

			RHITexture2D* tex = nullptr;
			float offsetDeg = 0;
			if (b.category == Category_Bullet) { tex = mHeroBulletTexture.get(); offsetDeg = 135.0f; }
			else if (b.category == Category_MonsterBullet) { tex = mMonsterBulletTexture.get(); offsetDeg = 0.0f; }

			if (tex)
			{
				float angle = Math::ATan2(b.vel.y, b.vel.x);
				float visualAngle = angle - Math::DegToRad(offsetDeg);
				float visualScale = (b.category == Category_Bullet) ? 0.8f : 2.5f;
				g.pushXForm();
				g.translateXForm(b.pos.x, b.pos.y);
				g.rotateXForm(visualAngle);
				g.setBrush(Color3f::White());
				g.setTexture(*tex);
				g.drawTexture(Vector2(-b.radius * visualScale, -b.radius * visualScale), Vector2(b.radius * 2 * visualScale, b.radius * 2 * visualScale));
				g.popXForm();
			}
			else
			{
				if (b.category == Category_Bullet) { g.setBrush(Color3f(1, 1, 0)); g.setPen(Color3f(1, 1, 0)); }
				else { g.setBrush(Color3f(1, 0.5f, 0)); g.setPen(Color3f(1, 0.5f, 0)); }
				g.drawCircle(b.pos, b.radius);
			}
		}

		// 6. Visual Effects
		for (auto const& v : scene.visuals)
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
				g.enablePen(false); g.enableBrush(true);
				g.beginBlend(alpha * 0.5f);
				g.setBrush(Color3f(0.2f, 0.8f, 1.0f));
				g.drawCircle(v.pos, curR);
				g.endBlend();
				g.enablePen(true);
			}
			else if (v.type == VisualType::HitSpark)
			{
				g.setPen(Color3f(1, 1, 1), 1); g.setBrush(Color3f(1, 1, 1));
				g.drawCircle(v.pos, 5.0f * alpha);
			}
		}

		g.endRender();
	}

}//namespace Survivors
