#pragma once
#ifndef Tween_H_50802EDF_8D8D_47C6_B3C4_E1EB66015C6C
#define Tween_H_50802EDF_8D8D_47C6_B3C4_E1EB66015C6C

#include "Meta/MetaBase.h"
#include "Meta/Select.h"
#include "DataStructure/Array.h"

#include <functional>
#include <algorithm>

namespace Tween
{
	typedef std::function< void () > FinishCallback;
	typedef std::function< void () > StartCallback;

	class Access
	{
		class ValueType;
		class DataType;
		void operator()( DataType& data , ValueType const& value );
	};

	template< class TimeType , template< class > class CallbackPolicy > class GroupTweener;
	template< class TimeType , template< class > class CallbackPolicy > class SquenceTweener;

	class EmptyBuilder { };

	template< class T >
	struct ValueAccess
	{
		typedef T ValueType;
		typedef T DataType;
		void operator()( DataType& data , ValueType const& value ){ data = value; }
	};

	class NullParam 
	{
	public:
		template< class TFunc , class T >
		T operator()( float t, T const& b, T const& c, float const& d )
		{
			return TFunc()( t , b , c , d );
		}
	};

	template< class Access , class FuncParam = NullParam , bool bHoldRef = true >
	struct TPropData : public FuncParam
	{
		using PropData = TPropData< Access, FuncParam, bHoldRef >;
		using ValueType = typename Access::ValueType;
		using DataType  = typename Access::DataType;

		TPropData( DataType& data , ValueType const& from , ValueType const& to )
			:mData( data )
			,mFrom( from )
			,mDiff( to - from )
		{

		}

		TPropData( DataType& data , ValueType const& from , ValueType const& to , FuncParam const& param )
			:FuncParam( param )
			,mData( data )
			,mFrom( from )
			,mDiff( to - from )
		{

		}

		template< class TFunc , class TimeType >
		void update( TimeType t , TimeType duration )
		{
			Access()( mData  , FuncParam::operator()< TFunc >( t , mFrom , mDiff , duration ) );
		}

		using HoldType = typename TSelect< bHoldRef , DataType& , DataType >::Type;
		HoldType  mData;
		ValueType mFrom;
		ValueType mDiff;
	};


	template< class TimeType >
	class IComponentT
	{
	public:
		virtual ~IComponentT(){}
		virtual TimeType update( TimeType time ) = 0;
		virtual void     modify( TimeType dt ) = 0;
		//virtual void     stop( bool goEnd ) = 0;
		virtual bool     isFinished() = 0;
		virtual void     reset() = 0;
	};


	
	template< class T >
	class CallbackPolicy
	{
		void finishCB();
	};


	template< class T >
	class StdFunCallback 
	{
		T*   _this(){ return static_cast< T* >( this ); }
	public:
		T&   finishCallback( FinishCallback func ){ mFinishFunc = func; return *_this(); }
	protected:
		void finishCB(){ if ( mFinishFunc ) mFinishFunc(); }
	private:
		FinishCallback mFinishFunc;
	};

	template< class T >
	class NoCallback 
	{
		void finishCB(){}
	};

	template< class TIME , template< class > class CallbackPolicy = StdFunCallback > 
	class Detail
	{
	public:
		typedef TIME TimeType;

		typedef IComponentT< TimeType > IComponent;

		template< class T >
		class TweenBaseT : public CallbackPolicy< T >
		{
			T* _this(){ return static_cast< T* >( this ); }

			typedef CallbackPolicy< T > CBP;
		public:

			TweenBaseT( TimeType duration , TimeType delay = 0 )
				:mDuration( duration )
				,mCurTime( -delay )
				,mDelay( delay )
			{
				mRepeatTime = mDuration;
				mRepeat     = 0;
				mTotalRepeat= 0;
			}

			void     doReset()
			{
				mRepeat  = mTotalRepeat;
				mCurTime = -mDelay;
			}

			void     doModify( TimeType dt )
			{
				if ( isFinished() )
					return;
				TimeType time = mCurTime + dt;
				if ( time < mDuration )
				{
					_this()->assignValue( time );
				}
				else
				{
					_this()->assignValue( mDuration );
				}
			}

			//  delay    duration    repeat delay   duration  repeat delay
			//xxxxxxxxx|===========|-------------|===========|-------------|===========|----->

