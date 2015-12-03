#include "ZumaPCH.h"
#include "ZLevelManager.h"

#include "XmlLoader.h"
#include "IRenderSystem.h"



namespace Zuma
{
	static char const* g_QuakeMapID[]=
	{
		"riverbed" , "serpents"
	};
	int const g_NumQuakeMap = sizeof( g_QuakeMapID ) / sizeof( g_QuakeMapID[0] );
	ZQuakeMap g_QuakeMap[ g_NumQuakeMap ];

	struct Token
	{
		Token( std::string& _str , char _key ) 
			:str(_str),key(_key)
		{
			assert( !_str.empty() );
			dotPos = &_str[0];
		}
		char const* getToken()
		{
			if ( dotPos == NULL )
				return NULL;

			char const* out = dotPos;
			while ( *dotPos != key )
			{
				if ( *dotPos == '\0' )
				{
					dotPos = NULL;
					return out;
				}
				++dotPos;
			}

			*dotPos ='\0';
			++dotPos;
			return out;
		}

		char   key;
		std::string& str;
		char*  dotPos;
	};


	ZLevelManager::ZLevelManager() 
#if USE_NEW_XML_LEVEL

#else
		:mXmlLoader( new XmlLoader )
#endif
	{

	}

	ZLevelManager::~ZLevelManager()
	{
	}

	bool ZLevelManager::init( char const* path )
	{
		mRS = &Global::getRenderSystem();

		std::string filePath = Global::getResManager().getWorkDir();
		filePath += path;
#if USE_NEW_XML_LEVEL
		mDoc = IXmlDocument::createFromFile( filePath.c_str() );
		if ( !mDoc )
			return false;

#else
		bool result =  mXmlLoader->loadFile( filePath.c_str() );
		if ( !result )
			return false;
#endif

		try
		{
			loadQuakeMap();
		}

#if USE_NEW_XML_LEVEL
		catch ( XmlException& e)
#else
		catch (boost::property_tree::xml_parser_error& e)
#endif
		{
			e.what();
			return false;
		}
		return true;
	}


#if USE_NEW_XML_LEVEL

	void ZLevelManager::loadStageInternal( unsigned  stage , LevelInfoVec& levelInfoVec )
	{
		IXmlNodePtr nodeLevels = mDoc->getRoot()->getChild("Levels");
		IXmlNodePtr nodeAttr   = nodeLevels->getChild("StageProgression")->getAttriute();

		FixString< 32 > stageStr;
		stageStr.format( "stage%u" ,  stage );
		std::string mapStr = nodeAttr->getStringProperty( stageStr );
		stageStr.format( "diffi%u" ,  stage );
		std::string difStr = nodeAttr->getStringProperty( stageStr );

		//{
		//	Token tkMap( mapStr , ',' );
		//	Token tkDif( difStr , ',' );

		//	int numLvMap = 0;
		//	char const* lvMap = tkMap.getToken();
		//	while ( lvMap ){ ++numLvMap; lvMap = tkMap.getToken(); }
		//	int numDif = 0;
		//	char const* lvDif = tkDif.getToken();
		//	while ( lvDif ){ ++numDif; lvDif = tkDif.getToken(); }

		//	assert( numLvMap >= numDif );

		//	//levelInfoVec.insert( levelInfoVec.end() , numLvMap , ZLevelInfo() );

		//}

		{
			Token tkMap( mapStr , ',' );
			Token tkDif( difStr , ',' );

			char const* lvMap = tkMap.getToken();
			char const* lvDif = tkDif.getToken();

			while ( lvMap && lvDif )
			{
				levelInfoVec.push_back( ZLevelInfo() );
				ZLevelInfo& info = levelInfoVec.back();

				info.idMap        = lvMap;
				info.idDifficulty = lvDif;

				IXmlNodePtr nodeFind = findChildFromId( nodeLevels , "Graphics" , info.idMap.c_str() );
				assert( nodeFind );

				loadGraphics( nodeFind , info );

				lvMap = tkMap.getToken();
				lvDif = tkDif.getToken();
			}
		}

		mDoc->cleanupCacheObject();
	}

