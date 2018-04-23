#include "ColorMap.h"

#include <cassert>

ColorMap::ColorMap(int iSize):
	m_iRotion(0),
	m_iMapSize(iSize),
	m_iNumCPList(0),
	m_bSmoothLine(false)
{
	m_parColorMap[0]=new BYTE[m_iMapSize];
	m_parColorMap[1]=new BYTE[m_iMapSize];
	m_parColorMap[2]=new BYTE[m_iMapSize];

	memset(m_parColorMap[0],0,m_iMapSize);
	memset(m_parColorMap[1],0,m_iMapSize);

	memset(m_arMemPoolUsed,0,MaxCPSize);
}

ColorMap::~ColorMap()
{
	delete[] m_parColorMap[0];
	delete[] m_parColorMap[1];
	delete[] m_parColorMap[2];
}


bool ColorMap::addColorPoint(int iPos,BYTE R,BYTE G,BYTE B)
{
	if(m_iNumCPList>=MaxCPSize) return false;
	int index=-1;
	for(int i=0;i<MaxCPSize;++i)
	{
		if(!m_arMemPoolUsed[i])
		{
			index=i;
			break;
		}
	}
	if (index==-1) return false;
	
	m_arCPMemPool[index].R=R;
	m_arCPMemPool[index].G=G;
	m_arCPMemPool[index].B=B;
	m_arCPMemPool[index].pos=iPos;
	m_arpCPlist[index]=&m_arCPMemPool[index];
	m_arMemPoolUsed[index]=true;
	
	++m_iNumCPList;
	sortCPList();
	return true;
}

bool ColorMap::addColorPoint(int iPos)
{
	if(m_iNumCPList>=MaxCPSize) return false;
	int index=-1;
	for(int i=0;i<MaxCPSize;++i)
	{
		if(!m_arMemPoolUsed[i])
		{
			index=i;
			break;
		}
	}
	if (index==-1) return false;

	m_arCPMemPool[index].R=m_parColorMap[0][iPos];
	m_arCPMemPool[index].G=m_parColorMap[1][iPos];
	m_arCPMemPool[index].B=m_parColorMap[2][iPos];
	m_arCPMemPool[index].pos=iPos;
	m_arpCPlist[index]=&m_arCPMemPool[index];
	m_arMemPoolUsed[index]=true;

	++m_iNumCPList;
	sortCPList();
	return true;
}

bool ColorMap::deleteColorPoint(int index)
{
	if(m_iNumCPList <= 2) return false;
	if(index < 0 || index >= m_iNumCPList) return false;

	m_arpCPlist[index]=m_arpCPlist[m_iNumCPList-1];
	m_arMemPoolUsed[index]=false;
	--m_iNumCPList;
	sortCPList();

	return true;
}

void ColorMap::sortCPList()
{
	for(int i=0;i<m_iNumCPList-1;++i)
	{
		int min=i;
		for(int j=i+1;j<m_iNumCPList;++j)
		{
			if (m_arpCPlist[min]->pos > m_arpCPlist[j]->pos)
				min=j;
		}
		if( min!=i )
		{
			ColorPoint* temp=m_arpCPlist[i];
			m_arpCPlist[i]=m_arpCPlist[min];
			m_arpCPlist[min]=temp;
		}
	}
}

void ColorMap::getColor(float fVal,BYTE arColor[])
{
	int rotion=m_iRotion*m_iMapSize/CPPosRange();
	int index=(int(fVal*m_iMapSize)-rotion)%m_iMapSize;
	if (index<0) index+=m_iMapSize;

	arColor[0]=m_parColorMap[0][index];
	arColor[1]=m_parColorMap[1][index];
	arColor[2]=m_parColorMap[2][index];
}



void ColorMap::setColorPointPos(int index,int Pos)
{
	if(index<0 || index>=m_iNumCPList) return;
	m_arpCPlist[index]->pos=Pos-getRotion();
	if (m_arpCPlist[index]->pos<0)
		m_arpCPlist[index]->pos+=CPPosRange();
	sortCPList();
	computeColorMap();
}

