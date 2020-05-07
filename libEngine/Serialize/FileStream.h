#pragma once
#ifndef FileStream_H_FD26BF8C_47A3_4547_AD8F_E03F1D9EC3D6
#define FileStream_H_FD26BF8C_47A3_4547_AD8F_E03F1D9EC3D6

#include "Serialize/DataStream.h"

#include <fstream>


class FileVersionData
{
	std::unordered_map< HashString, int32 > versionMap;
};


template< class FileSteamType >
class TFileFileSerializer : public IStreamSerializer
{
public:
	bool isValid() const { return mFS.good(); }
	void close()
	{
		mFS.close();
	}
	void flush()
	{
		mFS.flush();
	}
protected:
	FileSteamType mFS;

};

class InputFileSerializer : public TFileFileSerializer< std::ifstream >
{
public:
	bool open(char const* path);
	virtual void read(void* ptr, size_t num) override;
	virtual void write(void const* ptr, size_t num) override;
};

class OutputFileSerializer : public TFileFileSerializer< std::ofstream >
{
public:
	bool open(char const* path);
	virtual void read(void* ptr, size_t num) override;
	virtual void write(void const* ptr, size_t num) override;
};

class IOFileSerializer : public TFileFileSerializer< std::fstream >
{
public:
	bool open(char const* path, bool bRemoveOldData = false);
	virtual void read(void* ptr, size_t num) override;
	virtual void write(void const* ptr, size_t num) override;
	size_t getSize();
};



#endif // FileStream_H_FD26BF8C_47A3_4547_AD8F_E03F1D9EC3D6
