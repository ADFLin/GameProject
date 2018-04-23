#pragma once

#include <cmath>
#include <memory.h>

#include "CRSpline.h"

#define COLOR_R 0
#define COLOR_G 1
#define COLOR_B 2

struct ColorPoint
{
	union
	{
		struct
		{
			BYTE R,G,B;
		};
		BYTE RGB[3];
	};
	
	int pos;
	BYTE getColor(int color)
	{
		return RGB[ color ];
	}
	void setColor(int color,BYTE val)
	{
		RGB[color] = val;
	}
};

class ColorMap
{
public:
	ColorMap(int iSize);
	~ColorMap();

	bool addColorPoint(int iPos);
	bool addColorPoint(int iPos,BYTE R,BYTE G,BYTE B);
	bool deleteColorPoint(int index);
	void computeColorMap_Smooth();
	void computeColorMap();
	void sortCPList();

	void setColorPointPos(int index,int Pos);
	void setColorPointPos(ColorPoint* pCP,int Pos);
	void setColorPointColor(int index,int color,BYTE val);
	void setColorPointColor(ColorPoint* pCP,int color,BYTE val);
	

	bool isSmoothLine(){ return m_bSmoothLine; }
	void setSmoothLine(bool isSmooth=true){ m_bSmoothLine=isSmooth; }
	ColorPoint& getColorPoint(int index){ return *m_arpCPlist[index]; }
	int   getCPIndex(ColorPoint* pCP);
	BYTE* getColorMap(int index){ return m_parColorMap[index]; }	
	void  getColor(float fVal,BYTE arColor[]);
	int   getRotion() { return m_iRotion; }
	void  setRotion(int iRotion){ m_iRotion=iRotion; }

	int   getCPListSize(){ return m_iNumCPList; }
	int   getMapSize(){ return m_iMapSize; }

	static int CPPosRange(){ return 1000; }

private:
	static const int MaxCPSize=20;
	int m_iRotion;
	int m_iMapSize;
	BYTE* m_parColorMap[3];

	bool m_bSmoothLine;
	
	int m_iNumCPList;
	bool        m_arMemPoolUsed[MaxCPSize];
	ColorPoint  m_arCPMemPool[MaxCPSize];
	ColorPoint* m_arpCPlist[MaxCPSize];
};
