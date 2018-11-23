#ifndef CmtLightTrace_h__
#define CmtLightTrace_h__

#include "CmtBase.h"

namespace Chromatron
{
	class LightTrace
	{
	public:
		LightTrace( Vec2i const& start , Color color , 
			        Dir dir ,int param = 0 , int age = 0 );

		void         advance(){  ++mAge; mEndPos += GetDirOffset( mDir );  }

		void         setColor( Color color ){ mColor = color; }
		Color        getColor()    const { return mColor; }
		int          getParam()    const { return mParam; }
		Dir   const& getDir()      const { return mDir; }
		Vec2i const& getStartPos() const { return mStartPos; }
		Vec2i const& getEndPos()   const { return mEndPos; }
		int          getAge()      const { return mAge; }

		static Vec2i GetDirOffset( Dir dir );

	private:
		Vec2i mStartPos;
		Vec2i mEndPos;
		Color mColor;
		Dir   mDir;
		int   mParam;
		int   mAge;
	};

}//namespace Chromatron

#endif // CmtLightTrace_h__
