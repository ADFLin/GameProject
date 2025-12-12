#include "TinyGamePCH.h"
#include "Localization.h"

#include <sstream>
#include <fstream>
#include <set>

#include "Core/String.h"

typedef std::set< String > StringList;
typedef std::map< char const*, String >                 StringMap;


class Dictionary;
class LocalizationImpl : public ILocalization
{
public:

	LocalizationImpl();
	~LocalizationImpl();

	
	virtual char const* translateText(char const* str) override;
	virtual void initialize(Language lan) override;
	virtual void changeLanguage(Language lan) override;
	virtual bool saveTranslateAsset(char const* path) override;

	bool loadTranslateAsset(char const* path);

	bool loadFromString(char const* contents);

	struct TranslateInfo
	{
		StringMap::iterator localStr;
	};

	typedef std::map< String, TranslateInfo  >      TranslateMap;
	typedef std::map< char const*, TranslateInfo* >  TranslatePtrMap;
	TranslateMap    mStrTransMap;
	TranslatePtrMap mPtrTransMap;

	Dictionary*  mUsingDict;
	Dictionary*  mLocals[LANGUAGE_NUM];

};


class Dictionary
{
public:
	Dictionary(Language lan)
		:mLanguage(lan)
	{

	}

	StringMap::iterator  findString(char const* ptr) { return mStrMap.find(ptr); }
	StringMap::iterator  addString(char const* ptr, char const* transStr);
	StringMap::iterator  getErrorIter() { return mStrMap.end(); }
	Language    getLanguage() { return mLanguage; }
	char const* getLanguageName() { return mLanguageName; }

	Language    mLanguage;
	StringMap   mStrMap;
	char const* mLanguageName;

};

StringList gUnknownStrPtrList;

LocalizationImpl::LocalizationImpl()
{
	mUsingDict = NULL;
	for( int i = 0; i < LANGUAGE_NUM; ++i )
		mLocals[i] = NULL;
}

LocalizationImpl::~LocalizationImpl()
{
	for( int i = 0; i < LANGUAGE_NUM; ++i )
		delete mLocals[i];
}


char const* LocalizationImpl::translateText(char const* str)
{
	TranslatePtrMap::iterator iter = mPtrTransMap.find(str);
	if( iter != mPtrTransMap.end() )
	{
		TranslateInfo& info = *iter->second;

		if( mUsingDict && info.localStr != mUsingDict->getErrorIter() )
			return info.localStr->second.c_str();
	}

	String const findStr(str);
	TranslateMap::iterator  sIter = mStrTransMap.find(findStr);
	if( sIter != mStrTransMap.end() )
	{
		TranslateInfo& info = sIter->second;

		if( mUsingDict && info.localStr != mUsingDict->getErrorIter() )
		{
			mPtrTransMap.insert(std::make_pair(str, &info));
			return info.localStr->second.c_str();
		}
	}

	gUnknownStrPtrList.insert(findStr);
	//DevMsg( 20 , "Cant translate text %s" , str );
	return str;
}


bool LocalizationImpl::loadTranslateAsset(char const* path)
{

#if 0
	char token[512];
	char str[ 512 ];
	char transStr[512];

	for( int i = 0 ; i < num ; ++i )
	{
		std::stringstream ss( contents[i] );
		ss.getline( str , sizeof( str ) );

		if ( *str != '#' )
			continue;

		TranslateMap::iterator iter = mStrTransMap.insert( 
			std::make_pair( String(str + 1) , TranslateInfo() ) ).first;

		TranslateInfo& info      = iter->second;
		String const& refString  = iter->first; 

		ss.peek();
		while(  ss )
		{
			ss.getline( token , sizeof( token ) );
			int lan = -1;
			int result = sscanf(  token , "<%d>%512[^\n]" , &lan , transStr );

			if ( result != 2 )
				break;

			if ( !mLocals[ lan ] )
				mLocals[ lan ] = new Dictionary( Language(lan));

			Dictionary* dict = mLocals[ lan ];
			dict->addString( refString.c_str() , transStr );

			//if ( mDefult && mDefult->getLanguage() == lan )
			//	info.localStr = mDefult->addString( refString.c_str() , transStr );

		}
		ss.peek();
	}
	return true;
#else
	return false;
#endif
}

