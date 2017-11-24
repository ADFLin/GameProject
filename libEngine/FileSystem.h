#ifndef FileSystem_h__
#define FileSystem_h__

#include <string>
#include <vector>

#include "PlatformConfig.h"
#include "Core/IntegerType.h"


#ifdef SYS_PLATFORM_WIN
#include "WindowsHeader.h"
#endif

class FileUtility
{
public:
	static char const*    GetSubName( char const* fileName );
	static char const*    GetDirPathPos(char const* filePath);
	static wchar_t const* GetDirPathPos(wchar_t const* filePath);
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
	static bool FindFile( char const* dir , char const* subName , FileIterator& iterator );
	static bool IsExist( FilePath const& path ){ return IsExist( path.getString() ); }
	static bool IsExist( char const* path );
	static bool GetFileSize( char const* path , int64& size );
	static bool DeleteFile(char const* path);
};




#endif // FileSystem_h__