	void ZLevelManager::loadQuakeMap()
	{
		IXmlNodePtr nodelv = mDoc->getRoot()->getChild("Levels");

		for( int i = 0 ; i < g_NumQuakeMap ; ++i )
		{
			IXmlNodePtr nodeFind = findChildFromId( nodelv , "Graphics" , g_QuakeMapID[i] );
			ZQuakeMap& map = g_QuakeMap[i];

			map.numBGAlapha = 0;

			for ( IXmlNodeIterPtr iter = nodeFind->getChildren();
				iter->haveMore(); iter->goNext() )
			{
				char const* name = iter->getName();
				if (  strcmp( name , "BackgroundAlpha" ) == 0 )
				{
					load( iter->getNode()->getAttriute() , 
						g_QuakeMapID[i] ,
						map.bgAlpha[ map.numBGAlapha++] 
					);
				}
			}
		}
	}

	void ZLevelManager::load( IXmlNodePtr node , ZLvTPoint& point )
	{
		float x = node->getFloatProperty("x");
		float y = node->getFloatProperty("y");

		point.pos.setValue( x , y );
		if ( node->tryGetProperty( "dist1" , point.dist[0] ) )
			point.idxDist = 1;
		if ( node->tryGetProperty( "dist2" ,  point.dist[1]) )
			point.idxDist = 2;
	}

	void ZLevelManager::load( IXmlNodePtr node , char const* mapID , ZLvCutout& catout )
	{
		float x = node->getFloatProperty("x");
		float y = node->getFloatProperty("y");

		catout.pos.setValue( x , y );
		catout.pri = node->getIntProperty("pri");

		String path = "levels/";
		path += mapID;
		path += "/_";
		path += node->getStringProperty("image");
		path +=".gif";

		catout.image.reset( mRS->createEmptyTexture() );
		catout.image->setPath( path );
	}

	void ZLevelManager::load( IXmlNodePtr node , char const* mapID , ZBGAlpha& bgAlpha )
	{
		String path = "Levels/";
		path += mapID ;
		path += "/_";
		path += node->getStringProperty( "image" );
		path += ".gif";

		bgAlpha.image.reset( mRS->createEmptyTexture() );
		bgAlpha.image->setPath( path );

		float x = node->getFloatProperty("x");
		float y = node->getFloatProperty("y");

		bgAlpha.pos.setValue( x , y );

		x = node->getFloatProperty( "vx" );
		y = node->getFloatProperty( "vy" );

		bgAlpha.vel.setValue( x , y );
	}

	void ZLevelManager::loadGraphics( IXmlNodePtr node , ZLevelInfo& info )
	{
		info.numPt = 0;
		info.numCutout = 0;
		IXmlNodePtr nodeAttr = node->getAttriute();

		std::string baseDir = Global::getResManager().getWorkDir();
		String path = baseDir + "levels/";
		path += info.idMap;
		path += "/";
		path += nodeAttr->getStringProperty("image");
		path +=".jpg";

		info.BGImage.reset( mRS->createEmptyTexture() );
		info.BGImage->setPath( path );

		info.nameDisp = nodeAttr->getStringProperty( "dispname" );


		info.pathCurve[0] = baseDir + "curves/";
		info.pathCurve[0]+= nodeAttr->getStringProperty( "curve" );
		info.pathCurve[0]+= ".cvd";
		info.numCurve = 1;

		char const* str;
		if ( nodeAttr->tryGetProperty( "curve2" , str ) )
		{
			info.pathCurve[info.numCurve] = baseDir + "curves/";
			info.pathCurve[info.numCurve] += str;
			info.pathCurve[info.numCurve] += ".cvd";
			++info.numCurve;
		}

		float x = nodeAttr->getFloatProperty( "gx" );
		float y = nodeAttr->getFloatProperty( "gy" );

		info.frogPos.setValue( x , y );

		for ( IXmlNodeIterPtr iter = node->getChildren(); 
			iter->haveMore() ; iter->goNext() )
		{
			char const* name = iter->getName();

			if ( strcmp( name , "TreasurePoint" ) == 0 )
			{
				assert( info.numPt < MAX_TPOINT_NUM );
				load( iter->getNode()->getAttriute() , info.tPoint[ info.numPt++ ] );
			}
			else if ( strcmp( name , "Cutout" ) == 0 )
			{
				assert( info.numCutout < MAX_CUTOUT_NUM );
				load( iter->getNode()->getAttriute() , info.idMap.c_str() , info.cutout[ info.numCutout++ ] );
			}
			//else if ( strcmp( name ,  "BackgroundAlpha" ) == 0 )
			//{
			//	load( pr.second.get_child("<xmlattr>") , info)
			//}
		}
	}

#else //USE_NEW_XML

