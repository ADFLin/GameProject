
#include <vector>
namespace Render
{

	class FrameGraphBuilder;
	class FrameNodeInstnace;


	class IRenderContext
	{




	};

	class RenderPassResources
	{





	};

	class FrameGraphResource
	{


	};
	class FrameNode
	{
	public:
		virtual void setup( FrameGraphBuilder& builder ) {}
		virtual void execute( IRenderContext& context , RenderPassResources& resources ){}
	};

	class FrameGraph
	{
	public:
		void compileNode(FrameNode* root )
		{

		}


		void execute(IRenderContext& context, RenderPassResources& resources)
		{


		}


		FrameNode* mRootNode;
		std::vector< FrameNode* > mRegisteredNodes;
	};

	class FrameGraphBuilder
	{
		void write(FrameGraphResource);
		void read(FrameGraphResource);
		void useRenderTarget();


	};

}