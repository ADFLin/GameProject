#ifndef GLUtility_h__
#define GLUtility_h__

#include "GLCommon.h"
#include "RHICommand.h"
#include "MarcoCommon.h"
#include <string>


namespace RenderGL
{


	struct OBJMaterialInfo
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
		char const* ambientTextureName;
		char const* diffuseTextureName;
		char const* specularTextureName;
		char const* normalTextureName;
		std::vector < std::pair< char const*, char const* > > unknownParameters;

		char const* findParam(char const* name) const
		{
			for( auto& pair : unknownParameters )
			{
				if( strcmp(name, pair.first) == 0 )
					return pair.second;
			}
			return nullptr;
		}
	};

	class OBJMaterialBuildListener
	{
	public:
		virtual void build(int idxSection, OBJMaterialInfo const* mat) = 0;
	};

	class Mesh
	{
	public:

		Mesh();
		~Mesh();

		bool createBuffer(void* pVertex, int nV, void* pIdx = nullptr, int nIndices = 0, bool bIntIndex = false);
		void draw();
		void draw(LinearColor const& color);

		void drawShader();
		void drawShader(LinearColor const& color);

		void drawSection(int idx, bool bUseVAO = false);

		void drawInternal(int idxStart, int num, LinearColor const* color = nullptr);
		void drawShaderInternal(int idxStart, int num, LinearColor const* color = nullptr);

		void bindVAO(LinearColor const* color = nullptr);
		void unbindVAO()
		{
			glBindVertexArray(0);
		}

		PrimitiveType       mType;
		VertexDecl          mDecl;
		RHIVertexBufferRef  mVertexBuffer;
		RHIIndexBufferRef   mIndexBuffer;
		uint32              mVAO;

		struct Section
		{
			int start;
			int num;
		};
		std::vector< Section > mSections;
	};

	class MeshBuild
	{
	public:
		static bool Tile( Mesh& mesh , int tileSize , float len , bool bHaveSkirt = true );
		static bool UVSphere( Mesh& mesh , float radius , int rings, int sectors);
		static bool IcoSphere( Mesh& mesh , float radius , int numDiv );
		static bool SkyBox( Mesh& mesh );
		static bool Cube( Mesh& mesh , float halfLen = 1.0f );
		
		static bool Cone(Mesh& mesh, float height, int numSide);
		static bool Doughnut(Mesh& mesh , float radius , float ringRadius , int rings , int sectors);
		static bool PlaneZ( Mesh& mesh , float len, float texFactor );
		static bool Plane( Mesh& mesh , Vector3 const& offset , Vector3 const& normal , Vector3 const& dir , float len , float texFactor);

		static bool SimpleSkin(Mesh& mesh, float width, float height, int nx, int ny);

		static bool LightSphere(Mesh& mesh);
		static bool LightCone(Mesh& mesh);

		struct MeshData
		{
			float* position;
			int    numPosition;
			float* normal;
			int    numNormal;
			int*   indices;
			int    numIndex;
		};
		static bool TriangleMesh(Mesh& mesh, MeshData& data);



		static bool LoadObjectFile(
			Mesh& mesh, char const* path , 
			Matrix4* pTransform = nullptr , 
			OBJMaterialBuildListener* listener = nullptr ,
			int * skip = nullptr );

	};




}//namespace GL

#endif // GLUtility_h__
