#pragma once
#ifndef FileStream_H_FD26BF8C_47A3_4547_AD8F_E03F1D9EC3D6
#define FileStream_H_FD26BF8C_47A3_4547_AD8F_E03F1D9EC3D6

#include "Serialize/DataStream.h"
#include "HashString.h"
#include "LogSystem.h"

#include <fstream>


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

template< class TStreamType >
class TFileStreamSerializer : public IStreamSerializer
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

	int32 getVersion(HashString name) override
	{
		while(redirectName(name)){}

		if (name == EName::None)
			return mMasterVersion;

		if (mVersionData)
		{
			return mVersionData->getVersion(name);
		}
		return 0;
	}

	void redirectVersion(HashString from, HashString to) override
	{
		if (from == to)
		{
			mRedirectVersionNameMap.erase(from);
		}
		else
		{
			mRedirectVersionNameMap[from] = to;
#if _DEBUG
			if (detectCycleRedirect(from))
			{
				LogError("Cycle Redirect detect!! %s -> %s", from.c_str(), to.c_str());
				mRedirectVersionNameMap.erase(from);
			}
#endif
		}
	}

	bool redirectName(HashString& inoutName)
	{
		auto iter = mRedirectVersionNameMap.find(inoutName);
		if (iter == mRedirectVersionNameMap.end())
			return false;
		inoutName = iter->second;
		return true;
	}

	bool detectCycleRedirect(HashString from)
	{
		HashString a = from;
		HashString b = from;

		for(;;)
		{
			if (!redirectName(a))
				break;
			if (!redirectName(b))
				break;
			if (!redirectName(b))
				break;
			if (a == b)
				return true;
		}
		return false;
	}
protected:
	std::unordered_map< HashString, HashString > mRedirectVersionNameMap;
	std::unique_ptr< FileVersionData > mVersionData;
	int mMasterVersion = 0;
	TStreamType mFS;
};

class InputFileSerializer : public TFileStreamSerializer< std::ifstream >
{
public:
	bool open(char const* path, bool bCheckLegacy = false);
	bool isEOF();
	virtual void read(void* ptr, size_t num) override;
	virtual void write(void const* ptr, size_t num) override;

	size_t getSize();
	std::ios::pos_type mEOFPos;

	using IStreamSerializer::read;

};

class OutputFileSerializer : public TFileStreamSerializer< std::ofstream >
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
class IOFileSerializer : public TFileStreamSerializer< std::fstream >
{
public:
	bool open(char const* path, bool bRemoveOldData = false);
	virtual void read(void* ptr, size_t num) override;
	virtual void write(void const* ptr, size_t num) override;
	size_t getSize();
};
#endif



#endif // FileStream_H_FD26BF8C_47A3_4547_AD8F_E03F1D9EC3D6
