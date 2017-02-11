#ifndef ZLevelManager_h__
#define ZLevelManager_h__

#include "ZBase.h"
#include "Singleton.h"

#include "RefCount.h"

#define USE_NEW_XML_LEVEL 1

#if USE_NEW_XML_LEVEL
#include "XmlLoader.h"
#else //USE_NEW_XML_LEVEL
class XmlLoader;
namespace boost {
	namespace property_tree {

		template < class Key, class Data, class KeyCompare >
		class basic_ptree;

		typedef basic_ptree<std::string, std::string> ptree;
	}
}

using boost::property_tree::ptree;
#endif //USE_NEW_XML_LEVEL

namespace Zuma
{
	class ITexture2D;

	typedef TRefCountPtr<ITexture2D> ITexture2DRef;

	int const MAX_BALLPATH_NUM = 2;
	int const MAX_CUTOUT_NUM = 6;
	int const MAX_TPOINT_NUM = 6;
	int const MAX_BGALPHA_NUM = 6;

	struct ZLvTPoint
	{
		Vec2D pos;
		float dist[2];
		int   idxDist;
	};

	struct ZLvCutout
	{
		ITexture2DRef image;
		Vec2D pos;
		int   pri;
	};

	struct ZBGAlpha
	{
		ITexture2DRef image;
		Vec2D pos;
		Vec2D vel;
	};


	struct ZLevelInfo
	{
		std::string nameDisp;
		std::string idMap;
		std::string idDifficulty;

		std::string pathCurve[ MAX_BALLPATH_NUM ];

		ITexture2DRef BGImage;

		int numCurve;
		int numPt;
		int numCutout;
		ZLvCutout cutout[ MAX_CUTOUT_NUM ];
		ZLvTPoint tPoint[ MAX_TPOINT_NUM ];

		Vec2D frogPos;

		int numStartBall;
		int numColor;
		int repetBall;
		int repetDec;

		void loadTexture();
		void release();
	};


	struct ZQuakeMap
	{
		int      numBGAlapha;
		ZBGAlpha bgAlpha[ MAX_BGALPHA_NUM ];

		void loadTexture();
	};

	extern int const g_NumQuakeMap;
	extern ZQuakeMap g_QuakeMap[];

	typedef std::vector< ZLevelInfo >  LevelInfoVec;


	class ZLevelManager : public SingletonT< ZLevelManager >
	{
	public:
		ZLevelManager();
		~ZLevelManager();
		bool init( char const* path );
		bool loadStage( unsigned stage , LevelInfoVec& lvList );

	private:
		void loadStageInternal( unsigned stage , LevelInfoVec& levelInfoVec );
		void loadQuakeMap();
		
#if USE_NEW_XML_LEVEL
		void loadGraphics( IXmlNodePtr node , ZLevelInfo& info );
		void load( IXmlNodePtr node , char const* mapID , ZBGAlpha& bgAlpha );
		void load( IXmlNodePtr node , char const* mapID , ZLvCutout& catout );
		void load( IXmlNodePtr node , ZLvTPoint& point );
		IXmlDocumentPtr mDoc;
#else

		void loadGraphics( ptree& pt , ZLevelInfo& info );
		void load( ptree& pt , char const* mapID , ZBGAlpha& bgAlpha );
		void load( ptree& pt , char const* mapID , ZLvCutout& catout );
		void load( ptree& pt , ZLvTPoint& point );
		
		std::auto_ptr< XmlLoader >  mXmlLoader;
#endif
		IRenderSystem* mRS;
	};

}//namespace Zuma

#endif // ZLevelManager_h__
