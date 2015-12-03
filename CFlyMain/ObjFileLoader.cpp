#include "CFlyPCH.h"
#include "ObjFileLoader.h"

#include <sstream>
#include <fstream>

namespace CFly
{
	std::ostream& operator << ( std::ostream& os , Vector3& vec )
	{
		os << vec.x << " " << vec.y << " " << vec.z;
		return os;
	}

	std::istream& operator >> ( std::istream& is , Vector3& vec )
	{
		is >> vec.x >> vec.y >> vec.z;
		return is;
	}

	String getFileName( char const* path )
	{
		int len = strlen( path );
		char const* dotPos = NULL;
		char const* pos = path + len;

		while( *pos != '\\' && *pos != '/' && pos != path )
		{
			if ( *pos == '.' &&  dotPos == NULL )
				dotPos = pos;

			--pos;
		}
		if ( !dotPos )
			dotPos = path + len;

		if ( *pos == '\\' || *pos == '/' )
			++pos;

		return String( pos , dotPos );
	}

	void saveCW3File( char const* path , ObjModelData & data )
	{
		std::ofstream s( path );

		s << "Model v 1\n";

		s << "Name " << getFileName( path ) << "\n";


		s << "BeginModel" << "\n";
		s << "VertexType Position Normal Texture 1 2\n";

		s << "Material " << data.matVec.size() << "\n";


		int index = 0;
		for ( ObjModelData::MatVec::iterator iter = data.matVec.begin(); 
			iter != data.matVec.end() ; ++iter )
		{
			ObjModelData::MatInfo& mat = *iter;

			s << "Name " << "Mat_" <<  index  << " ";
			s << "Texture " << getFileName( mat.texName.c_str() ) << " ";
			s << "Ambient " << mat.ka << " ";
			s << "Diffuse " << mat.kd << " ";  
			s << "Specular "<< mat.ks << "\n";

			++index;
		}

		s << "Vertex 1 " << data.vertex.size() <<"\n";
		for (int i = 0 ; i < data.vertex.size() ; ++i )
		{
			s << data.vertex[i] << " " << data.normal[i] << " " 
				<< data.texV[i].u << " " << data.texV[i].v << "\n";
		}

		int totalS = 0;
		for ( int i = 0 ; i < data.groupVec.size(); ++ i)
		{
			totalS += data.groupVec[i].surface.size();
		}

		s << "Polygon " << totalS << "\n";

		for ( int i = 0 ; i < data.groupVec.size(); ++i )
		{
			ObjModelData::Group_t& group = data.groupVec[i];

			for ( int n = 0 ; n < group.surface.size(); ++n)
			{
				ObjModelData::tri_t& tri = group.surface[n];
				s << "3 "
				  << group.index  << " "
				  << tri.v[0] - 1 << " "
				  << tri.v[1] - 1 << " "
				  << tri.v[2] - 1 << "\n"; 
			}
		}

		s << "EndModel\n";
	}


	void LoadObjFile( char const* path , ObjModelData& data )
	{
		std::ifstream s( path );

		char buf[256];

		bool defineG = false;
		ObjModelData::Group_t group;
		Vector3 vec;
		ObjModelData::texVtx tex;
		ObjModelData::tri_t tri;

		int index = 0;

		while ( s )
		{
			s.getline( buf , sizeof(buf)/sizeof(char) );

			String decr;
			std::stringstream ss( buf );

			ss >> decr;


			if ( decr == "v" )
			{
				ss >> vec;
				data.vertex.push_back( vec );
			}
			else if ( decr == "vt" )
			{
				ss >> tex.u >> tex.v;
				data.texV.push_back( tex );
			}
			else if ( decr == "vn" )
			{
				ss >> vec;
				data.normal.push_back( vec );
			}

			else if ( decr == "g" )
			{
				if ( defineG )
					data.groupVec.push_back( group );

				ss >> group.name;
				group.surface.clear();

				defineG = true;
			}
			else if ( decr == "f" )
			{
				char sbuf[64];
				int v[3];
				for ( int i = 0 ; i < 3 ; ++ i )
				{
					ss >> sbuf;
					sscanf(  sbuf , "%d/%d/%d" , &v[0] , &v[1] , &v[2]);
					tri.v[i] = v[0];
				}
				group.surface.push_back( tri );
			}
			else if ( decr == "usemtl")
			{
				ss >> group.matName;
				group.index = index;
				++index;
			}
			else if ( decr == "mtllib" )
			{
				loadMatFile( buf + strlen("mtllib") + 1 , data.matVec );
			}
		}

		if ( defineG )
		{
			data.groupVec.push_back( group );
		}
	}

	void loadMatFile( char const* name , ObjModelData::MatVec& matVec )
	{
		std::ifstream ss( name );
		char buf[256];
		String decr;

		ObjModelData::MatInfo mat;
		String matName;

		bool defineMat = false;

		while ( ss )
		{
			ss.getline( buf , sizeof(buf)/sizeof(char) );
			std::stringstream ss( buf );
			decr.clear();
			ss >> decr;
			if ( decr == "newmtl")
			{
				if ( defineMat )
				{
					matVec.push_back( mat );
				}

				mat.texName.clear();
				mat.ka = Vector3(0,0,0);
				mat.kd = Vector3(0,0,0);
				mat.ks = Vector3(0,0,0);
				ss >> matName;
				defineMat = true;
			}
			else if ( decr == "Ka" )
			{
				ss >> mat.ka[0] >> mat.ka[1] >> mat.ka[2]; 
			}
			else if ( decr == "Kd")
			{
				ss >> mat.kd[0] >> mat.kd[1] >> mat.kd[2]; 
			}
			else if ( decr == "Ks")
			{
				ss >> mat.ks[0] >> mat.ks[1] >> mat.ks[2]; 
			}
			else if  ( decr == "map_Kd")
			{
				ss >> mat.texName;
			}
		}


		if ( defineMat )
		{
			matVec.push_back( mat );
		}
	}

} //namespace CFly