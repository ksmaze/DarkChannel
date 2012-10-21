#include <stdio.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/legacy/legacy.hpp>
#include <fstream>
#include <iostream>
#include <vector>
using namespace std;
using namespace cv;
#define local_min(a,b) (a<b)?(a):(b)
#define local_max(a,b) (a<b)?(b):(a)


void getDarkChannel(const Mat &src, Mat &dst, int patch_size)	{
	int width = patch_size/2;
	int i, j;
	double t, temp;
	Mat dark_channel(src.size(), CV_8U);
	Mat img_template(src);
	vector<Mat> rgb_channel;
	Mat img_padded(src.rows + 2*width, src.cols + 2*width, src.type());
	src.copyTo(img_padded(Rect(width, width, src.cols, src.rows)));
	split(img_padded, rgb_channel);
	for(i = width; i < img_padded.rows-width; i++)	{
			for(j = width; j < img_padded.cols-width; j++)	{
				minMaxLoc(rgb_channel[0](Rect(j-width, i-width, patch_size, patch_size)),&t);
				minMaxLoc(rgb_channel[1](Rect(j-width, i-width, patch_size, patch_size)),&temp);
				t = local_min(t, temp);
				minMaxLoc(rgb_channel[2](Rect(j-width, i-width, patch_size, patch_size)),&temp);
				t = local_min(t, temp);
				dark_channel.at<uchar>(i-width,j-width) = (uchar)t;
			}
		}
	dark_channel.copyTo(dst);
}

int main(int argc, char* argv[])	{
	int patch_size = 60;
	double w = 0.95;
	double t0 = 0.01;
	int i, j, k;

	string file;
	size_t found;
	if (argc > 1)	{
		for (k = 1; k < argc; k++)	{
			file = string(argv[k]);
			Mat img_origin = imread(file);
			found = file.rfind(".");
			if (found != string::npos)	{
				file.replace(found, 1, "_dc.");
			}
			Mat dark_channel;
			getDarkChannel(img_origin, dark_channel, patch_size);
			imwrite(file, dark_channel);

			Mat img_transmission(dark_channel);
			for(i = 0; i < img_transmission.rows; i++)	{
				for(j = 0; j < img_transmission.cols; j++)	{
					img_transmission.at<uchar>(i,j) = (uchar)(255.0 - w*(local_max(25, dark_channel.at<uchar>(i,j))));
				}
			}
			//bilateralFilter(dark_channel1, dark_channel, d, sigmacolor, sigmaspace);
			found = file.rfind("_dc.");
			if (found != string::npos)	{
				file.replace(found, 4, "_trans.");
			}
			imwrite(file, img_transmission);
		}
	}
	return 0;
}

