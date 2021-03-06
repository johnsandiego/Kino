#include "EdgeDetectorModule.hpp"

EdgeDetectorModule::EdgeDetectorModule() {

}

EdgeDetectorModule::~EdgeDetectorModule() {

}

void EdgeDetectorModule::ProcessFrames(InputArray inLeft, InputArray inRight, OutputArray outLeft, OutputArray outRight) {
	ProcessFrame(inLeft, outLeft);
	if (!inRight.empty()) {
		ProcessFrame(inRight, outRight);
	}
}

void EdgeDetectorModule::ProcessFrame(cv::InputArray in, cv::OutputArray out) {

	if (in.empty()) {
		return;
	}

	cv::UMat src;
	in.copyTo(src);

	TS_START_NIF("Copy In");
	cv::UMat latestStep;
	cv::UMat finalFrame;
	cv::UMat gray;
	TS_STOP_NIF("Copy In");

	int originalWidth = src.cols;
	int originalHeight = src.rows;

	if (doScreenshot) {
		cv::imwrite("Edge Detection 1.png", src);
	}

	//Downsample
	if (doDownsampling) {
		TS_START_NIF("Downsample");
		cv::resize(src, latestStep, cv::Size(), downSampleRatio, downSampleRatio, INTER_NEAREST);
		TS_STOP_NIF("Downsample");
	}
	else {
		src.copyTo(latestStep);
	}

	//Condense the source image into a single channel for use with the Canny algorithm
	TS_START_NIF("Condense Channels");
	CondenseImage(latestStep, gray, currentChannelType);
	TS_STOP_NIF("Condense Channels");

	//threshold(edges, edges, 0, 255, THRESH_BINARY | THRESH_OTSU);

	//Blur the source image to reduce noise and texture details
	TS_START_NIF("Blur");
	//cv::Mat temp;
	//latestStep.copyTo(temp);
	////temp = latestStep.clone();
	BlurImage(gray, gray, currentBlurType);
	//temp.copyTo(latestStep);
	TS_STOP_NIF("Blur");

	//Super, super slow
	//fastNlMeansDenoising(latestStep, latestStep, 3, 7, 21);

	//Perform erosion and dilution if requested
	if (doErosionDilution) {
		TS_START_NIF("Erode / Dilute");
		erode(gray, gray, UMat(), cv::Point(-1, -1), erosionIterations, BORDER_CONSTANT, morphologyDefaultBorderValue());
		dilate(gray, gray, UMat(), cv::Point(-1, -1), dilutionIterations, BORDER_CONSTANT, morphologyDefaultBorderValue());
		TS_STOP_NIF("Erode / Dilute");
	}

	//Perform edge detection

	//Canny step--------------------------------------
	//If the image has more than one color channel, then it wasn't condensed.
	//Divide it up and run Canny on each channel.
	//TODO: This should probably be run without any blurring beforehand.
	TS_START_NIF("Canny");
	if (gray.channels() > 1) {
		std::vector<cv::Mat> channels;
		cv::split(gray, channels);

		//Separate the three color channels and perform Canny on each
		Canny(channels[0], channels[0], cannyThresholdLow, cannyThresholdLow * cannyThresholdRatio, 3);
		Canny(channels[1], channels[1], cannyThresholdLow, cannyThresholdLow * cannyThresholdRatio, 3);
		Canny(channels[2], channels[2], cannyThresholdLow, cannyThresholdLow * cannyThresholdRatio, 3);

		//Merge the three color channels into one image
		//merge(channels, canny_output);
		bitwise_or(channels[0], channels[1], channels[1]); //Merge 0 and 1 into 1
		bitwise_or(channels[1], channels[2], gray); //Merge 1 and 2 into result
	}
	else {
		Canny(gray, gray, cannyThresholdLow, cannyThresholdLow * cannyThresholdRatio, 3); //0, 30, 3
	}
	TS_STOP_NIF("Canny");

	//Contour step------------------------------------
	if (useContours) {
		TS_START_NIF("Contour Detection");
		cv::UMat contourOutput;
		gray.copyTo(contourOutput);
		{
			cv::Mat grayMat = gray.getMat(cv::ACCESS_READ);
			cv::Mat contourOutputMat = contourOutput.getMat(cv::ACCESS_RW);
			ParallelContourDetector::DetectContoursParallel(grayMat, contourOutputMat, contourSubdivisions, lineThickness, minContourSize);
			// all getMat copies need to be deallocated before we continue, hence scoping
		}
		//ParallelContourDetector::DetectContours(gray, contourOutput, lineThickness);
		contourOutput.copyTo(gray);
		TS_STOP_NIF("Contour Detection");
	}

	//Restore original image size
	if (doDownsampling) {
		TS_START_NIF("Upscale");
		cv::UMat grayCopy = gray; // Make shallow copy since resize is not "in-place"
		cv::resize(grayCopy, gray, cv::Size(originalWidth, originalHeight), INTER_NEAREST);
		TS_STOP_NIF("Upscale");
	}

	//Output step-------------------------------------
	TS_START_NIF("Create Output");
	if (showEdgesOnly) {
		cvtColor(gray, finalFrame, COLOR_GRAY2BGR);
	}
	else {
		in.copyTo(finalFrame);
	}
	cv::Scalar finalColor = ColorToScalar(lineColor);
	if (useSmartLineColors) {
		//finalColor = cv::Scalar::all(255) - cv::mean(in);
		finalColor = cv::mean(in);
		averageColor = ImVec4(finalColor[2] / 255.0f, finalColor[1] / 255.0f, finalColor[0] / 255.0f, 1.0f);

		//Convert BGR to HSL
		int r = finalColor[2];
		int g = finalColor[1];
		int b = finalColor[0];

		int h = 0;
		int s = 0;
		int l = 0;

		RGBtoHSL(r, g, b, h, s, l);

		h = (h + 180) % 360; //Get complementary color
		s = 100;
		l = (l + 50) % 100; //Make it dark if it's light, make it light if it's dark (?)

		HSLtoRGB(h, s, l, r, g, b);

		finalColor = cv::Scalar(b, g, r);
		complementaryColor = ImVec4(finalColor[2] / 255.0f, finalColor[1] / 255.0f, finalColor[0] / 255.0f, 1.0f);

	}
	finalFrame.setTo(finalColor, gray);

	if (doScreenshot) {
		Mat justLines(finalFrame.rows, finalFrame.cols, CV_8UC3, Scalar(0, 0, 0));
		Mat justLinesAlpha(finalFrame.rows, finalFrame.cols, CV_8UC4, Scalar(0, 0, 0, 0));
		justLines.setTo(finalColor, gray);
		justLinesAlpha.setTo(Scalar(finalColor[0], finalColor[1], finalColor[2], 1.0), gray);
		cv::imwrite("Edge Detection 2.png", justLines);
		cv::imwrite("Edge Detection 2 alpha.png", justLinesAlpha);
	}

	if (doScreenshot) {
		cv::imwrite("Edge Detection 3.png", finalFrame);
	}

	//Copy the result of all operations to the output frame.
	finalFrame.copyTo(out);
	TS_STOP_NIF("Create Output");

	gray.release();
	latestStep.release();
	finalFrame.release();

	if (doScreenshot) doScreenshot = false;

}


