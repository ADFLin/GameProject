
#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"

#include "AI/NeuralNetwork.h"

#include "RHI/RHICommand.h"
#include "GameGlobal.h"
#include "RHI/RHIGraphics2D.h"
#include "ProfileSystem.h"
#include "Async/AsyncWork.h"
#include "RenderDebug.h"
#include "FileSystem.h"
#include "Serialize/FileStream.h"
#include "AI/NNTrain.h"
#include "RandomUtility.h"

#define USE_ANNT 1
#if USE_ANNT
#include "ANNT/ANNT.hpp"
#endif


#include <random>
#include "Async/Coroutines.h"


#include "C-ATTL3/Cattle.hpp"

void Test_C_ATTL3()
{

	using namespace cattle;
	// Load the MNIST data labelled as 'real' into memory so that it can be pre-processed and shuffled.
	std::string mnist_folder = "mnist/";
	MNISTDataProvider<float> real_file_train_prov(mnist_folder + "train-images.idx3-ubyte", mnist_folder + "train-labels.idx1-ubyte");
	Tensor<float, 4> mnist_train_data = real_file_train_prov.get_data(60000).first;
	mnist_train_data = (mnist_train_data - mnist_train_data.constant(127.5)) / mnist_train_data.constant(127.5);
	Tensor<float, 4> mnist_train_label = Tensor<float, 4>(60000u, 1u, 1u, 1u).random() * .1f;
	mnist_train_label += mnist_train_label.constant(.9);
	MemoryDataProvider<float, 3, false> real_train_prov(
		TensorPtr<float, 4>(new Tensor<float, 4>(std::move(mnist_train_data))),
		TensorPtr<float, 4>(new Tensor<float, 4>(std::move(mnist_train_label))));
	MNISTDataProvider<float> real_file_test_prov(mnist_folder + "t10k-images.idx3-ubyte", mnist_folder + "t10k-labels.idx1-ubyte");
	Tensor<float, 4> mnist_test_data = real_file_test_prov.get_data(10000).first;
	mnist_test_data = (mnist_test_data - mnist_test_data.constant(127.5)) / mnist_test_data.constant(127.5);
	Tensor<float, 4> mnist_test_label = Tensor<float, 4>(10000u, 1u, 1u, 1u).random() * .1f;
	mnist_test_label += mnist_test_label.constant(.9);
	MemoryDataProvider<float, 3, false> real_test_prov(
		TensorPtr<float, 4>(new Tensor<float, 4>(std::move(mnist_test_data))),
		TensorPtr<float, 4>(new Tensor<float, 4>(std::move(mnist_test_label))));
	// Create the GAN.
	auto init = std::make_shared<GaussianParameterInitialization<float>>(2e-2);
	float lrelu_alpha = .2;
	float dropout = .3;
	std::vector<LayerPtr<float, 3>> generator_layers(9);
	generator_layers[0] = LayerPtr<float, 3>(new DenseKernelLayer<float, 3>({ 1u, 1u, 100u }, 256, init));
	generator_layers[1] = LayerPtr<float, 3>(new LeakyReLUActivationLayer<float, 3>(generator_layers[0]->get_output_dims(), lrelu_alpha));
	generator_layers[2] = LayerPtr<float, 3>(new DenseKernelLayer<float, 3>(generator_layers[1]->get_output_dims(), 512, init));
	generator_layers[3] = LayerPtr<float, 3>(new LeakyReLUActivationLayer<float, 3>(generator_layers[2]->get_output_dims(), lrelu_alpha));
	generator_layers[4] = LayerPtr<float, 3>(new DenseKernelLayer<float, 3>(generator_layers[3]->get_output_dims(), 1024, init));
	generator_layers[5] = LayerPtr<float, 3>(new LeakyReLUActivationLayer<float, 3>(generator_layers[4]->get_output_dims(), lrelu_alpha));
	generator_layers[6] = LayerPtr<float, 3>(new DenseKernelLayer<float, 3>(generator_layers[5]->get_output_dims(), 784, init));
	generator_layers[7] = LayerPtr<float, 3>(new TanhActivationLayer<float, 3>(generator_layers[6]->get_output_dims()));
	generator_layers[8] = LayerPtr<float, 3>(new ReshapeLayer<float, 3>(generator_layers[7]->get_output_dims(), { 28u, 28u, 1u }));
	NeuralNetPtr<float, 3, false> generator(new FeedforwardNeuralNetwork<float, 3>(std::move(generator_layers)));
	std::vector<LayerPtr<float, 3>> discriminator_layers(11);
	discriminator_layers[0] = LayerPtr<float, 3>(new DenseKernelLayer<float, 3>(generator->get_output_dims(), 1024, init));
	discriminator_layers[1] = LayerPtr<float, 3>(new LeakyReLUActivationLayer<float, 3>(discriminator_layers[0]->get_output_dims(), lrelu_alpha));
	discriminator_layers[2] = LayerPtr<float, 3>(new DropoutLayer<float, 3>(discriminator_layers[1]->get_output_dims(), dropout));
	discriminator_layers[3] = LayerPtr<float, 3>(new DenseKernelLayer<float, 3>(discriminator_layers[2]->get_output_dims(), 512, init));
	discriminator_layers[4] = LayerPtr<float, 3>(new LeakyReLUActivationLayer<float, 3>(discriminator_layers[3]->get_output_dims(), lrelu_alpha));
	discriminator_layers[5] = LayerPtr<float, 3>(new DropoutLayer<float, 3>(discriminator_layers[4]->get_output_dims(), dropout));
	discriminator_layers[6] = LayerPtr<float, 3>(new DenseKernelLayer<float, 3>(discriminator_layers[5]->get_output_dims(), 256, init));
	discriminator_layers[7] = LayerPtr<float, 3>(new LeakyReLUActivationLayer<float, 3>(discriminator_layers[6]->get_output_dims(), lrelu_alpha));
	discriminator_layers[8] = LayerPtr<float, 3>(new DropoutLayer<float, 3>(discriminator_layers[7]->get_output_dims(), dropout));
	discriminator_layers[9] = LayerPtr<float, 3>(new DenseKernelLayer<float, 3>(discriminator_layers[8]->get_output_dims(), 1, init));
	discriminator_layers[10] = LayerPtr<float, 3>(new SigmoidActivationLayer<float, 3>(discriminator_layers[9]->get_output_dims()));
	NeuralNetPtr<float, 3, false> discriminator(new FeedforwardNeuralNetwork<float, 3>(std::move(discriminator_layers)));
	std::vector<NeuralNetPtr<float, 3, false>> modules(2);
	modules[0] = std::move(generator);
	modules[1] = std::move(discriminator);
	StackedNeuralNetwork<float, 3, false> gan(std::move(modules));
	gan.init();
	// Define the GAN training hyper-parameters.
	const unsigned epochs = 100;
	const unsigned m = 100;
	const unsigned k = 1;
	const unsigned l = 4;
	// Specify the loss functions and the optimizers.
	auto loss = std::make_shared<BinaryCrossEntropyLoss<float, 3, false>>();
	float lr = 2e-4;
	float adam_beta = .5;
	cattle::AdamOptimizer<float, 3, false> disc_opt(loss, m, lr, adam_beta);
	cattle::AdamOptimizer<float, 3, false> gen_opt(loss, m, lr, adam_beta);
	disc_opt.fit(*gan.get_modules()[1]);
	gen_opt.fit(*gan.get_modules()[0]);
	PPMCodec<float, P2> ppm_codec;
	// Execute the GAN optimization algorithm.
	for (unsigned i = 0; i <= epochs; ++i) {
		std::string epoch_header = "*   GAN Epoch " + std::to_string(i) + "   *";
		std::cout << std::endl << std::string(epoch_header.length(), '*') << std::endl <<
			"*" << std::string(epoch_header.length() - 2, ' ') << "*" << std::endl <<
			epoch_header << std::endl <<
			"*" << std::string(epoch_header.length() - 2, ' ') << "*" << std::endl <<
			std::string(epoch_header.length(), '*') << std::endl;
		float disc_train_loss = 0;
		float gen_train_loss = 0;
		std::size_t total_m = 0;
		unsigned counter = 0;
		real_train_prov.reset();
		while (real_train_prov.has_more()) 
		{
			// First optimize the discriminator by using a batch of generated and a batch of real images.
			auto real_train_data = real_train_prov.get_data(k * m);
			auto actual_m = real_train_data.first.dimension(0);
			total_m += actual_m;
			Tensor<float, 4> train_data = gan.get_modules()[0]->infer(Tensor<float, 4>(actual_m, 1u, 1u, 100u).random())
				.concatenate(std::move(real_train_data.first), 0);
			Tensor<float, 4> train_label = Tensor<float, 4>(actual_m, 1u, 1u, 1u).constant(0).concatenate(std::move(real_train_data.second), 0);
			MemoryDataProvider<float, 3, false, false> disc_train_prov(
				TensorPtr<float, 4>(new Tensor<float, 4>(std::move(train_data))),
				TensorPtr<float, 4>(new Tensor<float, 4>(std::move(train_label))));
			disc_train_loss += disc_opt.train(*gan.get_modules()[1], disc_train_prov, 1) * actual_m;
			// Then optimize the generator by maximizing the GAN's loss w.r.t. the parameters of the generator.
			MemoryDataProvider<float, 3, false, false> gen_train_prov(
				TensorPtr<float, 4>(new Tensor<float, 4>(Tensor<float, 4>(l * actual_m, 1u, 1u, 100u).random())),
				TensorPtr<float, 4>(new Tensor<float, 4>(Tensor<float, 4>(l * actual_m, 1u, 1u, 1u).constant(1))));
			gan.get_modules()[1]->set_frozen(true);
			gen_train_loss += gen_opt.train(gan, gen_train_prov, 1) * actual_m;
			gan.get_modules()[1]->set_frozen(false);
			counter++;
		}
		std::cout << "\tdiscriminator training loss: " << (disc_train_loss / total_m) << std::endl;
		std::cout << "\tgenerator training loss: " << (gen_train_loss / total_m) << std::endl;
		// Perform tests on the GAN.
		real_test_prov.reset();
		auto real_test_data = real_test_prov.get_data();
		auto actual_m = real_test_data.first.dimension(0);
		Tensor<float, 4> test_data = gan.get_modules()[0]->infer(Tensor<float, 4>(actual_m, 1u, 1u, 100u).random())
			.concatenate(std::move(real_test_data.first), 0);
		Tensor<float, 4> test_label = Tensor<float, 4>(actual_m, 1u, 1u, 1u).constant(0).concatenate(std::move(real_test_data.second), 0);
		MemoryDataProvider<float, 3, false, false> disc_test_prov(
			TensorPtr<float, 4>(new Tensor<float, 4>(std::move(test_data))),
			TensorPtr<float, 4>(new Tensor<float, 4>(std::move(test_label))));
		float disc_test_loss = disc_opt.test(*gan.get_modules()[1], disc_test_prov, false);
		std::cout << "\tdiscriminator test loss: " << disc_test_loss << std::endl;
		MemoryDataProvider<float, 3, false, false> gen_test_prov(
			TensorPtr<float, 4>(new Tensor<float, 4>(Tensor<float, 4>(actual_m, 1u, 1u, 100u).random())),
			TensorPtr<float, 4>(new Tensor<float, 4>(Tensor<float, 4>(actual_m, 1u, 1u, 1u).constant(1))));
		gan.get_modules()[1]->set_frozen(true);
		float gen_test_loss = gen_opt.test(gan, gen_test_prov, false);
		gan.get_modules()[1]->set_frozen(false);
		std::cout << "\tgenerator test loss: " << gen_test_loss << std::endl;
		// Generate some fake image samples to track progress.
		for (unsigned j = 0; j < 10; ++j) {
			auto fake_image_sample = gan.get_modules()[0]->infer(Tensor<float, 4>(1u, 1u, 1u, 100u).random());
			fake_image_sample = (fake_image_sample + fake_image_sample.constant(1)) * fake_image_sample.constant(127.5);
			ppm_codec.encode(TensorMap<float, 3>(fake_image_sample.data(), { 28u, 28u, 1u }), std::string("GAN/image") +
				std::to_string(i) + std::string("_") + std::to_string(j) + std::string(".pgm"));
		}
	}

}


