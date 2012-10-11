// OpenCV_Helloworld.cpp : Defines the entry point for the console application.
// Created for build/install tutorial, Microsoft Visual C++ 2010 Express and OpenCV 2.1.0
#include <stdio.h>
#include <tchar.h>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/legacy/legacy.hpp>
#include <fstream>
#include <iostream>
#include <vector>
using namespace std;
using namespace cv;
#define local_min(a,b) a<b?a:b

int _tmain(int argc, char* argv[])	{
	int patch_size = 15;
	int width = patch_size/2;
	int i, j;
	double w = 0.95;
	double t0 = 0.01;
	double temp, t;
	int A = 255;
	Mat img_origin = imread("canon7.jpg");
	Mat img_template(img_origin);
	
	Mat dark_channel(img_origin.size(), CV_8U);
	Mat transimssion(img_origin.size(), CV_8U);
	vector<Mat> rgb_channel;

	//padded
	Mat img_padded(img_origin.rows + 2*width, img_origin.cols + 2*width, img_origin.type());
	img_origin.copyTo(img_padded(Rect(width, width, img_origin.cols, img_origin.rows)));
	split(img_padded, rgb_channel);
	try	{
		for(i = width; i < img_padded.rows-width; i++)	{
			for(j = width; j < img_padded.cols-width; j++)	{
				minMaxLoc(rgb_channel[0](Rect(j-width, i-width, patch_size, patch_size)),&t);
				minMaxLoc(rgb_channel[1](Rect(j-width, i-width, patch_size, patch_size)),&temp);
				t = local_min(t, temp);
				minMaxLoc(rgb_channel[2](Rect(j-width, i-width, patch_size, patch_size)),&temp);
				t = local_min(t, temp);
				dark_channel.at<uchar>(i-width,j-width) = t;
				transimssion.at<uchar>(i-width,j-width) = 255-t;
				temp = img_template.data[img_template.step*(i-width)+ img_template.channels()*(j-width) + 0];
				img_template.data[img_template.step*(i-width)+ img_template.channels()*(j-width) + 0] = (int)(temp - t)/(1-t/A);
				temp = img_template.data[img_template.step*(i-width)+ img_template.channels()*(j-width) + 1];
				img_template.data[img_template.step*(i-width)+ img_template.channels()*(j-width) + 1] = (int)(temp - t)/(1-t/A);
				temp = img_template.data[img_template.step*(i-width)+ img_template.channels()*(j-width) + 2];
				img_template.data[img_template.step*(i-width)+ img_template.channels()*(j-width) + 2] = (int)(temp - t)/(1-t/A);
			}
		}
	
	} catch(Exception e)	{
		printf("%s\r\n", e.err.c_str());
	}
	imwrite("canon7_dc_15.jpg", dark_channel);
	imwrite("canon7_15.jpg", img_template);
	imwrite("canon7_trans_15.jpg", transimssion);
	imshow("image0", img_template);
	waitKey(0); //µÈ´ý°´¼ü
	return 0;
}

