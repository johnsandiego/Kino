#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/core/ocl.hpp>

#include "KinoGlobals.hpp"

#include "gui/UsesGUI.hpp"

#include <lenses/edge_detector/EdgeDetectorLens.hpp>
#include <lenses/face_detector/FaceDetectorLens.hpp>
#include <lenses/image_classifier/ClassifierLens.hpp>

#include "camera/CameraCapture.hpp"

#include "ofxTimeMeasurements.h"

/* 
Holds the code for grabbing images from a camera and performing 
OpenCV operations on them. Should not have any engine-specific code
(i.e. OpenFrameworks or Cinder code).
*/
class KinoCore: public UsesGUI {
public:

	KinoCore();
	~KinoCore();

	void Setup();
	void Update();

	void PrintCVDebugInfo();

	void DrawGUI();
	void DrawAllGUIs();

	EdgeDetectorLens edgeDetector;
	FaceDetectorLens faceDetector;
	ClassifierLens classifierLens;

	cv::UMat leftMat = cv::UMat();
	cv::UMat rightMat = cv::UMat();

	CameraCapture* capture1;
	CameraCapture* capture2;

	void ProcessCapture(CameraCapture *cap, cv::OutputArray output, string id);

	bool pauseCaptureUpdates = false;

private:
	cv::Mat rawFrame;
};