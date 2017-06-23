#include "ZumaPCH.h"

#include "XmlLoader.h"
#include "ZResManager.h"
#include "ResID.h"
#include "IRenderSystem.h"

#include "FileSystem.h"

bool isFileExist( char const* path , char const* subFileName )
{
	std::string pathFile = std::string(path) + subFileName;
	return FileSystem::isExist( pathFile.c_str() );
}

namespace Zuma
{
	ResBase* ResLoader::getResource( ResID id )
	{
		return resManager->getResource( id );
	}

	ResManager::ResManager()
#if USE_NEW_XML
		
#else
		:mXmlLoader( new XmlLoader )
#endif
	{
		mResVec.resize( RES_NUM_RESOURCES , NULL );
		mCountLoading = 0;
		registerResID();
	}


	ResManager::~ResManager()
	{
		cleanup();
	}

	bool ResManager::init( char const* path )
	{
		std::string filePath = mBaseDir + path;
#if USE_NEW_XML
		mDocRes = IXmlDocument::CreateFromFile( filePath.c_str() );
		return mDocRes;
#else
		return mXmlLoader->loadFile( filePath.c_str() );
#endif
	}

#if USE_NEW_XML

	IXmlNodePtr findChildFromId( IXmlNodePtr node , char const* nameGroup ,char const* nameID )
	{
		for( IXmlNodeIterPtr iter = node->getChildGroup( nameGroup ) ; 
			iter->haveMore() ; iter->goNext() )
		{
			IXmlNodePtr nodeTest = iter->getNode();
			IXmlNodePtr nodeAttr = nodeTest->getAttriute();

			if ( !nodeAttr )
				continue;

			char const* str;
			if ( nodeAttr->tryGetProperty( "id" , str ) &&
				 strcmp( str , nameID ) == 0 )
			{
				return nodeTest;
			}
		}
		return NULL;
	}

	int ResManager::countResNum( char const* resID )
	{
		try
		{
			IXmlNodePtr node = findChildFromId( 
				mDocRes->getRoot()->getChild( "ResourceManifest" ) , 
				"Resources" , resID );

			if ( node == NULL )
				return false;

			return node->countChildGroup("Image") + 
				   node->countChildGroup("Sound") + 
				   node->countChildGroup("Font");
		}
		catch (boost::property_tree::xml_parser_error& e)
		{
			return 0;
		}
	}

	bool ResManager::load( char const* resID )
	{
		try
		{
			IXmlNodePtr resNode = findChildFromId( 
				mDocRes->getRoot()->getChild("ResourceManifest") , 
				"Resources" , resID );

			if ( resNode == NULL )
				return false;

			mCountLoading = resNode->countChildGroup( "Image" ) +
				            resNode->countChildGroup( "Sound" ) +
							resNode->countChildGroup( "Font" );

			for( IXmlNodeIterPtr iter = resNode->getChildren(); 
				  iter->haveMore() ; iter->goNext() )
			{
				IXmlNodePtr node = iter->getNode();
				char const* name = iter->getName();
				if ( strcmp( name , "SetDefaults" ) == 0 )
				{
					IXmlNodePtr nodeAttr = node->getAttriute();

					mCurDir = mBaseDir ;
					mCurDir += nodeAttr->getStringProperty( "path" );
					mCurDir += "/";
					mIdPrefix = nodeAttr->getStringProperty( "idprefix" );
				}
				else if ( strcmp( name , "Image" ) == 0 )
				{
					IXmlNodePtr nodeAttr = node->getAttriute();
					loadImage( nodeAttr );
					--mCountLoading;
				}
				else if ( strcmp( name , "Sound" ) == 0 )
				{
					IXmlNodePtr nodeAttr = node->getAttriute();
					loadSound( nodeAttr );
					--mCountLoading;
				}
				else if ( strcmp( name , "Font" ) == 0 )
				{
					IXmlNodePtr nodeAttr = node->getAttriute();
					loadFont( nodeAttr );
					--mCountLoading;
				}
			}
		}
		catch (boost::property_tree::xml_parser_error& e)
		{
			mCountLoading = 0;
			e.what();
			return false;
		}

		mCountLoading = 0;
		mDocRes->cleanupCacheObject();
		return true;
	}


