#pragma once

#ifndef NNModel_H_EE6FF54E_79AC_469D_8510_5470208E47DD
#define NNModel_H_EE6FF54E_79AC_469D_8510_5470208E47DD

#include "NeuralNetwork.h"
#include "Math/TVector3.h"

#include <memory>
#include <type_traits>
#include <utility>

namespace NNModel
{
	using IntVector3 = TVector3<int>;

	class INNExprNode;
	struct SetupContext
	{
		IntVector3 inputSize;
		int parameterOffset = 0;

		IntVector3 nodeInputSize;
		IntVector3 nodeOutputSize;
		int nodePassOutputNum = 0;
		int nodeParameterNum = 0;
	};

	FORCEINLINE int GetLength(IntVector3 const& size)
	{
		int result = 1;
		if (size.x) result *= size.x;
		if (size.y) result *= size.y;
		if (size.z) result *= size.z;
		return result;
	}

	struct InferenceContext
	{
		IntVector3 inputSize;
		NNScalar const* parameters = nullptr;
		NNScalar const* inputs = nullptr;
		NNScalar* outputs = nullptr;

		int tempDataSize = 0;
		NNScalar* tempData = nullptr;
	};

	struct ForwardContext
	{
		IntVector3 inputSize;
		NNScalar const* parameters = nullptr;
		NNScalar const* inputs = nullptr;
		NNScalar* outputs = nullptr;
	};

	struct BackwardContext
	{
		IntVector3 inputSize;
		NNScalar const* parameters = nullptr;
		NNScalar const* inputs = nullptr;
		NNScalar const* outputs = nullptr;

		NNScalar* parameterGrads = nullptr;
		NNScalar const* lossGradsInput = nullptr;
		NNScalar* lossGrads = nullptr;
		NNScalar* lossGradsOutput = nullptr;
	};

	class IExprNode
	{
	public:
		virtual ~IExprNode() = default;
		virtual void setup(SetupContext& context) = 0;
		virtual NNScalar* inference(InferenceContext& context) = 0;
		virtual NNScalar* forward(ForwardContext& context) = 0;
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
		TransformExprNode() = default;
		TransformExprNode(NNTransformLayer const& layer)
			: NNTransformLayer(layer)
		{
		}

		virtual void setup(SetupContext& context) override
		{
			int len = GetLength(context.inputSize);
			context.nodeParameterNum = 0;
			context.nodeInputSize = IntVector3(len, 0, 0);
			context.nodeOutputSize = context.nodeInputSize;
			context.nodePassOutputNum = len;
		}

		virtual NNScalar* inference(InferenceContext& context) override
		{
			return FNNAlgo::Inference(*this, context.inputSize.x, context.inputs, context.outputs);
		}

		virtual NNScalar* forward(ForwardContext& context) override
		{
			return FNNAlgo::Forward(*this, context.inputSize.x, context.inputs, context.outputs);
		}

		virtual void backward(BackwardContext& context) override
		{
			if (context.lossGrads == nullptr)
				return;

			FNNAlgo::Backward(*this, context.inputSize.x, context.inputs, context.outputs, context.lossGradsInput, context.lossGrads);
		}
	};
	template<>
	struct TLayerTraits< NNTransformLayer > { using ExprNode = TransformExprNode; };

	class LinearExprNode : public IExprNode
	{
	public:
		LinearExprNode() = default;
		LinearExprNode(NNLinearLayer const& layer)
			: mLayer(layer)
		{
		}

		virtual void setup(SetupContext& context) override
		{
			int inputLength = GetLength(context.inputSize);
			mLayer.inputLength = inputLength;

			mLayer.weightOffset = context.parameterOffset;
			mLayer.biasOffset = mLayer.weightOffset + mLayer.getWeightLength();

			context.nodeParameterNum = mLayer.getParameterLength();
			context.nodeInputSize = IntVector3(inputLength, 0, 0);
			context.nodeOutputSize = IntVector3(mLayer.numNode, 0, 0);
			context.nodePassOutputNum = mLayer.getPassOutputNum();
		}

		virtual NNScalar* inference(InferenceContext& context) override
		{
			return FNNAlgo::Inference(mLayer, context.parameters, context.inputs, context.outputs);
		}

		virtual NNScalar* forward(ForwardContext& context) override
		{
			return FNNAlgo::Forward(mLayer, context.parameters, context.inputs, context.outputs);
		}

