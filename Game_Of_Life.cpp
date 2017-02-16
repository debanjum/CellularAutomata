// CA Game Of Life.cpp : Defines the entry point for the console application.

/*
                                          Game Of Life
	Conditions:
		.Alive:3 Moore Neighbours OR 2 Moore Neighbours and Previously Alive
		.Dead: If Not Alive

	Kernel:
		.3*3 Matrix
		.Centre Element = 0;
		.All the Rest   = 1; [Kept 1/8 in program so as to keep convolution sum within matrix range(8*1/8=1)]

	Program Description:
		.Captures Image From Camera To Obtain Randomised Initial Generation Condition
		.Convolution Done Using Standard Filtering Func in OpenCV[Filter2D()]
		.Shows Program Evolution in Detail[Colour Image] :
			.Dead			= White;
			.Dying			= Red;
			.Coming Alive	= Blue;
			.Staying Alive	= Green;

	Program Optimisation
		.Image Pyred Down for Evaluation[Displayed In Original Size]
		.Most Likely Condition Kept First[>3 | <2]
		.Camera Released After Initial Image Capture
		.'if' Statements Used in Nested Format

	Program Flexibilty
		.Convolution Kernel can be Modifed Easily[For Testing Different Kernels]
		.Game Rules Can be Modified Easily.[For Testing Different Rules]
		.Colour Evolution gives more details about the Evolution Process.
		.Evolution can be carried out on Original Image.[For Testing Robustness & Dependence on I.C](IMP: Try in HSV)

	Performance Characteristics
		.Average Time For Evaluating Evolution ~ 75ms

	To Do
		.HSV Thresholded Image Based Initial Condition
		.Rabbits & Wolves Rules
		.Rabbits & Wolves Rules Based on HSV Thresholded Image[Say Blue Thresh For Rabbit, Red Thresh for Wolf]
*/


#include "stdio.h"
//#include <cv.h>
//#include <highgui.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace cv;
using namespace std;

// Record the execution time of some code, in milliseconds.
#define DECLARE_TIMING(s)  int64 timeStart_##s; double timeDIF0f_##s; double timeTally_##s = 0; int countTally_##s = 0
#define START_TIMING(s)    timeStart_##s = cvGetTickCount()
#define STOP_TIMING(s) 	   timeDIF0f_##s = (double)(cvGetTickCount() - timeStart_##s); timeTally_##s += timeDIF0f_##s; countTally_##s++
#define GET_TIMING(s) 	   (double)(timeDIF0f_##s / (cvGetTickFrequency()*1000.0))
#define GET_AVERAGE_TIMING(s)   (double)(countTally_##s ? timeTally_##s/ ((double)countTally_##s * cvGetTickFrequency()*1000.0) : 0)
#define CLEAR_AVERAGE_TIMING(s) timeTally_##s = 0; countTally_##s = 0

#define DEALLOC() cvReleaseCapture(&Cap);	cvDestroyAllWindows()

