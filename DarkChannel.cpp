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

int Q[601*601][2];
int CA[610*60][2] = {0};
int dir[8][2] = {{0,1},{1,0},{0,-1},{-1,0},{1,1},{1,-1},{-1,1},{-1,-1}};


int getAL(Mat img, int patch_size)
{
	int range = img.rows*img.cols*0.1;
	int width = patch_size/2;
//	cout<<range<<endl;
	int A = 0,i,j,temp[256] = {0};
	double t,t1;
	Mat DCA(img.size(), CV_8U);
	vector<Mat> rgb_channel;
	split(img, rgb_channel);
	for(i = width; i < img.rows-width; i++)	{
		for(j = width; j < img.cols-width; j++)	{
			minMaxLoc(rgb_channel[0](Rect(j-width, i-width, patch_size, patch_size)),&t);
			minMaxLoc(rgb_channel[1](Rect(j-width, i-width, patch_size, patch_size)),&t1);
			t = local_min(t, t1);
			minMaxLoc(rgb_channel[2](Rect(j-width, i-width, patch_size, patch_size)),&t1);
			t = local_min(t, t1);
			DCA.at<uchar>(i-width,j-width) = t;
			temp[(int)t]++;
		}
	}	
	
	int count = 0;
	for(i = 255;i >= 0;i--){
		count = count+temp[i];
		if(count >= range)
			break;
	}

	int thre = i;
	int cla = 1;
	cout<<thre<<" "<<count<<endl;

	for(i = 0;i < img.rows;i++){
		for(j = 0;j < img.cols;j++){
			if((int)DCA.at<uchar>(i,j) < thre) {
	//			cout<<"sdf"<<endl;continue;
				continue;
			}
			int head = 0, tail = 1;
			Q[head][0] = i;
			Q[head][1] = j;
			CA[cla][0] = 1;
			CA[cla][1] = (int)DCA.at<uchar>(i,j);
			DCA.at<uchar>(i,j) = 0;
			while(head != tail){
				CA[cla][0]++;
				if(CA[cla][1] < (int)DCA.at<uchar>(i,j)) CA[cla][1] = (int)DCA.at<uchar>(i,j);
				for(int k = 0;k < 8;k++)
				{
					int u = Q[head][0]+dir[k][0],v = Q[head][1]+dir[k][1];
					if((u >= 0) && (u < img.rows) && (v >= 0) && (v < img.cols))
					{
						if((int)DCA.at<uchar>(u,v) >= thre){
							Q[tail][0] = u; Q[tail][1] = v;
							DCA.at<uchar>(u,v) = 0;
							tail++;
//						cout<<u<<" "<<v<<endl;
//						cout<<(int)DCA.at<uchar>(u,v)<<endl;
						}
					}

				}
//				cin>>t;
				head++;
//				cout<<"head = "<<head<<" tail = "<<tail<<endl;
			}
			cla++;
		}
	}
	int maxS = CA[1][0];
	A = CA[1][1];
	cout<<cla<<endl;
	cout<<CA[1][0]<<endl;
	for(i = 2;i < cla;i++){
//		cout<<CA[i][0]<<endl;
		if(maxS < CA[i][0]){
			maxS = CA[i][0];
			A = CA[i][1];
			cout<<maxS<<endl;
		}
	}
	return A; 
}

int _tmain(int argc, char* argv[])	{
	int patch_size = 15;
	int width = patch_size/2;
	int i, j;
	double w = 0.95;
	double t0 = 0.01;
	double temp, t;

	Mat img_origin = imread("canon7.jpg");
	Mat img_template(img_origin);

	Mat dark_channel(img_origin.size(), CV_8U);
	Mat transimssion(img_origin.size(), CV_8U);
	vector<Mat> rgb_channel;
//	int x = getAL(img_origin);
	//padded
	Mat img_padded(img_origin.rows + 2*width, img_origin.cols + 2*width, img_origin.type());
	img_origin.copyTo(img_padded(Rect(width, width, img_origin.cols, img_origin.rows)));
	split(img_padded, rgb_channel);
	int A = getAL(img_padded, patch_size);
	cout<<A<<endl;
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
//	imwrite("canon7_dc_15.jpg", dark_channel);
//	imwrite("canon7_16.jpg", img_template);
//	imwrite("canon7_trans_15.jpg", transimssion);
	imshow("image0", img_template);
	waitKey(0); //µÈ´ý°´¼ü
	return 0;
}

