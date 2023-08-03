#include "Scene.h"


#include "RenderContext.h"

namespace Render
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
				float d = 0.5 * radius / Math::Cos(Math::DegToRad(spotAngle.y));
				return view.frustumTest(pos + d * Math::GetNormal(dir), d);
			}
			break;
		case LightType::Point:
			return view.frustumTest(pos, radius);
		}

		return true;
	}

}//namespace Render