	void ResManager::loadImage( IXmlNodePtr node )
	{
		ImageResInfo info;

		info.flag = 0;
		char const* str;
		if ( node->tryGetProperty( "path" , str ) )
		{
			info.path = mCurDir + str;

			if ( isFileExist( info.path.c_str() , ".gif") )
			{
				info.path += ".gif";
				info.flag |= IMG_FMT_GIF;
			}
			else if ( isFileExist( info.path.c_str() , ".jpg"))
			{
				info.path += ".jpg";
				info.flag |= IMG_FMT_JPG;
			}
			else if ( isFileExist( info.path.c_str() , ".bmp"))
			{
				info.path += ".bmp";
				info.flag |= IMG_FMT_BMP;
			}
		}

		info.numCol = node->getProperty( "cols" , -1 );
		info.numRow = node->getProperty( "rows" , -1 );


		if ( node->tryGetProperty( "alphaimage"  , str ) )
		{
			info.pathAlpha = mCurDir;
			info.pathAlpha += str;
			info.pathAlpha += ".gif";
			if ( !info.path.empty() )
				info.flag |= IMG_ALPHA_NORMAL;
			else
			{
				info.flag |= IMG_ALPHA_ONLY | IMG_FMT_GIF;
			}
		}
		else if ( node->tryGetProperty( "alphagrid" , str  ) )
		{
			info.pathAlpha = mCurDir + str + ".gif" ;
			info.flag |= IMG_ALPHA_GRID;
		}
		else if ( node->tryGetProperty( "path" , str ) &&  str[0] != '_' )
		{
			std::string path = mCurDir + "_" + str;

			if ( isFileExist( path.c_str() , ".gif" ) )
			{
				if ( info.path.empty() )
					info.flag |= IMG_ALPHA_ONLY;
				else
					info.flag |= IMG_ALPHA_NORMAL;
				info.pathAlpha = path + ".gif";
			}
			else
				info.flag |= IMG_ALPHA_NONE;
		}
		else
			info.flag |= IMG_ALPHA_NONE;

	
		loadResource( mIdPrefix + node->getStringProperty( "id" ) , info );
	}

	void ResManager::loadSound( IXmlNodePtr node )
	{
		SoundResInfo info;
		info.flag = 0;

		std::string str;
		info.path = mCurDir + node->getStringProperty( "path" );

		if ( isFileExist( info.path.c_str() , ".wav") )
		{
			info.path += ".wav";
			info.flag |= SND_FMT_WAV;
		}
		else if ( isFileExist( info.path.c_str() , ".ogg"))
		{
			info.path += ".ogg";
			info.flag |= SND_FMT_OGG;
		}
		else if ( isFileExist( info.path.c_str() , ".mp3"))
		{
			info.path += ".mp3";
			info.flag |= SND_FMT_MP3;
		}
		else
		{
			return;
		}

		info.volume = node->getProperty( "volume" , 1.0f );

		loadResource( mIdPrefix + node->getStringProperty( "id" ) , info );
	}


	void ResManager::loadFont( IXmlNodePtr node )
	{
		FontResInfo info;
		char const* str;

		node->tryGetProperty( "path" , str );

		if ( strcmp( str , "!program" ) == 0 )
			return;

		info.path = mCurDir  + str;

		node->tryGetProperty( "id" , str );
		loadResource( mIdPrefix + str  , info );
	}

#else  // USE_XML_NEW

	int ResManager::countResNum( char const* resID )
	{
		try
		{
			ptree* pt = mXmlLoader->findID( 
				mXmlLoader->getRoot().get_child("ResourceManifest") , 
				"Resources" , resID );

			if ( pt == NULL )
				return false;

			return pt->count("Image") + pt->count("Sound") + pt->count("Font");
		}
		catch (boost::property_tree::xml_parser_error& e)
		{
			return 0;
		}
	}


	bool ResManager::load( char const* resID )
	{
		try
		{
			ptree* pt = mXmlLoader->findID( 
				mXmlLoader->getRoot().get_child("ResourceManifest") , 
				"Resources" , resID );

			if ( pt == NULL )
				return false;

			mCountLoading = pt->count("Image") + pt->count("Sound") + pt->count("Font");

			BOOST_FOREACH( ptree::value_type& node , *pt )
			{
				if ( node.first == "SetDefaults" )
				{
					ptree& ptAttr = node.second.get_child( XML_ATTR_PATH );
					mCurDir = mBaseDir ;
					mCurDir += ptAttr.get< std::string >( "path" );
					mCurDir += "/";

					mIdPrefix = ptAttr.get< std::string >( "idprefix" );
				}
				else if ( node.first == "Image" )
				{
					ptree& ptAttr = node.second.get_child( XML_ATTR_PATH );
					loadImage( ptAttr );
					--mCountLoading;
				}
				else if ( node.first == "Sound" )
				{
					ptree& ptAttr = node.second.get_child( XML_ATTR_PATH );
					loadSound( ptAttr );
					--mCountLoading;
				}
				else if ( node.first == "Font" )
				{
					ptree& ptAttr = node.second.get_child( XML_ATTR_PATH );
					loadFont( ptAttr );
					--mCountLoading;
				}
			}
		}
		catch (boost::property_tree::xml_parser_error& e)
		{
			mCountLoading = 0;
			e.what();
			return false;
		}

		mCountLoading = 0;
		return true;
	}


