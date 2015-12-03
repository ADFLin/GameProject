#ifndef ResourcesLoader_h__
#define ResourcesLoader_h__

#include "ObjModel.h"
#include "BitmapDC.h"
#include "Singleton.h"

class BitmapDC;

namespace Shoot2D
{


	class BmpData
	{
	public:
		BmpData( HDC hdc , LPSTR file , int w ,int h )
			:bmpDC( hdc , file )
			,w(w)
			,h(h)
		{

		}
		BitmapDC bmpDC;
		int w;
		int h;
	};

	class ResourcesLoader : public SingletonT< ResourcesLoader >
	{
	public:

		ResourcesLoader();
		~ResourcesLoader(){}

		void     load();
		BmpData* getBmp(int id){ return m_Bmp[id]; }
		void     setDC( HDC dc ){ m_hDC = dc; }

		HDC m_hDC;
		BmpData* m_Bmp[MD_END];
	};


}//namespace Shoot2D


#endif // ResourcesLoader_h__
