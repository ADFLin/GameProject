#pragma once
#ifndef FileStream_H_FD26BF8C_47A3_4547_AD8F_E03F1D9EC3D6
#define FileStream_H_FD26BF8C_47A3_4547_AD8F_E03F1D9EC3D6

#include "Serialize/DataStream.h"
#include "HashString.h"

#include <fstream>
static_assert(TTypeSupportSerializeOPFunc<HashString>::Value);


struct FileVersionData
{
	std::unordered_map< HashString, int32 > versionMap;

	template< class OP >
	void serialize(OP& op)
	{
		op & versionMap;
	}

	int getVersion(HashString const& name)
	{
		auto iter = versionMap.find(name);
		if (iter != versionMap.end())
			return iter->second;

		return 0;
	}

	void addVersion(HashString const& name, int32 value)
	{
		versionMap[name] = value;
	}
};
TYPE_SUPPORT_SERIALIZE_FUNC(FileVersionData);


template< class TStreamType >
class TFileFileSerializer : public IStreamSerializer
{
public:
	bool isValid() const { return mFS.good(); }
	virtual void close()
	{
		mFS.close();
	}
	void flush()
	{
		mFS.flush();
	}

	virtual int32 getVersion(HashString name) override
	{
		if (name == EName::None)
			return mMasterVersion;

		if (mVersionData)
		{
			return mVersionData->getVersion(name);
		}
		return 0;
	}
protected:
	std::unique_ptr< FileVersionData > mVersionData;
	int mMasterVersion = 0;
	TStreamType mFS;

};

class InputFileSerializer : public TFileFileSerializer< std::ifstream >
{
public:
	bool open(char const* path, bool bForceLegacy = false);
	virtual void read(void* ptr, size_t num) override;
	virtual void write(void const* ptr, size_t num) override;
	size_t getSize();

	using IStreamSerializer::read;

};

class OutputFileSerializer : public TFileFileSerializer< std::ofstream >
{
public:
	~OutputFileSerializer();
	bool open(char const* path);
	virtual void close() override;
	virtual void read(void* ptr, size_t num) override;
	virtual void write(void const* ptr, size_t num) override;
	virtual void registerVersion(HashString name, int32 version);

	using IStreamSerializer::write;

private:
	bool mbNeedWriteVersionData = false;
	void writeVersionData();
};

#if 0
class IOFileSerializer : public TFileFileSerializer< std::fstream >
{
public:
	bool open(char const* path, bool bRemoveOldData = false);
	virtual void read(void* ptr, size_t num) override;
	virtual void write(void const* ptr, size_t num) override;
	size_t getSize();
};
#endif



#endif // FileStream_H_FD26BF8C_47A3_4547_AD8F_E03F1D9EC3D6
