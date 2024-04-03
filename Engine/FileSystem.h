#ifndef FileSystem_h__
#define FileSystem_h__

#include <string>
#include <vector>

#include "PlatformConfig.h"
#include "SystemPlatform.h"
#include "Core/IntegerType.h"
#include "Template/StringView.h"
#include "DataStructure/Array.h"

#if SYS_PLATFORM_WIN
#include "WindowsHeader.h"
#endif
#include "TemplateMisc.h"


template < class CharT >
class TFileUtility
{
public:
	static CharT const* GetExtension(CharT const* fileName);
	static CharT const* GetFileName(CharT const* filePath);
	static TStringView<CharT> GetDirectory(CharT const* filePath);
	static TStringView<CharT> GetBaseFileName(CharT const* filePath);

	static bool LoadToBuffer(CharT const* path, std::vector< uint8 >& outBuffer, bool bAppendZeroAfterLoad = false, bool bAppendToBuffer = false);
	static bool LoadToBuffer(CharT const* path, TArray< uint8 >& outBuffer, bool bAppendZeroAfterLoad = false, bool bAppendToBuffer = false);
	static bool SaveFromBuffer(CharT const* path, uint8 const* data, uint32 dataSize);
};



class FFileUtility : public TFileUtility<char> 
	               , public TFileUtility< wchar_t >
{
public:
#define FUNC_LIST(op)\
	op(GetExtension)\
	op(GetFileName)\
	op(GetDirectory)\
	op(GetBaseFileName)\
	op(LoadToBuffer)\
	op(SaveFromBuffer)

#define USING_OP( NAME )\
	using TFileUtility<char>::NAME;\
	using TFileUtility<wchar_t>::NAME;

	FUNC_LIST(USING_OP)

#undef FUNC_LIST
#undef USING_OP

	static bool LoadLines(char const* path, std::vector< std::string >& outLineList);
};

template< class CharT >
class TFilePath
{
public:
	TFilePath(CharT const* path )
		:mPath( path ){}

	CharT const* getString() const { return mPath.c_str(); }
	//char const* getSubName(){;}
	//bool isDirectory(){}
private:
	typename TStringTraits<CharT>::StdString mPath;
};

using FilePath = TFilePath<TChar>;

class FileIterator : public Noncopyable
{
#if SYS_PLATFORM_WIN

public:
	FileIterator();
	~FileIterator();

	char const* getFileName() const { return mFindData.cFileName; }
	DateTime getLastModifyDate() const
	{
		SYSTEMTIME systemTime = { 0 };
		::FileTimeToSystemTime(&mFindData.ftLastWriteTime, &systemTime);
		return DateTime(systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds);
	}
	bool   isDirectory() const;
	bool   haveMore(){ return mHaveMore; }
	void   goNext();

private:
	friend class FFileSystem;

	bool   mHaveMore;
	HANDLE mhFind;
	WIN32_FIND_DATAA mFindData;
#else


#endif
};

struct FileAttributes
{
	int64   size;
	DateTime lastWrite;
};

class FFileSystem
{
public:
	static bool FindFiles( char const* dir , char const* subName , FileIterator& iterator );

	template< class CharT >
	static bool IsExist( TFilePath< CharT > const& path ){ return IsExist( path.getString() ); }

	static bool IsExist( char const* path );
	static bool IsExist( wchar_t const* path);

	static bool CreateDirectory(char const* pathDir);
	static bool CreateDirectory(wchar_t const* pathDir);

	static bool CreateDirectorySequence(char const* pathDir);

	static std::string  ConvertToFullPath(char const* path);
	static std::wstring ConvertToFullPath(wchar_t const* path);

	static bool GetFileSize(char const* path, int64& size);
	static bool GetFileSize(wchar_t const* path, int64& size);

	static bool DeleteFile(char const* path);
	static bool DeleteFile(wchar_t const* path);

	static bool RenameFile(char const* path, char const* newFileName);
	static bool RenameFile(wchar_t const* path, wchar_t const* newFileName);

	static bool CopyFile(char const* path, char const* newFilePath, bool bFailIfExists = false);
	static bool CopyFile(wchar_t const* path, wchar_t const* newFilePath, bool bFailIfExists = false);

	static bool GetFileAttributes(char const* path, FileAttributes& outAttributes);
};




#endif // FileSystem_h__
