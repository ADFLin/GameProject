#pragma once
#ifndef SceneObject_H_39A04D57_E8EC_4831_855C_91B74D94377E
#define SceneObject_H_39A04D57_E8EC_4831_855C_91B74D94377E

#include "Renderer/Mesh.h"

#include "LazyObject.h"
#include "OpenGLCommon.h"
#include "Scene.h"

namespace Render
{
	class RenderContext;
	class Material;
	class PrimitivesCollection;

	//#MOVE
	enum class TrackMode
	{
		Once,
		Cycle,
		Oscillation ,
	};

	enum class CurveInterpMode
	{
		Linear ,
		Const ,
		Cubic ,
	};
	class CurveTrackBase
	{
	public:
		static float GetCycleTime(float time, float timeMin, float timeMax, int& cycleCount)
		{
			assert(timeMin <= timeMax);

			float delta = timeMax - timeMin;

			float result = time;
			if( time > timeMax )
			{
				cycleCount = Math::FloorToInt((time - timeMin) / delta);
				result = time - cycleCount * delta;
			}
			else if( time < timeMin )
			{
				cycleCount = Math::FloorToInt((timeMax - time) / delta);
				result = time + cycleCount * delta;
			}

			if( result == timeMin && time > timeMax )
			{
				result = timeMax;
			}
			if( result == timeMax && time < timeMin )
			{
				result = timeMin;
			}
			return result;
		}

	};
	template< class T >
	class TCurveTrack : public CurveTrackBase
	{
	public:
		TCurveTrack()
		{
			mode = TrackMode::Once;
		}
		struct CurveKey
		{
			CurveKey( float inTime, T const& inValue)
				:mode(CurveInterpMode::Cubic)
				,time(inTime) 
				,value(inValue)
			    ,tangentIn() 
				,tangentOut()
			{

			}
			CurveKey(float inTime, T const& inValue , T const& inTangentIn , T const& inTangentOut )
				:mode(CurveInterpMode::Cubic)
				mtime(inTime)
				,value(inValue)
				,tangentIn(inTangentIn)
				,tangentOut(inTangentOut)
			{

			}

			CurveInterpMode mode;
			float time;
			T     value;
			T     tangentIn;
			T     tangentOut;
		};

		int addKey(float time, T const& value)
		{
			int idx = getNextKeyIndex(time);
			if( idx != mKeys.size() )
				mKeys.insert(mKeys.begin() + idx, { time, value });
			else
				mKeys.push_back({ time, value });
			return idx;
		}

		void setKeyInterpMode(int idx, CurveInterpMode mode)
		{
			assert(0 <= idx && idx < mKeys.size());
			mKeys[idx].mode = mode;
		}
		void setKeyTangent(int idx, T const& tangentIn, T const& tangentOut)
		{
			assert(0 <= idx && idx < mKeys.size());
			mKeys[idx].tangentIn = tangentIn;
			mKeys[idx].tangentOut = tangentOut;
		}

		float remapTime( float time ) const
		{
			if( mKeys.size() < 2 )
				return time;

			if( mode != TrackMode::Cycle && mode != TrackMode::Oscillation )
			{
				return time;
			}

			float const timeMin = mKeys[0].time;
			float const timeMax = mKeys.back().time;
			
			int cycleCount = 0;
			float actualTime = GetCycleTime(time, timeMin, timeMax, cycleCount);

			if( mode == TrackMode::Oscillation )
			{
				if( (cycleCount % 2) == 1 )
				{
					actualTime = timeMin + ( timeMax - actualTime );
				}
			}
			return actualTime;
		}

