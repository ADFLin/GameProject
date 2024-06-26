#include "Stage/TestStageHeader.h"

#include "GameRenderSetup.h"
#include "RHI/RHIGraphics2D.h"
#include "RHI/RHICommand.h"
#include "RandomUtility.h"
#include "Math/GeometryPrimitive.h"

namespace RVO
{
	using namespace Render;

#define ROV_USE_OBSTACLE 0

	struct Obstacle
	{
		Vector2 point;
		Vector2 dir;
		bool    isConvex;

		Obstacle* next;
		Obstacle* previous;
	};

	float const RVO_EPSILON = 1e-6;


	using Plane = ::Math::Plane2D;

	struct Line
	{
		Vector2 point;
		Vector2 dir;
	};

	class Agent
	{
	public:
		float radius;
		float timeHorizon = 2.0;
		float maxSpeed;

		Vector2 pos;

		Vector2 velocity = Vector2::Zero();
		Vector2 velocityNew;

		Vector2 goal;
		void update(float dt)
		{
			velocity = velocityNew;
			pos += velocity * dt;
			trace.push_back(pos);
		}

		void updatePrefVelocity(float dt)
		{
			Vector2 dir = goal - pos;
			float len = dir.normalize();
			velocityPref = Math::Min( maxSpeed , len / dt ) * Math::GetNormal(dir);
			//velocity = velocityPref;
		}

		Vector2 velocityPref;
		TArray<Agent*> mAgentNeighbors;
		TArray<Line> mCachedOrcaLines;

		void computeNewVelocity2(float timeStep)
		{
			if (mAgentNeighbors.empty())
			{
				velocityNew = velocityPref;
				return;
			}


			Agent const& other = *mAgentNeighbors[0];
			Vector2 posRel = pos - other.pos;
			Vector2 velRel = velocity - other.velocity;

			float radiusCombined = radius + other.radius;
			float radiusCombinedSq = Math::Square(radiusCombined);
			float distSq = posRel.length2();

			if (distSq > radiusCombinedSq)
			{
				Vector2 dir = goal - pos;
				velocityNew = velocityPref;
			}
			else
			{
				Vector2 dir = Math::GetNormal(Vector2(posRel.y, -posRel.x));
				velocityNew = maxSpeed * dir;
			}
		}

		void computeNewVelocity(float timeStep);

		TArray< Vector2 > trace;

	};

	float Det(Vector2 const& a, Vector2 const& b)
	{
		return a.cross(b);
	}

	bool LinearProgram1(TArray<Line> const& lines, std::size_t lineNo, float radius, const Vector2 &optVelocity, bool directionOpt, Vector2 &result)
	{ 
		/* NOLINT(runtime/references) */

		Line const& line = lines[lineNo];

		const float dotProduct =  line.point.dot(line.dir);
		const float discriminant = dotProduct * dotProduct + radius * radius - line.point.length2();

		if (discriminant < 0.0F) 
		{
			/* Max speed circle fully invalidates line lineNo. */
			return false;
		}

		const float sqrtDiscriminant = Math::Sqrt(discriminant);
		float tLeft = -dotProduct - sqrtDiscriminant;
		float tRight = -dotProduct + sqrtDiscriminant;

		for (std::size_t i = 0U; i < lineNo; ++i) 
		{
			Line const& lineOther = lines[i];

			const float denominator = Det(line.dir, lineOther.dir);
			const float numerator = Det(lineOther.dir, line.point - lineOther.point);

			if (Math::Abs(denominator) <= RVO_EPSILON) 
			{
				/* Lines lineNo and i are (almost) parallel. */
				if (numerator < 0.0F) 
				{
					return false;
				}

				continue;
			}

			const float t = numerator / denominator;

			if (denominator >= 0.0F) 
			{
				/* Line i bounds line lineNo on the right. */
				tRight = std::min(tRight, t);
			}
			else 
			{
				/* Line i bounds line lineNo on the left. */
				tLeft = std::max(tLeft, t);
			}

			if (tLeft > tRight)
			{
				return false;
			}
		}

		if (directionOpt) 
		{
			/* Optimize direction. */
			if (optVelocity.dot(line.dir) > 0.0F) 
			{
				/* Take right extreme. */
				result = line.point + tRight * line.dir;
			}
			else 
			{
				/* Take left extreme. */
				result = line.point + tLeft * line.dir;
			}
		}
		else 
		{
			/* Optimize closest point. */
			const float t = line.dir.dot(optVelocity - line.point);

			if (t < tLeft) 
			{
				result = line.point + tLeft * line.dir;
			}
			else if (t > tRight) 
			{
				result = line.point + tRight * line.dir;
			}
			else
			{
				result = line.point + t * line.dir;
			}
		}

		return true;
	}

