#include "TFWRCore.h"

namespace TFWR
{
	char const* toString(EPlant::Type type)
	{
		switch (type)
		{
		case EPlant::Grass: return "Grass";
		case EPlant::Bush: return "Bush";
		case EPlant::Carrots: return "Carrots";
		case EPlant::Tree: return "Tree";
		case EPlant::Pumpkin: return "Pumpkin";
		case EPlant::Cactus: return "Cactus";
		case EPlant::Sunflower: return "Sunflower";
		case EPlant::Dionsaur: return "Dionsaur";
		}
		return "Unknown";
	}
}

