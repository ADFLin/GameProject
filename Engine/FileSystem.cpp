#include "FileSystem.h"
#include "PlatformConfig.h"

#include "CString.h"
#include "InlineString.h"
#include "Core/ScopeGuard.h"

#include <stdio.h>
#include <fstream>
#include <sstream>

#if SYS_PLATFORM_WIN
#include <tchar.h>
#include <Strsafe.h>
#include "WindowsHeader.h"
#include "ProfileSystem.h"



#undef FindFirstFile
#undef WIN32_FIND_DATA
#undef CreateDirectory
#undef GetFileAttributesEx
#undef MoveFile
#undef GetFullPathName

template< class T >
struct TWindowsType {};

template<>
struct TWindowsType< char > 
{
	using WIN32_FIND_DATA = WIN32_FIND_DATAA;
};
template<>
struct TWindowsType< wchar_t >
{
	using WIN32_FIND_DATA = WIN32_FIND_DATAW;
};

struct FWinApi
{
	static HANDLE CreateFile(char const* lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
		LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
	{
		return CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
	}
	static HANDLE CreateFile(wchar_t const* lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
		LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
	{
		return CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
	}
	static HANDLE FindFirstFile(char const* lpFileName, WIN32_FIND_DATAA* data)
	{
		return ::FindFirstFileA(lpFileName, data);
	}
	static HANDLE FindFirstFile(wchar_t const* lpFileName, WIN32_FIND_DATAW* data)
	{
		return ::FindFirstFileW(lpFileName, data);
	}

	static BOOL CreateDirectory(char const* lpFileName, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
	{
		return ::CreateDirectoryA(lpFileName, lpSecurityAttributes);
	}
	static BOOL CreateDirectory(wchar_t const* lpFileName, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
	{
		return ::CreateDirectoryW(lpFileName, lpSecurityAttributes);
	}

	static BOOL GetFileAttributesEx(char const* lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation)
	{
		return ::GetFileAttributesExA(lpFileName, fInfoLevelId, lpFileInformation);
	}
	static BOOL GetFileAttributesEx(wchar_t const* lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation)
	{
		return ::GetFileAttributesExW(lpFileName, fInfoLevelId, lpFileInformation);
	}

	static DWORD GetFileAttributes(char const* lpFileName)
	{
		return ::GetFileAttributesA(lpFileName);
	}
	static DWORD GetFileAttributes(wchar_t const* lpFileName)
	{
		return ::GetFileAttributesW(lpFileName);
	}

	static BOOL DeleteFile(char const* lpFileName)
	{
		return ::DeleteFileA(lpFileName);
	}
	static BOOL DeleteFile(wchar_t const* lpFileName)
	{
		return ::DeleteFileW(lpFileName);
	}

	static BOOL MoveFile(char const* lpExistingFileName, char const* lpNewFileName)
	{
		return ::MoveFileA(lpExistingFileName, lpNewFileName);
	}
	static BOOL MoveFile(wchar_t const* lpExistingFileName, wchar_t const* lpNewFileName)
	{
		return ::MoveFileW(lpExistingFileName, lpNewFileName);
	}

	static BOOL CopyFile(char const* lpExistingFileName, char const* lpNewFileName, BOOL bFailIfExists)
	{
		return ::CopyFileA(lpExistingFileName, lpNewFileName, bFailIfExists);
	}
	static BOOL CopyFile(wchar_t const* lpExistingFileName, wchar_t const* lpNewFileName, BOOL bFailIfExists)
	{
		return ::CopyFileW(lpExistingFileName, lpNewFileName, bFailIfExists);
	}

	static DWORD GetFullPathName(char const* lpFileName, DWORD nBufferLength, char* lpBuffer, char** lpFilePart)
	{
		return GetFullPathNameA(lpFileName, nBufferLength, lpBuffer, lpFilePart);
	}
	static DWORD GetFullPathName(wchar_t const* lpFileName, DWORD nBufferLength, wchar_t* lpBuffer, wchar_t** lpFilePart)
	{
		return GetFullPathNameW(lpFileName, nBufferLength, lpBuffer, lpFilePart);
	}
};

struct FWindowsFileSystem 
{

	template< class CharT >
	static bool IsExist(CharT const* path)
	{
#if 0
		TWindowsType< CharT >::WIN32_FIND_DATA data;
		HANDLE hFind = FWinApi::FindFirstFile(path, &data);
		if (hFind == INVALID_HANDLE_VALUE)
			return false;

		::FindClose(hFind);
		return true;
#else
		return FWinApi::GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES;
#endif

	}

	template< class CharT >
	static bool CreateDirectory(CharT const* pathDir)
	{
		if (!FWinApi::CreateDirectory(pathDir, NULL))
		{
			switch (GetLastError())
			{
			case ERROR_ALREADY_EXISTS:
				return true;
			case ERROR_PATH_NOT_FOUND:
				return false;
			}
			return false;
		}
		return true;
	}

	template< class CharT >
	static bool GetFileSize(CharT const* path, int64& size)
	{
		WIN32_FILE_ATTRIBUTE_DATA fad;
		if (!FWinApi::GetFileAttributesEx(path, GetFileExInfoStandard, &fad))
			return false; // error condition, could call GetLastError to find out more
		LARGE_INTEGER temp;
		temp.HighPart = fad.nFileSizeHigh;
		temp.LowPart = fad.nFileSizeLow;
		size = temp.QuadPart;
		return true;
	}

	template< class CharT >
	static bool DeleteFile(CharT const* lpFileName)
	{
		return !!FWinApi::DeleteFile(lpFileName);
	}

	template< class CharT >
	static bool RenameFile(CharT const* path, CharT const* newFileName)
	{
		TInlineString< MAX_PATH , CharT > newPath;
		newPath.assign(path, FFileUtility::GetFileName(path) - path);
		newPath += newFileName;
		return !!FWinApi::MoveFile(path, newPath);
	}

	template< class CharT >
	static std::basic_string<CharT> ConvertToFullPath(CharT const* path)
	{
		CharT full_path[MAX_PATH];
		FWinApi::GetFullPathName(path, MAX_PATH, full_path, NULL);
		return full_path;
	}

	template< class CharT >
	static bool CopyFile(CharT const* path, CharT const* newFilePath, bool bFailIfExists)
	{
		return !!FWinApi::CopyFile(path, newFilePath, bFailIfExists);
	}
};
#else
#define MAX_PATH 260
#endif

bool FFileSystem::IsExist( char const* path )
{
#if SYS_PLATFORM_WIN
	return FWindowsFileSystem::IsExist(path);
#else
	return false;
#endif
}

bool FFileSystem::IsExist(wchar_t const* path)
{
#if SYS_PLATFORM_WIN
	return FWindowsFileSystem::IsExist(path);
#else
	return false;
#endif
}

bool FFileSystem::CreateDirectory(char const* pathDir)
{
#if SYS_PLATFORM_WIN
	return FWindowsFileSystem::CreateDirectory(pathDir);
#else
	return false;
#endif
}

bool FFileSystem::CreateDirectory(wchar_t const* pathDir)
{
#if SYS_PLATFORM_WIN
	return FWindowsFileSystem::CreateDirectory(pathDir);
#else
	return false;
#endif
}

bool FFileSystem::CreateDirectorySequence(char const* pathDir)
{
	if( IsExist(pathDir) )
		return true;

	InlineString<MAX_PATH> path = pathDir;
	char* cur = path.data();
	while( *cur != 0 )
	{
		if( *cur == '\\' || *cur == '/' )
		{
			char temp = *cur;
			*cur = 0;
			if( !IsExist(path) )
			{
				if( !CreateDirectory(path) )
				{
					return false;
				}
			}
			*cur = temp;
			if( cur[1] == '/' || cur[1] == '\\' )
				++cur;
		}

		++cur;
	}

	if( !CreateDirectory(pathDir) )
	{
		return false;
	}
	return true;
}

bool FFileSystem::FindFiles( char const* dir , char const* subName , FileIterator& iter )
{
	PROFILE_ENTRY("FineFiles");
#if SYS_PLATFORM_WIN

	InlineString< MAX_PATH > szDir;
	// Check that the input path plus 3 is not longer than MAX_PATH.
	// Three characters are for the "\*" plus NULL appended below.
	size_t length_of_arg = FCString::Strlen(dir);
	if (length_of_arg > (MAX_PATH - 3))
	{
		return false;
	}
	// Prepare string for use with FindFile functions.  First, copy the
	// string to a buffer, then append '\*' to the directory name.
	szDir = dir;
	if (length_of_arg == 0)
		szDir += "*";
	else
		szDir += "\\*";

	if ( subName )
		szDir += subName;

	// Find the first file in the directory.
	iter.mhFind = FindFirstFileA( szDir, &iter.mFindData );

	if ( iter.mhFind == INVALID_HANDLE_VALUE )
		return false;

	iter.mHaveMore = true;
	return true;
#else

	return false;
#endif
}

std::string FFileSystem::ConvertToFullPath(char const* path)
{
#if SYS_PLATFORM_WIN
	return FWindowsFileSystem::ConvertToFullPath(path);
#else
	return path;
#endif
}

std::wstring FFileSystem::ConvertToFullPath(wchar_t const* path)
{
#if SYS_PLATFORM_WIN
	return FWindowsFileSystem::ConvertToFullPath(path);
#else
	return path;
#endif
}

bool FFileSystem::GetFileSize( char const* path , int64& size )
{
#if SYS_PLATFORM_WIN
	return FWindowsFileSystem::GetFileSize(path, size);
#else
	using namespace std;
	ifstream fs( path , ios::binary );
	if ( !fs.is_open() )
		return false;

	fs.seekg( 0 , ios::beg );
	size = (int64)fs.tellg();
	fs.seekg( 0 , ios::end );
	size = (int64)fs.tellg() - size;

	return true;
#endif
}

bool FFileSystem::GetFileSize(wchar_t const* path, int64& size)
{
#if SYS_PLATFORM_WIN
	return FWindowsFileSystem::GetFileSize(path, size);
#else
	using namespace std;
	ifstream fs(path, ios::binary);
	if (!fs.is_open())
		return false;

	fs.seekg(0, ios::beg);
	size = (int64)fs.tellg();
	fs.seekg(0, ios::end);
	size = (int64)fs.tellg() - size;

	return true;
#endif
}

bool FFileSystem::DeleteFile(char const* path)
{
#if SYS_PLATFORM_WIN
	return !!FWindowsFileSystem::DeleteFile(path);
#else
	return false;
#endif
}

bool FFileSystem::DeleteFile(wchar_t const* path)
{
#if SYS_PLATFORM_WIN
	return !!FWindowsFileSystem::DeleteFile(path);
#else
	return false;
#endif
}


bool FFileSystem::RenameFile(char const* path , char const* newFileName)
{
#if SYS_PLATFORM_WIN
	return FWindowsFileSystem::RenameFile(path, newFileName);
#endif
	return false;
}

bool FFileSystem::RenameFile(wchar_t const* path, wchar_t const* newFileName)
{
#if SYS_PLATFORM_WIN
	return FWindowsFileSystem::RenameFile(path, newFileName);
#endif
	return false;
}


bool FFileSystem::CopyFile(char const* path, char const* newFilePath, bool bFailIfExists)
{
#if SYS_PLATFORM_WIN
	return FWindowsFileSystem::CopyFile(path, newFilePath, bFailIfExists);
#endif
	return false;
}

bool FFileSystem::CopyFile(wchar_t const* path, wchar_t const* newFilePath, bool bFailIfExists)
{
#if SYS_PLATFORM_WIN
	return FWindowsFileSystem::CopyFile(path, newFilePath, bFailIfExists);
#endif
	return false;
}

bool FFileSystem::GetFileAttributes(char const* path, FileAttributes& outAttributes)
{
#if SYS_PLATFORM_WIN
	WIN32_FILE_ATTRIBUTE_DATA fad;
	if( !::GetFileAttributesExA(path, GetFileExInfoStandard, &fad) )
	{
		return false;
	}
	LARGE_INTEGER temp;
	temp.HighPart = fad.nFileSizeHigh;
	temp.LowPart = fad.nFileSizeLow;

	outAttributes.size = temp.QuadPart;
	SYSTEMTIME systemTime = { 0 };
	::FileTimeToSystemTime(&fad.ftLastWriteTime, &systemTime);
	outAttributes.lastWrite = DateTime(systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds);
	return true;
#else

	return false;

#endif
}

#if SYS_PLATFORM_WIN
FileIterator::FileIterator()
{
	mhFind = INVALID_HANDLE_VALUE;
}

FileIterator::~FileIterator()
{
	if ( mhFind != INVALID_HANDLE_VALUE )
		::FindClose( mhFind );
}

DateTime FileIterator::getLastModifyDate() const
{
	SYSTEMTIME systemTime = { 0 };
	::FileTimeToSystemTime(&mFindData.ftLastWriteTime, &systemTime);
	return DateTime(systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds);
}

bool FileIterator::isDirectory() const
{
	return ( mFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0;
}

void FileIterator::goNext()
{
	PROFILE_ENTRY("FindNextFileA");
	mHaveMore = ( FindNextFileA( mhFind , &mFindData ) == TRUE );
}
#endif

template < class CharT >
CharT const* TFileUtility< CharT >::GetExtension(CharT const* fileName)
{
	CharT const* pos = FCString::Strrchr(fileName, STRING_LITERAL(CharT, '.'));
	if (pos)
	{
		++pos;
		if (*pos != STRING_LITERAL(CharT, '/') || *pos != STRING_LITERAL(CharT, '\\'))
			return pos;
	}
	return nullptr;
}

template < class CharT >
CharT const* TFileUtility< CharT >::GetFileName(CharT const* filePath)
{
	CharT const* pos = FCString::Strrchr(filePath, STRING_LITERAL(CharT, '\\'));
	if (pos != nullptr)
	{
		filePath = pos + 1;
	}
		
	pos = FCString::Strrchr(filePath, STRING_LITERAL(CharT, '/'));
	if (pos != nullptr)
	{
		filePath = pos + 1;
	}

	return filePath;
}

template < class CharT >
TStringView<CharT> TFileUtility<CharT>::GetDirectory(CharT const* filePath)
{
	CharT const* fileName = GetFileName(filePath);
	if (fileName != filePath)
	{
		--fileName;
		CharT c = *(fileName - 1);
		if (c == STRING_LITERAL(CharT, '/') || c == STRING_LITERAL(CharT, '\\'))
			--fileName;
	}
	return TStringView<CharT>(filePath, fileName - filePath);
}

template < class CharT >
TStringView<CharT> TFileUtility<CharT>::GetBaseFileName(CharT const* filePath)
{
	CharT const* fileName = GetFileName(filePath);
	CharT const* subName = GetExtension(fileName);
	if (subName)
	{
		return TStringView<CharT>(fileName, subName - fileName - 1);
	}
	return fileName;
}


template < class CharT , typename TArrayType >
bool LoadToBufferImpl(CharT const* path, TArrayType& outBuffer, bool bAppendZeroAfterLoad /*= false*/, bool bAppendToBuffer /*= false*/)
{
#if SYS_PLATFORM_WIN
	HANDLE hFile = FWinApi::CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	ON_SCOPE_EXIT
	{
		::CloseHandle(hFile);
	};

	LARGE_INTEGER iSize;
	::GetFileSizeEx(hFile, &iSize);
	int64 size = iSize.QuadPart;
	uint8* pRead;
	int64 sizeAdded = size;
	if (bAppendZeroAfterLoad)
		sizeAdded += 1;

	if (bAppendToBuffer)
	{
		int64 oldSize = outBuffer.size();
		outBuffer.resize(sizeAdded + oldSize);
		pRead = outBuffer.data() + oldSize;
	}
	else
	{
		outBuffer.resize(sizeAdded);
		pRead = outBuffer.data();
	}
	::ReadFile(hFile, pRead, size, NULL, NULL);

	if (bAppendZeroAfterLoad)
		outBuffer[size] = 0;

	return true;
#else
	std::ifstream fs(path, std::ios::binary);

	if (!fs.is_open())
		return false;

	int64 size = 0;

	fs.seekg(0, std::ios::end);
	size = fs.tellg();
	fs.seekg(0, std::ios::beg);
	size -= fs.tellg();

	if (bAppendToBuffer)
	{
		int64 oldSize = outBuffer.size();
		outBuffer.resize(bAppendZeroAfterLoad ? (oldSize + size + 1) : oldSize + size);
		fs.read((char*)&outBuffer[oldSize], size);
	}
	else
	{
		outBuffer.resize(bAppendZeroAfterLoad ? (size + 1) : size);
		fs.read((char*)&outBuffer[0], size);
	}


	if (bAppendZeroAfterLoad)
		outBuffer[size] = 0;

	fs.close();
	return true;
#endif
}

template < class CharT >
bool TFileUtility<CharT>::LoadToBuffer(CharT const* path, std::vector< uint8 >& outBuffer, bool bAppendZeroAfterLoad /*= false*/, bool bAppendToBuffer /*= false*/)
{
	return LoadToBufferImpl(path, outBuffer, bAppendZeroAfterLoad, bAppendToBuffer);
}

template < class CharT >
bool TFileUtility<CharT>::LoadToBuffer(CharT const* path, TArray< uint8 >& outBuffer, bool bAppendZeroAfterLoad /*= false*/, bool bAppendToBuffer /*= false*/)
{
	return LoadToBufferImpl(path, outBuffer, bAppendZeroAfterLoad, bAppendToBuffer);
}

template < class CharT >
bool TFileUtility<CharT>::SaveFromBuffer(CharT const* path, uint8 const* data, uint32 dataSize)
{
#if SYS_PLATFORM_WIN
	HANDLE hFile = FWinApi::CreateFile(path, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	ON_SCOPE_EXIT
	{
		::CloseHandle(hFile);
	};

	if (!::WriteFile(hFile, data, dataSize, NULL, NULL))
		return false;

	return true;
#else

	std::ofstream fs(path, std::ios::binary);
	if (!fs.is_open())
		return false;

	fs.write((char const*)data, dataSize);
	fs.close();
	return true;
#endif
}

bool FFileUtility::LoadLines(char const* path, std::vector< std::string >& outLineList)
{
	std::ifstream fs(path);

	if (!fs.is_open())
		return false;

	std::string str;
	while (getline(fs , str))
	{
		outLineList.push_back(std::move(str));
	}

	return true;
}

bool FFileUtility::LoadToString(char const* path, std::string& outString)
{
	std::ifstream fs(path);

	if (!fs.is_open())
		return false;

	std::ostringstream oss;
	oss << fs.rdbuf();

	outString = oss.str();
	return true;
}

template class TFileUtility<char>;
template class TFileUtility<wchar_t>;