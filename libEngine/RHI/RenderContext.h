#pragma once
#ifndef RenderContext_H_6EE6BADB_9AEF_4C48_867D_600B4D1CF4DE
#define RenderContext_H_6EE6BADB_9AEF_4C48_867D_600B4D1CF4DE

#include "RHICommon.h"

#include "Material.h"

namespace Render
{
	class RenderContext;
	class VertexFactory;

	class SceneLight;

	enum class LightType
	{
		Spot,
		Point,
		Directional,
	};


	class SceneInterface
	{
	public:
		virtual void render(RenderContext& param) = 0;
		//virtual void renderShadow(LightInfo const& info , RenderContext& param) = 0;
		virtual void renderTranslucent(RenderContext& param) = 0;
	};

	//#MOVE
	// N.dot( V ) = d
	class  Plane
	{
	public:
		Plane() {}

		Plane(Vector3 const& v0, Vector3 const& v1, Vector3 const&  v2)
		{
			Vector3 d1 = v1 - v0;
			Vector3 d2 = v2 - v0;
			mNormal = d1.cross(d2);
			mNormal.normalize();

			mDistance = mNormal.dot(v0);
		}

		Plane(Vector3 const& normal, Vector3 const& pos)
			:mNormal(normal)
		{
			mNormal.normalize();
			mDistance = mNormal.dot(pos);
		}

		Plane(Vector3 const& n, float d)
			:mNormal(n)
			, mDistance(d)
		{
			mNormal.normalize();
		}

		static Plane FrameVector4(Vector4 const& v)
		{
			Vector3 n = v.xyz();
			float mag = n.normalize();
			return Plane(n, -v.w / mag);
		}

#if 0
		void swap(Plane& p)
		{
			using std::swap;

			swap(mNormal, p.mNormal);
			swap(mDistance, p.mDistance);
		}
#endif

		float calcDistance(Vector3 const& p) const
		{
			return p.dot(mNormal) - mDistance;
		}
	private:
		Vector3 mNormal;
		float   mDistance;
	};

	struct ViewInfo
	{
		Matrix4 worldToClip;
		Matrix4 worldToView;
		Matrix4 viewToWorld;
		Matrix4 viewToClip;
		Matrix4 clipToWorld;
		Matrix4 clipToView;

		Vector3 worldPos;
		Vector3 direction;
		float   gameTime;
		float   realTime;

		IntVector2 rectSize;
		IntVector2 rectOffset;


		Plane frustumPlanes[6];

		RHIUniformBufferRef mUniformBuffer;
		bool   mbDataDirty = true;

		bool frustumTest(Vector3 const& pos, float radius) const
		{
			for( int i = 0; i < 6; ++i )
			{
				if( frustumPlanes[i].calcDistance(pos) > radius )
					return false;
			}
			return true;
		}

		Vector3 getViewForwardDir() const
		{
			return TransformVector(Vector3(0, 0, -1), viewToWorld);
		}
		Vector3 getViewRightDir() const
		{
			return TransformVector(Vector3(1, 0, 0), viewToWorld);
		}
		Vector3 getViewUpDir() const
		{
			return TransformVector(Vector3(0, 1, 0), viewToWorld);
		}

		void setupTransform(Matrix4 const& inViewMatrix, Matrix4 const& inProjectMatrix);

		IntVector2 getViewportSize() const;
		void  setupShader(ShaderProgram& program);


		void updateFrustumPlanes();
	};

	

	class RenderTechnique
	{
	public:
		virtual MaterialShaderProgram* getMaterialShader(RenderContext& context, MaterialMaster& material , VertexFactory* vertexFactory ) { return nullptr; }
		virtual void setupMaterialShader(RenderContext& context, MaterialShaderProgram& program) {}
		virtual void setupWorld(RenderContext& context, Matrix4 const& mat );
		virtual bool isShaderPipline() { return true; }
	};

	class RenderContext
	{
	public:
		RenderContext( ViewInfo& view , RenderTechnique& techique)
			:mTechique(&techique)
			,mCurView( &view )
		{
			bBindAttrib  = techique.isShaderPipline();
			mUsageProgram = nullptr;
			bBindAttrib  = false;
		}

		ViewInfo& getView() { return *mCurView; }

		void setupTechique(RenderTechnique& techique)
		{
			mTechique = &techique;
		}

		void setWorld(Matrix4 const& mat)
		{
			if( mTechique )
				mTechique->setupWorld(*this, mat);

		}
		void beginRender()
		{
			mUsageProgram = nullptr;
			mbUseMaterialShader = false;
		}
		void endRender()
		{
			if( mUsageProgram )
			{
				mUsageProgram->unbind();
				mUsageProgram = nullptr;
			}
		}

		void setShader(ShaderProgram& program);
		MaterialShaderProgram* setMaterial(Material* material, VertexFactory* vertexFactory = nullptr);

		RenderTechnique* mTechique;
		ViewInfo*       mCurView;
		VertexFactory*  mUsageVertexFactory;
		ShaderProgram*  mUsageProgram;
		bool            mbUseMaterialShader;
		bool            bBindAttrib;
	};

}//namespace Render

#endif // RenderContext_H_6EE6BADB_9AEF_4C48_867D_600B4D1CF4DE