namespace NR
{
	char const* SampleFileFolder = "mnist";
	char const* TrainImageFileName = "train-images.idx3-ubyte";
	char const* TrainLabelFileName = "train-labels.idx1-ubyte";
	char const* TestImageFileName = "t10k-images.idx3-ubyte";
	char const* TestLabelFileName = "t10k-labels.idx1-ubyte";


	using namespace Render;

	void TestANNT()
	{
#if USE_ANNT
		using namespace ANNT;
		using namespace ANNT::Neuro;

		fvector_t input =
		{
			1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9,
			2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9,
		};

		fvector_t w =
		{
			1.1, 1.2, 1.3, 1.4, 
			2.1, 2.2, 2.3, 2.4,
			3.1, 3.2, 3.3, 3.4,
			4.1, 4.2, 4.3, 4.4,
			5.1, 5.2, 5.3, 5.4,
			6.1, 6.2, 6.3, 6.4,
			0.1, 0.2, 0.3,
		};

		XConvolutionLayer layer{ 3, 3, 2 , 2 , 2 , 3 };
		fvector_t output;
		output.resize(2 * 2 * 3);
		XNetworkContext ctx{ true };
		std::vector<fvector_t*> outputs = { &output };




		layer.SetWeights(w);
		fvector_t prevDelta;
		prevDelta.resize(3 * 3 * 2);
		fvector_t gradWeights;
		gradWeights.resize(w.size());
		fvector_t delta =
		{
			0.1, 0.2, 0.3, 0.4,
			0.5, 0.6, 0.7, 0.8,
			1.1, 1.2, 1.3, 1.4,
		};
		std::vector<fvector_t*> prevDeltas = { &prevDelta };

		layer.ForwardCompute({ &input }, outputs, ctx);
		layer.BackwardCompute({ &input }, { &output }, { &delta }, prevDeltas, gradWeights, ctx);


		NNConv2DLayer layer2;
		layer2.init(IntVector3(3, 3, 2), 2, 3);
		layer2.weightOffset = 0;
		layer2.biasOffset = layer2.getWeightLength();


		fvector_t w2 =
		{
			1.1, 1.2, 1.3, 1.4,
			3.1, 3.2, 3.3, 3.4,
			5.1, 5.2, 5.3, 5.4,
			2.1, 2.2, 2.3, 2.4,
			4.1, 4.2, 4.3, 4.4,
			6.1, 6.2, 6.3, 6.4,
			0.1, 0.2, 0.3,
		};

 		NNScalar output2[2 * 2 * 3];
		NNScalar gradWeights2[30] = { 0 };
		NNScalar prevDelta2[3 * 3 * 2];

		FNNAlgo::Forward(layer2, w2.data(), input.data(), output2);
		FNNAlgo::BackwardWeight(layer2, input.data(), output2, delta.data(), gradWeights2);
		FNNAlgo::BackwardLoss(layer2, w2.data(), delta.data(), prevDelta2);

		XAveragePooling layerB{ 6, 6, 2, 2 };
		fvector_t deltaB =
		{
			0.1, 0.2, 0.3,
			0.4, 0.5, 0.6,
			0.7, 0.8, 0.9,
			1.1, 1.2, 1.3,
			1.4, 1.5, 1.6,
			1.7, 1.8, 1.9,
		};
		fvector_t outputB;
		outputB.resize(3 * 3 * 2);
		std::vector<fvector_t*> outputsB = { &outputB };
		fvector_t prevDeltaB;
		prevDeltaB.resize(6 * 6 * 2);
		std::vector<fvector_t*> prevDeltasB = { &prevDeltaB };
		layerB.ForwardCompute({ &input }, outputsB, ctx);
		layerB.BackwardProcess({ &input }, outputsB, { &deltaB }, prevDeltasB, ctx);


		NNAveragePooling2DLayer layerB2;
		layerB2.init(IntVector3(6, 6, 2), 2);

		NNScalar outputB2[3 * 3 * 2];
		NNScalar gradWeightsB2[19] = { 0 };
		NNScalar prevDeltaB2[6 * 6 * 2];

		FNNAlgo::Forward(layerB2, input.data(), outputB2);
		FNNAlgo::Backward(layerB2, deltaB.data(), prevDeltaB2);

		int i = 1;
#endif

	}
	void Test()
	{
		NNMaxPooling2DLayer layer;
		layer.init(IntVector3(6, 6, 2), 2);

		NNScalar m[] =
		{
			 1, 2, 3, 4, 5, 6,
			 7, 8, 9,10,11,12,
			13,14,15,16,17,18,
			19,20,21,22,23,24,
			25,26,27,28,29,30,
			31,32,33,34,35,36,
			 1, 2, 3, 4, 5, 6,
			 7, 8, 9,10,11,12,
			13,14,15,16,17,18,
			19,20,21,22,23,24,
			25,26,27,28,29,30,
			31,32,33,34,35,36,
		};
		NNScalar output[3 * 3 * 2];
		int inputSize[] = { 6, 6 };
		FNNAlgo::Forward(layer, m, output);

		NNScalar nn[3 * 3 * 2] =
		{
			1,2,3,
			4,5,6,
			7,8,9,
			10,11,12,
			13,14,15,
			16,17,18,
		};
		NNScalar nno[6 * 6 * 2];
		FNNAlgo::Backward(layer, m, output, nn, nno);

		int i = 1;
	}