		virtual void backward(BackwardContext& context) override
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
		Conv2DExprNode() = default;
		Conv2DExprNode(NNConv2DLayer const& layer)
			: mLayer(layer)
		{
		}

		virtual void setup(SetupContext& context) override
		{
			mLayer.inputSize = context.inputSize;
			mLayer.dataSize[0] = context.inputSize.x - mLayer.convSize + 1;
			mLayer.dataSize[1] = context.inputSize.y - mLayer.convSize + 1;

			mLayer.weightOffset = context.parameterOffset;
			mLayer.biasOffset = mLayer.weightOffset + mLayer.getWeightLength();

			CHECK(context.inputSize.z > 0);
			context.nodeParameterNum = mLayer.getParameterLength();
			context.nodeInputSize = context.inputSize;
			context.nodeOutputSize = IntVector3(mLayer.dataSize[0], mLayer.dataSize[1], mLayer.numNode);
			context.nodePassOutputNum = mLayer.getPassOutputLength();
		}

		virtual NNScalar* inference(InferenceContext& context) override
		{
			return FNNAlgo::Inference(mLayer, context.parameters, context.inputs, context.outputs);
		}

		virtual NNScalar* forward(ForwardContext& context) override
		{
			return FNNAlgo::Forward(mLayer, context.parameters, context.inputs, context.outputs);
		}

		virtual void backward(BackwardContext& context) override
		{
			FNNAlgo::BackwardWeight(mLayer, context.inputs, context.outputs, context.lossGradsInput, context.parameterGrads);
			if (context.lossGrads)
			{
				FNNAlgo::BackwardLoss(mLayer, context.parameters, context.lossGradsInput, context.lossGrads);
			}
		}

		NNConv2DLayer mLayer;
	};
	template<>
	struct TLayerTraits< NNConv2DLayer > { using ExprNode = Conv2DExprNode; };

	class ConvTranspose2DExprNode : public IExprNode
	{
	public:
		ConvTranspose2DExprNode() = default;
		ConvTranspose2DExprNode(NNConvTranspose2DLayer const& layer)
			: mLayer(layer)
		{
		}

		virtual void setup(SetupContext& context) override
		{
			mLayer.init(context.inputSize, mLayer.numNode, mLayer.convSize, mLayer.padding, mLayer.stride);
			mLayer.setParameterOffset(context.parameterOffset);

			context.nodeParameterNum = mLayer.getParameterLength();
			context.nodeInputSize = context.inputSize;
			context.nodeOutputSize = mLayer.getOutputSize();
			context.nodePassOutputNum = mLayer.getPassOutputLength();
		}

		virtual NNScalar* inference(InferenceContext& context) override
		{
			return FNNAlgo::Inference(mLayer, context.parameters, context.inputs, context.outputs);
		}

		virtual NNScalar* forward(ForwardContext& context) override
		{
			return FNNAlgo::Forward(mLayer, context.parameters, context.inputs, context.outputs);
		}

		virtual void backward(BackwardContext& context) override
		{
			FNNAlgo::BackwardWeight(mLayer, context.inputs, context.outputs, context.lossGradsInput, context.parameterGrads);
			if (context.lossGrads)
			{
				FNNAlgo::BackwardLoss(mLayer, context.parameters, context.lossGradsInput, context.lossGrads);
			}
		}

		NNConvTranspose2DLayer mLayer;
	};
	template<>
	struct TLayerTraits< NNConvTranspose2DLayer > { using ExprNode = ConvTranspose2DExprNode; };

	class MaxPooling2DExprNode : public IExprNode
	{
	public:
		MaxPooling2DExprNode() = default;
		MaxPooling2DExprNode(NNMaxPooling2DLayer const& layer)
			: mLayer(layer)
		{
		}

		virtual void setup(SetupContext& context) override
		{
			mLayer.init(context.inputSize, mLayer.poolSize);

			context.nodeParameterNum = 0;
			context.nodeInputSize = context.inputSize;
			context.nodeOutputSize = IntVector3(mLayer.dataSize[0], mLayer.dataSize[1], mLayer.numNode);
			context.nodePassOutputNum = mLayer.getOutputLength();
		}

		virtual NNScalar* inference(InferenceContext& context) override
		{
			return FNNAlgo::Inference(mLayer, context.inputs, context.outputs);
		}