	/**
	 * @relates        Agent
	 * @brief          Solves a two-dimensional linear program subject to linear
	 *                 constraints defined by lines and a circular constraint.
	 * @param[in]      lines        Lines defining the linear constraints.
	 * @param[in]      radius       The radius of the circular constraint.
	 * @param[in]      optVelocity  The optimization velocity.
	 * @param[in]      directionOpt True if the direction should be optimized.
	 * @param[in, out] result       A reference to the result of the linear program.
	 * @return         The number of the line it fails on, and the number of lines
	 *                 if successful.
	 */
	std::size_t LinearProgram2(TArray<Line> const& lines, float radius, const Vector2 &optVelocity, bool directionOpt, Vector2 &result)
	{ 
		/* NOLINT(runtime/references) */
		if (directionOpt) 
		{
			/* Optimize direction. Note that the optimization velocity is of unit length
			 * in this case.
			 */
			result = radius * optVelocity;
		}
		else if (optVelocity.length2() > radius * radius) 
		{
			/* Optimize closest point and outside circle. */
			result = Math::GetNormal(optVelocity) * radius;
		}
		else 
		{
			/* Optimize closest point and inside circle. */
			result = optVelocity;
		}

		for (std::size_t i = 0U; i < lines.size(); ++i) 
		{
			if (Det(lines[i].dir, lines[i].point - result) > 0.0F) 
			{
				/* Result does not satisfy constraint i. Compute new optimal result. */
				const Vector2 tempResult = result;

				if (!LinearProgram1(lines, i, radius, optVelocity, directionOpt, result)) 
				{
					result = tempResult;

					return i;
				}
			}
		}

		return lines.size();
	}

	/**
	 * @relates        Agent
	 * @brief          Solves a two-dimensional linear program subject to linear
	 *                 constraints defined by lines and a circular constraint.
	 * @param[in]      lines        Lines defining the linear constraints.
	 * @param[in]      numObstLines Count of obstacle lines.
	 * @param[in]      beginLine    The line on which the 2-d linear program failed.
	 * @param[in]      radius       The radius of the circular constraint.
	 * @param[in, out] result       A reference to the result of the linear program.
	 */
	void LinearProgram3(TArray<Line> const& lines, std::size_t numObstLines, std::size_t beginLine, float radius, Vector2 &result)
	{
		/* NOLINT(runtime/references) */
		float distance = 0.0F;

		for (std::size_t i = beginLine; i < lines.size(); ++i)
		{
			Line const& lineCur = lines[i];
			if (Det(lineCur.dir, lineCur.point - result) > distance)
			{
				/* Result does not satisfy constraint of line i. */
				TArray<Line> projLines(
					lines.begin(),
					lines.begin() + static_cast<std::ptrdiff_t>(numObstLines)
				);

				for (std::size_t j = numObstLines; j < i; ++j) 
				{
					Line line;

					const float determinant = Det(lineCur.dir, lines[j].dir);

					if (Math::Abs(determinant) <= RVO_EPSILON) 
					{
						/* Line i and line j are parallel. */
						if (lineCur.dir.length2() > 0.0F) 
						{
							/* Line i and line j point in the same direction. */
							continue;
						}

						/* Line i and line j point in opposite direction. */
						line.point = 0.5F * (lineCur.point + lines[j].point);
					}
					else 
					{
						line.point = lineCur.point + (Det(lines[j].dir,
							lineCur.point - lines[j].point) /
							determinant) *
							lineCur.dir;
					}

					line.dir = Math::GetNormal(lines[j].dir - lineCur.dir);
					projLines.push_back(line);
				}

				const Vector2 tempResult = result;

				if (LinearProgram2(projLines, radius, Vector2(-lineCur.dir.y, lineCur.dir.x), true, result) < projLines.size()) 
				{
					/* This should in principle not happen. The result is by definition
					 * already in the feasible region of this linear program. If it fails,
					 * it is due to small floating point error, and the current result is
					 * kept. */
					result = tempResult;
				}

				distance = Det(lineCur.dir, lineCur.point - result);
			}
		}
	}



