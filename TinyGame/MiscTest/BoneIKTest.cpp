#include "Stage/TestStageHeader.h"

#include "WidgetUtility.h"
#include "Math/Base.h"
#include "Math/Math2D.h"

namespace BoneIK
{
	using namespace Math2D;

	typedef Vector2D Vec2f;
	struct BoneState
	{
		Vec2f pos;
		float rotation;
	};

	struct Bone
	{
		Vec2f pos;
		float angles[2];
	};
	struct BoneContraint
	{
		bool  bUsed;
		float value[2];
	};

	struct BoneLink
	{
		//int   LinkBone;
		float dist;
		BoneContraint contraint;
	};



	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		TestStage() {}

		virtual bool onInit()
		{
			if( !BaseClass::onInit() )
				return false;

			Vec2f v1(1, 0);
			Vec2f v2 = Normalize(Vec2f(1,-1));

			Rotation rotation = Rotation::Make(v1, v2);


			Vec2f v3 = rotation.rotate(v1);

			initBones({ Vec2f(0,0) , Vec2f(0,10) , Vec2f(0,30) , Vec2f(0,35) , Vec2f(0,50) });
			::Global::GUI().cleanupWidget();
			auto frame = WidgetUtility::CreateDevFrame();
			restart();
			return true;
		}

		virtual void onEnd()
		{
			BaseClass::onEnd();
		}

		virtual void onUpdate(long time)
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for( int i = 0; i < frame; ++i )
				tick();