			TimeType doUpdate( TimeType time )
			{
				if ( isFinished() )
					return TimeType( 0 );

				mCurTime += time;
				TimeType result = time; 

				for ( ;; )
				{
					if ( mCurTime < 0 )
						break;

					if ( mCurTime < mDuration )
					{
						_this()->assignValue( mCurTime );
						break;
					}
					else if ( mRepeat != 0 )
					{
						if ( mRepeat > 0)
							mRepeat -= 1;
						mCurTime -= mRepeatTime;
					}
					else // mCurTime >= mDuration
					{
						_this()->assignValue( mDuration );
						CBP::finishCB();
						return time - ( mDuration - mCurTime );
					}
				}
				return time;
			}

			void assignValue( TimeType t ){}


			bool  isFinished() const           { return mRepeat == 0 && mCurTime >= mDuration; }
			//T&    finishCallback( FinishCallback func ){ mFinishFunc = func; return *_this(); }
			T&    repeat( int num )            { mTotalRepeat = mRepeat = num; return *_this(); }
			T&    delay( TimeType time )       { mDelay = time ; mCurTime = -time; return *_this(); }
			T&    repeatDelay( TimeType time ) { mRepeatTime = mDuration + time; return *_this(); }
			T&    cycle()                      { mTotalRepeat = mRepeat = -1; return *_this(); }

		protected:
			//FinishCallback mFinishFunc;

			int       mTotalRepeat;
			int       mRepeat;
			TimeType  mDelay;
			TimeType  mCurTime;
			TimeType  mDuration;
			TimeType  mRepeatTime;
		};

		template< class Impl , class PropData , class TFunc >
		class TweenCoreT : public TweenBaseT< Impl >
			             , private PropData
		{
			Impl* _this(){ return static_cast< Impl* >( this ); }

		public:
			typedef TweenCoreT< Impl , TFunc, PropData > TweenCore;
			typedef TweenBaseT< Impl > TweenBase;


			template< class DataType , class ValueType >
			TweenCoreT( DataType& data , ValueType const& from , ValueType const& to , TimeType duration , TimeType delay = 0 )
				:TweenBase( duration , delay )
				,PropData( data , from , to )
			{

			}

			template< class DataType , class ValueType , class FuncParam >
			TweenCoreT( DataType& data , ValueType const& from , ValueType const& to , TimeType duration , TimeType delay , FuncParam const& param )
				:TweenBase( duration , delay )
				,PropData( data , from , to , param )
			{

			}

			void assignValue( TimeType t )
			{
				PropData::update< TFunc >( t , mDuration );
			}
		};

		template< class Impl >
		class MultiTweenCoreT : public TweenBaseT< Impl >
		{
			Impl* _this(){ return static_cast< Impl* >( this ); }
		public:
			typedef MultiTweenCoreT< Impl > MultiTweenCore;
			typedef TweenBaseT< Impl > TweenBase;

		public:

			MultiTweenCoreT( TimeType duration , TimeType delay = 0 )
				:TweenBase( duration , delay )
			{

			}

			~MultiTweenCoreT()
			{
				for( PropValueVec::iterator iter = mProps.begin() , end = mProps.end() ;
					iter != end ; ++iter )
				{
					IPropValue* pv = *iter;
					delete pv;
				}
			}

			struct IPropValue
			{
				virtual ~IPropValue(){}
				virtual void update( TimeType t , TimeType duration ) = 0;
			};

			template< class TFunc , class PropData >
			struct CPropValue : public IPropValue
				, private PropData
			{
				template< class DataType , class ValueType>
				CPropValue( DataType& data , ValueType const& from , ValueType const& to )
					:PropData( data , from , to ){}

				template< class DataType , class ValueType , class FuncParam >
				CPropValue( DataType& data , ValueType const& from , ValueType const& to , FuncParam const& param )
					:PropData( data , from , to , param ){}

				void update( TimeType t , TimeType duration )
				{
					PropData::update< TFunc >( t , duration );
				}
			};

			template< class TFunc , class T >
			Impl& addValue( T& data , T const& from , T const& to )
			{  
				typedef CPropValue< TFunc , TPropData< ValueAccess< T > >  > MyProp;
				MyProp* prop = new MyProp( data , from , to );
				mProps.push_back( prop );
				return *_this();
			}


			template< class TFunc , class T , class FuncParam >
			Impl& addValue( T& data , T const& from , T const& to , FuncParam const& param )
			{  
				typedef CPropValue< TFunc , TPropData< ValueAccess< T > , FuncParam >  > MyProp;
				MyProp* prop = new MyProp( data , from , to , param );
				mProps.push_back( prop );
				return *_this();
			}

