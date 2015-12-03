#ifndef Entity_h__
#define Entity_h__

#include <vector>

namespace Game
{
	enum ComponentFlag
	{
		CMF_UPDATE ,

	};

	class IComponent
	{
	public:
		virtual ~IComponent(){}
		virtual void update( long time ){}
	private:
		friend class Entity;
	};

	class Entity
	{
	public:
		static int const MAX_COMPONENT_INDEX = 16;

		IComponent* getComponent( int idx ){ return mCompMap[ idx ]; }

		void addComponent( int idx , IComponent* comp )
		{

		}

		template< T >
		T* getComponentT( int idx )
		{ 
			IComponent* comp = getComponent( idx );
#ifdef _DEBUG
			return dynamic_cast< T* >( comp );
#else
			return static_cast< T* >( comp );
#endif
		}

		
		IComponent* mCompMap[ MAX_COMPONENT_INDEX ];
	};
}

#endif // Entity_h__
