#include <stdio.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/legacy/legacy.hpp>
#include <fstream>
#include <iostream>
#include <vector>
#include <time.h>
using namespace std;
using namespace cv;

#define local_min(a,b) (a<b)?(a):(b)
#define local_max(a,b) (a<b)?(b):(a)

struct Diff8uC3
{
	Diff8uC3(cv::Vec3b _lo, cv::Vec3b _up)
	{
		for( int k = 0; k < 3; k++ )
			lo[k] = _lo[k], interval[k] = _lo[k] + _up[k];
	}
	bool operator()(const cv::Vec3b &a, const cv::Vec3b &b) const
	{
		return (unsigned)(a.val[0] - b.val[0] + lo[0]) <= interval[0] &&
			(unsigned)(a.val[1] - b.val[1] + lo[1]) <= interval[1] &&
			(unsigned)(a.val[2] - b.val[2] + lo[2]) <= interval[2];
	}
	unsigned lo[3], interval[3];
};


int floodFillC( const Mat &_image, Mat & _mask, vector<Point> &queue,
	Point seedPoint, int d, Scalar newVal, Rect* rect,
	Scalar loDiff, Scalar upDiff, int flags )
{
	int w = (d-1)/2;
	Vec3b ld_buf, ud_buf;
	for (int i = 0; i < 3; i++)
	{
		ld_buf.val[i] = loDiff.val[0];
		ud_buf.val[i] = upDiff.val[0];
	}
	Diff8uC3 diff(ld_buf, ud_buf);
	int head = 0, tail = 1;
	Point seed = seedPoint;
	_mask.at<uchar>(seedPoint) = 1;
	queue.push_back(seedPoint);
	//while (head != tail)	{
	//	seed = queue.at(head);
		for (int i = -w; i <= w; i++)	{
			for (int j = -w; j <= w; j++)	{
				if (seed.x+i >= 0 && seed.x+i < _image.cols
					&& seed.y+j >= 0 && seed.y+j < _image.rows
					&& !_mask.at<uchar>(seed.y+j, seed.x+i)
					&& diff(_image.at<Vec3b>(seed), _image.at<Vec3b>(seed.y+j, seed.x+i)))	{
						queue.push_back(Point(seed.x+i, seed.y+j));
						_mask.at<uchar>(seed.y+j, seed.x+i) = 1;
						tail++;
				}
			}
		}
	//	head++;
	//}
	return 0;
}

