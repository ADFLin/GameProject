#include "FileSystem.h"
#include "PlatformConfig.h"

#include "CString.h"
#include "FixString.h"

#include <tchar.h>
#include <stdio.h>

#include <Strsafe.h>
#include <fstream>

#if SYS_PLATFORM_WIN
#include "WindowsHeader.h"
#endif

bool FileSystem::IsExist( char const* path )
{
#ifdef SYS_PLATFORM_WIN
	WIN32_FIND_DATAA data;
	HANDLE hFind = ::FindFirstFileA( path , &data );
	if ( hFind == INVALID_HANDLE_VALUE )
		return false;

	::FindClose( hFind );
	return true;
#else
	return false;
#endif
}

bool FileSystem::CreateDirectory(char const* pathDir)
{
#ifdef SYS_PLATFORM_WIN
	if( !::CreateDirectoryA(pathDir, NULL) )
	{
		switch( GetLastError() )
		{
		case ERROR_ALREADY_EXISTS:
			return false;
		case ERROR_PATH_NOT_FOUND:
			return false;
		} 
		return false;
	}
	return true;
#else
	return false;
#endif
}

bool FileSystem::CreateDirectorySequence(char const* pathDir)
{
	if( IsExist(pathDir) )
		return true;

	FixString<MAX_PATH> path = pathDir;
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

bool FileSystem::FindFiles( char const* dir , char const* subName , FileIterator& iter )
{
	char szDir[MAX_PATH];

	DWORD dwError=0;

	// Check that the input path plus 3 is not longer than MAX_PATH.
	// Three characters are for the "\*" plus NULL appended below.
	size_t length_of_arg;
	StringCchLengthA( dir , MAX_PATH, &length_of_arg);
	if (length_of_arg > (MAX_PATH - 3))
	{
		return false;
	}
	// Prepare string for use with FindFile functions.  First, copy the
	// string to a buffer, then append '\*' to the directory name.
	StringCchCopyA(szDir, MAX_PATH, dir );
	if ( length_of_arg == 0 )
		StringCchCatA(szDir, MAX_PATH, "*" );
	else
		StringCchCatA(szDir, MAX_PATH, "\\*" );
	if ( subName )
		StringCchCatA( szDir , MAX_PATH , subName );

	// Find the first file in the directory.
	iter.mhFind = FindFirstFileA( szDir, &iter.mFindData );

	if ( iter.mhFind == INVALID_HANDLE_VALUE )
		return false;

	iter.mHaveMore = true;
	return true;
}

bool FileSystem::GetFileSize( char const* path , int64& size )
{
#ifdef SYS_PLATFORM_WIN
	WIN32_FILE_ATTRIBUTE_DATA fad;
	if (!GetFileAttributesExA( path , GetFileExInfoStandard , &fad))
		return false; // error condition, could call GetLastError to find out more
	LARGE_INTEGER temp;
	temp.HighPart = fad.nFileSizeHigh;
	temp.LowPart = fad.nFileSizeLow;
	size = temp.QuadPart;

	return true;
#else

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

bool FileSystem::DeleteFile(char const* path)
{
#if SYS_PLATFORM_WIN
	return !!::DeleteFileA(path);
#endif
	return false;

}

bool FileSystem::RenameFile(char const* path , char const* newFileName)
{
#if SYS_PLATFORM_WIN
	FixString< MAX_PATH > newPath;
	newPath.assign(path, FileUtility::GetFileName(path) - path);
	newPath += newFileName;
	return !!::MoveFileA(path , newPath);
#endif
	return false;
}

bool FileSystem::GetFileAttributes(char const* path, FileAttributes& outAttributes)
{
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
}

#ifdef SYS_PLATFORM_WIN
FileIterator::FileIterator()
{
	mhFind = INVALID_HANDLE_VALUE;
}

FileIterator::~FileIterator()
{
	if ( mhFind != INVALID_HANDLE_VALUE )
		::FindClose( mhFind );
}

bool FileIterator::isDirectory() const
{
	return ( mFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0;
}

void FileIterator::goNext()
{
	mHaveMore = ( FindNextFileA( mhFind , &mFindData ) == TRUE );
}
#endif

template< class CharT >
CharT const* GetExtensionImpl(CharT const* fileName)
{
	CharT const* pos = FCString::Strrchr(fileName, STRING_LITERAL(CharT, '.'));
	if( pos )
	{
		++pos;
		if( *pos != STRING_LITERAL(CharT, '/') || *pos != STRING_LITERAL(CharT, '\\') )
			return pos;
	}
	return nullptr;
}

char const* FileUtility::GetExtension( char const* fileName )
{
	return GetExtensionImpl(fileName);
}

template< class CharT >
CharT const* GetFileNameImpl(CharT const* filePath)
{
	CharT const* pos = FCString::Strrchr(filePath, STRING_LITERAL(CharT ,'\\'));
	if( pos == nullptr )
	{
		pos = FCString::Strrchr(filePath, STRING_LITERAL(CharT, '/'));
		if( pos == nullptr )
			return filePath;
	}
	return pos + 1;
}


char const* FileUtility::GetFileName( char const* filePath )
{
	return GetFileNameImpl(filePath);
}

wchar_t const* FileUtility::GetFileName(wchar_t const* filePath)
{
	return GetFileNameImpl(filePath);
}


bool FileUtility::LoadToBuffer(char const* path, std::vector< char >& outBuffer , bool bAppendZeroAfterEnd , bool bAppendToBuffer )
{
	std::ifstream fs(path , std::ios::binary);

	if( !fs.is_open() )
		return false;


	int64 size = 0;

	fs.seekg(0, std::ios::end);
	size = fs.tellg();
	fs.seekg(0, std::ios::beg);
	size -= fs.tellg();

	if( bAppendToBuffer )
	{
		int64 oldSize = outBuffer.size();
		outBuffer.resize(bAppendZeroAfterEnd ? (oldSize + size + 1) : oldSize + size);
		fs.read(&outBuffer[oldSize], size);
	}
	else
	{
		outBuffer.resize(bAppendZeroAfterEnd ? (size + 1) : size);
		fs.read(&outBuffer[0], size);
	}


	if( bAppendZeroAfterEnd )
		outBuffer[size] = 0;

	fs.close();
	return true;
}

bool FileUtility::SaveFromBuffer(char const* path, char const* data, uint32 dataSize)
{
	std::ofstream fs(path, std::ios::binary);
	if( !fs.is_open() )
		return false;

	fs.write(data, dataSize);
	fs.close();
	return true;
}

std::string FileUtility::GetFullPath(char const* path)
{
#if SYS_PLATFORM_WIN
	char full_path[MAX_PATH];
	GetFullPathNameA( path , MAX_PATH, full_path, NULL);
	return full_path;
#endif
}

StringView FileUtility::GetDirectory(char const* filePath)
{
	char const* fileName = GetFileName(filePath);
	if( fileName != filePath )
	{
		--fileName;
		char c =*( fileName - 1 );
		if( c == '/' || c == '\\' )
			--fileName;
	}
	return StringView(filePath, fileName - filePath);
}

StringView FileUtility::CutDirAndExtension(char const* filePath)
{
	char const* fileName = GetFileName(filePath);
	char const* subName = GetExtension(fileName);
	if( subName )
	{
		return StringView(fileName, subName - fileName - 1);
	}
	return fileName;
}
