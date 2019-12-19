#pragma once
#ifndef Font_H_9A126205_99D2_45E0_8375_78A1FA3950E3
#define Font_H_9A126205_99D2_45E0_8375_78A1FA3950E3

#include "RHI/RHICommand.h"
#include "RHI/TextureAtlas.h"
#include "RHI/DrawUtility.h"

#include "Singleton.h"
#include "HashString.h"
#include "Core/TypeHash.h"
#include "CoreShare.h"

#include <unordered_map>



namespace Render
{
	struct CharImageData
	{
		int    width;
		int    height;
		float  kerning;
		float  advance;
		int    imageWidth;
		int    imageHeight;
		int    pixelSize;
		std::vector<uint8> imageData;
	};


	struct FontFaceInfo
	{
		FontFaceInfo() {}
		FontFaceInfo(char const* name, int size, bool bBold)
			:name(name), size(size), bBold(bBold)
		{
		}

		bool operator == (FontFaceInfo const& rhs) const
		{
			return name == rhs.name &&
				   size == rhs.size &&
				   bBold == rhs.bBold &&
				   bUnderLine == rhs.bUnderLine;
		}

		HashString name;
		int  size;
		bool bBold = false;
		bool bUnderLine = false;
	};



	struct FontFaceKey
	{
		FontFaceKey(FontFaceInfo const& fontFace)
			:fontFace(fontFace)
		{
			cachedHash = hash_value(fontFace.name);
			HashCombine(cachedHash, fontFace.size);
			HashCombine(cachedHash, fontFace.bBold);
			HashCombine(cachedHash, fontFace.bUnderLine);

		}

		bool operator == (FontFaceKey const& rhs) const
		{
			return fontFace == rhs.fontFace;
		}

		uint32 getHash() const
		{
			return cachedHash;
		}

	private:
		FontFaceInfo   fontFace;
		uint32         cachedHash;
	};

	class ICharDataProvider
	{
	public:
		~ICharDataProvider() {}
		virtual bool getCharData(uint32 charWord, CharImageData& data) = 0;
		virtual int  getFontHeight() const = 0;

		static ICharDataProvider* Create(FontFaceInfo const& fontFace);
	};


	class CharDataSet
	{
	public:
		struct CharData
		{
			int     width;
			int     height;
			float   kerning;
			float   advance;
			int     atlasId;
			Vector2 uvMin;
			Vector2 uvMax;
		};

		bool initialize(FontFaceInfo const& font);
		void clearRHIResource();
		RHITexture2D& getTexture()
		{
			return mUsedTextAtlas->getTexture();
		}
		CharData const& findOrAddChar(uint32 charWord);
		int getFontHeight() const
		{
			return mFontHeight;
		}
		int mFontHeight;
		ICharDataProvider* mProvider;
		TextureAtlas* mUsedTextAtlas;
		std::unordered_map< uint32, CharData > mCharMap;

	};



	class FontCharCache
	{
	public:
		static CORE_API FontCharCache& Get();

		CORE_API bool initialize();
		CORE_API void finalize();
		CORE_API void releaseRHI();
		CORE_API CharDataSet* getCharDataSet(FontFaceInfo const& fontFace);

		TextureAtlas mTextAtlas;
		std::unordered_map< FontFaceKey , CharDataSet* , MemberFuncHasher > mCharDataSetMap;
		bool bInitialized = false;
#if SYS_PLATFORM_WIN
		HDC hDC;
#endif
	};

	class FontDrawer
	{
	public:
		FontDrawer();
		~FontDrawer();

		bool initialize(FontFaceInfo const& fontFace);
		bool isValid() const { return mCharDataSet != nullptr; }
		void cleanup();
		void draw(RHICommandList& commandList, Vector2 const& pos, char const* str);
		void draw(RHICommandList& commandList, Vector2 const& pos, wchar_t const* str);
		int  getSize() const { return mSize; }
		int  getFontHeight() const { return mCharDataSet->getFontHeight(); }
		Vector2 calcTextExtent(wchar_t const* str);
		Vector2 calcTextExtent(char const* str);
	private:
		void drawImpl(RHICommandList& commandList, Vector2 const& pos, wchar_t const* str);
		CharDataSet* mCharDataSet;
		struct TextVertex
		{
			Vector2 pos;
			Vector2 uv;
		};
		std::vector< TextVertex > mBuffer;
		void addQuad(Vector2 const& pos, Vector2 const& size, Vector2 const& uvMin, Vector2 const& uvMax);
		int     mSize;
	};

}





#endif // Font_H_9A126205_99D2_45E0_8375_78A1FA3950E3