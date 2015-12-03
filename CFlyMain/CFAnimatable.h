#ifndef CFAnimatable_h__
#define CFAnimatable_h__

namespace CFly
{

	class IPoseBlend
	{

	};

	class Lerp1DBlend : public IPoseBlend
	{

	};
	class Lerp2DBlend : public IPoseBlend
	{

	};

	enum KeyFrameFormat
	{
		KEF_QPS ,
		KFF_QP ,
		KFF_Q  ,
		KFF_QS ,
		KEY_PS ,
	};


	struct MotionKeyFrame
	{
		Quaternion rotation;
		Vector3    pos;
	};

	class IAnimatable
	{
	public:
		virtual void updateAnimation( float dt ) = 0;
	};

}//namespace CFly

#endif // CFAnimatable_h__