#ifndef ObjFileLoader_h__
#define ObjFileLoader_h__

#include "CFBase.h"
#include <map>

namespace CFly 
{
	struct ObjModelData
	{
		struct tri_t
		{
			int v[3];
		};

		struct texVtx
		{
			float u , v;
		};

		struct MatInfo
		{
			String  texName;
			Vector3 ka;
			Vector3 kd;
			Vector3 ks;

			float d;
			float s;
		};

		struct Group_t
		{
			String matName;
			std::vector< tri_t >   surface;
			int    index;
			String name;
		};

		std::vector< Vector3 > vertex;
		std::vector< texVtx  > texV;
		std::vector< Vector3 > normal;
		std::vector< Group_t > groupVec;

		typedef std::map< String , MatInfo > MatMap;
		typedef std::vector< MatInfo > MatVec;
		MatVec  matVec;
	};

	void saveCW3File( char const* path , ObjModelData& data );
	void LoadObjFile( char const* name , ObjModelData& data );
	void loadMatFile( char const* name , ObjModelData::MatVec& matVec );


	class ObjFileImport
	{
	public:





	};


}//namespace CFly 

#endif // ObjFileLoader_h__