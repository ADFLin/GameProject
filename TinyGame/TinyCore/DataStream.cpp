#include "TinyGamePCH.h"
#include "DataStream.h"

bool FileStream::open(char const* path, bool bRemoveOldData /*= false */)
{
	using namespace std;
	ios_base::openmode mode = ios::binary | ios::out | ios::in;
	if( mFS.is_open() )
		return true;

	mode |= bRemoveOldData ? ios::trunc : ios::app;
	mFS.open(path, mode);
	return mFS.is_open();
}

void FileStream::read(void* ptr, size_t num)
{
	mFS.read((char*)ptr, (std::streamsize) num);
}

void FileStream::write(void const* ptr, size_t num)
{
	mFS.write((char const*)ptr, (std::streamsize) num);
}

size_t FileStream::getSize()
{
	std::ios::pos_type cur = mFS.tellg();
	mFS.seekg(0, mFS.end);
	std::ios::pos_type result = mFS.tellg();
	mFS.seekg(cur, mFS.beg);
	return result;
}

bool InputFileStream::open(char const* path)
{
	using namespace std;
	mFS.open(path, ios::binary);
	return mFS.is_open();
}

void InputFileStream::read(void* ptr, size_t num)
{
	mFS.read((char*)ptr, (std::streamsize) num);
}

void InputFileStream::write(void const* ptr, size_t num)
{

}

bool OutputFileStream::open(char const* path)
{
	using namespace std;
	mFS.open(path, ios::binary);
	return mFS.is_open();
}

void OutputFileStream::read(void* ptr, size_t num)
{

}

void OutputFileStream::write(void const* ptr, size_t num)
{
	mFS.write((char const*)ptr, (std::streamsize) num);
}
