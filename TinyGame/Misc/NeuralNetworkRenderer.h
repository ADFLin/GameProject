#pragma once
#ifndef NeuralNetworkRenderer_H_0DFFC412_8BC2_45DD_BCFE_8A6C1E8816DE
#define NeuralNetworkRenderer_H_0DFFC412_8BC2_45DD_BCFE_8A6C1E8816DE

#include "AI/NeuralNetwork.h"
#include "GameGraphics2D.h"

class NeuralNetworkRenderer
{
public:
	NeuralNetworkRenderer(NNFullConLayout const& model)
		:model(model)
	{


	}

	void draw(IGraphics2D& g);

	void drawNode(IGraphics2D& g, Vector2 pos)
	{
		g.drawCircle(pos, nodeRadius);
	}

	void drawLink(IGraphics2D& g, Vector2 const& p1, Vector2 const& p2, float width);
	int  getValueColor(NNScalar value);

	float getOffsetY(int idx, int numNode)
	{
		return (float(idx) - 0.5f * float(numNode - 1)) * nodeOffset;
	}
	Vector2 getInputNodePos(int idx)
	{
		int numInput = model.getInputNum();
		float offsetY = getOffsetY(idx, numInput);
		return basePos + Vector2(0, offsetY);
	}
	Vector2 getLayerNodePos(int idxLayer, int idxNode)
	{
		NNLinearLayer const& layer = model.getLayer(idxLayer);
		float offsetY = getOffsetY(idxNode, layer.numNode);
		return basePos + Vector2((idxLayer + 1) * layerOffset, offsetY);
	}

	NNScalar* signals = nullptr;

	NNScalar* parameters = nullptr;
	bool bShowSignalLink = true;
	float scaleFactor = 1.5;
	float layerOffset = scaleFactor * 40;
	float nodeOffset = scaleFactor * 30;
	float nodeRadius = 8;
	Vector2 basePos = Vector2(0, 0);
	NNFullConLayout const& model;


	NNScalar getWeight(int idxLayer, int idxNode, int idxInput)
	{
		NNLinearLayer const& layer = model.mLayers[idxLayer];
		return parameters[layer.weightOffset + idxInput * layer.numNode + idxNode];
	}
};

#endif // NeuralNetworkRenderer_H_0DFFC412_8BC2_45DD_BCFE_8A6C1E8816DE
