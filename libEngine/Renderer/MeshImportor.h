#pragma once
#ifndef MeshImportor_H_6755FFD3_B5D1_4293_9F6A_53348D1F4211
#define MeshImportor_H_6755FFD3_B5D1_4293_9F6A_53348D1F4211

#include "Renderer/Mesh.h"

#include "HashString.h"
#include "CoreShare.h"
#include "Meta/IsBaseOf.h"
#include "MarcoCommon.h"

#include <memory>
#include <unordered_map>

namespace Render
{
	class Mesh;

	class MeshImportSettings
	{
	public:
		virtual void setupMaterial(int indexSection, void* pMaterialInfo) = 0;
	};

	class IMeshImporter
	{
	public:
		virtual ~IMeshImporter() {}
		virtual bool importFromFile(char const* filePath, Mesh& outMesh, MeshImportSettings* settings) = 0;
	};
	using IMeshImporterPtr = std::shared_ptr< IMeshImporter >;
	class IMeshImporterFactory
	{
	public:
		virtual IMeshImporterPtr create(HashString name) = 0;
	};

	using IMeshImporterFactoryPtr = std::shared_ptr< IMeshImporterFactory >;


	class CORE_API MeshImporterRegistry
	{
	public:

		static MeshImporterRegistry& Get();

		void registerMeshImporter(HashString name, IMeshImporterFactoryPtr const& importor);
		void unregisterMeshImporter(HashString name);

		template< class T >
		class TMeshImportorFactory : public IMeshImporterFactory
		{
		public:
			virtual IMeshImporterPtr create(HashString name)
			{
				return std::make_shared<T>();
			}
		};

		template< class T >
		auto registerMeshImporterT(HashString name) -> typename TEnableIf< TIsBaseOf< T, IMeshImporter >::Value >::Type
		{
			registerMeshImporter( name , std::make_shared< TMeshImportorFactory<T> >() );
		}

		void cleanup()
		{
			mFactoryMap.clear();
		}
		IMeshImporterPtr getMeshImprotor(HashString name);

		std::unordered_map< HashString, IMeshImporterFactoryPtr > mFactoryMap;
	};

	template< class T >
	class TMeshImporterAutoRegistry
	{
	public:
		TMeshImporterAutoRegistry(char const* name)
		{
			MeshImporterRegistry::Get().registerMeshImporterT<T>(name);
		}
	};

#define REGISTER_MESH_IMPORTER( CLASS , NAME )\
	Render::TMeshImporterAutoRegistry< CLASS > ANONYMOUS_VARIABLE(GMeshImporter)( NAME );

}//namespace Render

#endif // MeshImportor_H_6755FFD3_B5D1_4293_9F6A_53348D1F4211
