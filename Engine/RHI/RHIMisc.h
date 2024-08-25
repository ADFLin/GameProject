#pragma once
#ifndef RHIMisc_H_24D3F3E9_D037_4086_BA7E_40EC7C523D3F
#define RHIMisc_H_24D3F3E9_D037_4086_BA7E_40EC7C523D3F

#include "RHIDefine.h"

namespace Render
{
	template< typename TBuffer >
	bool DetermitPrimitiveTopologyUP(EPrimitive primitive, bool bPrimitiveSupported, int num, uint32 const* pIndices, TBuffer& buffer, EPrimitive& outPrimitiveDetermited, int& outIndexNum)
	{
		if (bPrimitiveSupported)
		{
			if (pIndices)
			{
				uint32 indexBufferSize = num * sizeof(uint32);
				void* pIndexBufferData = buffer.lock(indexBufferSize);
				if (pIndexBufferData == nullptr)
					return false;

				FMemory::Copy(pIndexBufferData, pIndices, indexBufferSize);
				buffer.unlock();
				outIndexNum = num;
			}

			outPrimitiveDetermited = primitive;
			return true;
		}

		switch (primitive)
		{
		case EPrimitive::Quad:
		{
			int numQuad = num / 4;
			int indexBufferSize = sizeof(uint32) * numQuad * 6;
			void* pIndexBufferData = buffer.lock(indexBufferSize);
			if (pIndexBufferData == nullptr)
				return false;

			uint32* pData = (uint32*)pIndexBufferData;
			if (pIndices)
			{
				for (int i = 0; i < numQuad; ++i)
				{
					pData[0] = pIndices[0];
					pData[1] = pIndices[1];
					pData[2] = pIndices[2];
					pData[3] = pIndices[0];
					pData[4] = pIndices[2];
					pData[5] = pIndices[3];
					pData += 6;
					pIndices += 4;
				}
			}
			else
			{
				for (int i = 0; i < numQuad; ++i)
				{
					int index = 4 * i;
					pData[0] = index + 0;
					pData[1] = index + 1;
					pData[2] = index + 2;
					pData[3] = index + 0;
					pData[4] = index + 2;
					pData[5] = index + 3;
					pData += 6;
				}
			}
			outPrimitiveDetermited = EPrimitive::TriangleList;
			buffer.unlock();
			outIndexNum = numQuad * 6;
			return true;
		}
		break;
		case EPrimitive::LineLoop:
		{
			int indexBufferSize = sizeof(uint32) * (num + 1);
			void* pIndexBufferData = buffer.lock(indexBufferSize);
			if (pIndexBufferData == nullptr)
				return false;
			uint32* pData = (uint32*)pIndexBufferData;

			if (pIndices)
			{
				for (int i = 0; i < num; ++i)
				{
					pData[i] = pIndices[i];
				}
				pData[num] = pIndices[0];
			}
			else
			{
				for (int i = 0; i < num; ++i)
				{
					pData[i] = i;
				}
				pData[num] = 0;
			}

			buffer.unlock();
			outIndexNum = num + 1;
			outPrimitiveDetermited = EPrimitive::LineStrip;
			return true;
		}
		break;
		case EPrimitive::Polygon:
		{
			if (num <= 2)
				return false;

			int numTriangle = (num - 2);

			int indexBufferSize = sizeof(uint32) * numTriangle * 3;
			void* pIndexBufferData = buffer.lock(indexBufferSize);
			if (pIndexBufferData == nullptr)
				return false;

			uint32* pData = (uint32*)pIndexBufferData;
			if (pIndices)
			{
				for (int i = 0; i < numTriangle; ++i)
				{
					pData[0] = pIndices[0];
					pData[1] = pIndices[i + 1];
					pData[2] = pIndices[i + 2];
					pData += 3;
				}
			}
			else
			{
				for (int i = 0; i < numTriangle; ++i)
				{
					pData[0] = 0;
					pData[1] = i + 1;
					pData[2] = i + 2;
					pData += 3;
				}
			}
			outPrimitiveDetermited = EPrimitive::TriangleList;
			buffer.unlock();
			outIndexNum = numTriangle * 3;
			return true;
		}
		break;
		}

		return false;
	}
}

#endif // RHIMisc_H_24D3F3E9_D037_4086_BA7E_40EC7C523D3F
