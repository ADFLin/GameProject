#include "TestRenderStageBase.h"


namespace Render
{

	struct OBJMaterialSaveInfo
	{
		char const* name;

		Vector3 ambient;
		Vector3 diffuse;
		Vector3 specular;
		Vector3 transmittance;
		Vector3 emission;
		float shininess;
		float ior;
		float dissolve;
		int   illum;
		std::string ambientTextureName;
		std::string diffuseTextureName;
		std::string specularTextureName;
		std::string normalTextureName;
		std::map< std::string, std::string > unknownParameters;

		template< class OP >
		void serialize(OP op)
		{
			op & ambient;
			op & diffuse;
			op & specular;
			op & transmittance;
			op & emission;
			op & shininess;
			op & ior;
			op & dissolve;
			op & illum;
			op & ambientTextureName;
			op & diffuseTextureName;
			op & specularTextureName;
			op & normalTextureName;
			op & unknownParameters;
		}
	};

	TYPE_SUPPORT_SERIALIZE_FUNC(OBJMaterialSaveInfo)

	bool LoadObjectMesh(StaticMesh& mesh, char const* path)
	{
		return false;
	}

}//namespace Render