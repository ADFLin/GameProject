#pragma once
#ifndef Scene_H_62290B6B_2869_454C_8342_59EEB7959545
#define Scene_H_62290B6B_2869_454C_8342_59EEB7959545

#include "RHICommon.h"
#include "PrimitiveBatch.h"
#include "RenderContext.h"

#include <memory>

#include "Renderer/SceneLighting.h"

namespace Render
{

	class  Material;
	struct ViewInfo;
	class RenderContext;



	class SceneLight
	{
	public:
		LightInfo info;
	};

	class SceneObject
	{
	public:
		virtual void getPrimitives(PrimitivesCollection& primitiveCollection){}
		virtual void getDynamicPrimitives(PrimitivesCollection& primitiveCollection, ViewInfo& view){}
		virtual bool checkVisibly()
		{
			return true;
		}
		virtual void tick(float deltaTime){}
	};

	class SceneListener
	{
	public:
		virtual void onRemoveLight(SceneLight* light){}
		virtual void onRemoveObject(SceneObject* object){}
	};
	class Scene : public SceneInterface
	{
	public:
		Scene()
		{
			bNeedUpdatePrimitive = true;
		}
		void tick(float deltaTime);
		void prepareRender(ViewInfo& info);


		virtual void render(RenderContext& context ) override;
		virtual void renderTranslucent(RenderContext& context) override;

		void addLight(SceneLight* light)
		{
			lights.push_back(SceneLightPtr(light));
		}

		void addObject(SceneObject* sceneObject)
		{
			auto ptr = SceneObjectPtr(sceneObject);
			assert(std::find(objectList.begin(), objectList.end(), ptr) == objectList.end());
			objectList.push_back( std::move(ptr));
			updatePrimitives(sceneObject);
		}

		void updatePrimitives(SceneObject* sceneObject)
		{
			sceneObject->getPrimitives(primitiveCollection);
		}

		void procRmoveObject(SceneObject* object)
		{
			if( mListener )
				mListener->onRemoveObject(object);

			assert(object);
		}

		void procRmoveLight(SceneLight* light)
		{
			assert(light);
			if( mListener )
				mListener->onRemoveLight(light);
		}
		void removeAll()
		{
			for ( SceneObjectPtr& object : objectList )
			{
				procRmoveObject(object.get());
			}
			objectList.clear();
			for( SceneLightPtr& light : lights )
			{
				procRmoveLight(light.get());
			}
			lights.clear();
			bNeedUpdatePrimitive = true;
		}
		SceneListener* mListener = nullptr;
		typedef std::unique_ptr< SceneLight > SceneLightPtr;
		typedef std::unique_ptr< SceneObject > SceneObjectPtr;
		PrimitivesCollection primitiveCollection;
		PrimitivesCollection dynamicPrimitiveCollection;
		TArray< SceneObjectPtr > objectList;
		TArray< SceneLightPtr > lights;
		friend class SceneRenderer;
		bool bNeedUpdatePrimitive;
	};

}//namespace Render


#endif // Scene_H_62290B6B_2869_454C_8342_59EEB7959545
