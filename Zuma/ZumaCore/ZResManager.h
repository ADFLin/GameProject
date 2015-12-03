#ifndef ZResManager_h__
#define ZResManager_h__

#include "ZBase.h"
#include "ResID.h"
#include <map>
#include <exception>

#define USE_NEW_XML 1

bool isFileExist( char const* path , char const* subFileName );

#if USE_NEW_XML

#include "XmlLoader.h"

#else //USE_NEW_XML

class XmlLoader;
namespace boost {
	namespace property_tree {

		template < class Key, class Data, class KeyCompare = std::less<Key> >
		class basic_ptree;

		typedef basic_ptree<std::string, std::string> ptree;
	}
}
using boost::property_tree::ptree;

#endif //USE_NEW_XML

namespace Zuma
{

#if USE_NEW_XML
	IXmlNodePtr findChildFromId( IXmlNodePtr node , char const* nameGroup ,char const* nameID );
#endif

	enum ResType
	{
		RES_IMAGE = 0,
		RES_FONT  ,
		RES_SOUND ,

		RES_TYPE_NUM ,
	};

	//class ResError : public std::exception
	//{
	//public:
	//    ResError( char const* str ): std::exception( str ){}
	//};

	enum  ResID;
	class ResManager;


	class ResBase
	{
	public:
		ResBase( ResType type )
			:mType( type ){}
		~ResBase(){}
		ResType getType() const { return mType; }
		virtual void release() = 0;

		std::string const& getPath() const { return mPath; }
		void        setPath( std::string const& path ){ mPath = path; }

	protected:

		std::string mPath;
		ResType     mType;
	};

	struct ResInfo
	{
		ResInfo( ResType type ): type( type ){}
		ResType     type;
		std::string path;
		int         flag;
	};

	class ResLoader
	{
	public:
		virtual ResBase* createResource( ResID id , ResInfo& info ) = 0;

		ResBase*     getResource( ResID id );
		ResManager*  getResManager() const { return resManager; }
	private:
		ResManager* resManager;
		friend class ResManager;
	};

	enum
	{
		IMG_ALPHA_NONE   = BIT(1),
		IMG_ALPHA_GRID   = BIT(2),
		IMG_ALPHA_NORMAL = BIT(3),
		IMG_ALPHA_ONLY   = BIT(4),

		IMG_ALPHA = IMG_ALPHA_NONE | IMG_ALPHA_GRID | IMG_ALPHA_NORMAL | IMG_ALPHA_ONLY,
		IMG_FMT_JPG      = BIT(6),
		IMG_FMT_GIF      = BIT(7),
		IMG_FMT_BMP      = BIT(8),

		SND_FMT_OGG      = BIT(10),
		SND_FMT_WAV      = BIT(11),
		SND_FMT_MP3      = BIT(12),

	};

	struct ImageResInfo : public ResInfo
	{
		ImageResInfo():ResInfo( RES_IMAGE ){}
		std::string pathAlpha;

		int numRow;
		int numCol;
	};

	class ImageRes : public ResBase
	{
	public:
		ImageRes():ResBase( RES_IMAGE ){}

		int    col;
		int    row;
	};

	struct SoundResInfo : public ResInfo
	{
		SoundResInfo():ResInfo( RES_SOUND ){}
		float volume;
	};


	struct FontResInfo : public ResInfo
	{
		FontResInfo():ResInfo( RES_FONT ){}
	};

	class ResManager
	{
	public:
		ResManager();
		~ResManager();

		bool        init( char const* path );
		bool        load( char const* resID );

		void        setWorkDir( char const* dir ){ mBaseDir = dir; }
		char const* getWorkDir(){ return mBaseDir.c_str();  }
		void        setResLoader( ResType type , ResLoader* loader );
		bool        isLoading() const{ return mCountLoading != 0; }
		int         getUnloadedResNum() const { return mCountLoading; }
		int         countResNum( char const* resID );
		ResBase*    getResource( ResID id );
		void        cleanup();

	protected:
		void       registerResID();
		ResLoader* getResLoader( ResType type ){ return mLoader[type]; }
		void       loadResource( std::string const& idName , ResInfo& info );

		struct StrCmp
		{
			bool operator()( char const* s1 , char const* s2 ) const
			{
				return strcmp( s1 , s2 ) < 0;
			}
		};
		typedef std::map< char const* , ResID , StrCmp > ResIDMap;

		ResLoader*  mLoader[ RES_TYPE_NUM ];
		std::string mBaseDir;
		ResIDMap    mResIDMap;
		int         mCountLoading;
		std::string mIdPrefix;
		std::string mCurDir;
		std::vector< ResBase* >    mResVec;

#if USE_NEW_XML
		void loadImage( IXmlNodePtr node );
		void loadSound( IXmlNodePtr node );
		void loadFont ( IXmlNodePtr node );
		IXmlDocumentPtr mDocRes;
#else
		void loadImage( ptree& pt );
		void loadSound( ptree& pt );
		void loadFont ( ptree& pt );

		std::auto_ptr< XmlLoader > mXmlLoader;
#endif

	};

}//namespace Zuma

#endif // ZResManager_h__
