#ifndef FileSystem_h__
#define FileSystem_h__

#include <string>
#include <vector>

#include "PlatformConfig.h"
#include "SystemPlatform.h"
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
	static bool LoadToBuffer(char const* path, std::vector< char >& outBuffer , bool bAppendZeroAfterEnd = false);
	static bool SaveFromBuffer(char const* path, char const* data, uint32 dataSize);
	static std::string   GetFullPath(char const* path);
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
	DateTime getLastModifyDate() const
	{
		SYSTEMTIME systemTime;
		::FileTimeToSystemTime(&mFindData.ftLastWriteTime, &systemTime);
		return DateTime(systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds);
	}
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
	static bool FindFiles( char const* dir , char const* subName , FileIterator& iterator );
	static bool IsExist( FilePath const& path ){ return IsExist( path.getString() ); }
	static bool IsExist( char const* path );
	static bool GetFileSize( char const* path , int64& size );
	static bool DeleteFile(char const* path);
	static bool RenameFile(char const* path, char const* newFileName);
};




#endif // FileSystem_h__
