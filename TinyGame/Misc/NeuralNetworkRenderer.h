#pragma once
#ifndef NeuralNetworkRenderer_H_0DFFC412_8BC2_45DD_BCFE_8A6C1E8816DE
#define NeuralNetworkRenderer_H_0DFFC412_8BC2_45DD_BCFE_8A6C1E8816DE

#include "AI/NeuralNetwork.h"
#include "GameGraphics2D.h"

class NeuralNetworkRenderer
{
public:
	NeuralNetworkRenderer(FCNeuralNetwork& inFNN)
		:FNN(inFNN)
	{


	}



	void draw(IGraphics2D& g);

	void drawNode(IGraphics2D& g, Vector2 pos)
	{
		g.drawCircle(pos, 10);
	}

	void drawLink(IGraphics2D& g, Vector2 const& p1, Vector2 const& p2, float width);
	int  getValueColor(NNScalar value);

	float getOffsetY(int idx, int numNode)
	{
		return (float(idx) - 0.5 * float(numNode - 1)) * nodeOffset;
	}
	Vector2 getInputNodePos(int idx)
	{
		int numInput = FNN.getLayout().getInputNum();
		float offsetY = getOffsetY(idx, numInput);
		return basePos + Vector2(0, offsetY);
	}
	Vector2 getLayerNodePos(int idxLayer, int idxNode)
	{
		NeuralFullConLayer const& layer = FNN.getLayout().getLayer(idxLayer);
		float offsetY = getOffsetY(idxNode, layer.numNode);
		return basePos + Vector2((idxLayer + 1) * layerOffset, offsetY);
	}

	NNScalar* signals = nullptr;
	bool bShowSignalLink = true;
	float scaleFactor = 1.5;
	float layerOffset = scaleFactor * 40;
	float nodeOffset = scaleFactor * 30;
	Vector2 basePos = Vector2(0, 0);
	FCNeuralNetwork& FNN;
};

#endif // NeuralNetworkRenderer_H_0DFFC412_8BC2_45DD_BCFE_8A6C1E8816DE