	struct NNSequenceLayer
	{
		void addLayer(NNConv2DLayer& inoutlayer)
		{
			int numParameters = inoutlayer.getParameterLength();

			inoutlayer.weightOffset = mParameterLength;
			inoutlayer.biasOffset = mParameterLength + inoutlayer.getWeightLength();
			inoutlayer.fastMethod = NNConv2DLayer::eNone;

			mOutputOffsets[mNumLayer] = mPassOutputLength;
			++mNumLayer;
			mPassOutputLength += inoutlayer.getOutputLength();
			mMaxOutputLength = Math::Max(mMaxOutputLength, inoutlayer.getOutputLength());

			mParameterLength += numParameters;
		}

		void addLayer(NNLinearLayer& inoutlayer)
		{
			int parameterLength = inoutlayer.getParameterLength();

			inoutlayer.weightOffset = mParameterLength;
			inoutlayer.biasOffset = mParameterLength + inoutlayer.getWeightLength();

			mOutputOffsets[mNumLayer] = mPassOutputLength;
			++mNumLayer;
			mPassOutputLength += inoutlayer.getOutputLength();
			mMaxOutputLength = Math::Max(mMaxOutputLength, inoutlayer.getOutputLength());

			mParameterLength += parameterLength;
		}

		void addLayer(NNMaxPooling2DLayer& inoutlayer)
		{
			mOutputOffsets[mNumLayer] = mPassOutputLength;
			++mNumLayer;
			mPassOutputLength += inoutlayer.getOutputLength();
			mMaxOutputLength = Math::Max(mMaxOutputLength, inoutlayer.getOutputLength());
		}