	void ResManager::loadImage( ptree& pt )
	{
		ImageResInfo info;

		info.flag = 0;
		std::string str;

		if ( XmlLoader::getVal( pt , str , "path" ) )
		{
			info.path = mCurDir  + str;

			if ( isFileExist( info.path.c_str() , ".gif") )
			{
				info.path += ".gif";
				info.flag |= IMG_FMT_GIF;
			}
			else if ( isFileExist( info.path.c_str() , ".jpg"))
			{
				info.path += ".jpg";
				info.flag |= IMG_FMT_JPG;
			}
			else if ( isFileExist( info.path.c_str() , ".bmp"))
			{
				info.path += ".bmp";
				info.flag |= IMG_FMT_BMP;
			}
		}

		if ( !XmlLoader::getVal( pt , info.numCol , "cols" ) )
			info.numCol = -1;

		if ( !XmlLoader::getVal( pt , info.numRow , "rows" ) )
			info.numRow = -1;

		if ( XmlLoader::getVal( pt , str , "alphaimage" ) )
		{
			info.pathAlpha = mCurDir + str + ".gif";
			if ( !info.path.empty() )
				info.flag |= IMG_ALPHA_NORMAL;
			else
			{
				info.flag |= IMG_ALPHA_ONLY | IMG_FMT_GIF;
			}
		}
		else if ( XmlLoader::getVal( pt , str , "alphagrid" ) )
		{
			info.pathAlpha = mCurDir + str + ".gif" ;
			info.flag |= IMG_ALPHA_GRID;
		}
		else if ( XmlLoader::getVal( pt , str , "path" ) &&  str[0] != '_' )
		{
			str = mCurDir + "_" + str;

			if ( isFileExist( str.c_str() , ".gif" ) )
			{
				if ( info.path.empty() )
					info.flag |= IMG_ALPHA_ONLY;
				else
					info.flag |= IMG_ALPHA_NORMAL;
				info.pathAlpha = str + ".gif";
			}
			else
				info.flag |= IMG_ALPHA_NONE;
		}
		else
			info.flag |= IMG_ALPHA_NONE;

		XmlLoader::getVal( pt , str , "id" );

		loadResource( mIdPrefix + str , info );
	}


	void ResManager::loadSound( ptree& pt )
	{
		SoundResInfo info;
		info.flag = 0;

		std::string str;

		XmlLoader::getVal( pt , str , "path" );

		info.path = mCurDir  + str;

		if ( isFileExist( info.path.c_str() , ".wav") )
		{
			info.path += ".wav";
			info.flag |= SND_FMT_WAV;
		}
		else if ( isFileExist( info.path.c_str() , ".ogg"))
		{
			info.path += ".ogg";
			info.flag |= SND_FMT_OGG;
		}
		else
		{
			return;
		}

		if ( !XmlLoader::getVal( pt , info.volume , "volume" ) )
			info.volume = 1.0;

		XmlLoader::getVal( pt , str , "id" );
		loadResource( mIdPrefix + str , info );

	}



	void ResManager::loadFont( ptree& pt )
	{
		FontResInfo info;
		std::string str;

		XmlLoader::getVal( pt , str , "path" );

		if ( str == "!program" )
			return;

		info.path = mCurDir  + str;

		XmlLoader::getVal( pt , str , "id" );
		loadResource( mIdPrefix + str  , info );
	}

#endif // USE_XML_NEW

	void ResManager::loadResource( std::string const& idName  , ResInfo& info )
	{
		ResLoader* loader = getResLoader( info.type );

		if ( loader == NULL )
		{
			ErrorMsg("Res Type %d don't have loader" , info.type );
			return;
		}

		ResIDMap::iterator iter = mResIDMap.find( idName.c_str() );

		if ( iter == mResIDMap.end() )
			return;

		ResID id = iter->second;

		if ( id == IMAGE_DIALOG_CHECKBOXCAP )
		{
			int i = 1;
		}

		if ( mResVec[ id ] == NULL )
			mResVec[ id ] = loader->createResource( id , info );

		if ( mResVec[ id ] == NULL )
		{
			::WarmingMsg( 0 , "Can't load Resource : %d %s" , id , idName.c_str() );
		}

	}
#include "ResID.h"
	void ResManager::registerResID()
	{
#define RegisterOp( A ) mResIDMap.insert( std::make_pair( #A , A ) );
		RES_ID_LIST( RegisterOp )
#undef RegisterOp
	}

	ResBase* ResManager::getResource( ResID id )
	{
		return mResVec[id];
	}

	void ResManager::setResLoader( ResType type , ResLoader* loader )
	{
		mLoader[ type ] = loader;
		loader->resManager = this;
	}

	void ResManager::cleanup()
	{
		for ( int i = 0 ; i < mResVec.size() ; ++i )
		{
			ResBase* res = mResVec[i];
			if ( res )
			{
				res->release();
				delete res;
			}
		}
		mResVec.clear();
	}

}//namespace Zuma