			template< class TFunc , class Access >
			Impl& add( typename Access::DataType& data , 
				typename Access::ValueType const& from , 
				typename Access::ValueType const& to )
			{  
				typedef CPropValue< TFunc , TPropData< Access >  > MyProp;
				MyProp* prop = new MyProp( data , from , to );
				mProps.push_back( prop );
				return *_this();
			}


			template< class TFunc , class Access , class FuncParam >
			Impl& add( typename Access::DataType& data , 
				typename Access::ValueType const& from , 
				typename Access::ValueType const& to ,
				FuncParam const& param )
			{  
				typedef CPropValue< TFunc , TPropData< Access , FuncParam >  > MyProp;
				MyProp* prop = new MyProp( data , from , to , param );
				mProps.push_back( prop );
				return *_this();
			}

			void assignValue(TimeType t )
			{
				for( PropValueVec::iterator iter = mProps.begin() , end = mProps.end() ;
					iter != end ; ++iter )
				{
					IPropValue* pv = *iter;
					pv->update( t , mDuration );
				}
			}

		private:
			typedef TArray< IPropValue* > PropValueVec;
			TArray< IPropValue* > mProps;
		};


		template< class TFunc , class Access , class FuncParam = NullParam >
		class CTween : public IComponent
			         , public TweenCoreT< CTween< TFunc , Access , FuncParam > ,  TPropData< Access, FuncParam > , TFunc >
		{
		public:
			using Core = TweenCoreT< CTween< TFunc, Access, FuncParam >, TPropData< Access, FuncParam>, TFunc >;

			using DataType = typename Access::DataType;
			using ValueType = typename Access::ValueType;

			CTween( DataType& data , ValueType const& from , ValueType const& to , TimeType duration , TimeType delay )
				:Core( data ,from , to , duration , delay ){}

			CTween( DataType& data , ValueType const& from , ValueType const& to , TimeType duration , TimeType delay , FuncParam const& param )
				:Core( data ,from , to , duration , delay , param ){}

			void     modify( TimeType dt ){ Core::doModify( dt ); }
			bool     isFinished(){ return Core::isFinished(); }
			TimeType update( TimeType time ){ return Core::doUpdate( time ); }
			void     reset(){ return Core::doReset(); }
		};

		class CMultiTween : public IComponent
			              , public MultiTweenCoreT< CMultiTween >
		{
		public:
			using Core = MultiTweenCoreT< CMultiTween >;

			CMultiTween( TimeType duration , TimeType delay )
				:Core( duration , delay ){}

			void     modify( TimeType dt ){ Core::doModify( dt ); }
			bool     isFinished(){ return Core::isFinished(); }
			TimeType update( TimeType time ){ return Core::doUpdate( time ); }
			void     reset(){ return Core::doReset(); }
		};


		template< class Impl , class Access >
		class SimpleTweenerT
		{
		public:
			Impl* _this(){ return static_cast< Impl* >( this ); }

			using DataType = typename Access::DataType;
			using ValueType = typename Access::ValueType;

			template< class TFunc , class T >
			CTween< TFunc , ValueAccess< T > >&
			 tween( DataType& data , ValueType const& from , ValueType const& to , TimeType duration , TimeType delay = 0 )
			{
				typedef CTween< TFunc , ValueAccess< T >  > MyTween;
				MyTween* t = new MyTween( data , from , to , duration , delay );
				_this()->addComponent( t );
				return *t;
			}

		};

		template< class Impl >
		class TweenerT : public IComponent
		{
			Impl* _this(){ return static_cast< Impl* >( this ); }

		public:
			template< class TFunc , class T >
			CTween< TFunc , ValueAccess< T > >&
				tweenValue( T& data , T const& from , T const& to , TimeType duration , TimeType delay = 0 )
			{
				typedef CTween< TFunc , ValueAccess< T >  > MyTween;
				return createAndAddComponent< MyTween >(data, from, to, duration, delay);
			}

			template< class TFunc , class T , class FuncParam >
			CTween< TFunc , ValueAccess< T > , FuncParam >&
				tweenValue( T& data , T const& from , T const& to  , TimeType duration , TimeType delay  , FuncParam const& param )
			{
				typedef CTween< TFunc , ValueAccess< T > , FuncParam  > MyTween;
				return createAndAddComponent< MyTween >(data, from, to, duration, delay , param);
			}