		void addLayer(NNAveragePooling2DLayer& inoutlayer)
		{
			mOutputOffsets[mNumLayer] = mPassOutputLength;
			++mNumLayer;
			mPassOutputLength += inoutlayer.getOutputLength();
			mMaxOutputLength = Math::Max(mMaxOutputLength, inoutlayer.getOutputLength());
		}

		void addLayer(NNTransformLayer const& layer, int inputLength)
		{
			mOutputOffsets[mNumLayer] = mPassOutputLength;
			++mNumLayer;
			mPassOutputLength += inputLength;
			mMaxOutputLength = Math::Max(mMaxOutputLength, inputLength);
		}

		void addSoftMaxLayer(int inputLength)
		{
			mOutputOffsets[mNumLayer] = mPassOutputLength;
			++mNumLayer;
			mPassOutputLength += inputLength;
			mMaxOutputLength = Math::Max(mMaxOutputLength, inputLength);
		}

		int mParameterLength = 0;
		int mPassOutputLength = 0;
		int mOutputOffsets[32];
		int mNumLayer = 0;
		int mMaxOutputLength = 0;

		static void Randomize(NNConv2DLayer const& layer, TArrayView<NNScalar> parameters)
		{
			NNScalar* pWeight = parameters.data() + layer.weightOffset;
			NNScalar* pBias = parameters.data() + layer.biasOffset;

			float halfRange = sqrt(3.0f / (layer.dataSize[0] * layer.dataSize[1] * layer.inputSize.z));
			int numWeights = layer.getWeightLength();
			for (int i = 0; i < numWeights; ++i)
			{
				pWeight[i] = (static_cast<float_t>(rand()) / RAND_MAX) * (float_t(2) * halfRange) - halfRange;
			}
			for (int i = 0; i < layer.numNode; ++i)
			{
				pBias[i] = 0;
			}
		}

		static void Randomize(NNLinearLayer const& layer, TArrayView<NNScalar> parameters)
		{
			NNScalar* pWeight = parameters.data() + layer.weightOffset;
			NNScalar* pBias = parameters.data() + layer.biasOffset;

			float_t halfRange = sqrt(float_t(3) / layer.inputLength);

			for (int i = 0, n = layer.getWeightLength(); i < n; ++i)
			{
				pWeight[i] = (static_cast<float_t>(rand()) / RAND_MAX) * (float_t(2) * halfRange) - halfRange;
			}
			for (int i = 0; i < layer.numNode; ++i)
			{
				pBias[i] = 0;
			}

		}


	};

	struct Model : public NNSequenceLayer
	{
		Model(int const imageSize[], int numCategory)
		{
			layerReLU.setFuncionT< NNFunc::ReLU >();

			layer1.init(IntVector3(imageSize[0], imageSize[1], 1), 3, 8);
			addLayer(layer1);
			addLayer(layerReLU, layer1.getOutputLength());
			layer2.init(IntVector3(layer1.dataSize[0], layer1.dataSize[1], layer1.numNode), 3, 8);
			addLayer(layer2);
			addLayer(layerReLU, layer2.getOutputLength());

			layer3.init(layer2.getOutputSize(), 2);
			addLayer(layer3);

			layer4.init(IntVector3(layer3.dataSize[0], layer3.dataSize[1], layer3.numNode), 3, 10);
			addLayer(layer4);
			addLayer(layerReLU, layer4.getOutputLength());

			layer5.init(IntVector3(layer4.dataSize[0], layer4.dataSize[1], layer4.numNode), 3, 10);
			addLayer(layer5);
			addLayer(layerReLU, layer5.getOutputLength());

			layer6.init(layer5.getOutputSize(), 2);
			addLayer(layer6);

			layer7.init(layer6.getOutputLength(), numCategory);
			addLayer(layer7);


			addSoftMaxLayer(layer7.numNode);

			CHECK(mNumLayer == 12);
		}

		NNConv2DLayer layer1;
		NNConv2DLayer layer2;
		NNMaxPooling2DLayer layer3;
		NNConv2DLayer layer4;
		NNConv2DLayer layer5;
		NNMaxPooling2DLayer layer6;
		NNLinearLayer layer7;

		NNTransformLayer layerReLU;


		void randomize(TArrayView<NNScalar> parameters)
		{
			Randomize(layer1, parameters);
			Randomize(layer2, parameters);
			Randomize(layer4, parameters);
			Randomize(layer5, parameters);
			Randomize(layer7, parameters);
		}


		int getParameterLength() const
		{
			return mParameterLength;
		}