int main()
{
	//Initialising Timer
	DECLARE_TIMING(T);

	//Initialising Camera
	IplImage* IF;
	CvCapture* Cap = cvCreateCameraCapture(0);

	//Capturing Image
	for(int i=0; i<20; i++)
		IF = cvQueryFrame(Cap);

//	IF = cvLoadImage("C:\\New Downloads\\Dot1.jpg");

	//Error Handling
	if(!IF)	return 0;

	//Initialising Image Data Structures
	IplImage* IFT		= cvCreateImage(cvSize(IF->width,IF->height), 8, 3);
	IplImage* IG		= cvCreateImage(cvSize(IF->width/2,IF->height/2), 8, 3);
	IplImage* IGT		= cvCreateImage(cvSize(IF->width/2,IF->height/2), 8, 3);
	IplImage* IGray		= cvCreateImage(cvSize(IF->width/2,IF->height/2), 8, 1);
	IplImage* ISave4	= cvCreateImage(cvSize(IF->width*4,IF->height*4), 8, 3);
	IplImage* ISave2	= cvCreateImage(cvSize(IF->width*2,IF->height*2), 8, 3);

	//Scaling Down Captured Image
	cvPyrDown(IF,IG);

	//Converting 2 Grayscale
	cvCvtColor(IG,IG,CV_BGR2HSV);
//	cvCvtColor(IG,IGray,CV_HSV2GRAY);
	cvSplit(IG,IGray,0,0,0);

	//Copying IG to IGT
	cvCvtColor(IG,IGT,CV_BGR2RGB);
	cvCvtColor(IGT,IGT,CV_RGB2BGR);

	//Release Camera
	cvReleaseCapture(&Cap);
	cvReleaseImage(&IF);

	//Initialising GOL Matrices
	Mat C = cvarrToMat(IGray);			//Converting Convolution Image to Mat Format-[3 Channel]
	Mat B = (C>100 & C < 130);			//Obtaining Binary Image in Mat Format[C>15 has no effect apart from defining B as B&W Image]
	IplImage GrayScale = C;
	IplImage SeedState = B;
	//Performing Histogram Equlaization
	equalizeHist(C,B);
	cvShowImage("Original",&GrayScale);
	cvShowImage("Histogram Equalized",&SeedState);

	//Clean Image
//	Mat C = Mat::zeros(IGray->height,IGray->width,CV_64F);
//	Mat B = Mat::zeros(IGray->height,IGray->width,CV_64F);

	//Image Converted To 64-Bit Floating Type & Scaled to 1
	B.convertTo (B, CV_64F, 1.0 / 255.0);
	C.convertTo (C, CV_64F, 1.0 / 255.0);

	//Defining Kernel
	double N = 1.0 / 8.0;
	Mat Ker = Mat::ones(3,3,CV_64F)*N;	//Setting Elements to N;
//	Ker.at<double>(2,2) = 0;			//Setting Centre Element to 0;
//	Ker.at<double>(0,0) = 0;
//	Ker.at<double>(0,2) = 0;
//	Ker.at<double>(2,0) = 0;
	Ker.at<double>(1,1) = 0;

	//Evolution Generations
	int Gen = 0;
	int K = 0;	//Size Of Centre Square of Chaotic Evolution

	//Rule Definition
	int Down = 2;
	int Up = 3;

	// Init Birth, Deaths
	int Births = 0;
	int Deaths = 0;

	//Open Birth, Death Logs CSV File
	FILE *fp = fopen("birth_deaths.csv", "w+");
	
	//Initial Square Alive
	for(int i=B.rows/2-K;i<B.rows/2+K;i++)
	    {	for(int j=B.cols/2-K;j<B.cols/2+K;j++)
		{
			B.at<double>(i,j) = 1;
		}
	}

	//Evolution Loop Started
	while(1)
	{
		//Loop Timing
		START_TIMING(T);

		// Reset Births, Deaths for new Generation
		Births = 0;
		Deaths = 0;

		//Performing Convolution On 'B'(Source{Initialised}) wrt 'Ker'(Kernel) & Storing in 'C'(Destination)
		filter2D(B, C, -1, Ker, Point(-1,-1), 0, BORDER_REFLECT);

		//Updating Generation
		for(int i=0;i<C.rows;i++)
		{
			for(int j=0;j<C.cols;j++)
			{
				if(i<C.rows/2+K && i>C.rows/2-K && j<C.cols/2+K && j>C.cols/2-K)
				{
					//If <2 OR >3 Moore Neighbours
					if ((int)(C.at<double>(i,j)/N)<Down | (int)(C.at<double>(i,j)/N)>Up)
					{
						if ((int)(B.at<double>(i,j)/N) == 0)	//Dead
						 {
							 B.at<double>(i,j) = 0;
							 cvSet2D(IGT, i, j, cvScalar(255,255,255));
							//cvSet2D(IGT, i, j, cvGet2D(IGT,i,j));//For Evolution On Original Image
						 }

						 else									//Dying
						 {
							 B.at<double>(i,j) = 0;
							 cvSet2D(IGT, i, j, cvScalar(0,0,255));
							 Deaths += 1;
						 }
					}

					//If 2 Moore Neighbours
					else if ((int)(C.at<double>(i,j)/N) == Down)
					{
						if((int)(B.at<double>(i,j)/N) == 0)		//Dead
						{
							 B.at<double>(i,j) = 1;
							 cvSet2D(IGT, i, j, cvScalar(0,255,0));
							 //cvSet2D(IGT, i, j, cvGet2D(IGT,i,j));//For Evolution On Original Image
						}

						else									//Staying Alive
						{
							 B.at<double>(i,j) = 1;
							 cvSet2D(IGT, i, j, cvScalar(0,255,0));
						}
					}

					//If 3 Moore Neighbours
					else
					{
						if((int)(B.at<double>(i,j)/N) == 0)			//Coming Alive
						{
							 B.at<double>(i,j) = 1;
							 cvSet2D(IGT, i, j, cvScalar(255,0,0));
							 Births += 1;
						}

						else									//Staying Alive
						{
							 B.at<double>(i,j) = 1;
							 cvSet2D(IGT, i, j, cvScalar(0,255,0));
						}
					}
				}

/*				else if(i<2*K && i>0 && j<2*K && j>0)
				{
						if ((int)(B.at<double>(i,j)/N) == 0)	//Dead
						 {
							 B.at<double>(i,j) = 0;
							 cvSet2D(IGT, i, j, cvScalar(255,255,255));
							//cvSet2D(IGT, i, j, cvGet2D(IGT,i,j));//For Evolution On Original Image
						 }

						 else									//Dying
						 {
							 B.at<double>(i,j) = 0;
							 cvSet2D(IGT, i, j, cvScalar(0,0,255));
						 }
				}
*/
				else
				{
					//If <2 OR >3 Moore Neighbours
					if ((int)(C.at<double>(i,j)/N)<Down | (int)(C.at<double>(i,j)/N)>Up)
					{
						if ((int)(B.at<double>(i,j)/N) == 0)	//Dead
						 {
							 B.at<double>(i,j) = 0;
							 cvSet2D(IGT, i, j, cvScalar(255,255,255));
							//cvSet2D(IGT, i, j, cvGet2D(IGT,i,j));//For Evolution On Original Image
						 }

						 else									//Dying
						 {
							 B.at<double>(i,j) = 0;
							 cvSet2D(IGT, i, j, cvScalar(0,0,255));
							 Deaths += 1;
						 }
					}

					//If 2 Moore Neighbours
					else if ((int)(C.at<double>(i,j)/N) == Down)
					{
						if((int)(B.at<double>(i,j)/N) == 0)		//Dead
						{
							 B.at<double>(i,j) = 0;
							 cvSet2D(IGT, i, j, cvScalar(255,255,255));
							 //cvSet2D(IGT, i, j, cvGet2D(IGT,i,j));//For Evolution On Original Image
						}

						else									//Staying Alive
						{
							 B.at<double>(i,j) = 1;
							 cvSet2D(IGT, i, j, cvScalar(0,255,0));
						}
					}

					//If 3 Moore Neighbours
					else
					{
						if((int)(B.at<double>(i,j)/N) == 0)			//Coming Alive
						{
							 B.at<double>(i,j) = 1;
							 cvSet2D(IGT, i, j, cvScalar(255,0,0));
							 Births += 1;
						}

						else									//Staying Alive
						{
							 B.at<double>(i,j) = 1;
							 cvSet2D(IGT, i, j, cvScalar(0,255,0));
						}
					}
				}
			}
		}

		//Displaying Colour Evolution[Image Size Scaled Up]
//		cvLine(IGT,cvPoint(0,0),cvPoint(0,2*K),cvScalar(0,255,255));
//		cvLine(IGT,cvPoint(0,2*K),cvPoint(2*K,2*K),cvScalar(0,255,255));
//		cvLine(IGT,cvPoint(2*K,2*K),cvPoint(2*K,0),cvScalar(0,255,255));
//		cvLine(IGT,cvPoint(2*K,0),cvPoint(0,0),cvScalar(0,255,255));

		cvLine(IGT,cvPoint(IGT->width/2-K,IGT->height/2-K),cvPoint(IGT->width/2+K,IGT->height/2-K),cvScalar(255,255,0));
		cvLine(IGT,cvPoint(IGT->width/2+K,IGT->height/2-K),cvPoint(IGT->width/2+K,IGT->height/2+K),cvScalar(255,255,0));
		cvLine(IGT,cvPoint(IGT->width/2+K,IGT->height/2+K),cvPoint(IGT->width/2-K,IGT->height/2+K),cvScalar(255,255,0));
		cvLine(IGT,cvPoint(IGT->width/2-K,IGT->height/2+K),cvPoint(IGT->width/2-K,IGT->height/2-K),cvScalar(255,255,0));

		cvPyrUp(IGT,IFT);
		cvShowImage("Evolution Environment : GOL_Small",IGT);
		cvShowImage("Evolution Environment : GOL",IFT);

		//Displaying B&W Evolution[Image Size Scaled Down]
		//cvShowImage("Evolution Environment : GOL",IGray);
		//imshow("B&W Evolution Environment",B);

		//Copying Back Original Image(For Evolution on Original Image)
		cvCvtColor(IG,IGT,CV_BGR2RGB);
		cvCvtColor(IGT,IGT,CV_RGB2BGR);

		//Evolution Generations
		Gen++;

		//Loop Timing
		STOP_TIMING(T);
		//printf("%fms\t",GET_TIMING(T));
		fprintf(fp, "%d, %d\n", Births, Deaths);
		
		//Exiting Evolution Loop
		if((char)cvWaitKey(20)==27)
		{
			//Evaluating & Displaying Average Loop Timing
			printf("\n\nAverage Loop Time:%fms\nEvolution Generations: %d",GET_AVERAGE_TIMING(T),Gen);
			cvPyrUp(IFT,ISave2);
			cvPyrUp(ISave2,ISave4);
			cvSaveImage("C:\\New Downloads\\Life_2.jpg",ISave4);

			//Escape Sequence
			if((char)cvWaitKey(0)==27)
			{
				CLEAR_AVERAGE_TIMING(T);
				DEALLOC();
				fclose(fp);
				return 0;
			}
		}
	}

	//Deallocating & Exiting Program
	DEALLOC();
	fclose(fp);
	return 0;
}

