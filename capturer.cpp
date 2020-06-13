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

std::string MY_DIR = "";		// The string pointing to the output directory
stringstream file_space;

char *media;

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

bool storage_check(){

   	struct statvfs fiData;
		
    //Lets loopyloop through the argvs
	if((statvfs(media, &fiData)) < 0){
		cout << "\nFailed to stat:"  << endl;
		return false;
	} else {
		
		float free_space = (long long)(fiData.f_bfree)*(long long)fiData.f_bsize * pow(10,-9);

		#ifdef DBG
		cout << free_space << endl;
		#endif
		
		// free space less than 500 MB
		if (free_space<0.5)	return false;
	}	
	return true;
}

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

void helpMenu()
{
	cout << "Usage: " << endl;
	cout << "./capturer [OUTPUT] " << endl;
	cout << "\nIf you don't specify output, current working directory will be used" << endl;
}

int main(int argc , char *argv[])
{
	char *file;
	std::string file_str;
	file 	= (char *)malloc(500 * sizeof(char));
	media 	= (char *)malloc(500 * sizeof(char));
	
	if((file == NULL) || (media == NULL)) {
		cout << "Memory could not be allocated..." << endl;
		free(file);
		return -1;
	}

	if(argc == 1){

		strcpy(file,".");

	}else{
		
		if(argv[1][strlen(argv[1])-1] == '/'){
			strncpy(file,argv[1],strlen(argv[1])-1);
			file[strlen(argv[1])-1] = '\0';
		}else{
			strcpy(file,argv[1]);
		}
	}
	
	
	

	
	file_space.str(file);
	
	DIR *cap_dir = opendir(file);
	if (cap_dir == NULL)
	{
		cerr << "Could not locate directory" << endl;
		free(file);
		exit(1);
	}
	closedir(cap_dir);
	
	file_space.str("");
	file_space << file;
	file_str = file_space.str();

	//cout << file_space.str() << endl;
	const char* dir = file_str.c_str();

	strcpy(media, dir);

	#ifdef DBG
	cout << "media: " << media << endl;
	#endif

	cap_dir = opendir(dir);
	if (cap_dir == NULL){
		cerr << "\nDirectory does not exist ...\n" << endl;
		free(file);
		exit(1);
	}
	
	file_space << "/Captured";
	file_str = file_space.str();

	dir = file_str.c_str();	
	// Checking if Captured Dir is available
	cap_dir = opendir(dir);
	//cout << "\nCap_dir: " << cap_dir << endl;
	if (cap_dir == NULL)
	{
		// if NOT we create it
		const int dir_err = mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if (-1 == dir_err)
		{
			cerr << "\nError creating directory!! \n" << endl;
			free(file);
			exit(1);
		}
	}
	closedir(cap_dir);
	
	// Creating sub-directory regarding date and time
	stringstream current_date_dir;
	time_t t = time(0);   // get time now
    struct tm * now = localtime( & t );
    current_date_dir << file_space.str() << "/" << (now->tm_year + 1900) 
					 << '_' << (now->tm_mon + 1) 
					 << '_' << now->tm_mday
					 << '_' << now->tm_hour 
					 << '_' << now->tm_min 
					 << '_' << now->tm_sec;
	
	std::string date_dir_str = current_date_dir.str();
	const char *cstr = date_dir_str.c_str();
	const int dir_err = mkdir(cstr, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (dir_err == -1)
	{
		cout << "\nError creating directory!! \n" << endl;
		free(file);
		exit(1);
	}

	Mat feed, remapped, remapped_cm;
	
	feed		= Mat (FRAME_HEIGHT, FRAME_WIDTH, CV_16UC1);
	remapped	= Mat (FRAME_HEIGHT, FRAME_WIDTH, CV_8U);
	remapped_cm	= Mat (FRAME_HEIGHT, FRAME_WIDTH, CV_8UC3);

	pOriginal = feed.data;

	current_date_dir << "/";
	// Finalizing setting up directories
	MY_DIR = current_date_dir.str();
	std::cout << MY_DIR << std::endl;

	bool view = true;
		
	std::cout << "main:" << std::endl;
	{
		Capture* c = new Capture();
		c->capture();
		
		while(waitKey(50) != 27){ // ESC
		    try{
				if(!storage_check()){
					cerr << "\nOut of storage..." << endl;
					break;
				}

				if(frame_num > photo_old){
					{
						//std::lock_guard<std::mutex> guard(mutex_bmp_update);
						mutex_bmp_update.lock();

						photo_old = frame_num;

						if(frame_num % 10 == 0) cout << "OK..." << endl;

						// setting up the names of the photos
						stringstream photo_name,photo_RGB_name;
						photo_name 		<< MY_DIR << "FLIR_" << frame_num << ".tif";
						photo_RGB_name	<< MY_DIR << "FLIR_" << frame_num << ".png";

						// Saving RAW -> .tiff photo
						imwrite(photo_name.str(), feed);
					
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
		
						// Saving RGB -> .png photo
						imwrite(photo_RGB_name.str(), remapped);

						if(view){
							applyColorMap(remapped, remapped_cm, COLORMAP_TURBO); // COLORMAP_JET , COLORMAP_INFERNO
							imshow("Feed", remapped_cm);
						}

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