			updateFrame(frame);
		}

		void onRender(float dFrame)
		{
			Graphics2D& g = Global::getGraphics2D();

			bool haveIKState = mBoneState.empty() == false;

			RenderUtility::setPen(g, haveIKState ? Color::eGray : Color::eGreen);
			RenderUtility::setBrush(g, Color::eNull);

			Vec2i const RectSize(8, 8);
			for( int i = 0; i < mBones.size(); ++i )
			{
				Vec2i rPos = ToScreenPos(mBones[i].pos);
				g.drawRect(rPos - RectSize / 2 , RectSize );
			}



			if( haveIKState )
			{
				RenderUtility::setPen(g, Color::eGreen);
				RenderUtility::setBrush(g, Color::eNull);

				for( int i = 0; i < mLinks.size(); ++i )
				{
					Vec2i rPos1 = ToScreenPos(mBoneState[i].pos);
					Vec2i rPos2 = ToScreenPos(mBoneState[i + 1].pos);

					g.drawLine(rPos1, rPos2);
				}

				for( int i = 0; i < mBoneState.size(); ++i )
				{
					Vec2i rPos = ToScreenPos(mBoneState[i].pos);
					g.drawRect(rPos - RectSize / 2 , RectSize );
				}
			}
		}

		Vec2f ToScreenPos(Vec2f const& pos)
		{
			return 4 * pos + Vec2f(400, 300);
		}
		Vec2f ToWorldPos(Vec2f const& sPos)
		{
			return (sPos - Vec2f(400, 300)) / 4;
		}
		void restart() {}
		void tick() {}
		void updateFrame(int frame) {}
		bool onMouse(MouseMsg const& msg)
		{
			if( !BaseClass::onMouse(msg) )
				return false;

			if( msg.onLeftDown() )
			{
				mLastTargetPos = ToWorldPos(msg.getPos());
				solveIK();
			}
			return true;
		}

		void solveIK()
		{

			mBoneState.resize(mBones.size());
			for( int i = 0; i < mBones.size(); ++i )
			{
				mBoneState[i].pos = mBones[i].pos;
			}

			switch( mUsageMethod )
			{
			case BoneIK::TestStage::FABRIK:
				solveFabrik(mBoneState, mLastTargetPos, 0);
				break;
			case BoneIK::TestStage::CCD:
				solveCCD(mBoneState, mLastTargetPos, 0);
				break;
			default:
				break;
			}
		}

		enum SolveMethod
		{
			FABRIK,
			CCD,
		};
		void changeMethod(SolveMethod method)
		{
			if( mUsageMethod == method )
				return;
			mUsageMethod = method;
			solveIK();
		}

		bool onKey(unsigned key, bool isDown)
		{
			if( !isDown )
				return false;
			switch( key )
			{
			case Keyboard::eR: restart(); break;
			case Keyboard::eNUM1:
				changeMethod( SolveMethod::FABRIK );
				break;
			case Keyboard::eNUM2:
				changeMethod( SolveMethod::CCD );
				break;

			}
			return false;
		}



		void initBones(std::vector< Vec2f > const& positions)
		{
			mBones.resize(positions.size());
			for( int i = 0; i < positions.size(); ++i )
			{
				mBones[i].pos = positions[i];
			}
			mLinks.resize(mBones.size() - 1);
			for( int i = 0; i < mLinks.size(); ++i )
			{
				auto& link = mLinks[i];
				link.dist = Distance(mBones[i + 1].pos, mBones[i].pos);
				//link.LinkBone = i + 1;
			}
		}



		SolveMethod mUsageMethod = SolveMethod::FABRIK;
		std::vector< Bone > mBones;
		std::vector< BoneLink > mLinks;
		std::vector< BoneState > mBoneState;

		Vec2f mLastTargetPos;

		void solveFabrik(std::vector< BoneState >& inoutState , Vec2f const& targetPos , float targetRotation )
		{
			assert(mBones.size() <= inoutState.size());

			int indexEffector = mBones.size() - 1;

			float maxDistance = 0;
			for( auto const& link : mLinks )
			{
				maxDistance += link.dist;
			}

			Vec2f targetOffset = targetPos - inoutState[0].pos;
			if( targetOffset.length2() > maxDistance * maxDistance )
			{
				for( int i = 0; i < indexEffector; ++i )
				{
					float dist = Distance(targetOffset, inoutState[i].pos);
					inoutState[i+1].pos = Math::LinearLerp(inoutState[i].pos, targetOffset, mLinks[i].dist / dist);
				}
			}
			else
			{
				int numIterator = 0;
				Vec2f startPos = inoutState[0].pos;
				float tol = 1e-2;
				float dist = Distance(inoutState[indexEffector].pos, targetPos);
				while( dist > tol )
				{
					inoutState[indexEffector].pos = targetPos;
					inoutState[indexEffector].rotation = targetRotation;
					for( int i = indexEffector - 1; i >= 0; --i )
					{
						Vec2f dir = Normalize(inoutState[i + 1].pos - inoutState[i].pos);
						inoutState[i].pos = inoutState[i+1].pos -  mLinks[i].dist * dir;
						float rotation = Math::ATan2(dir.y, dir.x);
					}

					inoutState[0].pos = startPos;
					for( int i = 0; i < indexEffector; ++i )
					{
						Vec2f dir = Normalize(inoutState[i + 1].pos - inoutState[i].pos);
						inoutState[i+1].pos = inoutState[i].pos + mLinks[i].dist * dir;
					}

					dist = Distance(inoutState[indexEffector].pos, targetPos);

					++numIterator;
				}

				::Msg("FABRIK IterNum = %d", numIterator);
			} 
		}

		void solveCCD(std::vector< BoneState >& inoutState, Vec2f const& targetPos, float targetRotation)
		{

			assert(mBones.size() <= inoutState.size());

			int indexEffector = mBones.size() - 1;

			float maxDistance = 0;
			for( auto const& link : mLinks )
			{
				maxDistance += link.dist;
			}

			Vec2f targetOffset = targetPos - inoutState[0].pos;
			if( targetOffset.length2() > maxDistance * maxDistance )
			{
				for( int i = 0; i < indexEffector; ++i )
				{
					float dist = Distance(targetOffset, inoutState[i].pos);
					inoutState[i + 1].pos = Math::LinearLerp(inoutState[i].pos, targetOffset, mLinks[i].dist / dist);
				}
			}
			else
			{
				int numIterator = 0;
				Vec2f startPos = inoutState[0].pos;
				float tol = 1e-2;
				float dist = Distance(inoutState[indexEffector].pos, targetPos);
				while( dist > tol )
				{
					for( int i = indexEffector - 1; i >= 0; --i )
					{
						Vec2f dirA = Normalize(inoutState[indexEffector].pos - inoutState[i].pos);
						Vec2f dirB = Normalize(targetPos - inoutState[i].pos);
						Rotation rotation = Rotation::Make(dirA, dirB);
						for( int n = indexEffector ; n != i; --n )
						{
							inoutState[n].pos = inoutState[i].pos + rotation.rotate(inoutState[n].pos - inoutState[i].pos);
						}

					}

					dist = Distance(inoutState[indexEffector].pos, targetPos);
					++numIterator;
				}

				::Msg("CCD IterNum = %d", numIterator);
			}

		}
	protected:
	};



	REGISTER_STAGE("Bone IK", TestStage, StageRegisterGroup::PhyDev);

}