		T getValue(float time) const
		{
			if( mKeys.empty() )
				return T();

			float actualTime = remapTime(time);

			if( actualTime <= mKeys[0].time )
				return mKeys[0].value;
			if( actualTime >= mKeys.back().time )
				return mKeys.back().value;

			int idx = getNextKeyIndex(actualTime);
			CurveKey const& keyNext = mKeys[idx];
			CurveKey const& keyCur = mKeys[idx - 1];
			float alpha = (actualTime - keyCur.time ) / (keyNext.time - keyCur.time );

			switch( keyCur.mode )
			{
			case CurveInterpMode::Const:
				return keyCur.value;
			case CurveInterpMode::Cubic:
				return Math::BezierLerp(keyCur.value, keyCur.value + keyCur.tangentOut / 3,
										keyNext.value - keyNext.tangentIn / 3, keyNext.value, alpha);
			case CurveInterpMode::Linear:
				return Math::LinearLerp(keyCur.value, keyNext.value, alpha);
			}

			return keyCur.value;
		}


		int getNextKeyIndex(float time) const
		{
			int count = mKeys.size();
			int idxCur = 0;
			while( count > 0 )
			{
				int step = count / 2;
				int idx = idxCur + step;
				if( !(time < mKeys[idx].time) )
				{
					idxCur = idx + 1;
					count -= step + 1;
				}
				else
				{
					count = step;
				}
			}

			return idxCur;
		}

		bool bSmooth = true;
		TrackMode mode;
		std::vector< CurveKey > mKeys;
	};

	class VectorCurveTrack : public TCurveTrack< Vector3 > {};
	class ScaleCurveTrack : public TCurveTrack< float > {};

	struct CycleTrack
	{
		Vector3 planeNormal;
		Vector3 center;
		float   radius;
		float   period;

		Vector3 getValue(float time)
		{
			float theta = 2 * Math::PI * (time / period);
			Quaternion q;
			q.setRotation(planeNormal, theta);

			Vector3 p;
			if( planeNormal.z < planeNormal.y )
				p = Vector3(planeNormal.y, -planeNormal.x, 0);
			else
				p = Vector3(0, -planeNormal.z, planeNormal.y);
			p.normalize();
			return center + radius * q.rotate(p);
		}
	};

	typedef std::shared_ptr< Material > MaterialPtr;
	class StaticMesh : public Mesh
	{
	public:
		void postLoad()
		{



		}

		void render(Matrix4 const& worldTrans, RenderContext& context, Material* material);
		void render(Matrix4 const& worldTrans, RenderContext& context);

		void setMaterial(int idx, MaterialPtr material)
		{
			if( idx >= mMaterials.size() )
				mMaterials.resize(idx + 1);
			mMaterials[idx] = material;
		}
		Material* getMaterial(int idx)
		{
			if( idx < mMaterials.size() )
				return mMaterials[idx].get();
			return nullptr;
		}
		std::vector< MaterialPtr > mMaterials;
		std::string name;
	};

	class StaticMeshObject : public SceneObject
	{
	public:
		TLazyObjectPtr<StaticMesh> meshPtr;
		Matrix4  worldTransform;

		virtual void getPrimitives(PrimitivesCollection& primitiveCollection);
	};

	class SimpleMeshObject : public SceneObject
	{
	public:
		Mesh*     mesh;
		Matrix4   worldTransform;
		Vector4   color;
		std::shared_ptr< Material > material;

		virtual void getPrimitives(PrimitivesCollection& primitiveCollection);

		REFLECT_OBJECT_BEGIN(SimpleMeshObject)
			REF_PROPERTY(mesh)
			REF_PROPERTY(worldTransform)
			REF_PROPERTY(color)
			REF_PROPERTY(material)
		REFLECT_OBJECT_END()
	};

	class DebugGeometryObject : public SceneObject
	{
	public:
		virtual void getDynamicPrimitives(PrimitivesCollection& primitiveCollection, ViewInfo& view);

		void addLine(Vector3 const& posStart, Vector3 const& posEnd, Vector3 const& color, float thickness = 1, float time = -1);

		virtual void tick( float deltaTime ) override;

		struct LineElement : LineBatch
		{
			float time;
		};

		struct PointElement
		{
			float time;
		};
		std::vector< LineElement > mLines;
		std::vector< PointElement > mPoints;

	};


	class SkeltalMesh : public SceneObject
	{



	};
}

#endif // SceneObject_H_39A04D57_E8EC_4831_855C_91B74D94377E