		int getOutputLength() const
		{
			return layer7.getOutputLength();
		}
		int getPassOutputLength() const
		{
			return mPassOutputLength;
		}
		int getTempLossGradLength() const
		{
			return 2 * mMaxOutputLength;
		}

		NNScalar* forward(
			NNScalar const parameters[],
			NNScalar const inputs[],
			NNScalar outputs[])
		{
			NNScalar const* pInput = inputs;
			pInput = FNNAlgo::Forward(layer1, parameters, pInput, outputs + mOutputOffsets[0]);
			pInput = FNNAlgo::Forward(layerReLU, layer1.getOutputLength(), pInput, outputs + mOutputOffsets[1]);
			pInput = FNNAlgo::Forward(layer2, parameters, pInput, outputs + mOutputOffsets[2]);
			pInput = FNNAlgo::Forward(layerReLU, layer2.getOutputLength(), pInput, outputs + mOutputOffsets[3]);
			pInput = FNNAlgo::Forward(layer3, pInput, outputs + mOutputOffsets[4]);
			pInput = FNNAlgo::Forward(layer4, parameters, pInput, outputs + mOutputOffsets[5]);
			pInput = FNNAlgo::Forward(layerReLU, layer4.getOutputLength(), pInput, outputs + mOutputOffsets[6]);
			pInput = FNNAlgo::Forward(layer5, parameters, pInput, outputs + mOutputOffsets[7]);
			pInput = FNNAlgo::Forward(layerReLU, layer5.getOutputLength(), pInput, outputs + mOutputOffsets[8]);
			pInput = FNNAlgo::Forward(layer6, pInput, outputs + mOutputOffsets[9]);
			pInput = FNNAlgo::Forward(layer7, parameters, pInput, outputs + mOutputOffsets[10]);

			int index = FNNMath::SoftMax(layer7.getOutputLength(), pInput, outputs + mOutputOffsets[11]);
			return outputs + mOutputOffsets[11];
		}

		void backward(
			NNScalar const parameters[],
			NNScalar const inInputs[],
			NNScalar const inOutputs[],
			NNScalar const inOutputLossGrads[],
			TArrayView<NNScalar> tempLossGrads,
			NNScalar inoutParameterGrads[],
			NNScalar* outLossGrads = nullptr)
		{

			NNScalar* pLossGrad = tempLossGrads.data();
			NNScalar* pOutputLossGrad = tempLossGrads.data() + tempLossGrads.size() / 2;

			NNScalar const* pOutput = inOutputs + mOutputOffsets[11];
			NNScalar const* pInput = inOutputs + mOutputOffsets[10];

			//FNNAlgo::SoftMaxBackward(layer7.getOutputLength(), pInput, pOutput, inOutputLossGrads, pLossGrad);

			//std::swap(pOutputLossGrad, pLossGrad);
			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[9];
			FNNAlgo::BackwardWeight(layer7, pInput, inOutputLossGrads, inoutParameterGrads);
			FNNAlgo::BackwardLoss(layer7, parameters, inOutputLossGrads, pLossGrad);

			std::swap(pOutputLossGrad, pLossGrad);
			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[8];
			FNNAlgo::Backward(layer6, pInput, pOutput, pOutputLossGrad, pLossGrad);

			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[7];
			FNNAlgo::Backward(layerReLU, layer5.getOutputLength(), pInput, pOutput, pLossGrad);

			std::swap(pOutputLossGrad, pLossGrad);
			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[6];
			FNNAlgo::BackwardWeight(layer5, pInput, pOutput, pOutputLossGrad, inoutParameterGrads);
			FNNAlgo::BackwardLoss(layer5, parameters, pOutputLossGrad, pLossGrad);

			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[5];
			FNNAlgo::Backward(layerReLU, layer4.getOutputLength(), pInput, pOutput, pLossGrad);

			std::swap(pOutputLossGrad, pLossGrad);
			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[4];
			FNNAlgo::BackwardWeight(layer4, pInput, pOutput, pOutputLossGrad, inoutParameterGrads);
			FNNAlgo::BackwardLoss(layer4, parameters, pOutputLossGrad, pLossGrad);

			std::swap(pOutputLossGrad, pLossGrad);
			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[3];
			FNNAlgo::Backward(layer3, pInput, pOutput, pOutputLossGrad, pLossGrad);

			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[2];
			FNNAlgo::Backward(layerReLU, layer2.getOutputLength(), pInput, pOutput, pLossGrad);

			std::swap(pOutputLossGrad, pLossGrad);
			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[1];
			FNNAlgo::BackwardWeight(layer2, pInput, pOutput, pOutputLossGrad, inoutParameterGrads);
			FNNAlgo::BackwardLoss(layer2, parameters, pOutputLossGrad, pLossGrad);

			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[0];
			FNNAlgo::Backward(layerReLU, layer1.getOutputLength(), pInput, pOutput, pLossGrad);

			std::swap(pOutputLossGrad, pLossGrad);
			pOutput = pInput;
			pInput = inInputs;
			FNNAlgo::BackwardWeight(layer1, pInput, pOutput, pOutputLossGrad, inoutParameterGrads);
			if (outLossGrads)
			{
				FNNAlgo::BackwardLoss(layer1, parameters, pOutputLossGrad, outLossGrads);
			}
		}
	};


	struct ModelB : public NNSequenceLayer
	{
		ModelB(int const imageSize[], int numCategory)
		{
			layerReLU.setFuncionT< NNFunc::LeakyReLU >();

			//28
			layer1.init(IntVector3(imageSize[0], imageSize[1], 1), 5, 6);
			addLayer(layer1);
			addLayer(layerReLU, layer1.getOutputLength());

			//24
			layer2.init(layer1.getOutputSize(), 2);
			addLayer(layer2);

			//12
			layer3.init(IntVector3(layer2.dataSize[0], layer2.dataSize[1], layer2.numNode), 5, 16);
			addLayer(layer3);
			addLayer(layerReLU, layer3.getOutputLength());

			//8
			layer4.init(layer3.getOutputSize(), 2);
			addLayer(layer4);

			//4
			layer5.init(IntVector3(layer4.dataSize[0], layer4.dataSize[1], layer2.numNode), 4, 120);
			addLayer(layer5);
			addLayer(layerReLU, layer5.getOutputLength());

			layer6.init(layer5.getOutputLength(), numCategory);
			addLayer(layer6);

			addSoftMaxLayer(layer6.numNode);
			CHECK(mNumLayer == 10);
		}