Vec3b getAdaptiveDarkChannel(Mat &src, Mat &dark_channel, Mat &trans, Vec3b &A, int patch_size)	{
	int width = patch_size/2;
	int i, j, ii;
	double lo = 3, hi = 5;
	int range = (src.rows)*(src.cols)*0.001;
	int a = 0,transNum[256] = {0};
	double t, temp;
	double w = 0.95;
	Vec3b HaveSky(0,0,0);

	Mat img_template(src.size(), src.type());
	bilateralFilter(src, img_template, 5, 25, 5);
	vector<Mat> rgb_channel;


	Mat img_padded(src.rows + 2*width, src.cols + 2*width, src.type(), Scalar(0,0,0));
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
	cout <<"Finished creating original dark channel" <<endl;
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
	Mat temp_mask(src.rows+2, src.cols+2, CV_8U, Scalar(0));
	lo = 2;
	hi = 10;
	int maskcount = 0;
	int blocksize = 0;
	for(i = 0;i < src.rows;i++){
		for(j = 0;j < src.cols;j++){
			temp = dark_channel.at<uchar>(i,j);
			if(dark_channel.at<uchar>(i,j) >= thre && mask.at<uchar>(i,j) == 0) {
				floodFill(src, mask, Point(j,i), Scalar(255,255,255), 0, Scalar(lo,lo,lo), Scalar(hi,hi,hi),FLOODFILL_MASK_ONLY);
				count = countNonZero(mask);
				if (count - maskcount > blocksize)	{
					A = src.at<Vec3b>(i,j);
					//A.val[0] = ((double)dark_channel.at<uchar>(i,j)/255.0)*A.val[0];
					//A.val[1] = ((double)dark_channel.at<uchar>(i,j)/255.0)*A.val[1];
					//A.val[2] = ((double)dark_channel.at<uchar>(i,j)/255.0)*A.val[2];
					Mat tt_mask(temp_mask.size(), temp_mask.type());
					temp_mask.copyTo(tt_mask);
					floodFill(src, tt_mask, Point(j,i), Scalar(255,255,255), 0, Scalar(2,2,2), Scalar(1,1,1),FLOODFILL_MASK_ONLY);
					int t_count = countNonZero(tt_mask);
					int t_blocksize = (t_count-maskcount);
					blocksize = count -maskcount;
					cout <<(double)t_blocksize/blocksize <<endl;
					if ((double)t_blocksize/blocksize > 0.2)	{
						//HaveSky.val[0] = (255.0-dark_channel.at<uchar>(i,j))*0.5;
						//HaveSky.val[1] = (255.0-dark_channel.at<uchar>(i,j))*0.5;
						//HaveSky.val[2] = (255.0-dark_channel.at<uchar>(i,j))*0.5;
						cout <<(double)t_blocksize/blocksize <<" sky region detected" <<endl;
						A.val[0] = A.val[0]-(255.0-dark_channel.at<uchar>(i,j)*0.9);
						A.val[1] = A.val[1]-(255.0-dark_channel.at<uchar>(i,j)*0.9);
						A.val[2] = A.val[2]-(255.0-dark_channel.at<uchar>(i,j)*0.9);
					}
				}
				maskcount = count;
			}
		}		
	}
	cout <<"AL: " <<(int)A.val[0] <<", " <<(int)A.val[1] <<", " <<(int)A.val[2] <<endl;
	cout <<"Finished finding the refined AL" <<endl;
	//refine dark channel and AL, and get the transmission
	temp_mask.copyTo(mask);
	Mat t_dark_channel(dark_channel.size(), dark_channel.type());
	dark_channel.copyTo(t_dark_channel);
	lo = 5;
	hi = 20;
	maskcount = 0;
	blocksize = 0;
	for(i = 0;i < img_template.rows;i++){
		for(j = 0;j < img_template.cols;j++){
			if(dark_channel.at<uchar>(i,j) >= thre && mask.at<uchar>(i,j) == 0) {
				//cout <<"fill point (" <<i <<"," <<j <<")"<<endl;
				vector<Point> queue;
				floodFillC(img_template, mask, queue, Point(j,i), 60, Scalar(255,255,255), 0, Scalar(lo,lo,lo), Scalar(hi,hi,hi),FLOODFILL_MASK_ONLY);
				//use a vector to record all point in this region
				temp = 255.0;
				for (ii = 0; ii < queue.size(); ii++)	{
					t = local_min(rgb_channel[0].at<uchar>(queue.at(ii).y+width, queue.at(ii).x+width), rgb_channel[1].at<uchar>(queue.at(ii).y+width, queue.at(ii).x+width));
					t = local_min(t, rgb_channel[2].at<uchar>(queue.at(ii).y+width, queue.at(ii).x+width));
					temp = local_min(t, temp);
				}
				for (ii = 0; ii < queue.size(); ii++)	{
					t_dark_channel.at<uchar>(queue.at(ii)) = temp;
				}
				queue.clear();
				mask.copyTo(temp_mask);
			}
		}
	}
	cout <<"start low point" <<endl;
	lo = 2;
	hi = 10;
	maskcount = 0;
	blocksize = 0;
	for(i = 0;i < img_template.rows;i++){
		for(j = 0;j < img_template.cols;j++){
			if(mask.at<uchar>(i,j) == 0) {
				//cout <<"fill point (" <<i <<"," <<j <<")"<<endl;
				vector<Point> queue;
				floodFillC(img_template, mask, queue, Point(j,i), 60, Scalar(255,255,255), 0, Scalar(lo,lo,lo), Scalar(hi,hi,hi),FLOODFILL_MASK_ONLY);
				//use a vector to record all point in this region
				temp = 255.0;
				for (ii = 0; ii < queue.size(); ii++)	{
					t = local_min(rgb_channel[0].at<uchar>(queue.at(ii).y+width, queue.at(ii).x+width), rgb_channel[1].at<uchar>(queue.at(ii).y+width, queue.at(ii).x+width));
					t = local_min(t, rgb_channel[2].at<uchar>(queue.at(ii).y+width, queue.at(ii).x+width));
					temp = local_min(t, temp);
				}
				for (ii = 0; ii < queue.size(); ii++)	{
					t_dark_channel.at<uchar>(queue.at(ii)) = temp;
				}
				queue.clear();
				mask.copyTo(temp_mask);
				//imshow("img1", dark_channel);
				//imshow("img2", trans);
				//waitKey(0);
			}
		}
	}
	GaussianBlur(t_dark_channel, dark_channel, Size(3,3), 0, 0);
	for(i = 0;i < src.rows;i++){
		for(j = 0;j < src.cols;j++){
			trans.at<uchar>(i,j) = 255.0 - w*dark_channel.at<uchar>(i,j);
		}
	}
	return HaveSky;
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
			Mat img_template(img_origin.size(), img_origin.type());
			img_origin.copyTo(img_template);
			found = file.rfind(".");
			if (found != string::npos)	{
				file.replace(found, 1, "_dc.");
			}
			Mat dark_channel(img_origin.size(), CV_8U, Scalar(0));
			Mat transmission(img_origin.size(), CV_8U, Scalar(0));
			Vec3b A, A2;
			int lo=2, hi = 10;
			Diff8uC3 diff(Vec3b(lo,lo,lo), Vec3b(hi,hi,hi));
			clock_t start, end;
			start = clock();
			Vec3b HaveSky = getAdaptiveDarkChannel(img_origin, dark_channel, transmission, A, patch_size);
			cout <<"A: "<<(int)A.val[0] <<", "<<(int)A.val[1] <<", " <<(int)A.val[2] <<endl; 
			cout <<"Begining recovering image" <<endl;
			//A = Vec3b(230, 230, 230);
			for(i = 0; i < img_template.rows; i++)	{
				for(j = 0; j < img_template.cols; j++)	{
					//if (diff(A, img_template.at<Vec3b>(i,j)))	{}
					t1 = (double)transmission.at<uchar>(i,j)/255.0;
					t2 = t1*0.99;
					//B
					temp = img_template.data[img_template.step*(i)+ img_template.channels()*(j) + 0];
					temp = local_max(10,(temp-(1-t1)*A.val[0]-HaveSky.val[0])/t2);
					temp = local_min(temp, 255);
					img_template.data[img_template.step*(i)+ img_template.channels()*(j) + 0] = (uchar) temp;
					//G
					temp = img_template.data[img_template.step*(i)+ img_template.channels()*(j) + 1];
					temp = local_max(10,(temp-(1-t1)*A.val[1]-HaveSky.val[0])/t2);
					temp = local_min(temp, 255);
					img_template.data[img_template.step*(i)+ img_template.channels()*(j) + 1] = (uchar) temp;
					//R
					temp = img_template.data[img_template.step*(i)+ img_template.channels()*(j) + 2];
					temp = local_max(10,(temp-(1-t1)*A.val[2]-HaveSky.val[0])/t2);
					temp = local_min(temp, 255);
					img_template.data[img_template.step*(i)+ img_template.channels()*(j) + 2] = (uchar) temp;
				}
			}
			end = clock();
			cout <<file <<"cost time: " <<end-start <<" ms" <<endl;
			imwrite(file, dark_channel);
			found = file.rfind("_dc.");
			if (found != string::npos)	{
				file.replace(found, 4, "_trans.");
			}
			imwrite(file, transmission);
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