		virtual NNScalar* forward(ForwardContext& context) override
		{
			return FNNAlgo::Forward(mLayer, context.inputs, context.outputs);
		}

		virtual void backward(BackwardContext& context) override
		{
			if (context.lossGrads == nullptr)
				return;

			FNNAlgo::Backward(mLayer, context.inputs, context.outputs, context.lossGradsInput, context.lossGrads);
		}

		NNMaxPooling2DLayer mLayer;
	};
	template<>
	struct TLayerTraits< NNMaxPooling2DLayer > { using ExprNode = MaxPooling2DExprNode; };

	class AveragePooling2DExprNode : public IExprNode
	{
	public:
		AveragePooling2DExprNode() = default;
		AveragePooling2DExprNode(NNAveragePooling2DLayer const& layer)
			: mLayer(layer)
		{
		}

		virtual void setup(SetupContext& context) override
		{
			mLayer.init(context.inputSize, mLayer.poolSize);

			context.nodeParameterNum = 0;
			context.nodeInputSize = context.inputSize;
			context.nodeOutputSize = IntVector3(mLayer.dataSize[0], mLayer.dataSize[1], mLayer.numNode);
			context.nodePassOutputNum = mLayer.getOutputLength();
		}

		virtual NNScalar* inference(InferenceContext& context) override
		{
			return FNNAlgo::Inference(mLayer, context.inputs, context.outputs);
		}

		virtual NNScalar* forward(ForwardContext& context) override
		{
			return FNNAlgo::Forward(mLayer, context.inputs, context.outputs);
		}

		virtual void backward(BackwardContext& context) override
		{
			if (context.lossGrads == nullptr)
				return;

			FNNAlgo::Backward(mLayer, context.lossGradsInput, context.lossGrads);
		}

		NNAveragePooling2DLayer mLayer;
	};
	template<>
	struct TLayerTraits< NNAveragePooling2DLayer > { using ExprNode = AveragePooling2DExprNode; };

	class BatchNormalizeExprNode : public IExprNode
	{
	public:
		BatchNormalizeExprNode() = default;
		BatchNormalizeExprNode(NNBatchNormlizeLayer const& layer)
			: mLayer(layer)
		{
		}

		virtual void setup(SetupContext& context) override
		{
			mLayer.numNode = context.inputSize.z ? context.inputSize.z : GetLength(context.inputSize);
			mLayer.length = GetLength(context.inputSize) / mLayer.numNode;
			mLayer.setParameterOffset(context.parameterOffset);

			context.nodeParameterNum = 4 * mLayer.numNode;
			context.nodeInputSize = context.inputSize;
			context.nodeOutputSize = context.inputSize;
			context.nodePassOutputNum = mLayer.getPassOutputLength();
		}

		virtual NNScalar* inference(InferenceContext& context) override
		{
			return FNNAlgo::Inference(mLayer, context.parameters, context.inputs, context.outputs);
		}

		virtual NNScalar* forward(ForwardContext& context) override
		{
			return FNNAlgo::Forward(mLayer, context.parameters, context.inputs, context.outputs);
		}

		virtual void backward(BackwardContext& context) override
		{
			FNNAlgo::BackwardWeight(mLayer, context.inputs, context.lossGradsInput, context.parameterGrads);
			if (context.lossGrads)
			{
				FNNAlgo::BackwardLoss(mLayer, context.parameters, context.inputs, context.outputs, context.lossGradsInput, context.lossGrads);
			}
		}

		NNBatchNormlizeLayer mLayer;
	};
	template<>
	struct TLayerTraits< NNBatchNormlizeLayer > { using ExprNode = BatchNormalizeExprNode; };

	struct Sequence : public IExprNode
	{
		Sequence() = default;

		template< typename ...TNNLayers >
		Sequence(TNNLayers&& ...layers)
		{
			(addLayer(std::forward<TNNLayers>(layers)), ...);
		}

		template< typename TNNLayer >
		void addLayer(TNNLayer&& layer)
		{
			using LayerType = std::decay_t<TNNLayer>;
			using ExprNodeType = typename TLayerTraits<LayerType>::ExprNode;

			Node node;
			node.expr = std::make_unique<ExprNodeType>(std::forward<TNNLayer>(layer));
			mNodes.push_back(std::move(node));
		}