/**
 Condenses the input image into one channel based on the ChannelType specified.
 */
void EdgeDetectorModule::CondenseImage(cv::InputArray in, cv::OutputArray out, int channelType = 0) {
	if (channelType == ChannelType::GRAYSCALE) {
		cvtColor(in, out, COLOR_BGR2GRAY);
	}
	else if (channelType == ChannelType::HUE) {
		//http://stackoverflow.com/questions/29156091/opencv-edge-border-detection-based-on-color
		std::vector<cv::Mat> channels;
		cv::Mat hsv;
		cv::cvtColor(in, hsv, CV_BGR2HSV);
		cv::split(hsv, channels);
		channels[0].copyTo(out);
		//cv::Mat shiftedH;
		//out.copyTo(shiftedH);
		//int shift = 25; // in openCV hue values go from 0 to 180 (so have to be doubled to get to 0 .. 360) because of byte range from 0 to 255
		//for (int j = 0; j<shiftedH.rows; ++j)
		//	for (int i = 0; i<shiftedH.cols; ++i)
		//	{
		//		shiftedH.at<unsigned char>(j, i) = (shiftedH.at<unsigned char>(j, i) + shift) % 180;
		//	}
		//shiftedH.copyTo(out);
		cv::imshow("wow", out);
		hsv.release();
	}
	else if (channelType == ChannelType::COLOR) {
		in.copyTo(out);
		//Do nothing
	}
}

