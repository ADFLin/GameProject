#include "Stage/TestStageHeader.h"

#include "RHI/Material.h"
#include "RHI/RenderContext.h"
#include "RHI/MaterialShader.h"
#include "RHI/ShaderManager.h"
#include "RHI/GpuProfiler.h"
#include "RHI/RHIGraphics2D.h"

#include "Renderer/MeshBuild.h"
#include "Renderer/SimpleCamera.h"

#include "StringParse.h"
#include "FileSystem.h"
#include "RandomUtility.h"
#include <memory>
#include "GL/wglew.h"
#include "GameRenderSetup.h"


namespace Render
{

	class MaterialAssetProvider
	{
	public:
		virtual Material*     createMaterial(char const* name) = 0;
		virtual RHITexture2D* getTexture2D(char const* name) = 0;
	};

	class MaterialFileParser
	{
	public:
		MaterialFileParser(MaterialAssetProvider& provider)
			:mProvider(provider)
		{

		}

		static bool TokenStringLiteral(Tokenizer& tokenizer, StringView& outString)
		{
			StringView token;
			auto tokenType = tokenizer.take(token);
			if( tokenType == FStringParse::eNoToken )
				return false;

			if( tokenType == FStringParse::eDelimsType )
			{
				if( token[0] != '"' )
					return false;

				int offset = tokenizer.skipToChar('"');
				if( tokenizer.nextChar() != '"' )
				{
					return false;
				}
				tokenizer.offset(1);

				outString = StringView(token.data() + 1, offset );
			}
			else
			{
				outString = token;
			}

			return true;
		}

		static bool VerifyTokenDelimChar(Tokenizer& tokenizer, char c)
		{
			StringView token;
			return tokenizer.take(token) == FStringParse::eDelimsType && token[0] == c;
		}

		static bool ExitBlock(Tokenizer& tokenizer)
		{
			int stack = 1;
			for(;;)
			{
				StringView token;
				auto tokenType = tokenizer.take(token);

				if( tokenType == FStringParse::eNoToken )
				{
					return false;
				}
				else if( tokenType == FStringParse::eDelimsType )
				{
					if( token[0] == '{' )
					{
						++stack;
					} 
					else if( token[0] == '}' )
					{
						--stack;
						if( stack == 0 )
							break;
					}
					//TODO : skip string
				}
			}
			return true;
		}

		static bool SkipBlock(Tokenizer& tokenizer)
		{
			if( !VerifyTokenDelimChar(tokenizer , '{') )
				return false;

			return ExitBlock(tokenizer);
		}

		bool load(char const* pData, int dataSize, char const* targetMaterialName)
		{
			Tokenizer tokenizer(pData, " \n\r\t", "{}\"=,()");

			StringView token;
			FStringParse::TokenType tokenType;
			

			bool bEnd = false;
			for( ;; )
			{
				FStringParse::TokenType tokenType = tokenizer.take(token);
				if ( tokenType == FStringParse::eNoToken )
					break;

				if( token == "Material" )
				{
					StringView materialName;
					if( tokenizer.take(materialName) != FStringParse::eStringType )
					{
						return false;
					}

					if( targetMaterialName && materialName != targetMaterialName )
					{
						if( !SkipBlock(tokenizer) )
						{
							return false;
						}

						continue;
					}

					if( !parseMaterialBlock(tokenizer) )
					{
						return false;
					}
					if ( mCurMaterial )
					{				
						mLoadedMaterials.emplace(materialName.toStdString(), std::move(mCurMaterial));
					}
				}
			}

			return true;
		}

		bool parseMaterialBlock(Tokenizer& tokenizer)
		{
			if( !VerifyTokenDelimChar(tokenizer, '{') )
				return false;

			StringView token;
			if( tokenizer.take(token) != FStringParse::eStringType || token != "Shader" )
			{
				return false;
			}

			if ( !TokenStringLiteral(tokenizer , token ) )
			{
				return false;
			}

			mCurMaterial.reset( mProvider.createMaterial(token.toCString()) );
			if( mCurMaterial == nullptr )
			{
				if( !ExitBlock(tokenizer) )
				{
					return false;
				}
				else
				{
					return true;
				}
			}

			for(;;)
			{

				FStringParse::TokenType tokenType = tokenizer.take(token);
				if( tokenType == FStringParse::eNoToken )
				{
					return false;
				}
				else if( tokenType == FStringParse::eDelimsType )
				{
					if( token[0] == '}' )
					{
						break;
					}
					else
					{
						return false;
					}
				}
				else if( token == "State" )
				{
					if( !parseStateBlock(tokenizer) )
						return false;
				}
				else if( token == "Param" )
				{
					if( !parseParamBlock(tokenizer) )
						return false;
				}
			}

			return true;
		}