		NNConv2DLayer layer1;
		NNAveragePooling2DLayer layer2;
		NNConv2DLayer layer3;
		NNAveragePooling2DLayer layer4;
		NNConv2DLayer layer5;
		NNLinearLayer layer6;

		NNTransformLayer layerReLU;


		void randomize(TArrayView<NNScalar> parameters)
		{
			Randomize(layer1, parameters);
			Randomize(layer3, parameters);
			Randomize(layer5, parameters);
			Randomize(layer6, parameters);
		}

		int getParameterLength() const
		{
			return mParameterLength;
		}

		int getOutputLength() const
		{
			return layer6.getOutputLength();
		}
		int getPassOutputLength() const
		{
			return mPassOutputLength;
		}
		int getTempLossGradLength() const
		{
			return 2 * mMaxOutputLength;
		}

		NNScalar* forward(
			NNScalar const parameters[],
			NNScalar const inputs[],
			NNScalar outputs[])
		{
			NNScalar const* pInput = inputs;
			pInput = FNNAlgo::Forward(layer1, parameters, pInput, outputs + mOutputOffsets[0]);
			pInput = FNNAlgo::Forward(layerReLU, layer1.getOutputLength(), pInput, outputs + mOutputOffsets[1]);
			pInput = FNNAlgo::Forward(layer2, pInput, outputs + mOutputOffsets[2]);

			pInput = FNNAlgo::Forward(layer3, parameters, pInput, outputs + mOutputOffsets[3]);
			pInput = FNNAlgo::Forward(layerReLU, layer3.getOutputLength(), pInput, outputs + mOutputOffsets[4]);
			pInput = FNNAlgo::Forward(layer4, pInput, outputs + mOutputOffsets[5]);
			
			pInput = FNNAlgo::Forward(layer5, parameters, pInput, outputs + mOutputOffsets[6]);
			pInput = FNNAlgo::Forward(layerReLU, layer4.getOutputLength(), pInput, outputs + mOutputOffsets[7]);

			pInput = FNNAlgo::Forward(layer6, parameters, pInput, outputs + mOutputOffsets[8]);

			int index = FNNMath::SoftMax(layer6.getOutputLength(), pInput, outputs + mOutputOffsets[9]);
			return outputs + mOutputOffsets[9];
		}

		void backward(
			NNScalar const parameters[],
			NNScalar const inInputs[],
			NNScalar const inOutputs[],
			NNScalar const inOutputLossGrads[],
			TArrayView<NNScalar> tempLossGrads,
			NNScalar inoutParameterGrads[],
			NNScalar* outLossGrads = nullptr)
		{

			NNScalar* pLossGrad = tempLossGrads.data();
			NNScalar* pOutputLossGrad = tempLossGrads.data() + tempLossGrads.size() / 2;

			NNScalar const* pOutput = inOutputs + mOutputOffsets[9];
			NNScalar const* pInput = inOutputs + mOutputOffsets[8];

			//FNNAlgo::SoftMaxBackward(layer7.getOutputLength(), pInput, pOutput, inOutputLossGrads, pLossGrad);

			//std::swap(pOutputLossGrad, pLossGrad);
			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[7];
			FNNAlgo::BackwardWeight(layer6, pInput, inOutputLossGrads, inoutParameterGrads);
			FNNAlgo::BackwardLoss(layer6, parameters, inOutputLossGrads, pLossGrad);

			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[6];
			FNNAlgo::Backward(layerReLU, layer4.getOutputLength(), pInput, pOutput, pLossGrad);

			std::swap(pOutputLossGrad, pLossGrad);
			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[5];
			FNNAlgo::BackwardWeight(layer5, pInput, pOutput, pOutputLossGrad, inoutParameterGrads);
			FNNAlgo::BackwardLoss(layer5, parameters, pOutputLossGrad, pLossGrad);

			std::swap(pOutputLossGrad, pLossGrad);
			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[4];
			FNNAlgo::Backward(layer4, pOutputLossGrad, pLossGrad);

			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[3];
			FNNAlgo::Backward(layerReLU, layer3.getOutputLength(), pInput, pOutput, pLossGrad);

			std::swap(pOutputLossGrad, pLossGrad);
			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[2];
			FNNAlgo::BackwardWeight(layer3, pInput, pOutput, pOutputLossGrad, inoutParameterGrads);
			FNNAlgo::BackwardLoss(layer3, parameters, pOutputLossGrad, pLossGrad);

			std::swap(pOutputLossGrad, pLossGrad);
			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[1];
			FNNAlgo::Backward(layer2, pOutputLossGrad, pLossGrad);

			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[0];
			FNNAlgo::Backward(layerReLU, layer1.getOutputLength(), pInput, pOutput, pLossGrad);

			std::swap(pOutputLossGrad, pLossGrad);
			pOutput = pInput;
			pInput = inInputs;
			FNNAlgo::BackwardWeight(layer1, pInput, pOutput, pOutputLossGrad, inoutParameterGrads);
			if (outLossGrads)
			{
				FNNAlgo::BackwardLoss(layer1, parameters, pOutputLossGrad, outLossGrads);
			}
		}
	};





	class TestThread : public RunnableThreadT< TestThread >
	{
	public:
		unsigned run()
		{
			Test_C_ATTL3();
			return 0;
		}

		bool init() { return true; }
		void exit() 
		{
			delete this;
		}
	};