/**
 Applies a blurring operation to the image based on blurType. Defaults to, uh,
 BlurType::DEFAULT, which is the regular OpenCV kernel blur function.
 */
void EdgeDetectorModule::BlurImage(cv::InputArray in, cv::OutputArray out, int blurType = 0) {
	switch (blurType) {
	default:
	case(BlurType::DEFAULT): //Homogeneous
		blur(in, out, cv::Size(7, 7));
		break;
	case(BlurType::GAUSSIAN):
		GaussianBlur(in, out, cv::Size(7, 7), 1.5, 1.5);
		break;
	case(BlurType::NONE):
		break;

	}
}

void EdgeDetectorModule::DrawGUI() {

	ImGui::SliderInt("Low Threshold", &cannyThresholdLow, 0, 255);
	ShowHelpMarker("Set lower to look for more edges.");
	ImGui::SliderFloat("Ratio", &cannyThresholdRatio, 2.0, 3.0);
	ShowHelpMarker("Fine tune edge results.");
	ImGui::Checkbox("Use Smart Line Colors", &useSmartLineColors);
	if (!useSmartLineColors) {
		//ImGui::ColorEdit3("Line Color", (float*)&lineColor);
		ImGuiExtensions::ColorPicker("Line Color", lineColor);
		ShowHelpMarker("Click the color to show the editor.\nDouble click a field to type your own value.");
	}
	else {
		ImGui::ValueColor("Average", averageColor);
		//ImGui::SameLine();
		ImGui::ValueColor("Complementary", complementaryColor);
	}
	ImGui::Checkbox("Edges Only", &showEdgesOnly);
	ShowHelpMarker("Show just the edges.");
	ImGui::Combo("Channel Type", &currentChannelType, channelTypes);

	//Contour settings
	ImGui::Spacing();
	ImGui::Text("Contour Settings");
	ImGui::Separator();
	ImGui::Checkbox("Use Contours", &useContours);
	ShowHelpMarker("Use contour detection to filter out short lines and noise in the edge data.");
	if (!useContours) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.2); //Push disabled style
	ImGui::SliderInt("Subdivisions", &contourSubdivisions, 1, 16);
	ShowHelpMarker("Number of chunks the image is divided into for parallel processing.");
	ImGui::SliderInt("Thickness", &lineThickness, -1, 8);
	ImGui::InputFloat("Minimum Length", &minContourSize);
	ShowHelpMarker("Any contours smaller than this length will be ignored.");
	if (!useContours) ImGui::PopStyleVar(); //Pop disabled style

	ImGui::Spacing();
	ImGui::Text("Image Tuning");
	ImGui::Separator();
	ImGui::Combo("Blur Type", &currentBlurType, blurTypes);
	ImGui::Checkbox("Erosion/Dilution", &doErosionDilution);
	ShowHelpMarker("Try to keep erosion and dilution at the same value.");
	if (!doErosionDilution) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.2); //Push disabled style
	ImGui::SliderInt("Erosion Iterations", &erosionIterations, 0, 6);
	ShowHelpMarker("Morphological open operation; removes bright spots on a dark background.");
	ImGui::SliderInt("Dilution Iterations", &dilutionIterations, 0, 6);
	ShowHelpMarker("Morphologial close operation; removes dark spots on a bright background.");
	if (!doErosionDilution) ImGui::PopStyleVar(); //Pop disabled style

	ImGui::Checkbox("Downsample", &doDownsampling);
	if (!doDownsampling) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.2); //Push disabled style
	ImGui::SliderFloat("Downsample Ratio", &downSampleRatio, 0.01f, 1.0f, "%.2f");
	ShowHelpMarker("Multiplier for decreasing the resolution of the processed image.");
	if (!doDownsampling) ImGui::PopStyleVar(); //Pop disabled style

	ImGui::Spacing();
	if (ImGui::Button("Screenshot")) {
		doScreenshot = true;
	}

}