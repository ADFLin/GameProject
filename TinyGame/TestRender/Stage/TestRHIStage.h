#pragma once
#ifndef TestRHIStage_H_CAE7EE6B_BBA3_4470_80BE_5B234E975DD8
#define TestRHIStage_H_CAE7EE6B_BBA3_4470_80BE_5B234E975DD8

#include "Stage/TestRenderStageBase.h"

namespace Render
{
	struct TestInitContext
	{
	public:
		TestInitContext(SharedAssetData& assetData)
			:mAssetData(assetData)
		{
		}
		SharedAssetData& getAssetData()
		{
			return mAssetData;
		}

		SharedAssetData& mAssetData;
	};

	struct TestRenderContext
	{
		TestRenderContext(ViewInfo& view)
			:mView(view)
		{

		}
		ViewInfo& getView() { return mView; }
		ViewInfo& mView;
	};

	class TestExample
	{
	public:
		virtual ~TestExample(){}
		virtual bool enter(TestInitContext& context) { return true; }
		virtual void exit(){}
		virtual void render(TestRenderContext& context){}

	};
	class TestRHIStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		bool onInit() override;
		void onEnd() override;
		void onRender(float dFrame);


		using ExampleCreationFunc = std::function< TestExample* () >;
		std::unordered_map< HashString, ExampleCreationFunc > mExampleMap;

		template< typename TClass >
		void registerTest(HashString name)
		{
			mExampleMap.emplace(
				name,
				[]()
				{
					return new TClass;
				}
			);
		}

		void changeExample(HashString name);


		virtual bool setupRenderResource(ERenderSystem systemName) override;

		TestExample* mCurExample = nullptr;
		

		ERenderSystem getDefaultRenderSystem() override;


		bool isRenderSystemSupported(ERenderSystem systemName) override;
		void preShutdownRenderSystem(bool bReInit) override;

	};
}
#endif // TestRHIStage_H_CAE7EE6B_BBA3_4470_80BE_5B234E975DD8