	class TestStage : public StageBase
		            , public IGameRenderSetup
	{
		typedef StageBase BaseClass;
	public:
		TestStage()
			:mModel(ImageSize, 10)
		{

		}

		virtual bool onInit()
		{
			if (!BaseClass::onInit())
				return false;


			auto thread = new TestThread();
			thread->start();

			TestANNT();
			Test();

			loadTrainSample();
			loadTestSample();
			mOrderedSamples.resize(mTrainSamples.size());
			for (int i = 0; i < mTrainSamples.size(); ++i)
			{
				mOrderedSamples[i] = mTrainSamples[i];
			}

			mMainData.init(mModel);
			initParameters();
			mOptimizer.init(mModel.getParameterLength());


			::Global::GUI().cleanupWidget();
			auto frame = WidgetUtility::CreateDevFrame();

			frame->addButton("Clear", [this](int event, GWidget*) -> bool
			{
				return false;
			});
			frame->addCheckBox("bAutoTrain", bAutoTrain);


			restart();

			return true;
		}


		struct SampleData
		{
			TArray<NNScalar> image;
			uint8 label;
		};


		static int constexpr ImageSize[2] = {28 , 28 };

		TArray< SampleData* > mTrainSamples;
		TArray< SampleData* > mTestSamples;
		TArray< SampleData* > mOrderedSamples;
		Model mModel;
		AdamOptimizer mOptimizer;



		static void EndianSwap(uint32& value)
		{
			uint8* ptr = (uint8*)&value;
			std::swap(ptr[0], ptr[3]);
			std::swap(ptr[1], ptr[2]);
		}

		bool loadTrainSample()
		{
			return loadSample(TrainImageFileName, TrainLabelFileName, mTrainSamples);
		}
		bool loadTestSample()
		{
			return loadSample(TestImageFileName, TestLabelFileName, mTestSamples);
		}
		bool loadSample(char const* imageFileName, char const* labelFileName, TArray<SampleData*>& outDatas)
		{
			InlineString<> imagePath;
			imagePath.format("%s/%s", SampleFileFolder, imageFileName);

			InputFileSerializer imageSerializer;
			if (!imageSerializer.openNoHeader(imagePath))
				return false;

			struct ImageHeader
			{
				uint32 magic;
				uint32 sampleNum;
				uint32 sizeX;
				uint32 sizeY;
			};

			ImageHeader imageHeader;
			imageSerializer >> imageHeader;
			if (imageHeader.magic != 0x3080000)
				return false;

			EndianSwap(imageHeader.sampleNum);
			EndianSwap(imageHeader.sizeX);
			EndianSwap(imageHeader.sizeY);

			if(imageHeader.sizeX != ImageSize[0] || imageHeader.sizeY != ImageSize[1])
				return false;

			TArray<uint8> image;
			image.resize(ImageSize[0] * ImageSize[1]);

			for (int i = 0; i < imageHeader.sampleNum; ++i)
			{
				SampleData* trainData = new SampleData;
				trainData->image.resize( ImageSize[0] * ImageSize[1] );
				imageSerializer.read( image.data(), image.size());
				for (int n = 0; n < image.size(); ++n)
				{
					trainData->image[n] = NNScalar(image[n]) / 255.0f;
				}

				outDatas.push_back(trainData);
			}

			InlineString<> labelPath;
			labelPath.format("%s/%s", SampleFileFolder, labelFileName);

			InputFileSerializer labelSerializer;
			if (!labelSerializer.openNoHeader(labelPath))
				return false;

			struct LabelHeader
			{
				uint32 magic;
				uint32 sampleNum;
			};

			LabelHeader labelHeader;
			labelSerializer >> labelHeader;
			if (labelHeader.magic != 0x1080000)
				return false;

			EndianSwap(labelHeader.sampleNum);
			if (imageHeader.sampleNum != labelHeader.sampleNum)
				return false;

			for (int i = 0; i < imageHeader.sampleNum; ++i)
			{
				SampleData* trainData = outDatas[i];
				labelSerializer.read(trainData->label);
			}

			return true;
		}

		RHITexture2DRef mTexShow;

		virtual bool setupRenderResource(ERenderSystem systemName)
		{
			mTexShow = RHICreateTexture2D(TextureDesc::Type2D(ETexture::R32F, ImageSize[0], ImageSize[1]));

			updateShowImage();
			return true;
		}

		virtual void onEnd()
		{
			BaseClass::onEnd();
		}

		struct TrainData
		{
			TArray< NNScalar > outputs;
			TArray< NNScalar > lossGrads;
			TArray< NNScalar > parameterGrads;
			TArray< NNScalar > batchNormParameters;
			NNScalar loss;
			int count;

			template< typename TModel >
			void init(TModel& model, int batchSize = 1)
			{
				batchSize = 1;
				outputs.resize(batchSize * model.getPassOutputLength());
				lossGrads.resize(batchSize * model.getTempLossGradLength());
				parameterGrads.resize(model.getParameterLength());
			}

			void reset()
			{
				std::fill_n(parameterGrads.data(), parameterGrads.size(), 0.0);
				loss = 0;
				count = 0;
			}

		};
		bool bAutoTrain = false;
		TrainData mMainData;
		void initParameters()
		{
			mParameters.resize(mModel.getParameterLength());

			mModel.randomize(mParameters);
#if 0

			NNScalar v = 0.5 * Math::Sqrt(2.0 / (ImageSize[0] * ImageSize[1]));
			for (NNScalar& x : mParameters)
			{
				x = RandFloat(0, v);
			}
#endif
		}

		NNScalar fit(SampleData const& sample, TrainData& trainData)
		{
			NNScalar const* pOutput = mModel.forward(mParameters.data(), sample.image.data(), trainData.outputs.data());

			int index = FNNMath::Max(mModel.getOutputLength(), pOutput);
			if (index == sample.label)
			{
				++trainData.count;
			}

			NNScalar lossGrads[10];
			NNScalar loss = 0;
			for (int i = 0; i < mModel.getOutputLength(); ++i)
			{
				NNScalar label = (sample.label == i) ? 1.0 : 0.0;
				lossGrads[i] = LossFunc::CalcSoftMaxDevivative(pOutput[i], label);
				loss += LossFunc::Calc(pOutput[i], label);
			}

			mModel.backward(mParameters.data(), sample.image.data(), trainData.outputs.data(), lossGrads, trainData.lossGrads, trainData.parameterGrads.data());

			return loss;
		}

		using LossFunc = FCrossEntropyLoss;

		TArray<NNScalar> mParameters;

		float learnRate = 1e-3;

		int mEpoch = 0;


		NNScalar trainBatch(TArrayView<SampleData*> samples)
		{
			mMainData.reset();
			NNScalar loss = 0;
			for (int i = 0; i < samples.size(); ++i)
			{
				loss += fit(*samples[i], mMainData);
			}

			for (int i = 0; i < mMainData.parameterGrads.size(); ++i)
			{
				auto& parameterGrad = mMainData.parameterGrads[i];
				parameterGrad /= samples.size();
#if 1
				if (!FNNMath::IsValid(parameterGrad))
				{
					parameterGrad = Math::Sign(parameterGrad) * 1e15;
				}
#endif

				parameterGrad += 2 * 0.001 * mParameters[i];
			}
			loss /= (float)samples.size();
			FNNMath::ClipNormalize(mMainData.parameterGrads.size(), mMainData.parameterGrads.data(), 1.0);

			mOptimizer.update(mParameters, mMainData.parameterGrads, learnRate);

#if 1
			for (int i = 0; i < mParameters.size(); ++i)
			{
				if (!FNNMath::IsValid(mParameters[i]))
				{
					LogMsg("AAAAAAAAAA, %g", mParameters[i]);
					break;
				}
			}
#endif

			return loss;
		}

		template< typename Func >
		void train(double timeLimit, Func& timeLimitCB)
		{
			uint64 startTime;
			Profile_GetTicks(&startTime);

			int const BatchSize = 30;
			std::mt19937 g(::rand());
			for (mEpoch = 0; mEpoch < 20; ++mEpoch)
			{

				std::shuffle(mOrderedSamples.begin(), mOrderedSamples.end(), g);

				int numBatch = (mOrderedSamples.size() + BatchSize - 1) / BatchSize;

				for (int i = 0; i < numBatch; ++i)
				{
					auto pSample = mOrderedSamples.data() + i * BatchSize;
					int numSample = Math::Min<int>(mOrderedSamples.size() - i * BatchSize, BatchSize);

					float loss = trainBatch(TArrayView<SampleData*>(pSample, numSample));
					LogMsg("%d %d: Loss = %f, %d / %d", mEpoch, i, loss, mMainData.count, numSample);

					if (timeLimit > 0)
					{
						uint64 endTime;
						Profile_GetTicks(&endTime);
						if (double(endTime - startTime) / 1000.0 > timeLimit)
						{
							startTime = endTime;
							timeLimitCB();
						}
					}
				}

				testSamples();
			}
		}



		virtual void onUpdate(GameTimeSpan deltaTime)
		{
			BaseClass::onUpdate(deltaTime);

			if (bAutoTrain)
			{
				Coroutines::Resume(mTrainHandle);
				test();
			}
		}

		Coroutines::ExecutionHandle mTrainHandle;
		
		void restart()
		{
			mTrainHandle = Coroutines::Start([this]
			{
				train(15, []()
				{
					CO_YEILD(nullptr);
				});
			});
		}

		int pred = 0;


		void onRender(float dFrame) override
		{
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			auto screenSize = ::Global::GetScreenSize();


			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.0, 0.0, 0.0, 0.0), 1);
			RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);

			RHIGraphics2D& g = ::Global::GetRHIGraphics2D();

			g.beginRender();
			RenderUtility::SetBrush(g, EColor::White);
			g.drawTexture(*mTexShow, Vector2(20,20), Vector2(200,200));
			RenderUtility::SetPen(g, EColor::White);
			RenderUtility::SetBrush(g, EColor::Null);
			g.drawRect(Vector2(20, 20), Vector2(200, 200));


			g.drawTextF(Vector2(10, 10), "Index = %d Number = %d Pred = %d", indexShow, mTestSamples[indexShow]->label, pred);
			g.endRender();
		}

		void testSamples()
		{
			int num = 0;
			for (int i = 0; i < mTestSamples.size(); ++i)
			{
				auto sample = mTestSamples[i];
				NNScalar* output = mModel.forward(mParameters.data(), sample->image.data(), mMainData.outputs.data());
				int pred = FNNMath::Max(10, output);
				if (pred == sample->label)
				{
					++num;
				}
			}

			LogMsg("%d: %f %%", mEpoch, float(100 * num) / mTestSamples.size());
		}


		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}

		void test()
		{
			indexShow = rand() % mTestSamples.size();
			auto sample = mTestSamples[indexShow];
			NNScalar* output = mModel.forward(mParameters.data(), sample->image.data(), mMainData.outputs.data());
			pred = FNNMath::Max(10, output);
			updateShowImage();
		}

		void updateShowImage()
		{
			auto sample = mTestSamples[indexShow];
			RHIUpdateTexture(*mTexShow, 0, 0, ImageSize[0], ImageSize[1], sample->image.data());
		}

		int indexShow = 0;
		MsgReply onKey(KeyMsg const& msg)
		{
			switch (msg.getCode())
			{
			case EKeyCode::X: ++indexShow; if (indexShow >= mTestSamples.size()) indexShow = 0; updateShowImage(); break;
			case EKeyCode::Z: --indexShow; if (indexShow < 0) indexShow = mTestSamples.size() - 1; updateShowImage(); break;
			default:
				break;
			}
			return MsgReply::Unhandled();
		}

		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
		{
			systemConfigs.screenWidth = 1080;
			systemConfigs.screenHeight = 768;
		}

	};

	REGISTER_STAGE_ENTRY("Number Recognizer", TestStage, EExecGroup::Dev, "AI");
}