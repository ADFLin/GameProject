#include "FileSystem.h"
#include "PlatformConfig.h"

#include <tchar.h>
#include <stdio.h>

#include <Strsafe.h>
#include <fstream>


bool FileSystem::isExist( char const* path )
{
	WIN32_FIND_DATAA data;
	HANDLE hFind = ::FindFirstFileA( path , &data );
	if ( hFind == INVALID_HANDLE_VALUE )
		return false;

	::FindClose( hFind );
	return true;
}

bool FileSystem::findFile( char const* dir , char const* subName , FileIterator& iter )
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

bool FileSystem::getFileSize( char const* path , int64& size )
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

char const* FileUtility::getSubName( char const* fileName )
{
	char const* pos = strrchr( fileName , '.' );
	if ( pos )
	{
		++pos;
		if ( *pos != '/' || *pos != '\\' )
			return pos;
	}
	return "";
}

char const* FileUtility::getDirPathPos( char const* filePath )
{
	char const* pos = strrchr( filePath , '\\' );
	if ( !pos )
		pos = strrchr( filePath , '/' );
	return pos;
}

wchar_t const* FileUtility::getDirPathPos(wchar_t const* filePath)
{
	wchar_t const* pos = wcsrchr(filePath, L'\\');
	if( !pos )
		pos = wcsrchr(filePath, L'/');
	return pos;
}

bool FileUtility::LoadToBuffer(char const* path, std::vector< char >& outBuffer)
{
	std::ifstream fs(path);

	if( !fs.is_open() )
		return false;

	int64 size = 0;

	fs.seekg(0, std::ios::end);
	size = fs.tellg();
	fs.seekg(0, std::ios::beg);
	size -= fs.tellg();

	outBuffer.resize(size + 1);
	fs.read(&outBuffer[0], size);

	outBuffer[size] = '\0';
	fs.close();
	return true;
}
