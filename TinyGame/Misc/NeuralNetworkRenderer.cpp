#include "NeuralNetworkRenderer.h"

#include "RenderUtility.h"

namespace
{
	Color3ub GetNodeColor(NNScalar value)
	{
		if (value > 0.9)
			return Color3ub(245, 245, 245);
		if (value > 0.75)
			return Color3ub(245, 215, 65);
		if (value > 0.5)
			return Color3ub(245, 145, 55);
		if (value > 0.25)
			return Color3ub(55, 115, 230);
		if (value > 0.1)
			return Color3ub(145, 145, 145);
		return Color3ub(12, 12, 12);
	}

	Color3ub GetLinkColor(NNScalar value)
	{
		return (value >= 0) ? Color3ub(35, 215, 100) : Color3ub(235, 65, 55);
	}
}

int NeuralNetworkRenderer::getValueColor(NNScalar value)
{
	if (value > 0.9)
		return EColor::White;
	if (value > 0.75)
		return EColor::Yellow;
	if (value > 0.5)
		return EColor::Orange;
	if (value > 0.25)
		return EColor::Blue;
	if (value > 0.1)
		return EColor::Gray;
	return EColor::Black;
}

void NeuralNetworkRenderer::draw(IGraphics2D& g)
{
	int const numLayer = (int)model.mLayers.size();

	if (parameters)
	{
		for (int idxLayer = numLayer - 1; idxLayer >= 0; --idxLayer)
		{
			NNLinearLayer const& layer = model.getLayer(idxLayer);
			int numPrevNode = (idxLayer == 0) ? model.getInputNum() : model.getLayer(idxLayer - 1).numNode;
			int idxPrevLayerSignal = (idxLayer == 0) ? 0 : model.getOutputSignalOffset(idxLayer - 1, false);

			for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
			{
				Vector2 pos = getLayerNodePos(idxLayer, idxNode);
				for (int idxPrevNode = 0; idxPrevNode < numPrevNode; ++idxPrevNode)
				{
					Vector2 prevPos = (idxLayer == 0) ? getInputNodePos(idxPrevNode) : getLayerNodePos(idxLayer - 1, idxPrevNode);
					NNScalar weight = getWeight(idxLayer, idxNode, idxPrevNode);
					NNScalar value = weight;
					if (bShowSignalLink && signals)
					{
						value *= signals[idxPrevLayerSignal + idxPrevNode];
					}

					g.setPen(GetLinkColor(value), 1 + Math::Min(3, Math::FloorToInt(2.5f * Math::Abs(weight))));
					drawLink(g, prevPos, pos, Math::Abs(weight));
				}
			}
		}
	}

	for (int i = 0; i < model.getInputNum(); ++i)
	{
		Vector2 pos = getInputNodePos(i);
		g.setPen(Color3ub(10, 10, 10), 2);
		g.setBrush(signals ? GetNodeColor(signals[i]) : Color3ub(150, 95, 210));
		drawNode(g, pos);
	}

	int idxSignal = model.getInputNum();
	for (int idxLayer = 0; idxLayer < numLayer; ++idxLayer)
	{
		NNLinearLayer const& layer = model.getLayer(idxLayer);
		for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
		{
			Vector2 pos = getLayerNodePos(idxLayer, idxNode);
			g.setPen(Color3ub(10, 10, 10), 2);
			g.setBrush(signals ? GetNodeColor(signals[idxSignal]) : Color3ub(150, 95, 210));
			drawNode(g, pos);
			++idxSignal;
		}
	}
}

void NeuralNetworkRenderer::drawLink(IGraphics2D& g, Vector2 const& p1, Vector2 const& p2, float width)
{
	Vector2 dir = p2 - p1;
	if (dir.normalize() < 1e-4)
		return;

	Vector2 start = p1 + nodeRadius * dir;
	Vector2 end = p2 - nodeRadius * dir;
	g.drawLine(start, end);
}
