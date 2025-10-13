#pragma once

#ifndef NNModel_H_EE6FF54E_79AC_469D_8510_5470208E47DD
#define NNModel_H_EE6FF54E_79AC_469D_8510_5470208E47DD

#include "NeuralNetwork.h"
#include "Math/TVector3.h"

namespace NNModel
{
	using IntVector3 = TVector3<int>;

	class INNExprNode;
	struct SetupContext
	{
		IntVector3 inputSize;
		int parameterOffset;

		IntVector3 nodeInputSize;
		IntVector3 nodeOutputSize;
		int nodePassOutputNum;
		int nodeParameterNum;
	};

	FORCEINLINE int GetLength(IntVector3 const& size)
	{
		int result = 1;
		if (size.x) result *= size.x;
		if (size.y) result *= size.y;
		if (size.z) result *= size.z;
		return result;
	}

	struct FowrwardContext
	{
		IntVector3 inputSize;
		NNScalar*  parameters;
		NNScalar*  inputs;
		NNScalar*  outputs;

	};

	struct BackwardContext
	{
		IntVector3 inputSize;
		NNScalar* parameters;
		NNScalar* inputs;
		NNScalar* outputs;

		NNScalar* parameterGrads;
		NNScalar* lossGradsInput;
		NNScalar* lossGrads;
		NNScalar* lossGradsOutput;
	};

	class IExprNode
	{
	public:
		virtual void setup(SetupContext& context) = 0;
		virtual NNScalar* forward(FowrwardContext& context) = 0;
		virtual void backward(BackwardContext& context) = 0;
	};

	template< typename TNNLayer >
	struct TLayerTraits
	{
	};


	class TransformExprNode : public IExprNode
		                     ,public NNTransformLayer
	{
	public:
		virtual void setup(SetupContext& context)
		{
			int len = GetLength(context.inputSize);
			context.nodeParameterNum = 0;
			context.nodeInputSize = IntVector3(len, 0, 0);
			context.nodeOutputSize = context.nodeInputSize;
			context.nodePassOutputNum = len;

		}
		virtual NNScalar* forwardPass(FowrwardContext& context)
		{
			return FNNAlgo::Forward(*this, context.inputSize.x, context.inputs, context.outputs);
		}

		virtual void backwardPass(BackwardContext& context)
		{
			FNNAlgo::Backward(*this, context.inputSize.x, context.inputs, context.lossGradsInput, context.lossGrads);
		}
	};
	template<>
	struct TLayerTraits< NNTransformLayer > { using ExprNode = TransformExprNode; };

	class LinearExprNode : public IExprNode
	{
	public:
		virtual void setup(SetupContext& context)
		{
			int inputLength = GetLength(context.inputSize);

			mLayer.biasOffset = context.parameterOffset;
			mLayer.weightOffset = mLayer.biasOffset + mLayer.numNode;

			context.nodeParameterNum = mLayer.getParameterLength(inputLength);
			context.nodeInputSize = IntVector3(inputLength, 0, 0);
			context.nodeOutputSize = IntVector3(mLayer.numNode, 0, 0);
			context.nodePassOutputNum = mLayer.getPassOutputNum();
		}

		virtual NNScalar* forwardPass(FowrwardContext& context)
		{
			return FNNAlgo::Forward(mLayer, context.parameters, context.inputs, context.outputs);
		}

		virtual void backwardPass(BackwardContext& context)
		{
			FNNAlgo::BackwardWeight(mLayer, context.inputs, context.lossGradsInput, context.parameterGrads);
			if (context.lossGrads)
			{
				FNNAlgo::BackwardLoss(mLayer, context.parameters, context.lossGradsInput, context.lossGrads);
			}
		}

		NNLinearLayer mLayer;
	};
	template<>
	struct TLayerTraits< NNLinearLayer > { using ExprNode = LinearExprNode; };