bool LocalizationImpl::loadFromString(char const* contents)
{
	char tokenLine[512];
	char transStr[512];

	TranslateInfo* curInfo = NULL;
	String const*  curText = NULL;

	std::stringstream ss(contents);

	ss.peek();

	while( ss )
	{
		ss.getline(tokenLine, sizeof(tokenLine));
		if( *tokenLine == '#' )
		{
			TranslateMap::iterator iter = mStrTransMap.insert(
				std::make_pair(String(tokenLine + 1), TranslateInfo())).first;

			curInfo = &iter->second;
			curText = &iter->first;
		}
		else if( curText )
		{
			int lan = -1;
			int result = sscanf(tokenLine, "<%d>%512[^\n]", &lan, transStr);

			if( result != 2 )
				continue;

			if( !mLocals[lan] )
				mLocals[lan] = new Dictionary(Language(lan));

			Dictionary* dict = mLocals[lan];
			dict->addString(curText->c_str(), transStr);
		}
		ss.peek();
	}
	return true;
}

void LocalizationImpl::changeLanguage(Language lan)
{
	mUsingDict = mLocals[lan];

	if( !mUsingDict )
	{
		return;
	}

	mPtrTransMap.clear();

	for( TranslateMap::iterator iter = mStrTransMap.begin(),
		iterEnd = mStrTransMap.end();
		iter != iterEnd; ++iter )
	{
		TranslateInfo& info = iter->second;
		String const& refString = iter->first;

		info.localStr = mUsingDict->findString(refString.c_str());
	}
}


void LocalizationImpl::initialize(Language lan)
{
	extern char const* translateData;

	loadFromString(translateData);
	//load( contents , sizeof( contents ) / sizeof( contents[0] )  );
	changeLanguage(lan);
}

bool LocalizationImpl::saveTranslateAsset(char const* path)
{
	Dictionary* dict[LANGUAGE_NUM];
	int numDict = 0;

	for( int i = 0; i < LANGUAGE_NUM; ++i )
	{
		if( mLocals[i] )
			dict[numDict++] = mLocals[i];
	}

	char const* textStart = "\"#";
	char const* textEnd = "\\n\"\n";

	char const* dictStart = "\t\"";
	char const* dictEnd = "\\n\"\n";

	std::ofstream fs(path);
	if( !fs.is_open() )
		return false;
	for( TranslateMap::iterator iter = mStrTransMap.begin(),
		iterEnd = mStrTransMap.end();
		iter != iterEnd; ++iter )
	{
		String const& refString = iter->first;

		fs << textStart << refString << textEnd;

		for( int i = 0; i < numDict; ++i )
		{
			StringMap::iterator iter = dict[i]->findString(refString.c_str());

			if( iter != dict[i]->getErrorIter() )
			{
				fs << dictStart << "<"
					<< (int)dict[i]->getLanguage()
					<< ">" << iter->second << dictEnd;
			}
		}

		fs << dictStart << "\\n\"\n";
		fs << "\n";
	}

	for( StringList::iterator iter = gUnknownStrPtrList.begin(),
		iterEnd = gUnknownStrPtrList.end();
		iter != iterEnd; ++iter )
	{
		fs << textStart << *iter << textEnd;
		fs << dictStart << "\\n\"\n";
		fs << "\n";
	}

	return true;
}

StringMap::iterator Dictionary::addString(char const* ptr, char const* transStr)
{
	std::pair< StringMap::iterator, bool > result =
		mStrMap.insert(std::make_pair(ptr, String(transStr)));

	if( !result.second )
		result.first->second = transStr;

	return result.first;
}



 ILocalization& ILocalization::Get()
 {
	 static LocalizationImpl sInstance;
	 return sInstance;
 }