	void ZLevelManager::loadStageInternal( unsigned  stage , LevelInfoVec& levelInfoVec )
	{
		ptree& nodeLevels = mXmlLoader->getRoot().get_child("Levels");
		ptree& nodeStages = nodeLevels.get_child("StageProgression");

		ptree::value_type& val = nodeStages.front();
		ptree& nodeAttr = nodeStages.get_child( XML_ATTR_PATH );

		char stageStr[ 128 ];

		sprintf( stageStr , "stage%u" ,  stage );
		std::string mapStr = nodeAttr.get< std::string >( stageStr );
		sprintf( stageStr , "diffi%u" ,  stage );
		std::string difStr = nodeAttr.get< std::string >( stageStr );


		//{
		//	Token tkMap( mapStr , ',' );
		//	Token tkDif( difStr , ',' );

		//	int numLvMap = 0;
		//	char const* lvMap = tkMap.getToken();
		//	while ( lvMap ){ ++numLvMap; lvMap = tkMap.getToken(); }
		//	int numDif = 0;
		//	char const* lvDif = tkDif.getToken();
		//	while ( lvDif ){ ++numDif; lvDif = tkDif.getToken(); }

		//	assert( numLvMap >= numDif );

		//	//levelInfoVec.insert( levelInfoVec.end() , numLvMap , ZLevelInfo() );

		//}

		{
			Token tkMap( mapStr , ',' );
			Token tkDif( difStr , ',' );

			char const* lvMap = tkMap.getToken();
			char const* lvDif = tkDif.getToken();

			while ( lvMap && lvDif )
			{
				levelInfoVec.push_back( ZLevelInfo() );
				ZLevelInfo& info = levelInfoVec.back();

				info.idMap        = lvMap;
				info.idDifficulty = lvDif;

				ptree* ptFind = mXmlLoader->findID( nodeLevels  , "Graphics" , info.idMap.c_str() );
				assert( ptFind );

				loadGraphics( *ptFind , info );

				lvMap = tkMap.getToken();
				lvDif = tkDif.getToken();
			}
		}
	}

	void ZLevelManager::loadQuakeMap()
	{
		ptree& nodelv = mXmlLoader->getRoot().get_child("Levels");

		for( int i = 0 ; i < g_NumQuakeMap ; ++i )
		{
			ptree* nodeFind = mXmlLoader->findID( nodelv  , "Graphics" , g_QuakeMapID[i] );
			assert( nodeFind );
			ZQuakeMap& map = g_QuakeMap[i];

			map.numBGAlapha = 0;

			BOOST_FOREACH( ptree::value_type& pr , *nodeFind )
			{
				if ( pr.first == "BackgroundAlpha" )
				{
					load( pr.second.get_child("<xmlattr>") , 
						g_QuakeMapID[i] ,
						map.bgAlpha[ map.numBGAlapha++] 
					);
				}
			}
		}
	}

	void ZLevelManager::load( ptree& node , ZLvTPoint& point )
	{
		float x = node.get<float>("x");
		float y = node.get<float>("y");

		point.pos.setValue( x , y );

		if ( XmlLoader::getVal( node , point.dist[0] , "dist1" ) )
			point.idxDist = 1;

		if ( XmlLoader::getVal( node , point.dist[1] , "dist2" ) )
			point.idxDist = 2;

	}
	void ZLevelManager::load( ptree& node , char const* mapID , ZLvCutout& catout )
	{
		float x = node.get<float>("x");
		float y = node.get<float>("y");

		catout.pos.setValue( x , y );
		catout.pri = node.get<int>("pri");

		String path = "levels/";
		path += mapID;
		path += "/_";
		path += node.get< std::string >("image");
		path +=".gif";

		catout.image.reset( mRS->createEmptyTexture() );
		catout.image->setPath( path );
	}

