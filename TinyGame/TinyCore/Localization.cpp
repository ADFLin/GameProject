#include "TinyGamePCH.h"
#include "Localization.h"

#include <sstream>
#include <fstream>
#include <set>

namespace Localization
{

	typedef std::string String;
	typedef std::set< String > StringList;
	typedef std::map< char const* , String >                 StringMap;


	class Dictionary;
	class Translator
	{
	public:

		Translator();
		~Translator();

		void setLanguage( Language lan );
		char const* translatePtr( char const* str );

		bool load( char const* path );
		bool loadFromString( char const* contents );
		void save( char const* path );
		struct TranslateInfo
		{
			StringMap::iterator localStr;
		};

		typedef std::map< String  , TranslateInfo  >      TranslateMap;
		typedef std::map< char const* , TranslateInfo* >  TranslatePtrMap;
		TranslateMap    mStrTransMap;
		TranslatePtrMap mPtrTransMap;

		Dictionary*  mDefult;
		Dictionary*  mLocals[ LANGUAGE_NUM ];
	};


	class Dictionary
	{
	public:
		Dictionary( Language lan )
			:mLanguage( lan )
		{

		}

		StringMap::iterator  findString( char const* ptr ){  return mStrMap.find( ptr ); }
		StringMap::iterator  addString( char const* ptr , char const* transStr );
		StringMap::iterator  getErrorIter(){ return mStrMap.end(); }
		Language  getLanguage(){ return mLanguage; }
		char const* getLanguageName(){ return mLanguageName; }

		Language    mLanguage;
		StringMap   mStrMap;
		char const* mLanguageName;

	};

	Translator g_Translator;
	StringList g_UnknownStrPtrList;

	Translator::Translator()
	{
		mDefult = NULL;
		for( int i = 0 ; i < LANGUAGE_NUM ; ++i )
			mLocals[ i ] = NULL;
	}

	Translator::~Translator()
	{
		for( int i = 0 ; i < LANGUAGE_NUM ; ++i )
			delete mLocals[ i ];
	}


	char const* Translator::translatePtr( char const* str )
	{
		TranslatePtrMap::iterator iter = mPtrTransMap.find( str );
		if ( iter != mPtrTransMap.end() )
		{
			TranslateInfo& info = *iter->second;

			if( mDefult && info.localStr != mDefult->getErrorIter() )
				return info.localStr->second.c_str();
		}

		String const findStr( str );
		TranslateMap::iterator  sIter = mStrTransMap.find( findStr );
		if ( sIter != mStrTransMap.end() )
		{
			TranslateInfo& info = sIter->second;

			if( mDefult && info.localStr != mDefult->getErrorIter() )
			{
				mPtrTransMap.insert( std::make_pair( str , &info ) );
				return info.localStr->second.c_str();
			}
		}

		g_UnknownStrPtrList.insert( findStr );
		//DevMsg( 20 , "Cant translate text %s" , str );
		return str;
	}


	bool Translator::load( char const* path )
	{

		//char token[512];
		//char str[ 512 ];
		//char transStr[512];

		//for( int i = 0 ; i < num ; ++i )
		//{
		//	std::stringstream ss( contents[i] );
		//	ss.getline( str , sizeof( str ) );

		//	if ( *str != '#' )
		//		continue;

		//	TranslateMap::iterator iter = mStrTransMap.insert( 
		//		std::make_pair( String(str + 1) , TranslateInfo() ) ).first;

		//	TranslateInfo& info      = iter->second;
		//	String const& refString  = iter->first; 

		//	ss.peek();
		//	while(  ss )
		//	{
		//		ss.getline( token , sizeof( token ) );
		//		int lan = -1;
		//		int result = sscanf(  token , "<%d>%512[^\n]" , &lan , transStr );

		//		if ( result != 2 )
		//			break;

		//		if ( !mLocals[ lan ] )
		//			mLocals[ lan ] = new Dictionary( Language(lan));

		//		Dictionary* dict = mLocals[ lan ];
		//		dict->addString( refString.c_str() , transStr );

		//		//if ( mDefult && mDefult->getLanguage() == lan )
		//		//	info.localStr = mDefult->addString( refString.c_str() , transStr );

		//	}
		//	ss.peek();
		//}

		return true;
	}

