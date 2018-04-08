#include "SceneScript.h"

#define CHAISCRIPT_NO_THREADS
#include "chaiscript/chaiscript.hpp"


#include "SceneObject.h"
#include "RenderGLStageAssetID.h"

namespace Chai = chaiscript;


namespace RenderGL
{
	template< class ModuleType >
	class TScriptCollector
	{

	public:
		TScriptCollector(ModuleType& inModule)
			:module(inModule)
		{}

		template< class T >
		void collect()
		{
			T::CollectReflection(*this);
		}

		template< class T >
		void beginClass( char const* name )
		{
			module.add(Chai::user_type< T >(), name);
		}
		template< class T >
		void addProperty(T var, char const* name)
		{
			module.add(Chai::fun(var), name);
		}
		ModuleType& module;
	};
#define SCRIPT_NAME( NAME ) #NAME

	class Common
	{
	public:
		template< class ModuleType >
		static void RegisterMath(ModuleType& script)
		{
			script.add(Chai::constructor< Vector3(float) >(), SCRIPT_NAME(Vector3));
			script.add(Chai::constructor< Vector3(float, float, float) >(), SCRIPT_NAME(Vector3));
			script.add(Chai::fun(static_cast<Vector3& (Vector3::*)(Vector3 const& ) >(&Vector3::operator=)), SCRIPT_NAME(=));
			script.add(Chai::fun(static_cast<Vector3 (*)(Vector3 const&, Vector3 const&)>(&Math::operator+)), SCRIPT_NAME(+));
			script.add(Chai::fun(static_cast<Vector3 (*)(Vector3 const&, Vector3 const&)>(&Math::operator-)), SCRIPT_NAME(-));
			script.add(Chai::fun(static_cast<Vector3 (*)(float, Vector3 const&)>(&Math::operator*)), SCRIPT_NAME(*));
		}


	};

	float Deg2RadFactor = Math::PI / 180.0;

	
	static Chai::ModulePtr CommonModule;

	class SceneScriptImpl : public ISceneScript
	{
	public:


		virtual bool setup(Scene& scene, IAssetProvider& assetProvider, char const* fileName ) override
		{

			static bool bInitModule = false;
			if( !bInitModule )
			{
				CommonModule.reset(new Chai::Module);
				Common::RegisterMath(*CommonModule);
				registerAssetId(*CommonModule);
				registerSceneObject(*CommonModule);

				bInitModule = true;
			}
			mAssetProvider = &assetProvider;
			mScriptScene = &scene;

			FixString< 512 > path;
			path.format("Script/%s.chai", fileName);
			Chai::ChaiScript script;
			script.add(CommonModule);
			registerSceneFun(script);
			try
			{
				script.eval_file(path.c_str());
			}
			catch( const std::exception& e )
			{
				LogMsgF("Load Scene Fail!! : %s", e.what());
				return false;
			}


			return true;
		}


		virtual void release() override
		{
			delete this;
		}

		static void registerSceneObject(Chai::Module& module)
		{
			module.add(Chai::user_type< Material >(), SCRIPT_NAME(Material));
			module.add(Chai::fun< void , Material , char const*, Vector3 const& >( &Material::setParameter ), SCRIPT_NAME(setParameter));
			module.add(Chai::fun< void, Material, char const*, float >(&Material::setParameter), SCRIPT_NAME(setParameter));
			module.add(Chai::fun< void, Material, char const*, Texture2D& >(&Material::setParameter), SCRIPT_NAME(setParameter));


			TScriptCollector< Chai::Module > collector(module);

			collector.collect< SimpleMeshObject >();
			collector.collect< LightInfo >();

			module.add(Chai::user_type< StaticMeshObject >(), SCRIPT_NAME(StaticMeshObject));

			module.add(Chai::user_type< CycleTrack >(), SCRIPT_NAME(CycleTrack));
			module.add(Chai::fun(&CycleTrack::center), SCRIPT_NAME(center));
			module.add(Chai::fun(&CycleTrack::planeNormal), SCRIPT_NAME(planeNormal));
			module.add(Chai::fun(&CycleTrack::radius), SCRIPT_NAME(radius));
			module.add(Chai::fun(&CycleTrack::period), SCRIPT_NAME(period));

			module.add(Chai::user_type< VectorCurveTrack >(), SCRIPT_NAME(VectorCurveTrack));
			module.add(Chai::base_class< TCurveTrack<Vector3>, VectorCurveTrack >());
			module.add(Chai::fun(&VectorCurveTrack::addKey), SCRIPT_NAME(addKey));
			module.add(Chai::fun(&VectorCurveTrack::setKeyTangent), SCRIPT_NAME(setKeyTangent));
			module.add(Chai::fun(&VectorCurveTrack::bSmooth), SCRIPT_NAME(bSmooth));
			module.add(Chai::fun([](VectorCurveTrack& track, int mode) { track.mode = TrackMode(mode); }), SCRIPT_NAME(setMode));
			module.add(Chai::fun([](VectorCurveTrack& track, int idx , int mode) { track.setKeyInterpMode( idx , CurveInterpMode(mode) ); }), SCRIPT_NAME(setKeyInterpMode));

		}

