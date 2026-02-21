#include "BasketballStage.h"
#include "RenderUtility.h"
#include "InputManager.h"
#include "StageRegister.h"

#include <cmath>
#include <algorithm>
#include "GameGUISystem.h"
#include "RHI/RHIGraphics2D.h"

namespace Basketball
{
	REGISTER_STAGE_ENTRY("Kids Basketball", LevelStage, EExecGroup::Dev);

	using namespace Render;

	LevelStage::LevelStage()
	{
		mAimPhase = 0.5f;
		mAimSpeed = 1.5f;
		mbAimDirectionPositive = true;
		mGravity = 980.0f; // Pixels per second squared
		mScore = 0;
		mHighScore = 0;

		mHoopPos = Vector2(650, 250);
		mHoopRadius = 35.0f;
		mPlayerPos = Vector2(150, 565);
		mScoreTimer = 0.0f;
		
		mShootCooldown = 0.0f;
		mBalls.clear();
	}

	LevelStage::~LevelStage()
	{
	}
	bool LevelStage::onInit()
	{
		if (!BaseClass::onInit())
			return false;

		// Initialize stars for background
		mStars.clear();
		for (int i = 0; i < 100; ++i)
		{
			Star s;
			s.pos = Vector2((float)(Global::Random() % 800), (float)(Global::Random() % 450));
			s.size = 0.5f + (Global::Random() % 15) * 0.1f;
			s.brightness = 0.4f + (Global::Random() % 6) * 0.1f;
			mStars.push_back(s);
		}

		::Global::GUI().cleanupWidget();

		// Initialize game state
		restart();
		return true;
	}

	void LevelStage::restart()
	{
		mScore = 0;
		mGameTimer = 20.0f;
		mbGameOver = false;
		mAimPhase = 0.5f;
		mScoreTimer = 0.0f;
		mShootCooldown = 0.0f;
		mBalls.clear();
	}

	void LevelStage::onUpdate(GameTimeSpan deltaTime)
	{
		BaseClass::onUpdate(deltaTime);
		
		float dt = deltaTime;

		if (mbGameOver)
		{
			updateBalls(dt);
			for (auto it = mParticles.begin(); it != mParticles.end(); )
			{
				it->pos += it->velocity * dt;
				it->velocity.y += 400.0f * dt; // Gravity
				it->life -= dt;
				if (it->life <= 0)
					it = mParticles.erase(it);
				else
					++it;
			}
			return;
		}

		mGameTimer -= dt;
		if (mGameTimer <= 0.0f)
		{
			mGameTimer = 0.0f;
			if (mBalls.empty())
			{
				mbGameOver = true;
			}
		}

		// Oscillate the aim meter ONLY when ready to shoot and time is not up
		if (mShootCooldown <= 0.0f && mGameTimer > 0.0f)
		{
			float move = mAimSpeed * dt;
			if (mbAimDirectionPositive)
			{
				mAimPhase += move;
				if (mAimPhase >= 1.0f)
				{
					mAimPhase = 1.0f;
					mbAimDirectionPositive = false;
				}
			}
			else
			{
				mAimPhase -= move;
				if (mAimPhase <= 0.0f)
				{
					mAimPhase = 0.0f;
					mbAimDirectionPositive = true;
				}
			}
		}

		bool bPrevCooldown = mShootCooldown > 0.0f;
		if (mShootCooldown > 0.0f)
		{
			mShootCooldown -= dt;
			if (bPrevCooldown && mShootCooldown <= 0.0f)
			{
				// Randomize aiming position and direction for the next throw
				mAimPhase = (float)(Global::Random() % 1000) / 1000.0f;
				mbAimDirectionPositive = (Global::Random() % 2) == 0;
			}
		}

		updateBalls(dt);

		if (mScoreTimer > 0)
			mScoreTimer -= dt;

		// Update particles
		for (auto it = mParticles.begin(); it != mParticles.end(); )
		{
			it->pos += it->velocity * dt;
			it->velocity.y += 400.0f * dt; // Gravity
			it->life -= dt;
			if (it->life <= 0)
				it = mParticles.erase(it);
			else
				++it;
		}
	}