		bool parseStateBlock(Tokenizer& tokenizer)
		{
			//TODO
			return SkipBlock(tokenizer);
		}

		bool parseFloatArray(Tokenizer& tokenizer, float outValues[], int& outValueNum)
		{
			int num = 0;
			for( ;; )
			{
				StringView token;
				FStringParse::TokenType tokenType = tokenizer.take(token);
				if( tokenType != FStringParse::eStringType )
				{
					return false;
				}

				if( !token.toValueCheck(outValues[num]) )
					return false;

				++num;
				tokenType = tokenizer.take(token);

				if( tokenType != FStringParse::eDelimsType )
					return false;

				if ( token[0] == ')' )
					break;

				if( token[0] != ',' )
					return false;
			}

			outValueNum = num;
			return true;
		}

		bool parseParamBlock(Tokenizer& tokenizer)
		{
			if( !VerifyTokenDelimChar(tokenizer, '{') )
				return false;

			for( ;; )
			{
				StringView key;
				FStringParse::TokenType tokenType = tokenizer.take(key);
				if( tokenType == FStringParse::eNoToken )
				{
					return false;
				}
				else if ( tokenType == FStringParse::eDelimsType )
				{
					if( key[0] == '}' )
					{
						break;
					}
					else
					{
						return false;
					}
				}
				
				if( !VerifyTokenDelimChar(tokenizer, '=') )
				{
					return false;
				}

				StringView valueType;
				if( tokenizer.take(valueType) != FStringParse::eStringType )
				{
					return false;
				}

				if( !VerifyTokenDelimChar(tokenizer, '(') )
				{
					return false;
				}

				if( valueType == "texture2d" )
				{
					StringView textureName;
					if ( !TokenStringLiteral( tokenizer , textureName ) )
					{
						return false;
					}

					RHITexture2D* texture = mProvider.getTexture2D(textureName.toCString());
					if( texture )
					{
						mCurMaterial->setParameter(key.toCString(), *texture);
					}
					else
					{

					}

					if( !VerifyTokenDelimChar(tokenizer, ')') )
					{
						return false;
					}
				}
				else if( valueType == "float" )
				{
					int numElement = 0;
					float values[32];
					if( !parseFloatArray(tokenizer, values, numElement) )
					{
						return false;
					}

					switch( numElement )
					{
					case 0: mCurMaterial->setParameter(key.toCString(), float(0)); break;
					case 1:
					default:
						mCurMaterial->setParameter(key.toCString(), float(values[0])); break;
					}
				}
				else if( valueType == "float2" )
				{
					int numElement = 0;
					float values[32];
					if( !parseFloatArray(tokenizer, values, numElement) )
					{
						return false;
					}

					switch( numElement )
					{
					case 0: mCurMaterial->setParameter(key.toCString(), Vector2::Zero()); break;
					case 1: mCurMaterial->setParameter(key.toCString(), Vector2(values[0], values[0])); break;
					case 2:
					default:
						mCurMaterial->setParameter(key.toCString(), Vector2(values[0], values[1])); break;
					}
				}
				else if( valueType == "float3" )
				{
					int numElement = 0;
					float values[32];
					if( !parseFloatArray(tokenizer, values, numElement) )
					{
						return false;
					}

					switch( numElement )
					{
					case 0: mCurMaterial->setParameter(key.toCString(), Vector3::Zero()); break;
					case 1: mCurMaterial->setParameter(key.toCString(), Vector3(values[0], values[0], values[0])); break;
					case 2: mCurMaterial->setParameter(key.toCString(), Vector3(values[0], values[1],0)); break;
					case 3: 
					default:
						mCurMaterial->setParameter(key.toCString(), Vector3(values[0], values[1], values[2])); break;
					}
				}
				else if( valueType == "float4" )
				{
					int numElement = 0;
					float values[32];
					if( !parseFloatArray(tokenizer, values, numElement) )
					{
						return false;
					}

					switch( numElement )
					{
					case 0: mCurMaterial->setParameter(key.toCString(), Vector4::Zero()); break;
					case 1: mCurMaterial->setParameter(key.toCString(), Vector4(values[0], values[0], values[0], values[0])); break;
					case 2: mCurMaterial->setParameter(key.toCString(), Vector4(values[0], values[1], 0,0)); break;
					case 3: mCurMaterial->setParameter(key.toCString(), Vector4(values[0], values[1], values[2],0)); break;
					case 4:
					default:
						mCurMaterial->setParameter(key.toCString(), Vector4(values[0], values[1], values[2], values[3])); break;
					}

				}
				else if( valueType == "mat3" )
				{
					int numElement = 0;
					float values[32];
					if( !parseFloatArray(tokenizer, values, numElement) )
					{
						return false;
					}
					if( numElement < 9 )
					{
						return false;
					}
					mCurMaterial->setParameter(key.toCString(), Matrix3(values[0], values[1], values[2], values[3], values[4], values[5], values[6], values[7], values[8]));

				}
				else if( valueType == "mat4" )
				{
					int numElement = 0;
					float values[32];
					if( !parseFloatArray(tokenizer, values, numElement) )
					{
						return false;
					}
					if( numElement < 16 )
					{
						return false;
					}
					mCurMaterial->setParameter(key.toCString(), Matrix4(values[0], values[1], values[2], values[3], values[4], values[5], values[6], values[7], values[8], values[9], values[10], values[11], values[12], values[13], values[14], values[15]));
				}
			}

			return true;
		}