		static void registerAssetId(Chai::Module& module)
		{
			
			Chai::utility::add_class< MaterialId::Enum >( module, SCRIPT_NAME(MaterialId),
			{
#define LIST_OP( NAME ) { MaterialId::NAME , SCRIPT_NAME( MaterialId_##NAME ) } ,
				MATERIAL_LIST(LIST_OP)
#undef LIST_OP
			});
			module.add(Chai::type_conversion< MaterialId::Enum, int >());
			
			
			Chai::utility::add_class< MeshId::Enum >(module, SCRIPT_NAME(MeshId),
			{
#define LIST_OP( NAME , PATH ) { MeshId::NAME , SCRIPT_NAME( MeshId_##NAME ) } ,
				MESH_LIST(LIST_OP)
#undef LIST_OP
			});
			module.add(Chai::type_conversion< MeshId::Enum, int >());
			
			Chai::utility::add_class< TextureId::Enum >(module, SCRIPT_NAME(TextureId),
			{
#define LIST_OP( NAME , PATH ) { TextureId::NAME , SCRIPT_NAME( TextureId_##NAME ) } ,
				TEXTURE_LIST(LIST_OP)
#undef LIST_OP
			});
			module.add(Chai::type_conversion< TextureId::Enum, int >());
		}
		void registerSceneFun( Chai::ChaiScript& script)
		{
			script.add(Chai::fun(&SceneScriptImpl::StaticMesh, this), SCRIPT_NAME(StaticMesh));
			script.add(Chai::fun(&SceneScriptImpl::SimpleMesh, this), SCRIPT_NAME(SimpleMesh));
			script.add(Chai::fun(&SceneScriptImpl::PointLight, this), SCRIPT_NAME(PointLight));
			script.add(Chai::fun(&SceneScriptImpl::GetTexture2D, this), SCRIPT_NAME(GetTexture2D));
			script.add(Chai::fun(&SceneScriptImpl::AddCycleTrack, this), SCRIPT_NAME(AddCycleTrack));
			script.add(Chai::fun(&SceneScriptImpl::AddCurveTrack, this), SCRIPT_NAME(AddCurveTrack));
			script.add(Chai::fun(&SceneScriptImpl::AddDebugLine, this), SCRIPT_NAME(AddDebugLine));
		}

		Scene* mScriptScene;
		IAssetProvider* mAssetProvider;
		StaticMeshObject* StaticMesh(int id, Vector3 const& pos, Vector3 const& scale, Vector3 const& rotation)
		{
			auto meshObject = new StaticMeshObject;
			meshObject->meshPtr = mAssetProvider->getMesh(id);
			meshObject->worldTransform = Matrix4::Scale(scale) * Matrix4::Rotate(Quaternion::EulerZYX(Deg2RadFactor * rotation)) * Matrix4::Translate(pos);
			mScriptScene->addObject(meshObject);
			return meshObject;
		}

		SimpleMeshObject* SimpleMesh(int meshId, int matId, Vector3 const& pos, Vector3 const& scale, Vector3 const& rotation)
		{
			auto meshObject = new SimpleMeshObject;
			meshObject->mesh = &mAssetProvider->getSimpleMesh(meshId);
			auto material = new MaterialInstance(mAssetProvider->getMaterial(matId));
			meshObject->material.reset(material);
			meshObject->color = Vector4(0.3, 0.3, 1, 1);
			meshObject->worldTransform = Matrix4::Scale(scale) * Matrix4::Rotate(Quaternion::EulerZYX(Deg2RadFactor * rotation)) * Matrix4::Translate(pos);
			mScriptScene->addObject(meshObject);
			return meshObject;
		}

		LightInfo* PointLight(Vector3 const& pos, Vector3 const& color, float instensity, float radius, bool bCastShadow)
		{
			auto light = new SceneLight;
			light->info.type = LightType::Point;
			light->info.pos = pos;
			light->info.color = color;
			light->info.intensity = instensity;
			light->info.radius = radius;
			light->info.bCastShadow = bCastShadow;
			mScriptScene->addLight(light);
			return &light->info;
		}

		Texture2D& GetTexture2D(int id)
		{
			return mAssetProvider->getTexture(id);
		}

		CycleTrack* AddCycleTrack(Vector3& value) 
		{ 
			CycleTrack* track = mAssetProvider->addCycleTrack(value);
			track->center = Vector3(0, 0, -20);
			track->radius = 1;
			track->period = 10;
			track->planeNormal = Vector3(0, 0, 1);
			return track;
		}

		VectorCurveTrack* AddCurveTrack(Vector3& value)
		{
			VectorCurveTrack* track = mAssetProvider->addCurveTrack(value);
			return track;
		}

		void AddDebugLine(Vector3 const& posStart, Vector3 const& posEnd, Vector3 const& color, float tickness, float time)
		{
			auto debugObject = mAssetProvider->getDebugGeometryObject();
			if( debugObject )
			{
				debugObject->addLine(posStart, posEnd, color, tickness, time);
			}
		}

	};

	std::string ISceneScript::GetFilePath(char const* fileName)
	{
		FixString< 512 > path;
		path.format("Script/%s.chai", fileName);
		return path;
	}

	ISceneScript* ISceneScript::Create()
	{
		return new SceneScriptImpl;
	}

}