		void setup(SetupContext& context)
		{
			if (mNodes.empty())
			{
				context.nodeParameterNum = 0;
				context.nodeInputSize = context.inputSize;
				context.nodeOutputSize = context.inputSize;
				context.nodePassOutputNum = 0;
				return;
			}

			SetupContext childContext;
			childContext.inputSize = context.inputSize;
			childContext.parameterOffset = context.parameterOffset;

			int passOutputOffset = 0;

			for (int i = 0; i < mNodes.size(); ++i)
			{
				auto& node = mNodes[i];
				node.expr->setup(childContext);

				node.inputSize = childContext.nodeInputSize;
				node.outputSize = childContext.nodeOutputSize;
				node.passOutputOffset = passOutputOffset;
				node.passOutputNum = childContext.nodePassOutputNum;
				passOutputOffset += childContext.nodePassOutputNum;

				childContext.parameterOffset += childContext.nodeParameterNum;
				childContext.inputSize = childContext.nodeOutputSize;
			}

			context.nodeParameterNum = childContext.parameterOffset - context.parameterOffset;
			context.parameterOffset = childContext.parameterOffset;
			context.nodeInputSize = context.inputSize;
			context.nodeOutputSize = childContext.nodeOutputSize;
			context.nodePassOutputNum = passOutputOffset;
		}

		NNScalar* inference(InferenceContext& context)
		{
			if (mNodes.empty())
				return const_cast<NNScalar*>(context.inputs);

			NNScalar* nodeOutputs = context.tempData ? context.tempData : context.outputs;
			CHECK(nodeOutputs);
			CHECK(context.tempData == nullptr || context.tempDataSize >= mNodes.back().passOutputOffset + mNodes.back().passOutputNum);

			InferenceContext childContext;
			childContext.inputs = context.inputs;
			childContext.parameters = context.parameters;

			for (int i = 0; i < mNodes.size(); ++i)
			{
				auto& node = mNodes[i];
				childContext.outputs = nodeOutputs + node.passOutputOffset;
				childContext.inputSize = node.inputSize;
				childContext.inputs = node.expr->inference(childContext);
			}

			NNScalar* result = const_cast<NNScalar*>(childContext.inputs);
			if (context.outputs && context.outputs != result)
			{
				int outputLength = GetLength(mNodes.back().outputSize);
				for (int i = 0; i < outputLength; ++i)
				{
					context.outputs[i] = result[i];
				}
				result = context.outputs;
			}

			return result;
		}

		NNScalar* forward(ForwardContext& context)
		{
			ForwardContext childContext;
			childContext.inputs = context.inputs;
			childContext.parameters = context.parameters;

			for (int i = 0; i < mNodes.size(); ++i)
			{
				auto& node = mNodes[i];
				childContext.outputs = context.outputs + node.passOutputOffset;
				childContext.inputSize = node.inputSize;
				childContext.inputs = node.expr->forward(childContext);
			}

			return const_cast<NNScalar*>(childContext.inputs);
		}

		void backward(BackwardContext& context)
		{
			if (mNodes.empty())
				return;

			CHECK(mNodes.size() == 1 || context.lossGrads);

			BackwardContext childContext;
			childContext.parameters = context.parameters;
			childContext.parameterGrads = context.parameterGrads;

			for (int i = int(mNodes.size()) - 1; i >= 0; --i)
			{
				auto& node = mNodes[i];

				childContext.inputSize = node.inputSize;
				childContext.inputs = (i == 0) ? context.inputs : context.outputs + mNodes[i - 1].passOutputOffset;
				childContext.outputs = context.outputs + node.passOutputOffset;
				childContext.lossGradsInput = (i + 1 == int(mNodes.size())) ? context.lossGradsInput : context.lossGrads + node.passOutputOffset;
				childContext.lossGrads = (i == 0) ? context.lossGradsOutput : context.lossGrads + mNodes[i - 1].passOutputOffset;

				node.expr->backward(childContext);
			}
		}

		struct Node
		{
			std::unique_ptr<IExprNode> expr;
			IntVector3 inputSize;
			IntVector3 outputSize;
			int passOutputOffset = 0;
			int passOutputNum = 0;
		};

		TArray< Node > mNodes;
	};

}

#endif // NNModel_H_EE6FF54E_79AC_469D_8510_5470208E47DD