	class Conv2DExprNode : public IExprNode
	{
	public:
		virtual void setup(SetupContext& context)
		{
			mLayer.dataSize[0] = context.inputSize.x - mLayer.convSize + 1;
			mLayer.dataSize[1] = context.inputSize.y - mLayer.convSize + 1;

			mLayer.biasOffset = context.parameterOffset;
			mLayer.weightOffset = mLayer.biasOffset + mLayer.numNode;

			CHECK(context.inputSize.z > 0);
			context.nodeParameterNum = mLayer.getParameterLength(context.inputSize.z);

			context.nodeInputSize = context.inputSize;
			context.nodeOutputSize = IntVector3(mLayer.dataSize[0], mLayer.dataSize[1], mLayer.numNode);

		}
		virtual NNScalar* forwardPass(FowrwardContext& context)
		{


		}
		virtual void backwardPass(BackwardContext& context)
		{


		}

		NNConv2DLayer mLayer;
	};
	template<>
	struct TLayerTraits< NNConv2DLayer > { using ExprNode = Conv2DExprNode; };




	struct Sequence
	{
		template< typename ...TNNLayers >
		Sequence(TNNLayers&& ...layers)
		{
			addLayer(layers)...;
		}


		template< typename TNNLayer >
		void addLayer(TNNLayer&& layer)
		{
			using ExprNodeType = TLayerTraits<TNNLayer>::ExprNode;

			ExprNodeType exprNode = new ExprNodeType(layer);
			mNodes.push_back(exprNode);
		}

		virtual void setup(SetupContext& context)
		{
			SetupContext childContext;
			childContext.inputSize = context.inputSize;
			childContext.parameterOffset = 0;

			int passOutputOffset = 0;

			for (int i = 0; i < mNodes.size(); ++i)
			{
				auto& node = mNodes[i];
				node.expr->setup(childContext);

				node.inputSize = childContext.nodeInputSize;
				node.passOutputOffset = passOutputOffset;
				passOutputOffset += childContext.nodePassOutputNum;

				childContext.parameterOffset += childContext.nodeParameterNum;
				childContext.inputSize = childContext.nodeOutputSize;
			}

			context.parameterOffset = childContext.parameterOffset;
			context.nodeInputSize = context.nodeInputSize;
			context.nodeOutputSize = childContext.nodeOutputSize;
		}

		virtual NNScalar* forward(FowrwardContext& context)
		{
			FowrwardContext childContext;
			childContext.inputs = context.inputs;
			childContext.parameters = context.parameters;

			for (int i = 0; i < mNodes.size(); ++i)
			{
				auto& node = mNodes[i];
				childContext.outputs = context.outputs + node.passOutputOffset;
				childContext.inputSize = node.inputSize;
				childContext.inputs = node.expr->forward(childContext);
			}

			return childContext.inputs;
		}

		virtual NNScalar* forwardPsss(FowrwardContext& context)
		{
			FowrwardContext childContext;
			childContext.inputs = context.inputs;
			childContext.parameters = context.parameters;

			for (int i = 0; i < mNodes.size(); ++i)
			{
				auto& node = mNodes[i];
				childContext.outputs = context.outputs + node.passOutputOffset;
				childContext.inputSize = node.inputSize;
				childContext.inputs = node.expr->forward(childContext);
			}

			return childContext.inputs;
		}

		virtual void backwardPass(BackwardContext& context)
		{
			BackwardContext childContext;
			childContext.parameters = context.parameters;
			childContext.parameterGrads = context.parameterGrads;

			childContext.lossGradsInput = childContext.lossGradsInput;
			childContext.lossGrads = childContext.lossGrads;
			childContext.lossGradsOutput = childContext.lossGrads;
			for (int i = mNodes.size() - 1; i > 0; --i)
			{
				auto& node = mNodes[i];

				childContext.outputs = context.outputs + node.passOutputOffset;
				childContext.lossGrads = context.lossGrads + node.passOutputOffset;

				node.expr->backward(childContext);

				childContext.lossGradsInput = childContext.lossGradsOutput;
			}

			childContext.inputs = context.inputs;
			childContext.lossGrads = context.lossGrads;
			mNodes[0].expr->backward(childContext);
		}
		struct Node
		{
			IExprNode* expr;
			IntVector3 inputSize;
			int        passOutputOffset;
		};

		TArray< Node > mNodes;
	};

}

#endif // NNModel_H_EE6FF54E_79AC_469D_8510_5470208E47DD
