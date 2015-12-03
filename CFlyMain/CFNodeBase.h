#ifndef CFNodeBase_h__
#define CFNodeBase_h__

#include "CFBase.h"
#include "IntrList.h"

namespace CFly
{
	class NodeBase;
	class NodeManager;


	class NodeHookData
	{
	protected:
		HookNode     mChildHook;
	};
	
	class NodeBase : public NodeHookData
	{
	public:
		
		enum
		{
			NEXT_NODE_FLAG ,
		};
		NodeBase( NodeManager* manager );
		NodeBase( NodeBase const& node );

		virtual ~NodeBase();
		//typedef VectorTypeIterWraper< NodeList > ChildIterator;

		void          setParent( NodeBase* parent );
		NodeBase*     getParent() const { return mParent; }
		int           getChildrenNum(){ return mChildren.size(); }
		//ChildIterator getChildren(){ return ChildIterator( mChildren ); }
		char const*   getName(){ return mName ? mName : ""; }
		bool          registerName( char const* name );

		bool          checkFlag    ( unsigned flag ) const { return ( mFlag & BIT( flag ) ) != 0; }
		bool          checkFlagBits( unsigned bits ) const { return ( mFlag & bits ) != 0 ; }
		void          addFlag   ( unsigned flag ) const { mFlag |= BIT( flag ); }
		void          enableFlag( unsigned flag ,bool beE );
		void          removeFlag( unsigned flag ) const { mFlag &= ~ BIT( flag ); }
		void          addFlagBits( unsigned bits ){ mFlag |= bits; }
		void          removeFlagBits( unsigned bits ){ mFlag &= ~bits; }

		/*
		class Visitor
		{
		public:
			void visit( NodeBase* node ){}
		};
		*/
		template< class Visitor >
		void visitAll( Visitor& visitor ){ visitAll_R( visitor ); }

		template< class Visitor >
		void visitChildren( Visitor& visitor )
		{
			for( NodeList::iterator iter ( mChildren.begin() ) , end( mChildren.end() );
				iter != end ; ++iter ) 
			{
				visitor.visit( *iter );
			}
		}

	private:

		template< class Visitor >
		void visitAll_R( Visitor& visitor )
		{
			visitor.visit( this );
			for( NodeList::iterator iter ( mChildren.begin() ) , end( mChildren.end() );
				iter != end ; ++iter ) 
			{
				(*iter)->visitAll_R( visitor );
			}
		}

		NodeManager*   mManager;
		char const*    mName;
		NodeBase*      mParent;
		friend class NodeManager;

	protected:
		typedef MemberHook< NodeHookData , &NodeHookData::mChildHook > ChildHook;
		typedef IntrList< NodeBase , ChildHook , PointerType > NodeList;
		//typedef std::list< NodeBase* > NodeList;
		NodeManager*  getManager(){ return mManager; }
		void          init();
		mutable unsigned  mFlag;
		NodeList          mChildren;
	};


	class NodeManager
	{
	public:

		NodeBase* findNodeByName( char const* name );
		void      removeNode( NodeBase* node );
		void      addNode( NodeBase* node );

	protected:
		friend class NodeBase;
		void      unregisterName( NodeBase* node );
		bool      registerName( NodeBase* node , char const* name );


		struct StrCmp
		{
			bool operator()( char const* s1 , char const* s2 ) const
			{
				return strcmp( s1 , s2 ) < 0;
			}
		};
		typedef std::map< String , NodeBase* > NodeNameMap;
		NodeNameMap mNameMap;
	};


}//namespace CFly

#endif // CFNodeBase_h__