	void LevelStage::updateBalls(float dt)
	{
		for (auto it = mBalls.begin(); it != mBalls.end(); )
		{
			Ball& ball = *it;
			
			ball.velocity.y += mGravity * dt;
			ball.pos += ball.velocity * dt;
			ball.rotation += 10.0f * dt;

			// Simple ground collision
			if (ball.pos.y > 565) // Ground level
			{
				ball.pos.y = 565;
				ball.velocity.y = -ball.velocity.y * 0.6f; // Bounce
				ball.velocity.x *= 0.8f; // Friction
				
				if (std::abs(ball.velocity.y) < 50 && std::abs(ball.velocity.y) > 0)
				{
					// Start fading out the ball when it stops bouncing
					ball.deadTimer += dt;
				}
			}

			// 1. Backboard Collision
			float boardX = mHoopPos.x + 40.0f; 
			float boardTopY = mHoopPos.y - 110.0f;
			float boardBotY = mHoopPos.y + 20.0f;
			float ballRadius = 22.0f;

			// Simple AABB against the backboard line
			// The board is at x = 690. When ball hits it, we want it to bounce back.
			if (ball.pos.x + ballRadius > boardX && ball.pos.x < boardX + 10.0f)
			{
				if (ball.pos.y > boardTopY && ball.pos.y < boardBotY)
				{
					ball.pos.x = boardX - ballRadius;
					// Increase bounce elasticity to prevent "dead drop" bank shots
					// Make it retain more horizontal speed so a bad shot bounces too far away
					ball.velocity.x = -ball.velocity.x * 0.85f; 
				}
			}

			// 2. Rim Collision (Front and Back points of the hoop)
			Vector2 frontRim(mHoopPos.x - mHoopRadius, mHoopPos.y);
			Vector2 backRim(mHoopPos.x + mHoopRadius, mHoopPos.y);
			
			auto collideRim = [&](Vector2 rimPos) {
				Vector2 diff = ball.pos - rimPos;
				float distSq = diff.length2();
				// Treat rim as a point and ball as a circle
				if (distSq < ballRadius * ballRadius)
				{
					float dist = std::sqrt(distSq);
					Vector2 normal = diff / (dist + 0.0001f);
					
					// Push out
					ball.pos = rimPos + normal * ballRadius;

					// Reflect velocity
					float dot = ball.velocity.dot(normal);
					if (dot < 0) 
					{
						ball.velocity = ball.velocity - normal * (2.0f * dot);
						ball.velocity *= 0.65f; // Energy loss on rim
					}
				}
			};

			collideRim(frontRim);
			collideRim(backRim);

			// 3. Collision check: detect if ball falls through the hoop's plane
			float prevY = ball.pos.y - ball.velocity.y * dt;
			if (ball.velocity.y > 0 && prevY < mHoopPos.y && ball.pos.y >= mHoopPos.y)
			{
				// Tighter scoring tolerance to require being INSIDE the rim
				float dx = std::abs(ball.pos.x - mHoopPos.x);
				if (dx < mHoopRadius - ballRadius * 0.4f)
				{
					if (!ball.bHasScored)
					{
						// Score!
						ball.bHasScored = true;
						mScore++;
						if (mScore > mHighScore)
							mHighScore = mScore;

						mScoreTimer = 1.0f;
						mScorePos = mHoopPos + Vector2(0, -50);

						// Spawn splash particles
						for (int i = 0; i < 15; ++i)
						{
							Particle p;
							p.pos = mHoopPos + Vector2(Global::Random() % 20 - 10, 0);
							p.velocity = Vector2(Global::Random() % 200 - 100, -(float)(Global::Random() % 300 + 100));
							p.life = 0.8f + (Global::Random() % 40) * 0.01f;
							p.color = Color3f(1.0f, 1.0f, 1.0f);
							mParticles.push_back(p);
						}
					}

					// Net friction: sharply slow down X and Y velocity so it drops straight down
					ball.velocity.x *= 0.15f;
					ball.velocity.y = std::max(150.0f, ball.velocity.y * 0.35f);
				}
			}
			
			// Out of bounds or dead check
			if (ball.pos.x > 900 || ball.pos.x < -100 || ball.pos.y < -200 || ball.deadTimer > 1.5f)
			{
				it = mBalls.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	void LevelStage::throwBall()
	{
		if (mShootCooldown > 0.0f)
			return;

		mShootCooldown = 0.5f;

		Ball newBall;
		newBall.pos = Vector2(150, 515); // "player's hands"
		newBall.rotation = 0.0f;
		newBall.bHasScored = false;
		newBall.deadTimer = 0.0f;

		// Calculate throw vector based on aim phase
		// 0.5 is perfect center
		float aimError = mAimPhase - 0.5f; // -0.5 to 0.5
		
		// Target is (650, 250), Start is (150, 515). dx = 500, dy = -265.
		// We want a high arc with a steep entry angle (approx 58 degrees entry),
		// so it goes cleanly into the basket without hitting the now-solid rim points.
		// Recalculated: vx = 340, vy = -900 gives a beautiful plunging curve in 1.47s
		float perfectVx = 340.0f;
		float perfectVy = -900.0f; 
		
		// Positive error (right) makes it fly over/longer, Negative (left) makes it fall short
		// We increase the multipliers so that even slightly outside the perfect zone
		// results in a massive overthrow (bricking hard against the backboard or fully missing) 
		// instead of getting an easy lucky bank shot.
		newBall.velocity.x = perfectVx + aimError * (aimError > 0 ? 750.0f : 225.0f); // Strongly penalize X offset
		newBall.velocity.y = perfectVy + aimError * 250.0f; 

		mBalls.push_back(newBall);
	}

	void LevelStage::onRender(float dFrame)
	{
		RHIGraphics2D& g = Global::GetRHIGraphics2D();

		Vec2i screenSize = Global::GetScreenSize();

		// We use a base virtual resolution of 800x600 for game logic and layout
		float const baseWidth = 800.0f;
		float const baseHeight = 600.0f;

		float scale = (float)screenSize.y / baseHeight;
		float virtualWidth = screenSize.x / scale;

		RHISetViewport(RHICommandList::GetImmediateList(), 0, 0, screenSize.x, screenSize.y);

		g.beginRender();

		g.pushXForm();
		// Scale to fit height, and expand width dynamically
		g.scaleXForm(scale, scale);
		// Center the 800x600 logical play area horizontally
		g.translateXForm((virtualWidth - baseWidth) / 2.0f, 0.0f);

		drawGame(g);

		g.popXForm();
		g.endRender();
	}

	void LevelStage::drawGame(RHIGraphics2D& g)
	{
		// 1. Sky Gradient (Bright Sunny Day)
		g.drawGradientRect(Vector2(-1000, -500), Color3f(0.3f, 0.7f, 1.0f), 
		                   Vector2(2000, 250), Color3f(0.5f, 0.85f, 1.0f), false);
		g.drawGradientRect(Vector2(-1000, 250), Color3f(0.5f, 0.85f, 1.0f), 
		                   Vector2(2000, 500), Color3f(0.8f, 0.95f, 1.0f), false);

		// 3. Fluffy Clouds (Reusing Moon/Stars logic but making it day)
		auto drawCloud = [&](Vector2 pos, float scale) {
			g.beginBlend(0.85f);
			g.setBrush(Color3f(1.0f, 1.0f, 1.0f));
			g.enablePen(false);
			g.drawCircle(pos + Vector2(0, 10 * scale), 20 * scale);
			g.drawCircle(pos + Vector2(20 * scale, 0), 25 * scale);
			g.drawCircle(pos + Vector2(40 * scale, 5 * scale), 18 * scale);
			g.drawCircle(pos + Vector2(15 * scale, 15 * scale), 20 * scale);
			g.enablePen(true);
			g.endBlend();
		};

		drawCloud(Vector2(-350, 120), 1.8f);
		drawCloud(Vector2(-150, 60), 1.3f);
		drawCloud(Vector2(100, 80), 1.5f);
		drawCloud(Vector2(400, 150), 1.2f);
		drawCloud(Vector2(650, 60), 2.0f);
		drawCloud(Vector2(950, 130), 1.4f);
		drawCloud(Vector2(1200, 70), 1.7f);

		// 4. Distant Cityscape
		auto drawBuilding = [&](float x, float w, float h, Color3f color) {
			Vector2 vp(400.0f, 500.0f);
			float bTop = 500.0f - h;
			float bBottom = 500.0f;
			float bLeft = x;
			float bRight = x + w;

			// Side face (Perspective volume - pastel shading)
			g.setBrush(Color3f(color.r * 0.85f, color.g * 0.85f, color.b * 0.85f));
			g.enablePen(false);
			
			float depth = 0.25f; // how much it projects towards VP
			if (bRight < vp.x)
			{
				Vector2 pts[4] = {
					Vector2(bRight, bTop),
					Vector2(bRight, bTop) + (vp - Vector2(bRight, bTop)) * depth,
					Vector2(bRight, bBottom) + (vp - Vector2(bRight, bBottom)) * depth,
					Vector2(bRight, bBottom)
				};
				g.drawPolygon(pts, 4);
			}
			else if (bLeft > vp.x)
			{
				Vector2 pts[4] = {
					Vector2(bLeft, bTop),
					Vector2(bLeft, bTop) + (vp - Vector2(bLeft, bTop)) * depth,
					Vector2(bLeft, bBottom) + (vp - Vector2(bLeft, bBottom)) * depth,
					Vector2(bLeft, bBottom)
				};
				g.drawPolygon(pts, 4);
			}

			// Front face
			g.setBrush(color);
			g.drawRect(Vector2(x, 500 - h), Vector2(w, h));
			// Windows (Bright cyan/white reflection for daytime)
			g.setBrush(Color3f(0.7f, 0.9f, 1.0f));
			g.enablePen(false);
			for (float wy = 500 - h + 15; wy < 490; wy += 25)
			{
				for (float wx = x + 10; wx < x + w - 10; wx += 20)
				{
					// Use a hash-like deterministic random for windows so they don't flicker
					int hash = (int)(x * 123 + wy * 456 + wx * 789);
					if (hash % 10 < 3) continue; 
					g.drawRoundRect(Vector2(wx, wy), Vector2(10, 12), Vector2(3, 3));
				}
			}
			g.enablePen(true);
		};

		// Pastel kid-friendly city expanded
		drawBuilding(-350, 120, 180, Color3f(0.8f, 0.9f, 0.6f));
		drawBuilding(-210, 100, 230, Color3f(1.0f, 0.7f, 1.0f));
		drawBuilding(0, 120, 150, Color3f(1.0f, 0.6f, 0.6f));   // Soft Red
		drawBuilding(140, 80, 220, Color3f(0.6f, 0.9f, 0.6f));  // Mint Green
		drawBuilding(240, 100, 120, Color3f(1.0f, 0.9f, 0.5f)); // Sunny Yellow
		drawBuilding(450, 110, 180, Color3f(0.7f, 0.7f, 1.0f)); // Periwinkle Blue
		drawBuilding(580, 90, 280, Color3f(1.0f, 0.7f, 0.9f));  // Cotton Candy Pink
		drawBuilding(700, 100, 100, Color3f(0.6f, 1.0f, 0.9f)); // Cyan
		drawBuilding(820, 130, 200, Color3f(1.0f, 0.8f, 0.6f)); // Orange peel
		drawBuilding(970, 160, 150, Color3f(0.6f, 0.8f, 1.0f)); // Sky Blue

		// 5. Court (Bright Hardwood/Playground floor) extended wide
		g.drawGradientRect(Vector2(-1000, 500), Color3f(1.0f, 0.85f, 0.6f),
		                   Vector2(2000, 1400), Color3f(0.9f, 0.7f, 0.4f), false);
		
		// Perspective Projection Helper
		float vpX = 400.0f;
		float vpY = 500.0f;
		float camH = 100.0f;
		auto project = [&](float cx, float cz) -> Vector2 {
			return Vector2(vpX + cx / cz, vpY + camH / cz);
		};

		// Court markings: Bright white playground lines
		g.beginBlend(0.7f);
		g.setPen(Color3f(1.0f, 1.0f, 1.0f), 3);
		g.enableBrush(false);

		// Scaling carefully so hoop pole sits right at baseline
		// and player 150px start point lies just outside the 3-point line
		float rightBaselineX = 460.0f;
		float leftBaselineX = rightBaselineX - 2000.0f;
		float halfCourtX = rightBaselineX - 1000.0f;
		
		float nearZ = 0.75f;  
		float farZ = 3.5f;    
		float hoopZ = 100.0f / 65.0f; // ~1.538 matches player Y = 565
		float hoopX = 385.0f;         // matches hoop rim screen X = 650

		// Court boundaries
		g.drawLine(project(leftBaselineX, nearZ), project(rightBaselineX, nearZ)); // Near Sideline
		g.drawLine(project(leftBaselineX, farZ), project(rightBaselineX, farZ));   // Far Sideline
		g.drawLine(project(rightBaselineX, nearZ), project(rightBaselineX, farZ)); // Right Baseline
		g.drawLine(project(leftBaselineX, nearZ), project(leftBaselineX, farZ));   // Left Baseline

		// Half-court line
		g.drawLine(project(halfCourtX, nearZ), project(halfCourtX, farZ));

		auto drawPerspCircle = [&](float cx, float cz, float rX, float rZ, float angleStart, float angleEnd, int segments = 32) {
			Vector2 prevPt = project(cx + std::cos(angleStart) * rX, cz + std::sin(angleStart) * rZ);
			for(int i=1; i<=segments; ++i) {
				float a = angleStart + (angleEnd - angleStart) * ((float)i / segments);
				Vector2 pt = project(cx + std::cos(a) * rX, cz + std::sin(a) * rZ);
				g.drawLine(prevPt, pt);
				prevPt = pt;
			}
		};

		// Center circle
		drawPerspCircle(halfCourtX, hoopZ, 160.0f, 0.3f, 0.0f, 3.14159f * 2.0f);

		// Right Key (Paint)
		float ftLineX = rightBaselineX - 420.0f; 
		float paintNearZ = hoopZ - 0.45f;
		float paintFarZ = hoopZ + 0.55f;
		g.drawLine(project(rightBaselineX, paintNearZ), project(ftLineX, paintNearZ));
		g.drawLine(project(rightBaselineX, paintFarZ), project(ftLineX, paintFarZ));
		g.drawLine(project(ftLineX, paintNearZ), project(ftLineX, paintFarZ));

		// Right Free throw circle
		drawPerspCircle(ftLineX, hoopZ, 120.0f, 0.25f, 0.0f, 3.14159f * 2.0f);

		// Right Three-point line
		float threePtArcRx = 750.0f;
		float threePtArcRz = 0.75f;
		drawPerspCircle(hoopX, hoopZ, threePtArcRx, threePtArcRz, 1.5708f, 1.5708f + 3.14159f, 40);
		// Connect endpoints to baseline
		g.drawLine(project(hoopX, hoopZ + threePtArcRz), project(rightBaselineX, hoopZ + threePtArcRz));
		g.drawLine(project(hoopX, hoopZ - threePtArcRz), project(rightBaselineX, hoopZ - threePtArcRz));

		// Left Key (Paint) (mostly offscreen but drawn for correctness)
		float leftFtLineX = leftBaselineX + 420.0f; 
		g.drawLine(project(leftBaselineX, paintNearZ), project(leftFtLineX, paintNearZ));
		g.drawLine(project(leftBaselineX, paintFarZ), project(leftFtLineX, paintFarZ));
		g.drawLine(project(leftFtLineX, paintNearZ), project(leftFtLineX, paintFarZ));

		// Left Free throw circle
		drawPerspCircle(leftFtLineX, hoopZ, 120.0f, 0.25f, 0.0f, 3.14159f * 2.0f);

		// Left Three-point line
		float leftHoopX = leftBaselineX + 75.0f;
		drawPerspCircle(leftHoopX, hoopZ, threePtArcRx, threePtArcRz, -1.5708f, 1.5708f, 40);
		g.drawLine(project(leftHoopX, hoopZ + threePtArcRz), project(leftBaselineX, hoopZ + threePtArcRz));
		g.drawLine(project(leftHoopX, hoopZ - threePtArcRz), project(leftBaselineX, hoopZ - threePtArcRz));
		g.endBlend();

		// Horizon Line (grass/ground border)
		g.setPen(Color3f(0.3f, 0.8f, 0.3f), 4);
		g.drawLine(Vector2(-1000, 500), Vector2(2000, 500)); 
		g.enableBrush(true);

		// Ball Shadows (Dynamic Height Scaling & Fading)
		for (auto const& ball : mBalls)
		{
			if (ball.pos.y <= 565)
			{
				float dist = std::max(0.0f, 565.0f - ball.pos.y);
				float t = std::min(dist / 500.0f, 1.0f); // 0 at ground, 1 at peak height
				
				// Shadow fades out as it gets higher
				float shadowAlpha = 0.65f * (1.0f - t * 0.8f); 
				
				// Fade out shadow if ball is dead
				shadowAlpha *= std::max(0.0f, 1.0f - ball.deadTimer * 2.0f);

				if (shadowAlpha > 0)
				{
					// Base size matches ball diameter (40), shrinks a bit as it goes up
					float shadowSizeX = 30.0f * (1.0f - t * 0.45f); // Slightly smaller than ball so it doesn't swallow it
					float shadowSizeY = shadowSizeX * 0.35f;
					g.beginBlend(shadowAlpha);
					g.setBrush(Color3f(0, 0, 0));
					g.enablePen(false);
					// Draw shadow at Y=575 (effectively under the 565 center coordinate of an on-ground ball)
					g.drawEllipse(Vector2(ball.pos.x, 575), Vector2(shadowSizeX, shadowSizeY));
					g.enablePen(true);
					g.endBlend();
				}
			}
		}

		// 6. Character
		float jumpY = 0.0f;
		// Jump lasts for the first 0.3s of the 0.5s cooldown
		if (mShootCooldown > 0.2f)
		{
			float t = (mShootCooldown - 0.2f) / 0.3f; // 1.0 (start of shot) down to 0.0
			jumpY = -45.0f * std::sin(t * 3.14159f);
		}

		g.pushXForm();
		g.translateXForm(mPlayerPos.x, mPlayerPos.y);
		// Shadow (Stays on the ground, fades and shrinks slightly during jump)
		float jumpFactor = std::abs(jumpY) / 45.0f;
		g.beginBlend(0.3f * (1.0f - jumpFactor * 0.4f));
		g.setBrush(Color3f(0, 0, 0));
		g.enablePen(false);
		g.drawEllipse(Vector2(0, 5), Vector2(25.0f * (1.0f - jumpFactor * 0.2f), 8));
		g.endBlend();

		// Character Body (Applies jump offset)
		g.translateXForm(0, jumpY);

		// Modern Style character body (Chibi style)
		g.enablePen(false); // Fix green outline carrying over
		
		// Shoes/Feet (Bright Blue sneakers)
		g.setBrush(Color3f(0.1f, 0.5f, 1.0f));
		g.drawRoundRect(Vector2(-18, -10), Vector2(16, 14), Vector2(5, 5));
		g.drawRoundRect(Vector2(2, -10), Vector2(16, 14), Vector2(5, 5));
		
		// Chibi Body (Brighter Red Jersey, wider)
		g.setBrush(Color3f(1.0f, 0.2f, 0.2f)); 
		g.drawRoundRect(Vector2(-20, -60), Vector2(40, 55), Vector2(8, 8));

		// Head (Larger for cuteness)
		g.setBrush(Color3f(1.0f, 0.85f, 0.75f));
		g.drawCircle(Vector2(0, -85), 24);

		// Big Anime Eyes
		g.setBrush(Color3f(0.15f, 0.15f, 0.15f));
		g.drawEllipse(Vector2(-10, -86), Vector2(4, 7)); // Left eye
		g.drawEllipse(Vector2(10, -86), Vector2(4, 7));  // Right eye
		// Eye highlights
		g.setBrush(Color3f(1.0f, 1.0f, 1.0f));
		g.drawCircle(Vector2(-11.5f, -88), 1.5f);
		g.drawCircle(Vector2(8.5f, -88), 1.5f);

		// Rosy Cheeks
		g.beginBlend(0.6f);
		g.setBrush(Color3f(1.0f, 0.5f, 0.6f));
		g.drawCircle(Vector2(-16, -78), 4);
		g.drawCircle(Vector2(16, -78), 4);
		g.endBlend();

		// Cute Smile
		g.enablePen(true);
		g.setPen(Color3f(0.15f, 0.15f, 0.15f), 2);
		g.enableBrush(false);
		g.drawArcLine(Vector2(0, -80), 5, 0.4f, 2.34f);
		g.enableBrush(true);
		g.enablePen(false);

		// Hair/Cap (Yellow backwards cap!)
		g.setBrush(Color3f(1.0f, 0.9f, 0.1f));
		g.drawRoundRect(Vector2(-22, -109), Vector2(44, 22), Vector2(10, 10)); // Cap dome
		g.drawRect(Vector2(-24, -100), Vector2(48, 8)); // Hat base
		g.drawRect(Vector2(-35, -99), Vector2(15, 6)); // Brim sticking out to the left

		// Number on jersey
		g.setTextColor(Color3f(1.0f, 0.9f, 0.1f));
		g.drawText(Vector2(-10, -50), "88");

		// Arms (Chubby Arms)
		g.enableBrush(false);
		g.setPen(Color3f(1.0f, 0.85f, 0.75f), 8);
		
		// Arm animation logic based on cooldown timer (0.5s total)
		if (mShootCooldown <= 0.0f)
		{
			// State 3 (Ready): Holding ball, aiming
			Vector2 ballLocal = Vector2(150, 515) - mPlayerPos;

			// Draw ball in hands first so arms overlap
			g.enableBrush(true);
			
			g.beginBlend(0.2f);
			g.enablePen(false);
			g.setBrush(Color3f(1.0f, 0.6f, 0.2f));
			g.drawCircle(ballLocal, 22.0f);
			g.endBlend();

			g.setBrush(Color3f(0.9f, 0.45f, 0.1f));
			g.setPen(Color3f(0, 0, 0), 2);
			g.drawCircle(ballLocal, 16.0f);
			// Seams
			g.drawLine(ballLocal + Vector2(-16, 0), ballLocal + Vector2(16, 0));
			g.drawLine(ballLocal + Vector2(0, -16), ballLocal + Vector2(0, 16));

			// Restore pen for arms
			g.enableBrush(false);
			g.setPen(Color3f(1.0f, 0.85f, 0.75f), 8);
			g.drawLine(Vector2(-20, -45), ballLocal);
			g.drawLine(Vector2(20, -45), ballLocal);
		}
		else if (mShootCooldown > 0.25f)
		{
			// State 1 (Just shot): Arms thrown up cheering! (shooting pose)
			g.drawLine(Vector2(-20, -45), Vector2(-28, -75));
			g.drawLine(Vector2(20, -45), Vector2(28, -75));
		}
		else
		{
			// State 2 (Recovering): Arms down state (Resting near waist)
			g.drawLine(Vector2(-20, -45), Vector2(-22, -25));
			g.drawLine(Vector2(20, -45), Vector2(22, -25));
		}
		g.popXForm();
		g.enablePen(true);

		// 7. Hoop
		// Hoop shadow
		g.beginBlend(0.3f);
		g.setBrush(Color3f(0, 0, 0));
		g.enablePen(false);
		g.drawEllipse(mHoopPos + Vector2(50, 315), Vector2(25, 10));
		g.enablePen(true);
		g.endBlend();

		// Pole (Bright Yellow instead of dark metal)
		g.drawGradientRect(mHoopPos + Vector2(46, -40), Color3f(1.0f, 0.8f, 0.2f),
		                   mHoopPos + Vector2(54, 315), Color3f(1.0f, 0.6f, 0.1f), true);
		// Board (Glassy but bright slightly cyan tint, NOW WITH 3D PERSPECTIVE)
		auto project3D = [&](float cx, float cy, float cz) -> Vector2 {
			return Vector2(400.0f + (cx - 400.0f) / cz, 500.0f + (cy - 500.0f) / cz);
		};

		float bX = 846.2f; // Board physical X (maps to screen X ~690 at Z=1.5385)
		float hZ = 1.5385f;
		float bYTop = 500.0f - (500.0f - (mHoopPos.y - 110.0f)) * hZ; 
		float bYBot = 500.0f - (500.0f - (mHoopPos.y + 20.0f)) * hZ;  
		float bZ1 = hZ - 0.25f; // Near edge
		float bZ2 = hZ + 0.25f; // Far edge

		Vector2 boardPts[4] = {
			project3D(bX, bYTop, bZ1), // Top near
			project3D(bX, bYTop, bZ2), // Top far
			project3D(bX, bYBot, bZ2), // Bot far
			project3D(bX, bYBot, bZ1)  // Bot near
		};

		g.beginBlend(0.8f);
		g.setBrush(Color3f(0.8f, 0.95f, 1.0f));
		g.enablePen(false);
		g.drawPolygon(boardPts, 4);

		// Draw border
		g.enableBrush(false);
		g.setPen(Color3f(1.0f, 1.0f, 1.0f), 4);
		g.drawPolygon(boardPts, 4);
		
		// Red target square
		float tYTop = 500.0f - (500.0f - (mHoopPos.y - 45.0f)) * hZ;
		float tYBot = 500.0f - (500.0f - mHoopPos.y) * hZ;
		float tZ1 = hZ - 0.08f;
		float tZ2 = hZ + 0.08f;
		Vector2 targetPts[4] = {
			project3D(bX, tYTop, tZ1),
			project3D(bX, tYTop, tZ2),
			project3D(bX, tYBot, tZ2),
			project3D(bX, tYBot, tZ1)
		};
		g.setPen(Color3f(1.0f, 0.2f, 0.2f), 3);
		g.drawPolygon(targetPts, 4);
		g.enableBrush(true);
		g.endBlend();

		// Net (Woven Criss-Cross Pattern)
		g.beginBlend(0.9f);
		g.enableBrush(false);
		g.setPen(Color3f(1.0f, 1.0f, 1.0f), 2);
		
		float netTopW = mHoopRadius * 1.9f;
		float netBotW = mHoopRadius * 1.1f;
		float netH = 45.0f;
		Vector2 netTop = mHoopPos + Vector2(-mHoopRadius * 0.95f, 0);
		
		int numStrands = 9;
		for (int i = 0; i <= numStrands; ++i)
		{
			// Left-to-Right downward weave
			float r1 = (float)i / numStrands;
			float r2 = std::min(1.0f, r1 + 0.35f); 
			Vector2 p1 = netTop + Vector2(r1 * netTopW, 0);
			Vector2 p2 = netTop + Vector2((netTopW - netBotW) / 2.0f + r2 * netBotW, netH);
			if (i <= numStrands - 3) g.drawLine(p1, p2);

			// Right-to-Left downward weave
			r1 = (float)i / numStrands;
			r2 = std::max(0.0f, r1 - 0.35f);
			p1 = netTop + Vector2(r1 * netTopW, 0);
			p2 = netTop + Vector2((netTopW - netBotW) / 2.0f + r2 * netBotW, netH);
			if (i >= 3) g.drawLine(p1, p2);
		}
		
		// Rim (drawn after net so it's on top)
		g.setPen(Color3f(1.0f, 0.4f, 0.0f), 4);
		g.drawEllipse(mHoopPos, Vector2(mHoopRadius, mHoopRadius * 0.3f));
		g.enableBrush(true);
		g.endBlend();

		// 8. Balls (Multiple)
		for (auto const& ball : mBalls)
		{
			g.pushXForm();
			g.translateXForm(ball.pos.x, ball.pos.y);
			g.rotateXForm(ball.rotation);
			
			// Ball Glow fade out depending on deadTimer
			float ballAlpha = std::max(0.0f, 1.0f - ball.deadTimer * 2.0f);
			
			// Ball Glow
			g.beginBlend(0.2f * ballAlpha);
			g.enablePen(false);
			g.setBrush(Color3f(1.0f, 0.6f, 0.2f));
			g.drawCircle(Vector2(0, 0), 22.0f);
			g.endBlend();

			g.beginBlend(ballAlpha);
			g.setBrush(Color3f(0.9f, 0.45f, 0.1f));
			g.setPen(Color3f(0, 0, 0), 2);
			g.drawCircle(Vector2(0, 0), 16.0f);
			// Seams
			g.drawLine(Vector2(-16, 0), Vector2(16, 0));
			g.drawLine(Vector2(0, -16), Vector2(0, 16));
			g.endBlend();
			
			g.popXForm();
		}

		// 9. Particles
		for (auto const& p : mParticles)
		{
			g.setBrush(p.color);
			g.enablePen(false);
			g.drawRect(p.pos, Vector2(2, 2));
		}

		// 10. Premium UI
		RenderUtility::SetFont(g, FONT_S16);

		// Score Panel
		g.beginBlend(0.3f);
		g.setBrush(Color3f(0, 0, 0));
		g.enablePen(false);
		g.drawRoundRect(Vector2(20, 20), Vector2(190, 60), Vector2(10, 10)); // drop shadow
		g.endBlend();

		g.beginBlend(0.85f);
		g.setBrush(Color3f(0.1f, 0.15f, 0.25f)); // Dark blue-gray
		g.setPen(Color3f(0.3f, 0.7f, 1.0f), 3); // Bright blue border
		g.drawRoundRect(Vector2(15, 15), Vector2(190, 60), Vector2(10, 10));
		g.endBlend();

		g.setTextColor(Color3ub(255, 200, 50));
		g.drawTextF(Vector2(35, 23), "SCORE:  %d", mScore);
		g.setTextColor(Color3ub(150, 220, 255));
		g.drawTextF(Vector2(35, 47), "HIGH:   %d", mHighScore);

		// Center Timer Display
		if (!mbGameOver || mGameTimer > 0.0f)
		{
			RenderUtility::SetFont(g, FONT_S24);
			InlineString<128> timerStr;
			timerStr.format("%.1f s", mGameTimer);
			
			float pillW = 140.0f;
			float pillH = 50.0f;
			// 400 is the center of the virtual 800x600 layout
			Vector2 pillPos(400.0f - pillW / 2.0f, 15.0f);

			// Drop shadow
			g.beginBlend(0.4f);
			g.enablePen(false);
			g.setBrush(Color3f(0, 0, 0));
			g.drawRoundRect(pillPos + Vector2(4, 4), Vector2(pillW, pillH), Vector2(pillH / 2.0f, pillH / 2.0f));
			g.endBlend();

			// Pill background
			g.beginBlend(0.9f);
			if (mGameTimer <= 5.0f && mGameTimer > 0.0f)
			{
				g.setBrush(Color3f(0.85f, 0.15f, 0.15f));
				g.setPen(Color3f(1.0f, 0.4f, 0.4f), 3);
			}
			else
			{
				g.setBrush(Color3f(0.15f, 0.4f, 0.85f));
				g.setPen(Color3f(0.4f, 0.7f, 1.0f), 3);
			}
			g.drawRoundRect(pillPos, Vector2(pillW, pillH), Vector2(pillH / 2.0f, pillH / 2.0f));
			g.endBlend();

			g.setTextColor(Color3ub(255, 255, 255)); // White text
			// We subtract half the height because drawText origin could be top-left depending on setup, but EHorizontalAlign::Center handles X. For Y we adjust manually slightly.
			g.drawText(pillPos, Vector2(pillW, pillH), timerStr, EHorizontalAlign::Center, EVerticalAlign::Center);
		}

		if (mbGameOver)
		{
			Vector2 panelSize(420, 220);
			Vector2 panelPos(400.0f - panelSize.x / 2.0f, 300.0f - panelSize.y / 2.0f);

			// Drop shadow
			g.beginBlend(0.5f);
			g.enablePen(false);
			g.setBrush(Color3f(0, 0, 0));
			g.drawRoundRect(panelPos + Vector2(8, 8), panelSize, Vector2(15, 15));
			g.endBlend();

			// Main Background
			g.beginBlend(0.95f);
			g.setBrush(Color3f(0.1f, 0.15f, 0.25f)); // Dark Slate
			g.setPen(Color3f(0.9f, 0.7f, 0.1f), 4); // Golden Yellow Border
			g.drawRoundRect(panelPos, panelSize, Vector2(15, 15));
			g.endBlend();

			// Decorative inner lines
			g.beginBlend(0.2f);
			g.enableBrush(false);
			g.setPen(Color3f(1.0f, 1.0f, 1.0f), 1);
			g.drawLine(panelPos + Vector2(20, 75), panelPos + Vector2(panelSize.x - 20, 75));
			g.drawLine(panelPos + Vector2(20, 165), panelPos + Vector2(panelSize.x - 20, 165));
			g.endBlend();

			RenderUtility::SetFont(g, FONT_S24);
			g.setTextColor(Color3ub(255, 80, 80)); // Red text
			g.drawText(Vector2(400, panelPos.y + 25), Vector2(0, 0), "TIME'S UP!", EHorizontalAlign::Center);

			g.setTextColor(Color3ub(255, 255, 255));
			InlineString<64> scoreStr;
			scoreStr.format("FINAL SCORE: %d", mScore);
			g.drawText(Vector2(400, panelPos.y + 105), Vector2(0, 0), scoreStr, EHorizontalAlign::Center);

			RenderUtility::SetFont(g, FONT_S16);
			g.setTextColor(Color3ub(120, 255, 120));
			g.drawText(Vector2(400, panelPos.y + 180), Vector2(0, 0), "PRESS [ENTER] TO REPLAY", EHorizontalAlign::Center);
		}
		else if (mGameTimer > 0.0f)
		{
			// Aiming Controls (Always visible to allow continuous throwing)
			{
				Vector2 mPos(250, 520);
				Vector2 mSize(300, 24);
				
				// Drop shadow
				g.beginBlend(0.4f);
				g.setBrush(Color3f(0, 0, 0));
				g.enablePen(false);
				g.drawRoundRect(mPos + Vector2(3, 4), mSize, Vector2(12, 12));
				g.endBlend();

				// Base Background
				g.beginBlend(0.85f);
				g.setBrush(Color3f(0.08f, 0.12f, 0.18f));
				g.setPen(Color3f(0.3f, 0.6f, 0.9f), 2);
				g.drawRoundRect(mPos, mSize, Vector2(12, 12));
				g.endBlend();

				// Target Zone
				g.beginBlend(0.85f);
				g.setBrush(Color3f(0.2f, 0.95f, 0.4f));
				g.enablePen(false);
				g.drawRect(mPos + Vector2(mSize.x * 0.44f, 2), Vector2(mSize.x * 0.12f, mSize.y - 4));
				g.endBlend();
				
				float cursorX = mPos.x + mAimPhase * mSize.x;
				g.beginBlend(0.95f);
				g.setBrush(Color3f(1.0f, 0.9f, 0.2f));
				g.setPen(Color3f(1.0f, 1.0f, 1.0f), 2);
				g.drawRoundRect(Vector2(cursorX - 4, mPos.y - 6), Vector2(8, mSize.y + 12), Vector2(4, 4));
				g.endBlend();
			}
		}

		RenderUtility::SetFont(g, FONT_S12);

		// Feedbacks
		if (mScoreTimer > 0)
		{
			float t = 1.0f - mScoreTimer;
			g.setBlendAlpha(mScoreTimer);
			g.setTextColor(Color3f(1.0f, 1.0f, 0.5f));
			g.drawText(mScorePos + Vector2(0, -t * 100), Vector2(0, 0), "PERFECT!", EHorizontalAlign::Center);
			g.setBlendAlpha(1.0f);
		}
	}


	MsgReply LevelStage::onKey(KeyMsg const& msg)
	{
		if (msg.isDown())
		{
			if (mbGameOver)
			{
				if (msg.getCode() == EKeyCode::Return || msg.getCode() == EKeyCode::Space)
				{
					restart();
					return MsgReply::Handled();
				}
			}
			else
			{
				if (msg.getCode() == EKeyCode::Space)
				{
					if (mShootCooldown <= 0.0f && mGameTimer > 0.0f)
					{
						throwBall();
						return MsgReply::Handled();
					}
				}
			}

			if (msg.getCode() == EKeyCode::R)
			{
				restart();
				return MsgReply::Handled();
			}
		}
		return BaseClass::onKey(msg);
	}

	ERenderSystem LevelStage::getDefaultRenderSystem()
	{
		return ERenderSystem::None;
	}

	bool LevelStage::setupRenderResource(ERenderSystem systemName)
	{
		// Setup resources if we have textures
		return true;
	}

	void LevelStage::preShutdownRenderSystem(bool bReInit)
	{
		// Cleanup
	}

	void LevelStage::configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs)
	{
		systemConfigs.screenWidth = 1920;
		systemConfigs.screenHeight = 1080;
		systemConfigs.bWasUsedPlatformGraphics = true;
	}

}
