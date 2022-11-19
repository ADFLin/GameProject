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
	struct CharImageDesc
	{
		int    width;
		int    height;
		float  kerning;
		float  advance;
	};

	struct CharImageData : CharImageDesc
	{
		int    imageWidth;
		int    imageHeight;
		int    pixelSize;
		std::vector<uint8> imageData;
	};


	struct FontFaceInfo
	{
		FontFaceInfo() {}
		FontFaceInfo(HashString inName, int inSize, bool inbBold)
			:name(inName), size(inSize), bBold(inbBold)
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
			cachedHash = HashValue(fontFace.name);
			cachedHash = HashCombine(cachedHash, fontFace.size);
			cachedHash = HashCombine(cachedHash, fontFace.bBold);
			cachedHash = HashCombine(cachedHash, fontFace.bUnderLine);

		}

		bool operator == (FontFaceKey const& rhs) const
		{
			return fontFace == rhs.fontFace;
		}

		uint32 getTypeHash() const
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
		virtual bool getCharData(uint32 charWord, CharImageData& outData) = 0;
		virtual bool getCharDesc(uint32 charWord, CharImageDesc& outDesc) = 0;
		virtual int  getFontHeight() const = 0;
		virtual void getKerningPairs(std::unordered_map< uint32 , float >& outKerningMap ) const = 0;

		static ICharDataProvider* Create(FontFaceInfo const& fontFace);
	};


	class CharDataSet
	{
	public:

		struct CharDesc
		{
			int     width;
			int     height;
			float   kerning;
			float   advance;
		};

		struct CharData : CharDesc
		{
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
		CharDesc const& getCharDesc(uint32 charWord);
		int getFontHeight() const
		{
			return mFontHeight;
		}

		bool getKerningPair(uint32 charA, uint32 charB, float& outKerning)
		{
			uint32 key = (uint32(charA) << 16) | uint32(charB);
			auto iter = mKerningPairMap.find(key);
			if (iter == mKerningPairMap.end())
				return false;

			outKerning = iter->second;
			return true;
		}
		int mFontHeight;
		std::unordered_map< uint32, float > mKerningPairMap;
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

	struct FontVertex
	{
		Vector2 pos;
		Vector2 uv;
	};

	class FontDrawer
	{
	public:
		FontDrawer();
		~FontDrawer();

		bool initialize(FontFaceInfo const& fontFace);
		bool isValid() const { return mCharDataSet != nullptr; }
		void cleanup();
		void generateVertices(Vector2 const& pos, char const* str, std::vector< FontVertex >& outVertices, Vector2* outBoundSize = nullptr);
		void generateVertices(Vector2 const& pos, wchar_t const* str, std::vector< FontVertex >& outVertices, Vector2* outBoundSize = nullptr);

		void draw(RHICommandList& commandList, Vector2 const& pos, Matrix4 const& transform, LinearColor const& color, char const* str);
		void draw(RHICommandList& commandList, Vector2 const& pos, Matrix4 const& transform, LinearColor const& color, wchar_t const* str);
		void draw(RHICommandList& commandList, Matrix4 const& transform, LinearColor const& color, std::vector< FontVertex > const& buffer);

		int  getFontHeight() const { return mCharDataSet->getFontHeight(); }
		Vector2 calcTextExtent(wchar_t const* str);

		Vector2 calcTextExtent(char const* str);
		RHITexture2D& getTexture()
		{
			return mCharDataSet->getTexture();
		}

	private:
		void drawImpl(RHICommandList& commandList, Vector2 const& pos, Matrix4 const& transform, LinearColor const& color, wchar_t const* str);
	
	public:
		CharDataSet* mCharDataSet;
	};


}





#endif // Font_H_9A126205_99D2_45E0_8375_78A1FA3950E3