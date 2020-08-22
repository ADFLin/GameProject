
#include <vector>
#include "RefCount.h"
#include "HashString.h"
#include <functional>


namespace Render
{
	class RHICommandList;

	class FRGBuilder;


	class FrameRenderContext
	{




	};

	class FRGPassBase
	{
	public:
		virtual void setup( FRGBuilder& builder ) {}
		virtual void execute(RHICommandList& commandlist, FrameRenderContext& context ){}
	};

	class FrameRenderPassParameters
	{


	};


	class DownsampleRenderPass : public FRGPassBase
	{



	};

	class BloomRenderPass : public FRGPassBase
	{




	};

	class FrameRenderGraph
	{
	public:

		void execute(RHICommandList& commandlist)
		{


		}


		FRGPassBase* mRootNode;
		std::vector< FRGPassBase* > mRegisteredNodes;
	};

	struct FRGResourceBase
	{
		HashString name;
	};


	template< class TResourceType >
	struct TFRGResource : FRGResourceBase
	{
	public:
		TResourceType* getResource() { return mResource; }

		TRefCountPtr< TResourceType > mResource;
	};

	class FRGBuilder
	{
	public:
		template< class TPass , class ...Args>
		TPass& createPass(Args&& ...args)
		{
			auto info = new PassInfo;
			TPass* pass = new TPass(std::forward<Args>(args)...);
			info->pass.reset(pass);
			return *pass;
		}

		template< class TPassData , class TSetupFunc , class TExecuteFunc >
		TPassData& createCallablePass( HashString const& name, TSetupFunc&& setupFunc , TExecuteFunc&& executeFunc )
		{
			auto info = new PassInfo;

			class TFRGCallablePass : public FRGPassBase
			{
			public:
				TFRGCallablePass(TSetupFunc&& inSetupFunc, TExecuteFunc&& inExecuteFunc)
					:setupFunc(std::forward<TSetupFunc>(inSetupFunc)), executeFunc(std::forward<TExecuteFunc>(inExecuteFunc))
				{

				}
				virtual void setup(FRGBuilder& builder)
				{
					setupFunc(mData, builder);
				}
				virtual void execute(RHICommandList& commandlist, FrameRenderContext& context)
				{
					executeFunc(mData, commandlist, context);
				}

				TSetupFunc   setupFunc;
				TExecuteFunc executeFunc;

				HashString mName;
				TPassData  mData;
			};
			auto pass = new TFRGCallablePass(name, setupFunc, executeFunc);
			info->pass = pass;
			return pass->mData;
		}




		struct PassLink
		{



		};


		struct PassInfo;
		
		

		struct ResourceInfo
		{
			FRGResourceBase* resource;
			PassInfo*  productor;
			std::vector< PassInfo* > users;

			RefCount   refCount;

			uint32  
		};

		struct PassInfo
		{
			enum Mask
			{
				eFlattened = BIT(0),
			};
			uint32 compilesFlags = 0;
			FRGPassBase* pass;
			std::vector< ResourceInfo* > inputs;
			std::vector< ResourceInfo* > outputs;

			ResourceInfo* findInput(FRGResourceBase* resource)
			{
				for (auto resInfo : inputs)
				{
					if (resInfo->resource == resource)
						return resInfo;
				}
				return nullptr;
			}


			ResourceInfo* findOutput(FRGResourceBase* resource)
			{
				for (auto resInfo : outputs)
				{
					if (resInfo->resource == resource)
						return resInfo;
				}
				return nullptr;
			}
		};

		std::unordered_map< FRGPassBase*, std::unique_ptr< PassInfo > > mPassMap;
		std::unordered_map< FRGResourceBase*, std::unique_ptr< ResourceInfo > > mResourceMap;

		void traverseDependentPass(ResourceInfo* resInfo , std::vector< PassInfo* >& outDependentPassList)
		{
			auto productor = resInfo->productor;
			for (auto inputInfo : productor->inputs)
			{
				traverseDependentPass(resInfo, outDependentPassList);
			}
			outDependentPassList.push_back(productor);
		}

		PassInfo* mCurrentSetupPass;
		struct ScopedPassSetup
		{
			ScopedPassSetup(FRGBuilder& builder , PassInfo* info)
				:mBuilder(builder)
			{
				CHECK(mBuilder.mCurrentSetupPass == nullptr);
				mBuilder.mCurrentSetupPass = info;
				info->pass->setup(builder);
			}

			~ScopedPassSetup()
			{
				mBuilder.mCurrentSetupPass = nullptr;
			}
			FRGBuilder& mBuilder;
		};


		void compile(std::vector< FRGResourceBase* > const& finalOuputResources )
		{
			//flatten pass list
			mPassMap.clear();
			std::vector< PassInfo* > flattenedPassList;
			for (auto outputRes : finalOuputResources)
			{
				ResourceInfo* resInfo = mResourceMap[outputRes].get();
				ResourceInfo* outputInfo = nullptr;

				CHECK(outputInfo);
				traverseDependentPass(outputInfo, flattenedPassList);
			}
		}


	};

}