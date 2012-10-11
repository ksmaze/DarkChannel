// chax.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <fstream>
#include <iostream>

using namespace cv;

int _tmain(int argc, _TCHAR* argv[])
{
	std::ofstream fout("loc.txt");
	fout.flush();
	const char* imagefile0 = argv[1];
	const char* imagefile1 = argv[2];
	int restrictx = -1, restricty = -1;
	if (argc>4)
	{
		restrictx = atoi(argv[3]);
		restricty = atoi(argv[4]);
	}
	Mat img0 = imread(imagefile0);
	Mat img1 = imread(imagefile1);
	vector<vector<Point> > contours;

	Mat img2;
	Mat img3(img0.size(), CV_8UC1);
	Mat img4(img0.size(), CV_8U);
	absdiff(img0, img1, img2);
	cvtColor(img2, img3, CV_BGR2GRAY);
	img3.convertTo(img3, CV_8U);
	try	{
		int ch[] = {2, 0};
		mixChannels(&img0, 1, &img4, 1, ch, 1);
		threshold(img4, img4, 254, 255, CV_THRESH_BINARY);
		for (int i = 1; i < img4.cols-1; i++)
		{
			img4.at<uchar>(0, i) = 255;
			img4.at<uchar>(img4.rows-1, i) = 255;
		}
		for (int i = 1; i < img4.rows-1; i++)
		{
			img4.at<uchar>(i, 0) = 255;
			img4.at<uchar>(i, img4.cols-1) = 255;
		}
		//imshow("img4", img4);
		findContours(img4, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);
		

		if (restrictx != -1)	
		{
			if (restrictx > 0)
				for (int i = 0; i < img3.cols; i++)
					img3.at<uchar>(restrictx-1, i) = 0;
			if (restrictx < img3.rows-1)
				for (int i = 0; i < img3.cols; i++)
					img3.at<uchar>(restrictx+1, i) = 0;
			for (int i = 0; i < img3.cols; i++)
				img3.at<uchar>(restrictx, i) = 0;

			if (restricty > 0)
				for (int i = 0; i < img3.rows; i++)
					img3.at<uchar>(i, restricty-1) = 0;
			for (int i = 0; i < img3.rows; i++)
				img3.at<uchar>(i, restricty) = 0;
			if (restricty < img3.cols-1)
				for (int i = 0; i < img3.rows; i++)
					img3.at<uchar>(i, restricty+1) = 0;
			for (int i = restrictx-4>0? restrictx-4: 0; i < (restrictx+3<img3.rows-1?restrictx+4:img3.rows-1); i++)
				for (int j = restricty-4>0? restricty-4: 0; j < (restricty+4<img3.cols-1?restricty+4:img3.cols-1); j++)
					img3.at<uchar>(i, j) = 0;
		}
		
		for (unsigned int i = 0; i < contours.size(); i++)	
			if (contours[i].size()>3)	{
				vector<vector<Point>> square;
				square.push_back(vector<Point>());
				square[0].push_back(Point(500, 500));
				square[0].push_back(Point(500, 0));
				square[0].push_back(Point(0, 0));
				square[0].push_back(Point(0, 500));
				for (unsigned int j = 0; j < contours[i].size(); j++)
				{
					square[0][0].x = min(square[0][0].x, contours[i][j].x);
					square[0][0].y = min(square[0][0].y, contours[i][j].y);
					square[0][2].x = max(square[0][2].x, contours[i][j].x);
					square[0][2].y = max(square[0][2].y, contours[i][j].y);
				}
				square[0][0].x = max(square[0][0].x-2, 0);
				square[0][0].y = max(square[0][0].y-2, 0);
				square[0][2].x = min(square[0][2].x+2, img4.cols-1);
				square[0][2].y = min(square[0][2].y+2, img4.rows-1);
				square[0][1].x = square[0][0].x;
				square[0][1].y = square[0][2].y;
				square[0][3].x = square[0][2].x;
				square[0][3].y = square[0][0].y;
				drawContours(img3, square, 0, Scalar(0), CV_FILLED, 8);
			}
		//imshow("image3", img3);
		dilate(img3, img3, Mat(), Point(-1,-1), 2);
		erode(img3, img3, Mat(),Point(-1,-1), 4);
		dilate(img3, img3, Mat(), Point(-1,-1), 2);
		threshold(img3, img3, 1, 255, CV_THRESH_BINARY);
		
		Mat img5;
		img3.copyTo(img5);
		for (int i = 0; i < img3.rows; i++)
			for (int j = 0; j < img3.cols; j++)
			{
				uchar idx = img3.at<uchar>(i,j);
				if (idx > 0)
				{
					Rect ccomp;
					floodFill(img3, Point(j,i), 0, &ccomp, Scalar(), Scalar(), 8);
					if (ccomp.width > 3 && ccomp.height > 3)	
					{
						int mnum = 0, mcount = 0, mincount = 10000;
						Point center;
						for (int x = ccomp.x; x < ccomp.x+ccomp.width; x++)	{
							for (int y = ccomp.y; y < ccomp.y+ccomp.height; y++)	{
								if (img5.at<uchar>(y, x) > 0)	{
									//circle(img0, Point(x, y), 1, Scalar(0, 0, 255, 0));
									mnum += y;
									mcount++;
								}
							}
							if (mcount > 4 && mcount < mincount)	{
								mincount = mcount;
								center.y = mnum / mcount;
							}
						}
						mnum = mcount = 0;
						for (int x = ccomp.x; x < ccomp.x+ccomp.width; x++)	{
							if (img5.at<uchar>(center.y, x) > 0)	{
									mnum += x;
									mcount++;
								}
						}
						center.x = mnum / mcount;
						fout <<center.y <<"," <<center.x << std::endl;
						//circle(img0, Point(center.x, center.y), 3, Scalar(0, 255, 0, 0));
						//circle(img1, Point(center.x, center.y), 3, Scalar(0, 255, 0, 0));
					}
				}
			}
			//imshow("img5", img5);
	} catch(Exception e)	{
		printf("%s", e.err.c_str());
	}
	fout.close();
	//imshow("image0", img0);
	//imshow("image1", img1);
	//waitKey(0); //等待按键
	return 0;
}

