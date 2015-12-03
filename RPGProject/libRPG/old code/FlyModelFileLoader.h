#ifndef FlyModelFileLoader_h__
#define FlyModelFileLoader_h__

#include "common.h"
#include <fstream>
#include <sstream>
#include <string>

struct FlyModelDataInfo
{
	Vec3D  pos;
	Quat   rotation;
	unsigned NumTri;
	unsigned NumVertex;
	int*   triIndex;
	Vec3D* vertex;
	bool isNeedFree;

	void releaseData()
	{
		delete [] vertex;
		delete [] triIndex;
	}
};

enum FlyVertexType
{
	FV_POSITION  = 1 << 0,
	FV_COLOR     = 1 << 1,


};

class TFileIO;

class FlyModelFileLoader 
{
public:

	~FlyModelFileLoader()
	{
		//delete [] m_triIndex;
		//delete [] m_vertex;
	}
	bool Load( char const* fileName , FlyModelDataInfo* info);
	void loadVertex( TFileIO& loader );
	void loadTri( TFileIO& loader );

protected:

	FlyModelDataInfo* m_info;
};


#endif // FlyModelFileLoader_h__