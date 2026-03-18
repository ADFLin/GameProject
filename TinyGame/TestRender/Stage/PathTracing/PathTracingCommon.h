

#pragma once

#ifndef PathTracingCommon_H_AAF7BFF4_C002_4DAA_9B79_2F29014CBB04
#define PathTracingCommon_H_AAF7BFF4_C002_4DAA_9B79_2F29014CBB04

#include "Math/Transform.h"
#include "Core/Color.h"

#include "RHI/ShaderCore.h"


namespace PathTracing
{
	using Math::Vector3;
	using Math::Quaternion;
	using Math::Transform;

	enum class EDebugDsiplayMode
	{
		None,
		BoundingBox,
		Triangle,
		Mix,
		HitNoraml,
		HitPos,
		BaseColor,
		EmissiveColor,
		Roughness,
		Specular,
		COUNT,
	};


	struct GPU_ALIGN MaterialData
	{
		DECLARE_BUFFER_STRUCT(Materials);

		Color3f baseColor;
		float   roughness;
		Color3f emissiveColor;
		float   specular;
		float   refractive;
		float   refractiveIndex;

		float   emissiveAngle = 0.0f; // 1.0 for isotropic
		float   emissiveFocus = 0.0f; // 0 for hard edge, >0 for smooth falloff

		MaterialData()
		{
			baseColor = { 1, 1, 1 };
			roughness = 0.5f;
			emissiveColor = { 0, 0, 0 };
			specular = 0.0f;
			refractive = 0.0f;
			refractiveIndex = 1.0f;
			emissiveAngle = 0.0f;
			emissiveFocus = 0.0f;
		}

		MaterialData(Color3f const& inBaseColor, float inRoughness, float inSpecular = 0, Color3f const& inEmissiveColor = Color3f::Black(), float inRefractive = 0, float inRefractiveIndex = 0)
			:baseColor(inBaseColor)
			, roughness(inRoughness)
			, emissiveColor(inEmissiveColor)
			, specular(inSpecular)
			, refractive(inRefractive)
			, refractiveIndex(inRefractiveIndex)
		{


		}

		REFLECT_STRUCT_BEGIN(MaterialData)
			REF_PROPERTY(baseColor)
			REF_PROPERTY(emissiveColor)
			REF_PROPERTY(roughness)
			REF_PROPERTY(specular)
			REF_PROPERTY(refractiveIndex)
			REF_PROPERTY(refractive)
			REF_PROPERTY(emissiveAngle)
			REF_PROPERTY(emissiveFocus)
		REFLECT_STRUCT_END()
	};

#define OBJ_SPHERE        0
#define OBJ_CUBE          1
#define OBJ_TRIANGLE_MESH 2
#define OBJ_QUAD          3
#define OBJ_DISC          4


	namespace EObjectType 
	{
		enum Type : int32
		{
			Sphere,
			Cube,
			Mesh,
			Quad,
			Disc,
		};
	}

	REF_ENUM_BEGIN(EObjectType::Type)
		REF_ENUM(Sphere)
		REF_ENUM(Cube)
		REF_ENUM(Mesh)
		REF_ENUM(Quad)
		REF_ENUM(Disc)
	REF_ENUM_END()


	template<typename T, typename Q>
	FORCEINLINE T AsValue(Q value)
	{
		static_assert(sizeof(T) == sizeof(Q));
		T* ptr = (T*)&value;
		return *ptr;
	}


	struct GPU_ALIGN ObjectData
	{
		DECLARE_BUFFER_STRUCT(Objects);

		Vector3 pos;
		EObjectType::Type   type;
		Quaternion rotation;
		Vector3 meta;
		int32   materialId;

		ObjectData()
		{
			pos = { 0, 0, 0 };
			type = EObjectType::Sphere;
			rotation = Quaternion::Identity();
			meta = { 1, 0, 0 };
			materialId = 0;
		}

		REFLECT_STRUCT_BEGIN(ObjectData)
			REF_PROPERTY(type)
			REF_PROPERTY(pos)
			REF_PROPERTY(rotation)
			REF_PROPERTY(meta)
			REF_PROPERTY(materialId)
		REFLECT_STRUCT_END()
	};

	struct FObject
	{
		static ObjectData Sphere(float radius, int materialId, Vector3 pos, Quaternion rotation = Quaternion::Identity())
		{
			ObjectData result;
			result.type = EObjectType::Sphere;
			result.pos = pos;
			result.rotation = rotation;
			result.materialId = materialId;
			result.meta = Vector3(radius, 0, 0);
			return result;
		}
		static ObjectData Box(Vector3 size, int materialId, Vector3 pos, Quaternion rotation = Quaternion::Identity())
		{
			ObjectData result;
			result.type = EObjectType::Cube;
			result.pos = pos;
			result.rotation = rotation;
			result.materialId = materialId;
			result.meta = 0.5 * size;
			return result;
		}

		static ObjectData Quad(Vector2 size, int materialId, Vector3 pos, Quaternion rotation = Quaternion::Identity())
		{
			ObjectData result;
			result.type = EObjectType::Quad;
			result.pos = pos;
			result.rotation = rotation;
			result.materialId = materialId;
			result.meta = 0.5 * Vector3(size.x, size.y, 0);
			return result;
		}

		static ObjectData Mesh(int32 meshId, float scale, int materialId, Vector3 pos, Quaternion rotation = Quaternion::Identity())
		{
			ObjectData result;
			result.type = EObjectType::Mesh;
			result.pos = pos;
			result.rotation = rotation;
			result.materialId = materialId;
			result.meta = Vector3(AsValue<float>(meshId), scale, 0);
			return result;
		}

		static ObjectData Sphere(float radius, int materialId, Math::Transform const& transform)
		{
			return Sphere(radius, materialId, transform.location, transform.rotation);
		}
		static ObjectData Box(Vector3 size, int materialId, Math::Transform const& transform)
		{
			return Box(size, materialId, transform.location, transform.rotation);
		}
		static ObjectData Quad(Vector2 size, int materialId, Math::Transform const& transform)
		{
			return Quad(size, materialId, transform.location, transform.rotation);
		}
		static ObjectData Mesh(int32 meshId, int materialId, Math::Transform const& transform)
		{
			return Mesh(meshId, transform.scale.x, materialId, transform.location, transform.rotation);
		}
	};

	struct MeshVertexData
	{
		DECLARE_BUFFER_STRUCT(MeshVertices);

		Vector3 pos;
		Vector3 normal;
	};

	struct GPU_ALIGN MeshData
	{
		DECLARE_BUFFER_STRUCT(Meshes);

		int32 startIndex;
		int32 numTriangles;
		int32 nodeIndex;
		int32 nodeIndexV4;
	};


}//namespace PathTracing


#endif // PathTracingCommon_H_AAF7BFF4_C002_4DAA_9B79_2F29014CBB04