#include "MeshImportor.h"

namespace Render
{

#if CORE_SHARE_CODE
	Render::MeshImporterRegistry& MeshImporterRegistry::Get()
	{
		static MeshImporterRegistry Instance;
		return Instance;
	}

	void MeshImporterRegistry::registerMeshImporter(HashString name, IMeshImporterFactoryPtr const& importor)
	{
		mFactoryMap.insert_or_assign(name, importor);
	}

	void MeshImporterRegistry::unregisterMeshImporter(HashString name)
	{
		mFactoryMap.erase(name);
	}

	IMeshImporterPtr MeshImporterRegistry::getMeshImprotor(HashString name)
	{
		auto iter = mFactoryMap.find(name);
		if (iter != mFactoryMap.end())
		{
			if (iter->second != nullptr)
			{
				return iter->second->create(name);
			}
		}
		return IMeshImporterPtr();
	}
#endif

}//namespace Render