			template< class TFunc  , class Access >
			inline CTween< TFunc , Access >&
				tween( typename Access::DataType& data , 
				typename Access::ValueType const& from , 
				typename Access::ValueType const& to ,
				TimeType duration , TimeType delay = 0 )
			{
				typedef CTween< TFunc , Access > MyTween;
				return createAndAddComponent< MyTween >(data, from, to, duration, delay);
			}

			template< class TFunc , class Access , class FuncParam >
			inline CTween< TFunc , Access , FuncParam >&
				tween( typename Access::DataType& data , 
				typename Access::ValueType const& from , 
				typename Access::ValueType const& to , 
				TimeType duration , TimeType delay , FuncParam const& param )
			{
				typedef CTween< TFunc , Access , FuncParam > MyTween;
				return createAndAddComponent< MyTween >(data, from, to, duration, delay, param);
			}

			inline CMultiTween& tweenMulti(TimeType duration, TimeType delay = 0) { return createAndAddComponent< CMultiTween >(duration, delay); }

			typedef GroupTweener< TimeType , CallbackPolicy >   CGroupTweener;
			typedef SquenceTweener< TimeType , CallbackPolicy > CSquenceTweener;

			inline CGroupTweener& group() { return createAndAddComponent< CGroupTweener >(false); }
			inline CSquenceTweener& sequence(){  return createAndAddComponent< CSquenceTweener >();  }

		protected:
			void destroyComponent( IComponent* comp ){ delete comp; }

			template< class T , class ...Args > 
			T&   createAndAddComponent( Args&& ...args )
			{
				auto t = new T( std::forward< Args >( args )... );
				_this()->addComponent(t);
				return *t;
			}
			void addComponent( IComponent* comp );
		};


		typedef GroupTweener< TimeType , CallbackPolicy >   CGroupTweener;
		typedef SquenceTweener< TimeType , CallbackPolicy > CSquenceTweener;

		template< class P , class Q >
		class Builder;


		template< class Q > 
		struct StorageTraits
		{
			typedef Q& Type;
		};

		template< > 
		struct StorageTraits< EmptyBuilder >
		{
			typedef EmptyBuilder Type;
		};
		template< class P , class Q >
		struct StorageTraits< Builder< P , Q > >
		{
			typedef Builder< P , Q > Type;
		};

		template< class P , class Q >
		class Builder
		{
			typedef Builder< P , Q > ThisType;
		public:
			Builder( P& p , Q& q )
				:mPrev( p ) , mCur( q ){}

			inline P& end(){ return mPrev; }

			//Tween func
			inline ThisType& finishCallback( FinishCallback func ){ mCur.finishCallback( func ); return *this; }
			inline ThisType& repeat( int num )                   { mCur.repeat( num ); return *this; }
			inline ThisType& delay( TimeType time )              { mCur.delay( time ); return *this; }
			inline ThisType& repeatDelay( TimeType time )        { mCur.repeatDelay( time ); return *this; }
			inline ThisType& cycle()                             { mCur.cycle(); return *this; }

			//MultiTween
			template< class TFunc , class T >
			inline ThisType& addValue( T& data , T const& from , T const& to )
			{  return makeBuilder( mCur.addValue< TFunc >( data , from , to ) );  }


			template< class TFunc , class T , class FuncParam >
			inline ThisType& addValue( T& data , T const& from , T const& to , FuncParam const& param )
			{  return makeBuilder( mCur.addValue< TFunc >( data , from , to , param ) );  }

			template< class TFunc , class Access >
			inline ThisType& add( typename Access::DataType& data , 
				typename Access::ValueType const& from , 
				typename Access::ValueType const& to )
			{  return makeBuilder( mCur.add< TFunc , Access >( data , from , to ) );  }


			template< class TFunc , class Access , class FuncParam >
			inline ThisType& add( typename Access::DataType& data , 
				typename Access::ValueType const& from , 
				typename Access::ValueType const& to ,
				FuncParam const& param )
			{  return makeBuilder( mCur.add< TFunc , Access >( data , from , to , param ) );  }

			//Tweener func
			template< class TFunc , class T >
			inline Builder< ThisType , CTween< TFunc , ValueAccess< T > > >
				tweenValue( T& data , T const& from , T const& to , TimeType duration , TimeType delay = 0 )
			{  return makeBuilder( mCur.tweenValue< TFunc >( data , from , to , duration , delay ) );  }

			template< class TFunc , class T , class FuncParam >
			inline Builder< ThisType , CTween< TFunc , ValueAccess< T > , FuncParam > >
				tweenValue( T& data , T const& from , T const& to , TimeType duration , TimeType delay , FuncParam const& param )
			{  return makeBuilder( mCur.tweenValue< TFunc >( data , from , to , duration , delay , param ) );  }