	/* Search for the best new velocity. */
	void Agent::computeNewVelocity(float timeStep)
	{
		mCachedOrcaLines.clear();


#if ROV_USE_OBSTACLE
		TArray<Obstacle*> obstacleNeighbors_;
		float timeHorizonObst_ = 1.0f;
		const float invTimeHorizonObst = 1.0F / timeHorizonObst_;


		/* Create obstacle ORCA lines. */
		for (std::size_t i = 0U; i < obstacleNeighbors_.size(); ++i) 
		{
			const Obstacle *obstacle1 = obstacleNeighbors_[i];
			const Obstacle *obstacle2 = obstacle1->next;

			const Vector2 relativePosition1 = obstacle1->point - pos;
			const Vector2 relativePosition2 = obstacle2->point - pos;

			/* Check if velocity obstacle of obstacle is already taken care of by
			 * previously constructed obstacle ORCA lines. */
			bool alreadyCovered = false;

			for (std::size_t j = 0U; j < mCachedOrcaLines.size(); ++j) 
			{
				if (Det(invTimeHorizonObst * relativePosition1 - mCachedOrcaLines[j].point, mCachedOrcaLines[j].dir) - 
						invTimeHorizonObst * radius >= -RVO_EPSILON &&
					Det(invTimeHorizonObst * relativePosition2 - mCachedOrcaLines[j].point, mCachedOrcaLines[j].dir) -
						invTimeHorizonObst * radius >= -RVO_EPSILON ) 
				{
					alreadyCovered = true;
					break;
				}
			}

			if (alreadyCovered) {
				continue;
			}

			/* Not yet covered. Check for collisions. */
			const float distSq1 = relativePosition1.length2();
			const float distSq2 = relativePosition2.length2();

			const float radiusSq = radius * radius;

			const Vector2 obstacleVector = obstacle2->point - obstacle1->point;
			const float s = -relativePosition1.dot(obstacleVector) / obstacleVector.length2();
			const float distSqLine = (-relativePosition1 - s * obstacleVector).length2();

			Line line;

			if (s < 0.0F && distSq1 <= radiusSq)
			{
				/* Collision with left vertex. Ignore if non-convex. */
				if (obstacle1->isConvex) 
				{
					line.point = Vector2(0.0F, 0.0F);
					line.dir = Math::GetNormal(Vector2(-relativePosition1.y, relativePosition1.x));
					mCachedOrcaLines.push_back(line);
				}

				continue;
			}

			if (s > 1.0F && distSq2 <= radiusSq) 
			{
				/* Collision with right vertex. Ignore if non-convex or if it will be
				 * taken care of by neighoring obstace */
				if (obstacle2->isConvex && Det(relativePosition2, obstacle2->dir) >= 0.0F) 
				{
					line.point = Vector2(0.0F, 0.0F);
					line.dir = Math::GetNormal(Vector2(-relativePosition2.y, relativePosition2.x));
					mCachedOrcaLines.push_back(line);
				}

				continue;
			}

			if (s >= 0.0F && s <= 1.0F && distSqLine <= radiusSq) 
			{
				/* Collision with obstacle segment. */
				line.point = Vector2(0.0F, 0.0F);
				line.dir = -obstacle1->dir;
				mCachedOrcaLines.push_back(line);
				continue;
			}

			/* No collision. Compute legs. When obliquely viewed, both legs can come
			 * from a single vertex. Legs extend cut-off line when nonconvex vertex. */
			Vector2 leftLegDirection;
			Vector2 rightLegDirection;

			if (s < 0.0F && distSqLine <= radiusSq)
			{
				/* Obstacle viewed obliquely so that left vertex defines velocity
				 * obstacle. */
				if (!obstacle1->isConvex)
				{
					/* Ignore obstacle. */
					continue;
				}

				obstacle2 = obstacle1;

				const float leg1 = std::sqrt(distSq1 - radiusSq);
				leftLegDirection =
					Vector2(
						relativePosition1.x * leg1 - relativePosition1.y * radius,
						relativePosition1.x * radius + relativePosition1.y * leg1) /
					distSq1;
				rightLegDirection =
					Vector2(
						relativePosition1.x * leg1 + relativePosition1.y * radius,
						-relativePosition1.x * radius + relativePosition1.y * leg1) /
					distSq1;
			}
			else if (s > 1.0F && distSqLine <= radiusSq)
			{
				/* Obstacle viewed obliquely so that right vertex defines velocity
				 * obstacle. */
				if (!obstacle2->isConvex)
				{
					/* Ignore obstacle. */
					continue;
				}

				obstacle1 = obstacle2;

				const float leg2 = std::sqrt(distSq2 - radiusSq);
				leftLegDirection =
					Vector2(
						relativePosition2.x * leg2 - relativePosition2.y * radius,
						relativePosition2.x * radius + relativePosition2.y * leg2) /
					distSq2;
				rightLegDirection =
					Vector2(
						relativePosition2.x * leg2 + relativePosition2.y * radius,
						-relativePosition2.x * radius + relativePosition2.y * leg2) /
					distSq2;
			}
			else
			{
				/* Usual situation. */
				if (obstacle1->isConvex)
				{
					const float leg1 = std::sqrt(distSq1 - radiusSq);
					leftLegDirection = Vector2(relativePosition1.x * leg1 -
						relativePosition1.y * radius,
						relativePosition1.x * radius +
						relativePosition1.y * leg1) /
						distSq1;
				}
				else
				{
					/* Left vertex non-convex; left leg extends cut-off line. */
					leftLegDirection = -obstacle1->dir;
				}

				if (obstacle2->isConvex)
				{
					const float leg2 = std::sqrt(distSq2 - radiusSq);
					rightLegDirection = Vector2(relativePosition2.x * leg2 +
						relativePosition2.y * radius,
						-relativePosition2.x * radius +
						relativePosition2.y * leg2) /
						distSq2;
				}
				else 
				{
					/* Right vertex non-convex; right leg extends cut-off line. */
					rightLegDirection = obstacle1->dir;
				}
			}

			/* Legs can never point into neighboring edge when convex vertex, take
			 * cutoff-line of neighboring edge instead. If velocity projected on
			 * "foreign" leg, no constraint is added. */
			const Obstacle *const leftNeighbor = obstacle1->previous;

			bool isLeftLegForeign = false;
			bool isRightLegForeign = false;

			if (obstacle1->isConvex &&
				Det(leftLegDirection, -leftNeighbor->dir) >= 0.0F) 
			{
				/* Left leg points into obstacle. */
				leftLegDirection = -leftNeighbor->dir;
				isLeftLegForeign = true;
			}

			if (obstacle2->isConvex &&
				Det(rightLegDirection, obstacle2->dir) <= 0.0F)
			{
				/* Right leg points into obstacle. */
				rightLegDirection = obstacle2->dir;
				isRightLegForeign = true;
			}

			/* Compute cut-off centers. */
			const Vector2 leftCutoff =
				invTimeHorizonObst * (obstacle1->point - pos);
			const Vector2 rightCutoff =
				invTimeHorizonObst * (obstacle2->point - pos);
			const Vector2 cutoffVector = rightCutoff - leftCutoff;

			/* Project current velocity on velocity obstacle. */

			/* Check if current velocity is projected on cutoff circles. */
			const float t = obstacle1 == obstacle2 ? 0.5F : (velocity - leftCutoff).dot(cutoffVector) / cutoffVector.length2();
			const float tLeft = (velocity - leftCutoff).dot(leftLegDirection);
			const float tRight = (velocity - rightCutoff).dot(rightLegDirection);

			if ((t < 0.0F && tLeft < 0.0F) ||
				(obstacle1 == obstacle2 && tLeft < 0.0F && tRight < 0.0F)) 
			{
				/* Project on left cut-off circle. */
				const Vector2 unitW = Math::GetNormal(velocity - leftCutoff);

				line.dir = Vector2(unitW.y, -unitW.x);
				line.point = leftCutoff + radius * invTimeHorizonObst * unitW;
				mCachedOrcaLines.push_back(line);
				continue;
			}

			if (t > 1.0F && tRight < 0.0F) 
			{
				/* Project on right cut-off circle. */
				const Vector2 unitW = Math::GetNormal(velocity - rightCutoff);

				line.dir = Vector2(unitW.y, -unitW.x);
				line.point = rightCutoff + radius * invTimeHorizonObst * unitW;
				mCachedOrcaLines.push_back(line);
				continue;
			}

			/* Project on left leg, right leg, or cut-off line, whichever is closest to
			 * velocity. */
			const float distSqCutoff = (t < 0.0F || t > 1.0F || obstacle1 == obstacle2) ? 
				std::numeric_limits<float>::infinity() : (velocity - (leftCutoff + t * cutoffVector)).length2();
			const float distSqLeft = tLeft < 0.0F ? 
				std::numeric_limits<float>::infinity() : (velocity - (leftCutoff + tLeft * leftLegDirection)).length2();
			const float distSqRight = tRight < 0.0F ? 
				std::numeric_limits<float>::infinity() : (velocity - (rightCutoff + tRight * rightLegDirection)).length2();

			if (distSqCutoff <= distSqLeft && distSqCutoff <= distSqRight)
			{
				/* Project on cut-off line. */
				line.dir = -obstacle1->dir;
				line.point =
					leftCutoff + radius * invTimeHorizonObst *
					Vector2(-line.dir.y, line.dir.x);
				mCachedOrcaLines.push_back(line);
				continue;
			}

			if (distSqLeft <= distSqRight)
			{
				/* Project on left leg. */
				if (isLeftLegForeign) 
				{
					continue;
				}

				line.dir = leftLegDirection;
				line.point = leftCutoff + radius * invTimeHorizonObst * Vector2(-line.dir.y, line.dir.x);
				mCachedOrcaLines.push_back(line);
				continue;
			}

			/* Project on right leg. */
			if (isRightLegForeign) 
			{
				continue;
			}

			line.dir = -rightLegDirection;
			line.point = rightCutoff + ( radius * invTimeHorizonObst ) * Vector2(-line.dir.y, line.dir.x);
			mCachedOrcaLines.push_back(line);
		}
		const std::size_t numObstLines = mCachedOrcaLines.size();
	#else
		const std::size_t numObstLines = 0;
	#endif

		const float invTimeHorizon = 1.0F / timeHorizon;

		/* Create agent ORCA lines. */
		for (std::size_t i = 0U; i < mAgentNeighbors.size(); ++i) 
		{
			Agent* other = mAgentNeighbors[i];

			const Vector2 relativePosition = other->pos - pos;
			const Vector2 relativeVelocity = velocity - other->velocity;
			const float distSq = relativePosition.length2();
			const float combinedRadius = radius + other->radius;
			const float combinedRadiusSq = combinedRadius * combinedRadius;

			Line line;
			Vector2 u;

			if (distSq > combinedRadiusSq)
			{
				/* No collision. */
				const Vector2 w = relativeVelocity - invTimeHorizon * relativePosition;
				/* Vector from cutoff center to relative velocity. */
				const float wLengthSq = w.length2();

				const float dotProduct = w.dot(relativePosition);

				if (dotProduct < 0.0F && dotProduct * dotProduct > combinedRadiusSq * wLengthSq) 
				{
					/* Project on cut-off circle. */
					const float wLength = Math::Sqrt(wLengthSq);
					const Vector2 unitW = w / wLength;

					line.dir = Vector2(unitW.y, -unitW.x);
					u = (combinedRadius * invTimeHorizon - wLength) * unitW;
				}
				else 
				{
					/* Project on legs. */
					const float leg = Math::Sqrt(distSq - combinedRadiusSq);

					if (Det(relativePosition, w) > 0.0F) 
					{
						/* Project on left leg. */
						line.dir = Vector2(
							relativePosition.x * leg - relativePosition.y * combinedRadius,
							relativePosition.x * combinedRadius + relativePosition.y * leg 
						) / distSq;
					}
					else 
					{
						/* Project on right leg. */
						line.dir = -Vector2(
							relativePosition.x * leg + relativePosition.y * combinedRadius,
							-relativePosition.x * combinedRadius + relativePosition.y * leg
						) / distSq;
					}

					u = relativeVelocity.dot(line.dir) * line.dir - relativeVelocity;
				}
			}
			else 
			{
				/* Collision. Project on cut-off circle of time timeStep. */
				const float invTimeStep = 1.0F / timeStep;

				/* Vector from cutoff center to relative velocity. */
				const Vector2 w = relativeVelocity - invTimeStep * relativePosition;

				const float wLength = w.length();
				const Vector2 unitW = w / wLength;

				line.dir = Vector2(unitW.y, -unitW.x);
				u = (combinedRadius * invTimeStep - wLength) * unitW;
			}

			float factor = maxSpeed / (maxSpeed + other->maxSpeed);
			line.point = velocity + factor * u;
			mCachedOrcaLines.push_back(line);
		}

		const std::size_t lineFail = LinearProgram2(mCachedOrcaLines, maxSpeed, velocityPref, false, velocityNew);

		if (lineFail < mCachedOrcaLines.size()) 
		{
			LinearProgram3(mCachedOrcaLines, numObstLines, lineFail, maxSpeed, velocityNew);
		}
	}

