//
//  ParallelContourDetector.cpp
//  FaceOff
//
//  Created by Jonathan Cole on 10/29/15.
//
//

#include "ParallelContourDetector.hpp"

using namespace cv;
using namespace std;

/**
 Constructor.
 */
ParallelContourDetector::ParallelContourDetector(cv::Mat& in, cv::Mat& out, int _subsections, int _lineThickness, float _minContourSize) {
	//in.copyTo(input);
	//out.copyTo(output);
	input = in;
	output = out;
	subsections = _subsections;
	lineThickness = _lineThickness;
	minContourSize = _minContourSize;

}

/**
 Runs parallel contour detection without the need for an instance of ParallelContourDetector.
 */
void ParallelContourDetector::DetectContoursParallel(cv::Mat in, cv::Mat& out,
	const int subsections, const int lineThickness, const float minContourSize) {

	//std::cout << "Output was empty\n";
	cv::parallel_for_(cv::Range(0, subsections), ParallelContourDetector(in, out, subsections, lineThickness, minContourSize));

}

void ParallelContourDetector::operator ()(const cv::Range &range) const {

	for (int i = range.start; i != range.end; i++) {
		vector< vector<cv::Point> > contourData;
		vector<Vec4i> contourHierarchy;
		cv::Mat in(input, cv::Rect(0, (input.rows / subsections)*i, input.cols, input.rows / subsections));
		cv::Mat out(output, cv::Rect(0, (output.rows / subsections)*i, output.cols, output.rows / subsections));
		try {
			TS_START_NIF("Detect");
			findContours(in, contourData, contourHierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
			TS_STOP_NIF("Detect");
			out = Scalar::all(0);
			Scalar color = Scalar(255, 255, 255, 255);
			//srand (time(NULL));

			TS_START_NIF("Draw");
			//Slow draw
			//drawContours(out, contourData, -1, color, lineThickness, 8, contourHierarchy);

			//Faster draw
			for (vector<cv::Point> contour : contourData) {
				//if(true) color = Scalar(rand()&255, rand()&255, rand()&255);
				//if(cv::contourArea(contour) > 1.0f)
				if (contour.size() > minContourSize) {
					polylines(out, contour, true, color, lineThickness, 8);
				}
			}
			TS_STOP_NIF("Draw");

		}
		catch (cv::Exception& e) {
			const char* err_msg = e.what();
			std::cout << "ParallelContourDetector exception caught: " << err_msg << std::endl;
		}

		//out.copyTo(output(cv::Rect(0, (output.rows/subsections)*i, output.cols, output.rows/subsections)));

		in.release();
		out.release();
	}

}

void ParallelContourDetector::DetectContours(cv::InputArray in, cv::OutputArray out, const int lineThickness) {
	cv::UMat src = in.getUMat();

	out.create(src.size(), src.type());
	cv::UMat dst = out.getUMat();


	vector< vector<cv::Point> > contourData;
	vector<Vec4i> contourHierarchy;
	findContours(in.getUMat(), contourData, contourHierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
	dst = Scalar::all(0);
	Scalar color = Scalar(255, 255, 255, 255);
	//srand (time(NULL));
	for (vector<cv::Point> contour : contourData) {
		//if(true) color = Scalar(rand()&255, rand()&255, rand()&255);
		//if(cv::contourArea(contour) > 1.0f)
		polylines(dst, contour, true, color, lineThickness, 8);
	}
}

void ParallelContourDetector::RepairImage(cv::Mat &mat) {
	for (int row = 2; row < mat.rows; ++row)
	{
		const uchar *ptr = mat.ptr(row);
		for (int col = 0; col < mat.cols; col++)
		{
			const uchar * this_pixel = ptr;

			const uchar * top_pixel = mat.ptr(row - 2);
			if (this_pixel[0] > 0 && top_pixel[0] > 0) {
				uchar * mid_pixel = mat.ptr(row - 1);
				mid_pixel[0] = 255;
				mid_pixel[1] = 255;
				mid_pixel[2] = 255;

			}

			ptr += 3;
		}
	}

}