
class AssetSerializer
{




};

class AssetObject
{
public:
	virtual void serialize(AssetSerializer& ar) = 0;
	virtual void postLoad(){}
};

class AssetPtrBase
{
public:
	bool isValid() const
	{		
		return !!mPtr;
	}



	AssetObject* get() { return mPtr; }

	AssetObject* mPtr;
};


template < typename T >
class TAssetPtr : public AssetPtrBase
{
public:

	void StaticAssert()
	{
		static_assert(std::is_base_of_v<T, AssetObject>);
	}
	T* operator->()
	{
		return static_cast<T*>(get());
	}

	T* get()
	{
		return static_cast<T*>(AssetPtrBase::get());
	}
};


class FAsset
{
public:

	static AssetObject* Load(char const* path)
	{



	}




};
