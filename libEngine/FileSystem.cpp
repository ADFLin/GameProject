#include "FileSystem.h"
#include "PlatformConfig.h"

#include "CString.h"

#include <tchar.h>
#include <stdio.h>

#include <Strsafe.h>
#include <fstream>


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
	return ::DeleteFileA(path) == TRUE;
#endif
	return false;

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

char const* FileUtility::GetSubName( char const* fileName )
{
	char const* pos = FCString::Strrchr( fileName , '.' );
	if ( pos )
	{
		++pos;
		if ( *pos != '/' || *pos != '\\' )
			return pos;
	}
	return nullptr;
}

char const* FileUtility::GetDirPathPos( char const* filePath )
{
	char const* pos = FCString::Strrchr( filePath , '\\' );
	if ( !pos )
		pos = FCString::Strrchr( filePath , '/' );
	return pos;
}

wchar_t const* FileUtility::GetDirPathPos(wchar_t const* filePath)
{
	wchar_t const* pos = FCString::Strrchr(filePath, L'\\');
	if( !pos )
		pos = FCString::Strrchr(filePath, L'/');
	return pos;
}


bool FileUtility::LoadToBuffer(char const* path, std::vector< char >& outBuffer , bool bAppendZeroAfterEnd)
{
	std::ifstream fs(path , std::ios::binary);

	if( !fs.is_open() )
		return false;

	int64 size = 0;

	fs.seekg(0, std::ios::end);
	size = fs.tellg();
	fs.seekg(0, std::ios::beg);
	size -= fs.tellg();

	outBuffer.resize( bAppendZeroAfterEnd ? (size + 1) : size);
	fs.read(&outBuffer[0], size);
	if( bAppendZeroAfterEnd )
		outBuffer[size] = '\0';

	fs.close();
	return true;
}
