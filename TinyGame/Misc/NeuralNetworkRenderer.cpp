#include "NeuralNetworkRenderer.h"

#include "RenderUtility.h"

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
	FCNNLayout const& NNLayout = FNN.getLayout();
	for (int i = 0; i < NNLayout.getInputNum(); ++i)
	{
		Vector2 pos = getInputNodePos(i);
		RenderUtility::SetPen(g, EColor::Black);
		RenderUtility::SetBrush(g, getValueColor(signals[i]));
		drawNode(g, pos);
	}

	int idxSignal = NNLayout.getInputNum();
	int idxPrevLayerSignal = 0;
	for (int i = 0; i <= NNLayout.getHiddenLayerNum(); ++i)
	{
		NeuralFullConLayer const& layer = NNLayout.getLayer(i);
		int numNodeWeight = NNLayout.getLayerInputNum(i);
		for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
		{
			Vector2 pos = getLayerNodePos(i, idxNode);
			RenderUtility::SetPen(g, EColor::Black);
			if (signals)
			{
				RenderUtility::SetBrush(g, getValueColor(signals[idxSignal]));
			}
			else
			{
				RenderUtility::SetBrush(g, EColor::Purple);
			}
			drawNode(g, pos);

			NNScalar* weights = FNN.getWeights(i, idxNode);
			for (int n = 0; n < numNodeWeight; ++n)
			{
				Vector2 prevPos = (i == 0) ? getInputNodePos(n) : getLayerNodePos(i - 1, n);
				NNScalar value = weights[n + 1];
				if (bShowSignalLink && signals)
				{
					value *= signals[idxPrevLayerSignal + n];
				}
				RenderUtility::SetPen(g, (value > 0) ? EColor::Green : EColor::Red);
				RenderUtility::SetBrush(g, (value > 0) ? EColor::Green : EColor::Red);
				drawLink(g, prevPos, pos, Math::Abs(value));
			}
			++idxSignal;
		}
		idxPrevLayerSignal += numNodeWeight;
	}
}

void NeuralNetworkRenderer::drawLink(IGraphics2D& g, Vector2 const& p1, Vector2 const& p2, float width)
{
	Vector2 dir = p2 - p1;
	if (dir.normalize() < 1e-4)
		return;

	float halfWidth = 0.25 * width;
	if (bShowSignalLink)
		halfWidth *= 4;

	halfWidth = std::min(10.0f, halfWidth);
	Vector2 normalOffset;
	normalOffset.x = dir.y;
	normalOffset.y = -dir.x;
	normalOffset *= halfWidth;
	Vector2 v[4] = { p1 + normalOffset , p1 - normalOffset , p2 - normalOffset , p2 + normalOffset };
	g.drawPolygon(v, 4);
}
