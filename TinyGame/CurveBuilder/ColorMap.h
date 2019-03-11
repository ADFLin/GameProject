#ifndef ColorMap_H
#define ColorMap_H

#include "Color.h"

class ColorMap;
typedef unsigned char uint8;

enum
{
	COLOR_R = 0,
	COLOR_G,
	COLOR_B,
};

struct ColorPoint
{
	Color3f color;
	int     pos;
};

class ColorMap
{
public:
	ColorMap(int iSize);
	ColorMap(ColorMap& CMap);
	~ColorMap();

	void addPoint(int iPos);
	void addPoint(int iPos, Color3f const& color);
	bool removePoint(int index);

	void calcColorMap( bool beForce = false );

	void setCPPos(int index,int Pos);
	void setCPColor(int index,int idxComp,float val);


	bool isSmoothLine(){ return m_bSmoothLine; }
	void setSmoothLine(bool isSmooth=true){ m_bSmoothLine=isSmooth; }
	ColorPoint& getColorPoint(int index){ return mSortedCPoints[index]; }

	void  getColor(float fVal, Color3f& outColor) const;
	int   getRotion() const { return m_iRotion; }
	void  setRotion(int iRotion){ m_iRotion=iRotion; }

	int getCPListSize() const { return (int)mSortedCPoints.size(); }
	int getMapSize() const { return m_iMapSize; }


	static int const CPPosRange = 1000;

private:
	void sortCPList();
	float getSizePosRatio() const { return float(m_iMapSize)/CPPosRange; }
	void computeColorMapSmooth();
	void computeColorMapLinear();


	bool    m_bSmoothLine;
	bool    m_isDirty;
	int     m_iRotion;
	int     m_iMapSize;

	std::vector< ColorPoint > mSortedCPoints;
	std::vector< Color3f > mColorData;
	
};


#endif //ColorMap_H