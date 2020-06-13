#include "thermalgrabber.h"
#include <sstream>
#include <stdbool.h>
#include <cstdlib>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <ctime>

#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <dirent.h>

//OpenCV includes
//#include <opencv2/contrib/contrib.hpp>
#include <opencv2/core/core.hpp>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

//#define DBG

using namespace std;
using namespace cv;

ThermalGrabber* tGr;			// Our thermal grabber

std::mutex mutex_bmp_update;    // Concurrency

volatile long frame_num = 0;	// photo serial number
int photo_old = -1;

unsigned int mWidth  = -1;		// Captured frame width
unsigned int mHeight = -1;		// Captured frame Height

const int FRAME_WIDTH 	= 640;
const int FRAME_HEIGHT 	= 512;
const int FRAME_SIZE	= FRAME_WIDTH * FRAME_HEIGHT * 2;

unsigned char 		*pOriginal;	// Raw Mat pointer
short unsigned int 	*pTauRaw;	// Bitmap pointer

class Capture
{
public:
    void capture();
};

void callbackTauImage(TauRawBitmap& tauRawBitmap, void* caller){
	
	if (tauRawBitmap.data == NULL || tauRawBitmap.width != mWidth || tauRawBitmap.height != mHeight) {
		return;
	}

    // Locked scope
	if(mutex_bmp_update.try_lock()){

		// remapping values
		pTauRaw = tauRawBitmap.data;
		memcpy(pOriginal , pTauRaw, FRAME_SIZE);

		frame_num++;

		mutex_bmp_update.unlock();
	}
}

void Capture::capture()
{
    std::cout << "Capture" << std::endl;
    tGr = new ThermalGrabber(callbackTauImage, this);

    mWidth 	= tGr->getResolutionWidth();
    mHeight = tGr->getResolutionHeight();
    std::cout << "Resolution w/h " << mWidth << "/" << mHeight << std::endl;

    // enable TLinear in high resolution on TauCores
    tGr->enableTLinearHighResolution();
}


int main(int argc , char *argv[])
{
	Mat feed, remapped, remapped_cm;
	
	feed		= Mat (FRAME_HEIGHT, FRAME_WIDTH, CV_16UC1);
	remapped	= Mat (FRAME_HEIGHT, FRAME_WIDTH, CV_8U);
	remapped_cm = Mat (FRAME_HEIGHT, FRAME_WIDTH, CV_8UC3);

	pOriginal = feed.data;

	std::cout << "main:" << std::endl;
	{
		Capture* c = new Capture();
		c->capture();
		
		while(waitKey(50) != 27){ // ESC
		    try{
				
				if(frame_num > photo_old){
					{
						//std::lock_guard<std::mutex> guard(mutex_bmp_update);
						mutex_bmp_update.lock();

						photo_old = frame_num;

						ushort min16bit=65535;
						ushort max16bit=0;

						// Remap
						ushort pixel;
						for(int i=0; i < FRAME_WIDTH; i++){
							for(int j=0; j < FRAME_HEIGHT; j++){
								pixel = feed.at<ushort>(j, i);
								if(pixel>max16bit){
									max16bit=pixel;
								}else if(pixel<min16bit){
									min16bit=pixel;
								}
							}
						}
					
						//cout << "\nMax - Min " << (max16bit - min16bit)<< endl;
						double lamda = double(255) / double(max16bit - min16bit);
						//cout << "\nLamda: " << lamda << endl;feed

						for(int i=0; i < FRAME_WIDTH; i++){
							for(int j=0; j < FRAME_HEIGHT; j++){
								remapped.at<uchar>(j, i) = (feed.at<ushort>(j, i) - min16bit)*lamda; //+ new_min;
							}
						}
		
						
						applyColorMap(remapped, remapped_cm, COLORMAP_TURBO); // COLORMAP_JET , COLORMAP_INFERNO, COLORMAP_TURBO
						imshow("Feed", remapped_cm);
						

						mutex_bmp_update.unlock();
					}
				
				}
		    }catch(Exception e){
				cerr << "Error" << endl;
			}
		}
		
		delete c;
		delete tGr;
	}
	cout << "Exitting..." << endl;
	// free unused memory
	feed.release();
	remapped.release();
	remapped_cm.release();

   return 0;
}