	class TestStage : public StageBase, public IGameRenderSetup
	{
		using BaseClass = StageBase;
	public:
		TestStage() {}

		TArray< std::unique_ptr<Agent> > mAgents;

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;
			::Global::GUI().cleanupWidget();
			restart();

			auto frame = WidgetUtility::CreateDevFrame();
			frame->addCheckBox("Simulate" , bSimulating);
			frame->addButton("Step", [this](int event, GWidget*)
			{
				simulateStep();
				return true;
			});
			FWidgetProperty::Bind(frame->addSlider("Time Horizon"), timeHorizon , 0.0f, 10.0f );
			return true;
		}

		void onEnd() override
		{
			BaseClass::onEnd();
		}


		bool bSimulating = true;
		float timeHorizon = 0.5;

		void restart()
		{
			mAgents.clear();


			float maxSpeed = 100;
#if 0
			{
				auto agent = std::make_unique<Agent>();
				agent->pos = Vector2(100, 300.1);
				agent->goal = Vector2(600, 300);
				agent->maxSpeed = maxSpeed;
				agent->radius = 50;
				agent->timeHorizon = timeHorizon;
				mAgents.push_back(std::move(agent));
			}
			{
				auto agent = std::make_unique<Agent>();
				agent->pos = Vector2(600, 300);
				agent->goal = Vector2(100, 300);
				agent->maxSpeed = maxSpeed;
				agent->radius = 50;
				agent->timeHorizon = timeHorizon;
				mAgents.push_back(std::move(agent));
			}
#else

			Vector2 center = 0.5 * Vector2(Global::GetScreenSize());
			float radius = 200;
			int num = 3;
			for (int i = 0; i < num; ++i)
			{
				auto agent = std::make_unique<Agent>();
				float s,c;
				Math::SinCos(2 * Math::PI * i / num , s, c);
				agent->pos = center + ( radius ) * Vector2(c, s) + 5 * Vector2(RandFloat(), RandFloat());
				agent->goal = center - radius * Vector2(c, s) + Vector2(0,0);
				agent->maxSpeed = maxSpeed;
				agent->radius = 15;
				agent->timeHorizon = timeHorizon;
				mAgents.push_back(std::move(agent));
			}
#endif

			for (int i = 0; i < mAgents.size(); ++i)
			{
				for (int j = 0; j < mAgents.size(); ++j)
				{
					if (i == j)
						continue;

					mAgents[i]->mAgentNeighbors.push_back(mAgents[j].get());
				}
			}
		}


