#pragma once
#ifndef RenderContext_H_6EE6BADB_9AEF_4C48_867D_600B4D1CF4DE
#define RenderContext_H_6EE6BADB_9AEF_4C48_867D_600B4D1CF4DE

#include "RHICommon.h"

#include "Material.h"

namespace RenderGL
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
		Vector3 worldPos;
		Vector3 direction;
		Matrix4 worldToClip;
		Matrix4 worldToView;
		Matrix4 viewToWorld;
		Matrix4 viewToClip;
		Matrix4 clipToWorld;
		Matrix4 clipToView;

		float   gameTime;
		float   realTime;


		Plane frustumPlanes[6];

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

		Vec2i getViewportSize() const;
		void  setupShader(ShaderProgram& program);


		void updateFrustumPlanes();
	};

	

	class RenderTechique
	{
	public:
		virtual ShaderProgram* getMaterialShader(RenderContext& context, MaterialMaster& material , VertexFactory* vertexFactory ) { return nullptr; }
		virtual void setupMaterialShader(RenderContext& context, ShaderProgram& shader) {}
		virtual void setupWorld(RenderContext& context, Matrix4 const& mat);
		virtual bool needUseVAO() { return false; }
	};

	class RenderContext
	{
	public:
		RenderContext( ViewInfo& view , RenderTechique& techique)
			:mTechique(&techique)
			,mCurView( &view )
		{
			mbUseVAO = techique.needUseVAO();
			mUsageShader = nullptr;
			mbUseVAO = false;
		}

		ViewInfo& getView() { return *mCurView; }

		void setTechique(RenderTechique& techique)
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
			mUsageShader = nullptr;
		}
		void endRender()
		{
			if( mUsageShader )
			{
				mUsageShader->unbind();
				mUsageShader = nullptr;
			}
		}

		void setShader(ShaderProgram& shader);
		void setupShader(Material* material, VertexFactory* vertexFactory = nullptr);
		void setShaderParameter(char const* name, Texture2D& texture)
		{
			if( mUsageShader )
			{
				RHITexture2D* textureRHI = texture.getRHI() ? texture.getRHI() : GDefaultMaterialTexture2D;
				mUsageShader->setTexture(name, *textureRHI);
			}
		}
		void setShaderParameter(char const* name, RHITexture2D& texture)
		{
			if( mUsageShader )
			{
				mUsageShader->setTexture(name, texture);
			}
		}
		void setShaderParameter(char const* name, Vector3 value)
		{
			if( mUsageShader )
			{
				mUsageShader->setParam(name, value);
			}
		}

		RenderTechique* mTechique;
		ViewInfo*       mCurView;
		VertexFactory*  mUsageVertexFactory;
		ShaderProgram*  mUsageShader;
		bool            mbUseVAO;
	};

}//namespace RenderGL

#endif // RenderContext_H_6EE6BADB_9AEF_4C48_867D_600B4D1CF4DE