			template< class TFunc  , class Access >
			inline Builder< ThisType , CTween< TFunc , Access > >
				tween( typename Access::DataType& data , 
				typename Access::ValueType const& from , 
				typename Access::ValueType const& to ,
				TimeType duration , TimeType delay = 0 )
			{  return makeBuilder( mCur.tween< TFunc , Access >( data , from , to , duration , delay ) );  }


			template< class TFunc , class Access , class FuncParam >
			inline Builder< ThisType , CTween< TFunc , Access , FuncParam > >
				tween(  typename Access::DataType& data , 
				typename Access::ValueType const& from , 
				typename Access::ValueType const& to , 
				TimeType duration , TimeType delay , FuncParam const& param )
			{  return makeBuilder( mCur.tween< TFunc , Access >( data , from , to , duration , delay , param ) );  }


			inline Builder< ThisType , CMultiTween > 
				tweenMulti( TimeType duration , TimeType delay = 0 )
			{
				return makeBuilder( mCur.tweenMulti( duration , delay ) );
			}

			inline Builder< ThisType , CGroupTweener >
				group(){  return makeBuilder( mCur.group() );  }

			inline Builder< ThisType , CSquenceTweener >
				sequence(){  return makeBuilder( mCur.sequence() );  }
		private:

			typename StorageTraits< Q >::Type mCur;
			typename StorageTraits< P >::Type mPrev;
		
			template< class U >
			Builder< ThisType , U > makeBuilder( U& u ){ return Builder< ThisType , U >( *this , u ); }
			template< class U , class V >
			Builder< ThisType , V > makeBuilder( Builder< U , V > const& b ){ return Builder< ThisType , V >( *this , b.mCur ); }
		};
	};

	template< class TimeType , template< class > class CallbackPolicy = StdFunCallback >
	class Define
	{
	public:
		//typedef typename Detail< TimeType >::IComponent IComponent;
	};

	template< class TimeType , template< class > class CallbackPolicy = StdFunCallback >
	class GroupTweener : public Detail< TimeType , CallbackPolicy >::template TweenerT< GroupTweener< TimeType , CallbackPolicy >  >
		               , public Define< TimeType , CallbackPolicy >
	{
		using ThisType = GroupTweener< TimeType , CallbackPolicy >;
	public:

		using CMultiTween = typename Detail< TimeType, CallbackPolicy >::CMultiTween;
		using IComponent = IComponentT< TimeType >;

		GroupTweener( bool beRM = true )
		{
			mAutoRemove = beRM;
		}
		~GroupTweener()
		{
			cleanup( true );
		}

	private:
		void modify( TimeType dt )
		{
			int size = (int)mActiveComps.size();
			for (IComponent* comp : mStorage)
			{
				comp->modify( dt );
			}
		}

	public:

		ThisType& autoRemove( bool beE ){ mAutoRemove = beE; return *this; }

		void remove( IComponent* comp )
		{
			{
				int index = mStorage.findIndex(comp);
				if (index == INDEX_NONE)
					return;

				mStorage.removeIndex(index);
				destroyComponent(comp);
			}
			{
				int index = mActiveComps.findIndex(comp);
				if (index == INDEX_NONE)
					return;
				mActiveComps.removeIndex(index);
			}
		}

		void reset()
		{
			mActiveComps.assign( mStorage.begin() , mStorage.end() );
			for (IComponent* comp : mStorage)
			{
				comp->reset();
			}
		}

		TimeType update( TimeType time )
		{
			int idx = 0;
			TimeType result = 0;
			for ( int i = 0; i < (int)mActiveComps.size() ; ++i )
			{
				IComponent* comp = mActiveComps[i];
				comp->update( time );
				if ( !comp->isFinished() )
				{
					if ( i != idx )
						mActiveComps[ idx ] = comp;

					++idx;
				}
				else if ( mAutoRemove )
				{
					mStorage.erase( std::find( mStorage.begin() , mStorage.end() , comp ) );
					destroyComponent( comp );
				}
			}

			if (idx != mActiveComps.size())
			{
				mActiveComps.resize(idx);
			}

			return 0;
		}

		void cleanup( bool bDestroy )
		{
			for (IComponent* comp : mStorage)
			{
				destroyComponent(comp);
			}

			if ( !bDestroy )
			{
				mStorage.clear();
				mActiveComps.clear();
			}
		}

		void clear(){ cleanup( false ); }

		bool isFinished(){ return getActiveNum() != 0; }
		void addComponent( IComponent* comp )
		{ 
			mStorage.push_back( comp ); 
			mActiveComps.push_back( comp ); 
		}
		int  getActiveNum() const{ return (int)mActiveComps.size(); }
		int  getTotalNum() const { return (int)mStorage.size(); }

		bool    mAutoRemove;
		typedef TArray< IComponent* > CompVec;
		CompVec mActiveComps;
		CompVec mStorage;
	};