	void ZLevelManager::load( ptree& pt , char const* mapID ,ZBGAlpha& bgAlpha )
	{
		String path = "Levels/";
		path += mapID ;
		path += "/_";
		path += pt.get< String >( "image" );
		path += ".gif";

		bgAlpha.image.reset( mRS->createEmptyTexture() );
		bgAlpha.image->setPath( path );

		float x = pt.get<float>("x");
		float y = pt.get<float>("y");

		bgAlpha.pos.setValue( x , y );

		x = pt.get< float >( "vx" );
		y = pt.get< float >( "vy" );

		bgAlpha.vel.setValue( x , y );
	}


	void ZLevelManager::loadGraphics( ptree& pt , ZLevelInfo& info )
	{
		info.numPt = 0;
		info.numCutout = 0;
		ptree& ptAttr = pt.get_child(XML_ATTR_PATH);


		std::string baseDir = Global::getResManager().getWorkDir();
		String path = baseDir + "levels/";
		path += info.idMap;
		path += "/";
		path += ptAttr.get< std::string >("image");
		path +=".jpg";

		info.BGImage.reset( mRS->createEmptyTexture() );
		info.BGImage->setPath( path );

		info.nameDisp = ptAttr.get< std::string >("dispname");

		std::string str = ptAttr.get< std::string >( "curve" );

		info.pathCurve[0] = baseDir + "curves/";
		info.pathCurve[0]+= str;
		info.pathCurve[0]+= ".cvd";
		info.numCurve = 1;

		if ( XmlLoader::getVal( ptAttr , str , "curve2") )
		{
			info.pathCurve[info.numCurve] = baseDir + "curves/";
			info.pathCurve[info.numCurve] += str;
			info.pathCurve[info.numCurve] += ".cvd";
			++info.numCurve;
		}

		float x = ptAttr.get<float>("gx");
		float y = ptAttr.get<float>("gy");

		info.frogPos.setValue( x , y );

		BOOST_FOREACH( ptree::value_type& pr , pt )
		{
			if ( pr.first == "TreasurePoint" )
			{
				assert( info.numPt < MAX_TPOINT_NUM );
				load( pr.second.get_child(XML_ATTR_PATH) , info.tPoint[ info.numPt++ ] );
			}
			else if ( pr.first == "Cutout")
			{
				assert( info.numCutout < MAX_CUTOUT_NUM );
				load( pr.second.get_child(XML_ATTR_PATH) , info.idMap.c_str() , info.cutout[ info.numCutout++ ] );
			}
			//else if ( pr.first == "BackgroundAlpha" )
			//{
			//	load( pr.second.get_child("<xmlattr>") , info)
			//}
		}
	}

#endif //USE_NEW_XML

	bool ZLevelManager::loadStage( unsigned  stage , LevelInfoVec&  lvList )
	{
		try
		{
			lvList.clear();
			loadStageInternal( stage , lvList );
		}
#if USE_NEW_XML
		catch ( XmlException& e)
#else
		catch (boost::property_tree::xml_parser_error& e)
#endif
		{
			return false;
		}
		return true;
	}

	void ZLevelInfo::release()
	{
		BGImage->release();

		for( int i = 0 ; i < numCutout ; ++i )
			cutout[i].image->release();
	}

	void ZLevelInfo::loadTexture()
	{
		IRenderSystem& renderSys = Global::getRenderSystem();

		BGImage->release();
		if ( !renderSys.loadTexture( *BGImage ,  BGImage->getPath().c_str() , NULL ) )
		{
			WarmingMsg( 0 , "Can't load Level BG Image %s" , BGImage->getPath().c_str() );
		}

		for( int i = 0 ; i < numCutout ; ++i )
		{
			ITexture2D* image = cutout[i].image.get();

			image->release();
			if( !renderSys.loadTexture( *image , NULL , image->getPath().c_str() ) )
			{
				WarmingMsg( 0 , "Can't load Level Mask Image %s" , image->getPath().c_str() );
			}
		}
	}

	void ZQuakeMap::loadTexture()
	{
		IRenderSystem& renderSys = Global::getRenderSystem();
		for ( int i = 0 ; i < numBGAlapha ; ++i )
		{
			ITexture2D* image = bgAlpha[i].image.get();

			image->release();
			renderSys.loadTexture( *image , NULL , image->getPath().c_str() );
		}
	}

}//namespace Zuma