void ColorMap::setColorPointPos(ColorPoint* pCP,int Pos)
{
	assert( pCP != NULL);

	pCP->pos=Pos-getRotion();
	if (pCP->pos<0) pCP->pos+=CPPosRange();
	sortCPList();
	computeColorMap();
}

void ColorMap::setColorPointColor(ColorPoint* pCP,int color,BYTE val)
{
	assert( pCP != NULL);

	pCP->setColor(color,val);
	sortCPList();
	computeColorMap();
}

void ColorMap::setColorPointColor(int index,int color,BYTE val)
{
	if(index<0 || index>=m_iNumCPList) return;
	m_arpCPlist[index]->setColor(color,val);
	sortCPList();
	computeColorMap();
}

int ColorMap::getCPIndex(ColorPoint* pCP)
{
	for(int i=0;i<getCPListSize();++i)
	{
		if (pCP==m_arpCPlist[i])
			return i;
	}
	return -1;
}
void ColorMap::computeColorMap_Smooth()
{
	//#FIXME
#if 0
	double arPos[4];
	double arColor[3][4];
	double* y=new double[m_iMapSize];
	CRSpline2D spline;
	for (int color=0;color<3;++color)
	{
		for(int n=0;n<m_iNumCPList+3;++n)
		{
			for(int i=0;i<4;++i)
			{
				int index=(i+n-2+m_iNumCPList)%m_iNumCPList;
				arPos[i]=(double)m_arpCPlist[index]->pos;
				arColor[0][i]=(double)m_arpCPlist[index]->R;
				arColor[1][i]=(double)m_arpCPlist[index]->G;
				arColor[2][i]=(double)m_arpCPlist[index]->B;
				if( (i+n-2)<0 ) arPos[i]-=ColorMap::CPPosRange();
				if( (i+n-2)>=m_iNumCPList) arPos[i]+=ColorMap::CPPosRange();
			}		
			int x1=arPos[1]*m_iMapSize/ColorMap::CPPosRange();
			int x2=arPos[2]*m_iMapSize/ColorMap::CPPosRange();
			int size=(arPos[2]-arPos[1])*m_iMapSize/ColorMap::CPPosRange();
			spline.create(arPos,&arColor[color][0],4);
			spline.compute_y(size,NULL,y,arPos[1],arPos[2]);
			for(int j=0;j<size;++j)
			{
				int val=(int)y[j];
				if (val>255) val=255;
				if (val<0) val=0;
				m_parColorMap[color][(x1+j+m_iMapSize)%m_iMapSize]=val;
			}
		}
	}
	delete[] y;
#endif
}
void ColorMap::computeColorMap()
{
	if (m_bSmoothLine)
	{
		computeColorMap_Smooth();
		return;
	}
	for (int n=0;n<m_iNumCPList;++n)
	{
		int now=n%m_iNumCPList;
		int next=(n+1)%m_iNumCPList;
		int start=m_arpCPlist[now]->pos*m_iMapSize/CPPosRange();
		int end =m_arpCPlist[next]->pos*m_iMapSize/CPPosRange();
		int dL=end-start;
		dL=(dL>0)?dL:(m_iMapSize+dL);

		float dR=(float)(m_arpCPlist[next]->R-m_arpCPlist[now]->R)/dL;
		float dG=(float)(m_arpCPlist[next]->G-m_arpCPlist[now]->G)/dL;
		float dB=(float)(m_arpCPlist[next]->B-m_arpCPlist[now]->B)/dL;
		float R=(float)m_arpCPlist[now]->R;
		float G=(float)m_arpCPlist[now]->G;
		float B=(float)m_arpCPlist[now]->B;
		for (int i=0;i<dL;++i)
		{
			int index=(start+i)%m_iMapSize;
	
			m_parColorMap[0][index]=(BYTE)R;		
			m_parColorMap[1][index]=(BYTE)G;
			m_parColorMap[2][index]=(BYTE)B;

			R+=dR;
			G+=dG;
			B+=dB;
		}
	}
}