	bool Translator::loadFromString( char const* contents )
	{
		char tokenLine[ 512 ];
		char transStr[512];

		TranslateInfo* curInfo = NULL;
		String const*  curText = NULL;

		std::stringstream ss( contents );

		ss.peek();

		while(  ss )
		{
			ss.getline( tokenLine , sizeof( tokenLine ) );
			if ( *tokenLine == '#' )
			{
				TranslateMap::iterator iter = mStrTransMap.insert( 
					std::make_pair( String(tokenLine + 1) , TranslateInfo() ) ).first;

				curInfo = &iter->second;
				curText = &iter->first;
			}
			else if ( curText )
			{
				int lan = -1;
				int result = sscanf( tokenLine , "<%d>%512[^\n]" , &lan , transStr );
			
				if ( result != 2 )
					continue;

				if ( !mLocals[ lan ] )
					mLocals[ lan ] = new Dictionary( Language(lan));

				Dictionary* dict = mLocals[ lan ];
				dict->addString( curText->c_str() , transStr );
			}
			ss.peek();
		}
		return true;
	}

	void Translator::setLanguage( Language lan )
	{
		mDefult = mLocals[ lan ];

		if ( !mDefult )
		{
			return;
		}

		mPtrTransMap.clear();

		for( TranslateMap::iterator iter = mStrTransMap.begin(),
			                        iterEnd = mStrTransMap.end();
			 iter != iterEnd; ++iter )
		{
			TranslateInfo& info      = iter->second;
			String const& refString  = iter->first;

			info.localStr = mDefult->findString( refString.c_str() );
		}
	}

	void Translator::save( char const* path )
	{
		Dictionary* dict[ LANGUAGE_NUM ];
		int numDict = 0;

		for( int i = 0 ; i < LANGUAGE_NUM ; ++i )
		{
			if ( mLocals[i] ) 
				dict[ numDict++ ] = mLocals[i];
		}

		char const* textStart = "\"#";
		char const* textEnd = "\\n\"\n";

		char const* dictStart = "\t\"";
		char const* dictEnd = "\\n\"\n";

		std::ofstream fs( path );
		for( TranslateMap::iterator iter = mStrTransMap.begin(),
			                     iterEnd = mStrTransMap.end();
			 iter != iterEnd; ++iter )
		{
			String const& refString  = iter->first;

			fs << textStart << refString << textEnd;

			for( int i = 0 ; i < numDict ; ++i )
			{
				StringMap::iterator iter = dict[i]->findString( refString.c_str() );

				if ( iter != dict[i]->getErrorIter() )
				{
					fs << dictStart << "<" 
					   << (int) dict[i]->getLanguage() 
					   << ">" <<  iter->second << dictEnd;
				}
			}

			fs << dictStart << "\\n\"\n";
			fs << "\n";
		}

		for ( StringList::iterator iter = g_UnknownStrPtrList.begin(),
			                    iterEnd = g_UnknownStrPtrList.end() ;
			  iter != iterEnd ; ++ iter )
		{
			fs << textStart << *iter << textEnd;
			fs << dictStart << "\\n\"\n";
			fs << "\n";
		}
	}

	StringMap::iterator Dictionary::addString( char const* ptr , char const* transStr )
	{
		std::pair< StringMap::iterator,bool > result = 
			mStrMap.insert( std::make_pair( ptr , String(transStr) ) );

		if ( !result.second )
			result.first->second = transStr;

		return result.first;
	}

}//namespace Localization


using namespace Localization;
extern char const* translateData;
void initLanguage( Language lan )
{
	g_Translator.loadFromString( translateData );
	//Localization::g_Translator.load( contents , sizeof( contents ) / sizeof( contents[0] )  );
	g_Translator.setLanguage( lan );
}


void setLanguage( Language lan )
{
	g_Translator.setLanguage( lan );
}

void saveTranslateAsset( char const* path )
{
	g_Translator.save( path );
}

 char const* translateString( char const* str )
{
	return g_Translator.translatePtr( str );
}