		void simulateStep()
		{
			float dt = float(gDefaultTickTime) / 1000;

			for (auto& agent : mAgents)
			{
				agent->updatePrefVelocity(dt);
			}

			for (auto& agent : mAgents)
			{
				agent->computeNewVelocity(dt);
			}

			for (auto& agent : mAgents)
			{
				agent->update(dt);
			}


		}
		void tick()
		{
			if (bSimulating)
			{
				simulateStep();
			}
		}
		void updateFrame(int frame) {}

		void onUpdate(long time) override
		{
			BaseClass::onUpdate(time);



			int frame = time / gDefaultTickTime;
			for (int i = 0; i < frame; ++i)
				tick();

			updateFrame(frame);
		}

		void onRender(float dFrame) override
		{
			RHIGraphics2D& g = Global::GetRHIGraphics2D();
			auto& commandList = RHICommandList::GetImmediateList();
			RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0.2, 0.2, 0.2, 1), 1);
			
			g.beginRender();

			RenderUtility::SetPen(g, EColor::Black);
			RenderUtility::SetBrush(g, EColor::Orange);
			for (auto const& agent : mAgents)
			{
				g.drawCircle(agent->pos, agent->radius);
			}

			for (auto const& agent : mAgents)
			{
				if (agent->trace.size() > 2)
				{
					RenderUtility::SetPen(g, EColor::Blue);
					g.drawLineStrip(agent->trace.data(), agent->trace.size());
				}

				RenderUtility::SetPen(g, EColor::Yellow);
				g.setPenWidth(2);
				g.drawLine(agent->pos, agent->pos + agent->velocity);
				g.setPenWidth(1);
			}

			for(auto other : mAgents[0]->mAgentNeighbors)
			{
				drawDebug(g, *mAgents[0], *other);
			}

			g.drawTextF(Vector2(10, 10), "Speed = %f", mAgents[0]->velocity.length());
			g.endRender();
		}

		void drawArea(RHIGraphics2D& g, Plane const& plane)
		{
			using namespace Math;
			EPlaneSide::Enum sides[4];
			float dists[4];
			Vector2 screenSize = ::Global::GetScreenSize();
			Vector2 screenVertices[] =
			{
				Vector2(0,0),
				Vector2(screenSize.x , 0),
				screenSize,
				Vector2(0, screenSize.y),
			};

			for (int i = 0; i < 4; ++i)
			{
				sides[i] = plane.testSide(screenVertices[i] , 0.01f, dists[i]);
			}

			Vector2 vertices[5];
			int nV= 0;


			EPlaneSide::Enum sidePrev = sides[3];
			for (int i = 0; i < 4; ++i)
			{
				auto sideCur = sides[i];
				if (sideCur != sidePrev && sideCur != 0)
				{
					Vector2 dir = screenVertices[i] - screenVertices[(i + 3)%4];
					dir.normalize();
					vertices[nV++] = screenVertices[i] - dir * (dists[i] / dir.dot(plane.getNormal()));
				}

				if (sideCur != EPlaneSide::Back)
				{
					vertices[nV++] = screenVertices[i];
				}

				sidePrev = sideCur;
			}
			g.drawPolygon(vertices, nV);
		}

		void drawDebug(RHIGraphics2D& g, Agent const& agent, Agent const& other)
		{
			const Vector2 relativePosition = other.pos - agent.pos;
			const Vector2 relativeVelocity = agent.velocity - other.velocity;
			const float distSq = relativePosition.length2();
			const float combinedRadius = agent.radius + other.radius;
			const float combinedRadiusSq = combinedRadius * combinedRadius;
			const float dist = Math::Sqrt(distSq);
			float leg = Math::Sqrt(distSq - combinedRadiusSq);

			float s = combinedRadius / distSq;
			float c = leg / distSq;

			Math::Matrix2 rotation;
			rotation.setValue(c, s, -s, c);
			Math::Matrix2 rotationInv;
			rotationInv.setValue(c, -s, s, c);

			Vector2 dirL = relativePosition * rotation;
			Vector2 dirR = relativePosition * rotationInv;
			float L = 1000 * dist / c;
			Vector2 rPos = agent.pos + other.velocity;
			Vector2 areaV[4] =
			{
				rPos + leg * dirL,
				rPos + L * dirL,
				rPos + L * dirR,
				rPos + leg * dirR,
			};


			g.enablePen(false);

			g.setCustomRenderState([](Render::RHICommandList& commandList, Render::RenderBatchedElement& element)
			{
				RHIClearRenderTargets(commandList, EClearBits::Stencil, nullptr, 0, 1.0, 0);
				RHISetDepthStencilState(commandList, TStaticDepthStencilState<
					false, ECompareFunc::Always, true, ECompareFunc::Always,
					EStencil::Keep, EStencil::Replace, EStencil::Replace, 0x1 >::GetRHI(), 0x1);
				RHISetBlendState(commandList, TStaticBlendState<CWM_None>::GetRHI());
			});

			if (distSq > combinedRadiusSq)
			{
				g.drawPolygon(areaV, ARRAY_SIZE(areaV));
				g.drawCircle(rPos + relativePosition, combinedRadius);
			}
			else
			{
				drawArea(g, Plane{ relativePosition , rPos + (1 - combinedRadius / dist) * relativePosition });
			}

			g.setCustomRenderState([](Render::RHICommandList& commandList, Render::RenderBatchedElement& element)
			{
				RHISetDepthStencilState(commandList, TStaticDepthStencilState<
					false, ECompareFunc::Always, true, ECompareFunc::Equal,
					EStencil::Keep, EStencil::Keep, EStencil::Keep, 0x1 >::GetRHI(), 0x1);
				RHISetBlendState(commandList, StaticTranslucentBlendState::GetRHI());
			});

			RenderUtility::SetBrush(g, EColor::Red);
			g.setBlendAlpha(0.2);
			g.drawRect(Vector2(0, 0), ::Global::GetScreenSize());

			g.resetRenderState();



			RenderUtility::SetBrush(g, EColor::Pink);
			g.beginBlend(0.2f , ESimpleBlendMode::Translucent);
			for( auto const& line : agent.mCachedOrcaLines )
			{
				drawArea(g, Plane{ Vector2(line.dir.y, line.dir.x) , agent.pos + line.point });
			}
			g.endBlend();
			g.setBlendAlpha(1.0);
			g.enablePen(true);
		}


		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::R: restart(); break;
				}
			}
			return BaseClass::onKey(msg);
		}

		bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch (id)
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::D3D11;
		}


		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
		{
			systemConfigs.bWasUsedPlatformGraphics = true;
		}

	protected:
	};

	REGISTER_STAGE_ENTRY("RVO Test", TestStage, EExecGroup::MiscTest, "AI");
}