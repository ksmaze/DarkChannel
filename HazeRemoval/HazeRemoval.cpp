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

static void onMouse( int event, int x, int y, int, void* data)
{
	Mat *aaa = (Mat *) data;
	if( event != CV_EVENT_LBUTTONDOWN )
		return;

	Point seed = Point(x,y);

	

	cout <<"x: " <<x <<"y: " <<y <<" p: " <<(int)aaa->at<uchar>(y,x) <<endl;
	aaa->at<uchar>(y,x) = 255;
	imshow("dc",*aaa);
}

Vec3b get_AL(Mat &img, const Mat &dark_channel)
{
	int temp;
	double lo = 5, hi =5;
	int range = (img.rows)*(img.cols)*0.01;
	int i,j,a = 0,transNum[256] = {0};
	for(i = 0; i < img.rows; i++)	{
		for(j = 0; j < img.cols; j++)	{
			transNum[dark_channel.at<uchar>(i,j)]++;
		}
	}
	int count = 0;
	for(i = 255;i >= 0;i--){
		count = count+transNum[i];
		if(count >= range)
			break;
	}
	cout<<i<<endl;
	int thre = i;
	Mat mask(img.rows+2, img.cols+2, CV_8U, Scalar(0));
	Mat tempi(img);
	int maskcount = 0;
	int blocksize = 0;
	Vec3b AL;
	//imshow("dc",dark_channel);
	//setMouseCallback( "dc", onMouse, (void*)&dark_channel );
	for(i = 0;i < img.rows;i++){
		for(j = 0;j < img.cols;j++){
			temp = dark_channel.at<uchar>(i,j);
			if(dark_channel.at<uchar>(i,j) > thre && mask.at<uchar>(i,j) == 0) {
				floodFill(img, mask, Point(j,i), Scalar(255,255,255), 0, Scalar(lo,lo,lo), Scalar(hi,hi,hi),FLOODFILL_MASK_ONLY);
				count = countNonZero(mask);
				cout <<i <<"," <<j <<" count: " <<count <<endl;
				if (count - maskcount > blocksize)	{
					AL = img.at<Vec3b>(i,j);
					blocksize = count -maskcount;
					cout <<"over "<<i <<"," <<j <<", " <<(int)dark_channel.at<uchar>(i,j) <<" " <<(int)mask.at<uchar>(i,j) <<endl;
				}
				//imshow("mask", tempi);
				//waitKey(0);
				maskcount = count;
			}
		}
	}
	
	
	return AL;
}

int main(int argc, char *argv[])
{
	int patch_size = 15;
	int width = patch_size/2;
	int i, j, k;
	double w = 0.95;
	double t0 = 0.01;
	double temp, t1, t2;
	int d = 11;
	double sigmaspace = 5, sigmacolor = 64;
	string file;
	size_t found;
	if (argc > 1)	{
		for (k = 1; k < argc; k++)	{
			file = string(argv[k]);
			Mat img_origin = imread(file);
			Mat img_template(img_origin);
			found = file.rfind(".");
			if (found != string::npos)	{
				file.replace(found, 1, "_dc.");
			}

			Mat dark_channel = imread(file, CV_8U);
			found = file.rfind("_dc.");
			if (found != string::npos)	{
				file.replace(found, 4, "_trans.");
			}
			Mat transmission = imread(file, CV_8U);
			Vec3b A = get_AL(img_origin, dark_channel);
			//Vec3b A = Vec3b(60, 180, 200);
			for(i = 0; i < img_template.rows; i++)	{
				for(j = 0; j < img_template.cols; j++)	{
					t1 = (double)transmission.at<uchar>(i,j)/255.0;
					t2 = t1*0.99;
					//B
					temp = img_template.data[img_template.step*(i)+ img_template.channels()*(j) + 0];
					temp = local_max(10,(temp-(1-t1)*A.val[0])/t2);
					temp = local_min(temp, 255);
					img_template.data[img_template.step*(i)+ img_template.channels()*(j) + 0] = (uchar) temp;
					//G
					temp = img_template.data[img_template.step*(i)+ img_template.channels()*(j) + 1];
					temp = local_max(10,(temp-(1-t1)*A.val[1])/t2);
					temp = local_min(temp, 255);
					img_template.data[img_template.step*(i)+ img_template.channels()*(j) + 1] = (uchar) temp;
					//R
					temp = img_template.data[img_template.step*(i)+ img_template.channels()*(j) + 2];
					temp = local_max(10,(temp-(1-t1)*A.val[2])/t2);
					temp = local_min(temp, 255);
					img_template.data[img_template.step*(i)+ img_template.channels()*(j) + 2] = (uchar) temp;
				}
			}
			found = file.rfind("_trans.");
			if (found != string::npos)	{
				file.replace(found, 7, "_rec.");
			}
			imwrite(file, img_template);
			//imshow("img1", img_template);
			//waitKey(0);
		}
	}
}