	template< class TimeType , template< class > class CallbackPolicy = StdFunCallback >
	class SquenceTweener : public Detail< TimeType , CallbackPolicy >::template TweenerT< SquenceTweener< TimeType , CallbackPolicy > >
	{
		using ThisType = SquenceTweener< TimeType, CallbackPolicy >;
	public:
		using CMultiTween = typename Detail< TimeType, CallbackPolicy >::CMultiTween;
		using IComponent = IComponentT< TimeType >;

		SquenceTweener()
		{
			mNumFinished = 0;
			mTotalRepeat = 0;
			mRepeat   = 0;
		}
		~SquenceTweener()
		{
			cleanup( true );
		}

		ThisType& cycle()
		{
			mRepeat = mTotalRepeat = -1;
			return *this;
		}
		void reset()
		{
			for (IComponent* comp : mStorage)
			{
				comp->reset();
			}

			mRepeat = mTotalRepeat;
			mNumFinished = 0;
		}

		void remove( IComponent* comp )
		{
			int idx = mStorage.findIndex(comp);
			if ( idx == INDEX_NONE )
				return;

			destroyComponent( mStorage[idx] );
			mStorage.erase( mStorage[ idx ] );

			if ( idx < mNumFinished )
			{
				--mNumFinished;
			}
		}

		void modify( TimeType dt )
		{
			if ( mNumFinished < (int)mStorage.size() )
			{



			}
		}

		TimeType update( TimeType time )
		{
			TimeType totalUseTime = 0;
			while (  mNumFinished < (int)mStorage.size() )
			{
				IComponent* comp = mStorage[mNumFinished];
				TimeType useTime = comp->update( time );
				totalUseTime += useTime;

				if ( !comp->isFinished() )		
					break;
				time -= useTime;

				++mNumFinished;
				if ( mNumFinished == mStorage.size() )
				{
					if ( mRepeat == 0 )
						break;

					if ( mRepeat > 0 )
						mRepeat -= 1;

					reset();
				}
			}
			return totalUseTime;
		}

		void clear(){ cleanup( false ); }
		bool isFinished(){ return mNumFinished == (int)mStorage.size(); }


		void addComponent( IComponent* comp )
		{ 
			mStorage.push_back( comp ); 
		}

		void cleanup( bool bDestroy )
		{
			for (IComponent* comp : mStorage)
			{
				destroyComponent(comp);
			}
			if ( !bDestroy )
			{
				mStorage.clear();
				mNumFinished = 0;
			}
		}

		int mNumFinished;
		int mTotalRepeat;
		int mRepeat;
		typedef TArray< IComponent* > CompVec;
		CompVec mStorage;
	};

	template< class T , class TimeType , template< class > class CallbackPolicy >
	class Builder: public Detail< TimeType , CallbackPolicy >::template Builder< EmptyBuilder, T >
	{
	public:
		Builder(T& t) :Detail< TimeType , CallbackPolicy >::template Builder< EmptyBuilder, T >(EmptyBuilder(), t){}
	};

	template< class TimeType , template< class > class CallbackPolicy  >
	typename Builder< GroupTweener< TimeType , CallbackPolicy >, TimeType , CallbackPolicy >
		Build( GroupTweener< TimeType , CallbackPolicy >& tweener )
	{ 
		return Builder< GroupTweener< TimeType ,CallbackPolicy >, TimeType , CallbackPolicy >(tweener);
	}

	template< class TimeType , template< class > class CallbackPolicy >
	typename Builder< SquenceTweener< TimeType , CallbackPolicy >, TimeType , CallbackPolicy >
		Build( SquenceTweener< TimeType , CallbackPolicy >& tweener )
	{ 
		return Builder< SquenceTweener< TimeType , CallbackPolicy >, TimeType , CallbackPolicy >(tweener);
	}

}//namespace Tween

#endif // Tween_H_50802EDF_8D8D_47C6_B3C4_E1EB66015C6C
