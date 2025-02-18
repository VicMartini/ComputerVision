
#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/dnn/all_layers.hpp>
#include <vector>
#include <opencv2/core.hpp>

using namespace std;
using namespace cv;
using namespace dnn;

int clip(int n, int lower, int upper)
{
    return std::max(lower, std::min(n, upper));
}

vector<string> getOutputsNames(const cv::dnn::Net &net)
{
    static vector<string> names;
    if (names.empty())
    {
        std::vector<int32_t> out_layers = net.getUnconnectedOutLayers();
        std::vector<string> layers_names = net.getLayerNames();
        names.resize(out_layers.size());
        for (size_t i = 0; i < out_layers.size(); ++i)
        {
            names[i] = layers_names[out_layers[i] - 1];
        }
    }
    return names;
}

int main(int argc, char **argv)
{
    std::vector<string> args(argv, argv + argc);
    bool useSmallModel = true;
    if (std::find(args.begin(), args.end(), "--small") != args.end())
    {
        useSmallModel = true;
    }

    // Use iPhone with Continuity Camera
    bool useContinuityCamera = false;
    if (std::find(args.begin(), args.end(), "--continuity") != args.end())
    {
        useContinuityCamera = true;
    }

    // Read Network
    string model = useSmallModel ? "./weights/model-small.onnx" : "./weights/model-f6b98070.onnx";

    // Read in the neural network from the files
    auto net = readNet(model);

    if (net.empty())
    {
        return -1;
    }
    std::cout << "Model loaded" << std::endl;

    // Run using OpenCL
    net.setPreferableBackend(DNN_BACKEND_OPENCV);
    net.setPreferableTarget(DNN_TARGET_OPENCL);

    // Open up the webcam
    VideoCapture cap(useContinuityCamera ? 1 : 0);

    // Loop running as long as webcam is open and "q" is not pressed
    while (cap.isOpened())
    {

        // Load in an image
        Mat image;
        cap.read(image);

        if (image.empty())
        {
            cv::waitKey(0);
            cout << "image not available!" << endl;
            break;
        }

        int image_width = image.rows;
        int image_height = image.cols;

        auto start = getTickCount();

        // Create Blob from Input Image
        Mat blob;
        if (useSmallModel)
        {
            // MiDaS v2.1 Small ( Scale : 1 / 255, Size : 256 x 256, Mean Subtraction : ( 123.675, 116.28, 103.53 ), Channels Order : RGB )
            blob = blobFromImage(image, 1 / 255.f, cv::Size(256, 256), cv::Scalar(123.675, 116.28, 103.53), true, false);
        }
        else
        {
            // MiDaS v2.1 Large ( Scale : 1 / 255, Size : 384 x 384, Mean Subtraction : ( 123.675, 116.28, 103.53 ), Channels Order : RGB )
            blob = blobFromImage(image, 1 / 255.f, cv::Size(384, 384), cv::Scalar(123.675, 116.28, 103.53), true, false);
        }

        // Set the blob to be input to the neural network
        net.setInput(blob);

        // Forward pass of the blob through the neural network to get the predictions
        Mat output = net.forward(getOutputsNames(net)[0]);

        // Convert Size to 384x384 from 1x384x384
        const std::vector<int32_t> size = {output.size[1], output.size[2]};
        output = cv::Mat(static_cast<int32_t>(size.size()), &size[0], CV_32F, output.ptr<float>());

        // Resize Output Image to Input Image Size
        cv::resize(output, output, image.size());

        // Visualize Output Image
        double min, max;
        cv::minMaxLoc(output, &min, &max);
        const double range = max - min;

        // 1. Normalize ( 0.0 - 1.0 )
        output.convertTo(output, CV_32F, 1.0 / range, -(min / range));

        // 2. Scaling ( 0 - 255 )
        output.convertTo(output, CV_8U, 255.0);

        // 3. Apply Color Map
        Mat coloredImage;
        applyColorMap(output, coloredImage, COLORMAP_JET);

        auto end = getTickCount();
        auto totalTime = (end - start) / getTickFrequency();
        auto fps = 1 / totalTime;

        putText(coloredImage, "FPS: " + to_string(int(fps)), Point(25, 25), FONT_HERSHEY_PLAIN, 1, Scalar(255, 255, 255), 2, false);

        // imshow("image", image);
        imshow("depth", coloredImage);

        if (waitKey(1) == 'q')
        {
            break;
        }
    }

    cap.release();
    destroyAllWindows();
}