/*
						ORIGINAL GOL
#include "stdafx.h"
#include <cv.h>
#include <highgui.h>

using namespace cv;
using namespace std;

// Record the execution time of some code, in milliseconds.
#define DECLARE_TIMING(s)  int64 timeStart_##s; double timeDIF0f_##s; double timeTally_##s = 0; int countTally_##s = 0
#define START_TIMING(s)    timeStart_##s = cvGetTickCount()
#define STOP_TIMING(s) 	   timeDIF0f_##s = (double)(cvGetTickCount() - timeStart_##s); timeTally_##s += timeDIF0f_##s; countTally_##s++
#define GET_TIMING(s) 	   (double)(timeDIF0f_##s / (cvGetTickFrequency()*1000.0))
#define GET_AVERAGE_TIMING(s)   (double)(countTally_##s ? timeTally_##s/ ((double)countTally_##s * cvGetTickFrequency()*1000.0) : 0)
#define CLEAR_AVERAGE_TIMING(s) timeTally_##s = 0; countTally_##s = 0

#define DEALLOC() cvReleaseCapture(&Cap);	cvDestroyAllWindows()

int main()
{
	//Initialising Timer
	DECLARE_TIMING(T);

	//Initialising Camera
	IplImage* IF;
	CvCapture* Cap = cvCreateCameraCapture(0);

	//Capturing Image
	for(int i=0; i<20; i++)
		IF = cvQueryFrame(Cap);

//	IF = cvLoadImage("C:\\New Downloads\\Dot1.jpg");

	//Error Handling
	if(!IF)	return 0;

	//Initialising Image Data Structures
	IplImage* IFT		= cvCreateImage(cvSize(IF->width,IF->height), 8, 3);
	IplImage* IG		= cvCreateImage(cvSize(IF->width/2,IF->height/2), 8, 3);
	IplImage* IGT		= cvCreateImage(cvSize(IF->width/2,IF->height/2), 8, 3);
	IplImage* IGray		= cvCreateImage(cvSize(IF->width/2,IF->height/2), 8, 1);

	//Scaling Down Captured Image
	cvPyrDown(IF,IG);

	//Converting 2 Grayscale
	cvCvtColor(IG,IG,CV_BGR2HSV);
//	cvCvtColor(IG,IGray,CV_HSV2GRAY);
	cvSplit(IG,IGray,0,0,0);

	//Copying IG to IGT
	cvCvtColor(IG,IGT,CV_BGR2RGB);
	cvCvtColor(IGT,IGT,CV_RGB2BGR);

	//Release Camera
	cvReleaseCapture(&Cap);

	//Initialising GOL Matrices
	Mat C = cvarrToMat(IGray);			//Converting Convolution Image to Mat Format-[3 Channel]
	Mat B = (C>100 & C < 130);					//Obtaining Binary Image in Mat Format[C>15 has no effect apart from defining B as B&W Image]
	IplImage GrayScale = C;
	IplImage SeedState = B;
	//Performing Histogram Equlaization
	equalizeHist(C,B);
	cvShowImage("Original",&GrayScale);
	cvShowImage("Histogram Equalized",&SeedState);

	//Image Converted To 64-Bit Floating Type & Scaled to 1
	B.convertTo (B, CV_64F, 1.0 / 255.0);

	C.convertTo (C, CV_64F, 1.0 / 255.0);

	//Defining Kernel
	double N = 1.0 / 8.0;
	Mat Ker = Mat::ones(3,3,CV_64F)*N;	//Setting Elements to N;
//	Ker.at<double>(2,2) = 0;			//Setting Centre Element to 0;
//	Ker.at<double>(0,0) = 0;
//	Ker.at<double>(0,2) = 0;
//	Ker.at<double>(2,0) = 0;
	Ker.at<double>(1,1) = 0;

	//Evolution Generations
	int Gen = 0;

	//Evolution Loop Started
	while(1)
	{
		//Loop Timing
		START_TIMING(T);

		//Performing Convolution On 'B'(Source{Initialised}) wrt 'Ker'(Kernel) & Storing in 'C'(Destination)
		filter2D(B, C, -1, Ker, Point(-1,-1), 0, BORDER_REFLECT);

		//Updating Generation
		for(int i=0;i<C.rows;i++)
		{
			for(int j=0;j<C.cols;j++)
			{

				//If <2 OR >3 Moore Neighbours
				if ((int)(C.at<double>(i,j)/N)<2 | (int)(C.at<double>(i,j)/N)>3)
				{
					if ((int)(B.at<double>(i,j)/N) == 0)	//Dead
					 {
						 B.at<double>(i,j) = 0;
						 cvSet2D(IGT, i, j, cvScalar(255,255,255));
						 //cvSet2D(IGT, i, j, cvGet2D(IGT,i,j));//For Evolution On Original Image
					 }

					 else									//Dying
					 {
						 B.at<double>(i,j) = 0;
						 cvSet2D(IGT, i, j, cvScalar(0,0,255));
					 }
				}

				//If 2 Moore Neighbours
				else if ((int)(C.at<double>(i,j)/N) == 2)
				{
					if((int)(B.at<double>(i,j)/N) == 0)		//Dead
					{
						 B.at<double>(i,j) = 0;
						 cvSet2D(IGT, i, j, cvScalar(255,255,255));
						 //cvSet2D(IGT, i, j, cvGet2D(IGT,i,j));//For Evolution On Original Image
					}

					else									//Staying Alive
					{
						 B.at<double>(i,j) = 1;
						 cvSet2D(IGT, i, j, cvScalar(0,255,0));
					}
				}

				//If 3 Moore Neighbours
				else
				{
					if((int)(B.at<double>(i,j)/N) == 0)			//Coming Alive
					{
						 B.at<double>(i,j) = 1;
						 cvSet2D(IGT, i, j, cvScalar(255,0,0));
					}

					else									//Staying Alive
					{
						 B.at<double>(i,j) = 1;
						 cvSet2D(IGT, i, j, cvScalar(0,255,0));
					}
				}
			}
		}

		//Displaying Colour Evolution[Image Size Scaled Up]
		cvPyrUp(IGT,IFT);
		cvShowImage("Evolution Environment : GOL",IFT);

		//Displaying B&W Evolution[Image Size Scaled Down]
		//cvShowImage("Evolution Environment : GOL",IGray);
		//imshow("B&W Evolution Environment",B);

		//Copying Back Original Image(For Evolution on Original Image)
		cvCvtColor(IG,IGT,CV_BGR2RGB);
		cvCvtColor(IGT,IGT,CV_RGB2BGR);

		//Evolution Generations
		Gen++;

		//Loop Timing
		STOP_TIMING(T);
		printf("%fms\t",GET_TIMING(T));

		//Exiting Evolution Loop
		if((char)cvWaitKey(20)==27)
		{
			//Evaluating & Displaying Average Loop Timing
			printf("\n\nAverage Loop Time:%fms\nEvolution Generations: %d",GET_AVERAGE_TIMING(T),Gen);

			//Escape Sequence
			if((char)cvWaitKey(0)==27)
			{
				CLEAR_AVERAGE_TIMING(T);
				DEALLOC();
				return 0;
			}
		}
	}

	//Deallocating & Exiting Program
	DEALLOC();
	return 0;
}

		LIFE RULES
				//If <2 OR >3 Moore Neighbours
				if (C.at<double>(i,j)<.20 | C.at<double>(i,j)>.38)
				{
					if (B.at<double>(i,j)<.90)	//Dead
					 {
						 B.at<double>(i,j) = 0;
						 cvSet2D(IGT, i, j, cvScalar(255,255,255));
						 //cvSet2D(IGT, i, j, cvGet2D(IGT,i,j));//For Evolution On Original Image
					 }

					 else						//Dying
					 {
						 B.at<double>(i,j) = 0;
						 cvSet2D(IGT, i, j, cvScalar(0,0,255));
					 }
				}

				//If 2 Moore Neighbours
				else if (C.at<double>(i,j)>.24 & C.at<double>(i,j)<.26)
				{
					if(B.at<double>(i,j)<.90)	//Dead
					{
						 B.at<double>(i,j) = 0;
						 cvSet2D(IGT, i, j, cvScalar(255,255,255));
						 //cvSet2D(IGT, i, j, cvGet2D(IGT,i,j));//For Evolution On Original Image
					}

					else						//Staying Alive
					{
						 B.at<double>(i,j) = 1;
						 cvSet2D(IGT, i, j, cvScalar(0,255,0));
					}
				}

				//If 3 Moore Neighbours
				else
				{
					if(B.at<double>(i,j)<.90)	//Coming Alive
					{
						 B.at<double>(i,j) = 1;
						 cvSet2D(IGT, i, j, cvScalar(255,0,0));
					}

					else						//Staying Alive
					{
						 B.at<double>(i,j) = 1;
						 cvSet2D(IGT, i, j, cvScalar(0,255,0));
					}
				}


		RABBITS & WOLVES RULES

	//Conditions For Rabbit Cell
				if(A[i][j]=='X')
				{
					if(T==4 || T==3)				//Conditions To Live
						B[i][j]='X';

					else							//Conditions For Death
						B[i][j]='O';

				}

 				//Conditions For Wolf Cell
				else if(A[i][j]=='Y')
				{
					if(T1>0 && T>0)				//Conditions To Live
						B[i][j]='Y';

					else						//Conditions For Death
						B[i][j]='O';
				}

				//Conditions For Inert Cell
				else if(A[i][j]=='O')
				{
					if(T==3 && T1<4)				//Condition For Birth
						B[i][j]='X';

					else if(T1>1 && T>0)			//Condition For Birth
						B[i][j]='Y';

					else							//Conditions To Remain Dead
						B[i][j]='O';

				}
*/

/*SEED RULES
				if((int)(C.at<double>(i,j)*8)==2)
				{
					if(B.at<double>(i,j) == 1)
					{
						B.at<double>(i,j) = 0;
						cvSet2D(IGT, i, j, cvScalar(0,0,255));
					}

					else
					{
						B.at<double>(i,j) = 1;
						cvSet2D(IGT, i, j, cvScalar(255,0,0));
					}
				}
				else
				{
					if(B.at<double>(i,j) == 1)
					{
						B.at<double>(i,j) = 0;
						cvSet2D(IGT, i, j, cvScalar(0,0,255));
					}

					else
					{
						B.at<double>(i,j) = 0;
						cvSet2D(IGT, i, j, cvScalar(255,255,255));
					}
				}
*/
