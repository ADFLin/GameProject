#include "Scene.h"


#include "GLCommon.h"
#include "RenderContext.h"

namespace RenderGL
{

	void Scene::tick(float deltaTime)
	{
		for( SceneObjectPtr& sceneObject : objectList )
		{
			sceneObject->tick(deltaTime);
		}

		bNeedUpdatePrimitive = true;
		if( bNeedUpdatePrimitive )
		{
			bNeedUpdatePrimitive = false;
			primitiveCollection.clear();
			for( SceneObjectPtr& sceneObject : objectList )
			{
				updatePrimitives(sceneObject.get());
			}
		}
	}

	void Scene::prepareRender(ViewInfo& info)
	{
		dynamicPrimitiveCollection.clear();
		for( SceneObjectPtr& sceneObject : objectList )
		{
			sceneObject->getDynamicPrimitives(dynamicPrimitiveCollection,info);
		}
	}

	void Scene::render(RenderContext& context)
	{
		for( auto meshBatch : primitiveCollection.mMeshBatchs )
		{
			meshBatch.draw(context);
		}

		dynamicPrimitiveCollection.drawDynamic(context);
	}

	void Scene::renderTranslucent(RenderContext& context)
	{

	}

	bool LightInfo::testVisible(ViewInfo const& view) const
	{
		switch( type )
		{
		case LightType::Spot:
			{
				float d = 0.5 * radius / Math::Cos(Math::Deg2Rad(spotAngle.y));
				return view.frustumTest(pos + d * Normalize(dir), d);
			}
			break;
		case LightType::Point:
			return view.frustumTest(pos, radius);
		}

		return true;
	}

}//namespace RenderGL