		using MaterialPtr = std::unique_ptr<Material>;
		MaterialPtr mCurMaterial;
		std::unordered_map< std::string, MaterialPtr > mLoadedMaterials;
		MaterialAssetProvider& mProvider;
	};



	struct ForwordLightingProgram : public MaterialShaderProgram
	{
		using BaseClass = MaterialShaderProgram;

		DECLARE_SHADER_PROGRAM(ForwordLightingProgram ,Material);

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);
			option.addDefine(SHADER_PARAM(FORWARD_SHADING), 1);
		}

		static char const* GetShaderFileName()
		{
			return "Shader/BasePass";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(BassPassVS) },
				{ EShader::Pixel  , SHADER_ENTRY(BasePassPS) },
			};
			return entries;
		}
	};

	IMPLEMENT_SHADER_PROGRAM(ForwordLightingProgram);

	class DrawCallOptimizationStage : public StageBase
		                            , public MaterialAssetProvider
		                            , public IGameRenderSetup
	{
		using BaseClass = StageBase;
	public:
		DrawCallOptimizationStage() {}


		enum MeshId
		{
#define MESH_MODEL_LIST(op)\
	op(Teapot , "Model/teapot.obj")\
	op(Mario, "Model/Mario.obj")\
	op(Lightning, "Model/lightning.obj")\
    op(Vanille , "Model/Vanille.obj")\

#define EnumOp( NAME , PATH ) NAME ,
			MESH_MODEL_LIST(EnumOp)
#undef EnumOp

			MODEL_MESH_END ,
			Cube = MODEL_MESH_END,
			Sphere ,

			MeshIdCount,
		};

		enum MaterialId
		{
#define MATERIAL_LIST(op)\
	op(SimpleA)op(SimpleB)op(SimpleC)op(SimpleD)

#define EnumOp( NAME ) NAME ,
			MATERIAL_LIST(EnumOp)
#undef EnumOp

			MaterialIdCount ,

		};

		Mesh mMeshs[MeshIdCount];


		struct ObjectParam
		{
			Matrix4     localToWorld;
			LinearColor color;
		};
		struct DrawObject
		{
			Mesh* mesh;
			Material* material;
			ObjectParam param;
		};


		std::vector<DrawObject>  mObjects;
		std::vector<DrawObject*> mDrawOrderObjects;

		enum EDrawOrderMethod
		{




		};

		class ViewFrustum
		{
		public:
			Matrix4 getPerspectiveMatrix()
			{
				return PerspectiveMatrix(mYFov, mAspect, mNear, mFar);
			}

			float mNear;
			float mFar;
			float mYFov;
			float mAspect;
		};

		ViewInfo      mView;
		ViewFrustum   mViewFrustum;
		SimpleCamera  mCamera;
		Material* mMaterials[MaterialIdCount];


		bool loadMaterial(char const* path)
		{
			std::vector< char > buffer;
			if( !FileUtility::LoadToBuffer(path, buffer, true) )
				return false;
			buffer.push_back(0);
			MaterialFileParser parser(*this);
			bool bOk = parser.load(buffer.data(), buffer.size(), nullptr);

#define NAME_OP( id ) #id ,
			char const* MaterialNames[] = { MATERIAL_LIST(NAME_OP) };

			for( int i = 0; i < MaterialIdCount; ++i )
			{
				auto& matPtr = parser.mLoadedMaterials[MaterialNames[i]];
				if( matPtr )
				{
					mMaterials[i] = matPtr.release();
				}
				else
				{
					mMaterials[i] = GDefalutMaterial;
				}
			}
			return true;
		}


		bool onInit() override
		{
			if( !BaseClass::onInit() )
				return false;

			if( !ShaderHelper::Get().init() )
				return false;


			::Global::GUI().cleanupWidget();


			VERIFY_RETURN_FALSE( loadMaterial("Material/SimpleTest.mat") );

			struct LoadModelInfo
			{
				int  id;
				char const* name;
			};

			Matrix4 MeshTransform[MODEL_MESH_END];
			for(auto& xform : MeshTransform)
			{
				xform.setIdentity();
			}
			Matrix4 FixRotation = Matrix4::Rotate(Vector3(1, 0, 0), Math::Deg2Rad(90));
			MeshTransform[MeshId::Teapot] = Matrix4::Scale(0.05) * FixRotation;
			MeshTransform[MeshId::Mario] = Matrix4::Scale(0.05) * FixRotation;
			MeshTransform[MeshId::Lightning] = Matrix4::Scale(0.03) *  FixRotation;
			MeshTransform[MeshId::Vanille] = Matrix4::Scale(0.03) *  FixRotation;

			LoadModelInfo ModelList[] =
			{
#define DataOp( NAME , PATH ) { MeshId::NAME , PATH } ,
				MESH_MODEL_LIST(DataOp)
#undef DataOp
			};
			int idx = 0;
			for( LoadModelInfo const& info : ModelList )
			{
				VERIFY_RETURN_FALSE( MeshBuild::LoadObjectFile(mMeshs[info.id], info.name, &MeshTransform[idx], nullptr, nullptr) );
				++idx;
			}
			VERIFY_RETURN_FALSE( MeshBuild::Cube(mMeshs[MeshId::Cube], 1) );
			VERIFY_RETURN_FALSE( MeshBuild::UVSphere(mMeshs[MeshId::Sphere], 1 , 30 , 30 ) );

			int NumObjects = 1000;
			float boundSize = 50;
			for( int i = 0; i < NumObjects; ++i )
			{
				DrawObject object;
				object.material = mMaterials[ rand() % MaterialIdCount ];
				object.mesh = &mMeshs[ rand() % MeshIdCount ];
				object.param.localToWorld = Matrix4::Translate(boundSize *(  2 * RandVector() - Vector3(1, 1,1) ) ) ;
				mObjects.push_back(object);
			}

			for( auto& object : mObjects )
			{
				mDrawOrderObjects.push_back(&object);
			}
#if 0
			std::sort(mDrawOrderObjects.begin(), mDrawOrderObjects.end(), [](DrawObject* lhs, DrawObject* rhs)
			{
				return lhs->material < rhs->material;
			});
#endif

			Vec2i screenSize = ::Global::GetScreenSize();
			mViewFrustum.mNear = 0.01;
			mViewFrustum.mFar = 800.0;
			mViewFrustum.mAspect = float(screenSize.x) / screenSize.y;
			mViewFrustum.mYFov = Math::Deg2Rad(60 / mViewFrustum.mAspect);

			mCamera.setPos(Vector3(50, 50, 50));
			mCamera.setViewDir(Vector3(-1, -1, -1), Vector3(0, 0, 1));

			restart();
			return true;
		}

		void onEnd() override
		{
			BaseClass::onEnd();
		}

		void restart() {}
		void tick() {}
		void updateFrame(int frame) {}

		void onUpdate(long time) override
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for( int i = 0; i < frame; ++i )
				tick();

			updateFrame(frame);
		}

		void onRender(float dFrame) override
		{
			Vec2i screenSize = ::Global::GetScreenSize();

			{
				GPU_PROFILE("Frame");
				RHICommandList& commandList = RHICommandList::GetImmediateList();

				mView.gameTime = 0;
				mView.realTime = 0;
				mView.rectOffset = IntVector2(0, 0);
				mView.rectSize = screenSize;

				Matrix4 matView = mCamera.getViewMatrix();
				mView.setupTransform(matView, mViewFrustum.getPerspectiveMatrix());


				glMatrixMode(GL_PROJECTION);
				glLoadMatrixf(mView.viewToClip);
				glMatrixMode(GL_MODELVIEW);
				glLoadMatrixf(mView.worldToView);

				glClearColor(0.2, 0.2, 0.2, 1);
				glClearDepth(1.0);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

				RHISetViewport(commandList, mView.rectOffset.x, mView.rectOffset.y, mView.rectSize.x, mView.rectSize.y);
				RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
				RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());


				{
					GPU_PROFILE("Draw Objects");
					int indexObject = 0;
					MaterialShaderProgram* curProgram = nullptr;
					for( auto pObject : mDrawOrderObjects )
					{
						DrawObject& object = *pObject;
						MaterialShaderProgram* program = object.material->getMaster()->getShaderT<ForwordLightingProgram>(nullptr);

						if( curProgram != program )
						{
							RHISetShaderProgram(commandList, program->getRHIResource());
							curProgram = program;
							mView.setupShader(commandList, *program);
						}

						object.material->setupShader(commandList, *curProgram);
						curProgram->setParam(commandList, SHADER_PARAM(Primitive.localToWorld), object.param.localToWorld);
						Matrix4 worldToLocal;
						float det;
						object.param.localToWorld.inverseAffine(worldToLocal, det);
						curProgram->setParam(commandList, SHADER_PARAM(Primitive.worldToLocal), worldToLocal);

						object.mesh->draw(commandList);

						++indexObject;
					}

					if( curProgram )
					{
						RHISetShaderProgram(commandList, nullptr);
						curProgram = nullptr;
					}
				}
			}

			RHIGraphics2D& g = ::Global::GetRHIGraphics2D();

			InlineString< 512 > str;

			g.beginRender();
			

			g.endRender();

		}

		bool onMouse(MouseMsg const& msg) override
		{
			static Vec2i oldPos = msg.getPos();

			if( msg.onLeftDown() )
			{
				oldPos = msg.getPos();
			}
			if( msg.onMoving() && msg.isLeftDown() )
			{
				float rotateSpeed = 0.01;
				Vector2 off = rotateSpeed * Vector2(msg.getPos() - oldPos);
				mCamera.rotateByMouse(off.x, off.y);
				oldPos = msg.getPos();
			}

			if( !BaseClass::onMouse(msg) )
				return false;
			return true;
		}

		bool onKey(KeyMsg const& msg) override
		{
			if( !msg.isDown())
				return false;
			switch(msg.getCode())
			{
			case EKeyCode::W: mCamera.moveFront(1); break;
			case EKeyCode::S: mCamera.moveFront(-1); break;
			case EKeyCode::D: mCamera.moveRight(1); break;
			case EKeyCode::A: mCamera.moveRight(-1); break;
			case EKeyCode::Z: mCamera.moveUp(0.5); break;
			case EKeyCode::X: mCamera.moveUp(-0.5); break;
			case EKeyCode::R: restart(); break;
			case EKeyCode::F2:
				{
					ShaderManager::Get().reloadAll();
					//initParticleData();
				}
				break;
			}
			return false;
		}

		bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch( id )
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}


		std::unordered_map< HashString, MaterialMaster* > mMaterialMap;

		Material*     createMaterial(char const* name) override
		{
			HashString hashName{ name };
			auto iter = mMaterialMap.find(hashName);
			if (iter != mMaterialMap.end() )
			{
				MaterialInstance* instance = new MaterialInstance(iter->second);
				return instance;
			}
			MaterialMaster* material = new MaterialMaster;
			if( !material->loadFile(name) )
			{
				delete material;
				return nullptr;
			}
			mMaterialMap.emplace(hashName, material);
			return material;
		}
		RHITexture2D* getTexture2D(char const* name) override
		{
			return RHIUtility::LoadTexture2DFromFile(name);
		}


	protected:
	};

	REGISTER_STAGE("Draw Call", DrawCallOptimizationStage, EStageGroup::FeatureDev, "Render");

}