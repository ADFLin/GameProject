#include "FileStream.h"

bool IOFileSerializer::open(char const* path, bool bRemoveOldData /*= false */)
{
	using namespace std;
	ios_base::openmode mode = ios::binary | ios::out | ios::in;
	if( mFS.is_open() )
		return true;

	mode |= bRemoveOldData ? ios::trunc : ios::app;
	mFS.open(path, mode);
	return mFS.is_open();
}

void IOFileSerializer::read(void* ptr, size_t num)
{
	if( mFS.good() )
		mFS.read((char*)ptr, (std::streamsize) num);
}

void IOFileSerializer::write(void const* ptr, size_t num)
{
	if( mFS.good() )
		mFS.write((char const*)ptr, (std::streamsize) num);
}

size_t IOFileSerializer::getSize()
{
	std::ios::pos_type cur = mFS.tellg();
	mFS.seekg(0, mFS.end);
	std::ios::pos_type result = mFS.tellg();
	mFS.seekg(cur, mFS.beg);
	return result;
}

bool InputFileSerializer::open(char const* path)
{
	using namespace std;
	mFS.open(path, ios::binary);
	return mFS.is_open();
}

void InputFileSerializer::read(void* ptr, size_t num)
{
	if( mFS.good() )
		mFS.read((char*)ptr, (std::streamsize) num);
}

void InputFileSerializer::write(void const* ptr, size_t num)
{

}

bool OutputFileSerializer::open(char const* path)
{
	using namespace std;
	mFS.open(path, ios::binary);
	return mFS.is_open();
}

void OutputFileSerializer::read(void* ptr, size_t num)
{

}

void OutputFileSerializer::write(void const* ptr, size_t num)
{
	if( mFS.good() )
		mFS.write((char const*)ptr, (std::streamsize) num);
}
