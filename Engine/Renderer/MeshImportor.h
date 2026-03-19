#pragma once
#ifndef MeshImportor_H_6755FFD3_B5D1_4293_9F6A_53348D1F4211
#define MeshImportor_H_6755FFD3_B5D1_4293_9F6A_53348D1F4211

#include "Renderer/Mesh.h"

#include "HashString.h"
#include "CoreShare.h"
#include "Meta/IsBaseOf.h"
#include "MacroCommon.h"
#include "DataStructure/Array.h"

#include <memory>
#include <unordered_map>


namespace Render
{
	class Mesh;

	struct MeshImportData
	{
		TArray<uint8>  vertices;
		TArray<uint32> indices;
		int numVertices = 0;
		InputLayoutDesc desc;
		TArray<MeshSection> sections;

		void initSections();

		void append(MeshImportData const& other);

		VertexElementReader makeAttributeReader( EVertex::Attribute attribute);

		template< class Op >
		void serialize(Op& op)
		{
			op & vertices & indices & numVertices & desc & sections;
		}
	};

	class MeshImportSettings
	{
	public:
		virtual void setupMaterial(int indexMesh, int indexSection, void* pMaterialInfo) = 0;
	};

	class IMeshImporter
	{
	public:
		virtual ~IMeshImporter() {}
		virtual bool importFromFile(char const* filePath, Mesh& outMesh, MeshImportSettings* settings) = 0; 
		virtual bool importFromFile(char const* filePath, MeshImportData& outMeshData) = 0;
		virtual bool importMultiFromFile(char const* filePath, TArray< Mesh >& outMeshes, MeshImportSettings* settings) = 0;
	};
	using IMeshImporterPtr = std::shared_ptr< IMeshImporter >;
	class IMeshImporterFactory
	{
	public:
		virtual ~IMeshImporterFactory() = default;
		virtual IMeshImporterPtr create(HashString name) = 0;
	};

	using IMeshImporterFactoryPtr = std::shared_ptr< IMeshImporterFactory >;


	class MeshImporterRegistry
	{
	public:

		CORE_API static MeshImporterRegistry& Get();

		struct ImporterInfo
		{
			template< typename T >
			ImporterInfo(T*)
			{
				factory = std::make_shared< TMeshImporterFactory<T> >();
			}

			IMeshImporterFactoryPtr factory;
			IMeshImporterPtr cachedInstance;
		};
		CORE_API void registerMeshImporter(HashString name, ImporterInfo&& info);
		CORE_API void unregisterMeshImporter(HashString name);

		template< class T >
		class TMeshImporterFactory : public IMeshImporterFactory
		{
		public:
			virtual IMeshImporterPtr create(HashString name)
			{
				return std::make_shared<T>();
			}
		};


		template< class T , TEnableIf_Type< TIsBaseOf< T, IMeshImporter >::Value  , bool > = true >
		void registerMeshImporterT(HashString name)
		{
			registerMeshImporter( name , ImporterInfo((T*)nullptr) );
		}

		void cleanupCache()
		{
			for (auto& pair : mFactoryMap)
			{
				pair.second.cachedInstance = nullptr;
			}
		}
		void cleanup()
		{
			mFactoryMap.clear();
		}
		CORE_API IMeshImporterPtr getMeshImproter(HashString name);

		std::unordered_map< HashString, ImporterInfo > mFactoryMap;
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
