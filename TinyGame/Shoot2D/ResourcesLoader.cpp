#include "ResourcesLoader.h"

#include <memory>

namespace Shoot2D
{

	ResourcesLoader::ResourcesLoader()
	{
		::memset( m_Bmp , 0 , sizeof( BmpData*) * MD_END );
	}

	void ResourcesLoader::load()
	{
		m_Bmp[MD_PLAYER] = new BmpData(  m_hDC, "data\\scout.bmp", 72 , 72 );
		m_Bmp[MD_BASE  ] = new BmpData(  m_hDC, "data\\scout.bmp", 72 , 72 );
		m_Bmp[MD_BULLET_1] = new BmpData( m_hDC , "data\\missile.bmp" , 32 , 32);
		m_Bmp[MD_SMALL_BOW] = new BmpData( m_hDC , "data\\bow1.bmp" , 32 , 32);
		m_Bmp[MD_BIG_BOW] = new BmpData( m_hDC , "data\\bow2.bmp" , 180 , 140 );
	}



}//namespace Shoot2D
