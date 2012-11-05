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

void getAdaptiveDarkChannel(const Mat &src, Mat &dst, Mat &trans, Vec3b &A, int patch_size)	{
	int width = patch_size/2;
	int i, j, ii, jj;
	double lo = 10, hi = 10, dc;
	int range = (src.rows)*(src.cols)*0.01;
	int a = 0,transNum[256] = {0};
	double t, temp;
	double w = 0.95;


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
	for(i = 0; i < src.rows; i++)	{
		for(j = 0; j < src.cols; j++)	{
			transNum[dark_channel.at<uchar>(i,j)]++;
		}
	}
	int count = 0;
	for(i = 255;i >= 0;i--){
		count = count+transNum[i];
		if(count >= range)
			break;
	}
	int thre = i;
	Mat mask(src.rows+2, src.cols+2, CV_8U, Scalar(0));
	Mat temp_mask(mask);

	Vec3b AL;
	int maskcount = 0;
	int blocksize = 0;
	for(i = 0;i < src.rows;i++){
		for(j = 0;j < src.cols;j++){
			temp = dark_channel.at<uchar>(i,j);
			if(dark_channel.at<uchar>(i,j) >= thre && mask.at<uchar>(i,j) == 0) {
				floodFill(src, mask, Point(j,i), Scalar(255,255,255), 0, Scalar(lo,lo,lo), Scalar(hi,hi,hi),FLOODFILL_MASK_ONLY);
				count = countNonZero(mask);
				if (count - maskcount > blocksize)	{
					AL = src.at<Vec3b>(i,j);
					blocksize = count -maskcount;
					//if (blocksize > 0.2*(src.rows)*(src.cols))	{
					//	AL.val[0] = AL.val[0]*0.8;
					//	AL.val[1] = AL.val[1]*0.8;
					//	AL.val[2] = AL.val[2]*0.8;
					//}
					//try this first
					//this is the method for improving sky region
					AL.val[0] = AL.val[0]*(1-(double)blocksize/((src.rows)*(src.cols)));
					AL.val[1] = AL.val[1]*(1-(double)blocksize/((src.rows)*(src.cols)));
					AL.val[2] = AL.val[2]*(1-(double)blocksize/((src.rows)*(src.cols)));
					//cout <<"over "<<i <<"," <<j <<", " <<(int)dark_channel.at<uchar>(i,j) <<" " <<(int)mask.at<uchar>(i,j) <<endl;
				}
				maskcount = count;
			}
		}
	}

	//refine dark channel and AL, and get the transmission
	temp_mask.copyTo(mask);
	maskcount = 0;
	blocksize = 0;
	for(i = 0;i < src.rows;i++){
		for(j = 0;j < src.cols;j++){
			temp = dark_channel.at<uchar>(i,j);
			if(mask.at<uchar>(i,j) == 0) {
				floodFill(src, mask, Point(j,i), Scalar(255,255,255), 0, Scalar(lo,lo,lo), Scalar(hi,hi,hi),FLOODFILL_MASK_ONLY);
				//use a vector to record all point in this region
				vector<Point> queue;
				dc = 255;
				for(ii = 0;ii < src.rows;ii++){
					for(jj = 0;jj < src.cols;jj++){
						if (mask.at<uchar>(i,j) && !temp_mask.at<uchar>(i,j))	{
							queue.push_back(Point(jj, ii));
							t = local_min(src.data[src.step*(ii)+ src.channels()*(jj) + 0]/AL.val[0],src.data[src.step*(ii)+ src.channels()*(jj) + 1]/AL.val[1]);
							t = local_min(t, src.data[src.step*(ii)+ src.channels()*(jj) + 2]/AL.val[2]);
							dc = local_min(255.0*t, dc);
						}
					}
				}
				for (ii = 0; ii < queue.size(); ii++)	{
					dark_channel.at<uchar>(queue.at(ii)) = dc;
					trans.at<uchar>(queue.at(ii)) = 255.0 - w * dc;
				}
				queue.clear();
				//the region that between to mask is the region we should use same dark_channel;
				mask.copyTo(temp_mask);
			}
		}
	}
	dark_channel.copyTo(dst);
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
			Mat dark_channel;
			Mat transmission;
			Vec3b A;
			getAdaptiveDarkChannel(img_origin, dark_channel, transmission, A, patch_size);
			imwrite(file, dark_channel);
			found = file.rfind("_dc.");
			if (found != string::npos)	{
				file.replace(found, 4, "_trans.");
			}
			imwrite(file, transmission);
			
			//Vec3b A = Vec3b(230, 230, 230);
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