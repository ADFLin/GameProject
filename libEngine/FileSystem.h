#ifndef FileSystem_h__
#define FileSystem_h__

#include <string>
#include <vector>

#include "PlatformConfig.h"
#include "IntegerType.h"

#ifdef SYS_PLATFORM_WIN
#include "WindowsHeader.h"
#endif

class FileUtility
{
public:
	static char const* getSubName( char const* fileName );
	static char const* getDirPathPos( char const* filePath );
	static wchar_t const* getDirPathPos(wchar_t const* filePath);
	static bool LoadToBuffer(char const* path, std::vector< char >& outBuffer);
};

class FilePath
{
public:
	FilePath( char const* path )
		:mPath( path ){}

	char const* getString() const { return mPath.c_str(); }
	//char const* getSubName(){;}
	//bool isDirectory(){}
private:
	std::string mPath;
};

class FileIterator
{
#ifdef SYS_PLATFORM_WIN

public:
	FileIterator();
	~FileIterator();

	char const* getFileName() const { return mFindData.cFileName; }
	bool   isDirectory() const;
	bool   haveMore(){ return mHaveMore; }
	void   goNext();
private:
	FileIterator( FileIterator& );
	FileIterator& operator = ( FileIterator const& );
	friend class FileSystem;

	bool   mHaveMore;
	HANDLE mhFind;
	WIN32_FIND_DATAA mFindData;
#else


#endif
};

class FileSystem
{
public:
	static bool findFile( char const* dir , char const* subName , FileIterator& iterator );
	static bool isExist( FilePath const& path ){ return isExist( path.getString() ); }
	static bool isExist( char const* path );
	static bool getFileSize( char const* path , int64& size );
};




#endif // FileSystem_h__
