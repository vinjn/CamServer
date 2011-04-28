#include "ofxKinectCLNUI.h"

ofxKinectCLNUI::ofxKinectCLNUI()
{
	kinectCam = NULL;
}

bool ofxKinectCLNUI::initKinect(int wid,int hei,double lowbound,double upbound)
{
	bool b = false;
	upBound = upbound;
	lowBound = lowbound;

	camWidth = wid;
	camHeight = hei;
	kinectCam= CreateNUICamera();
	b = StartNUICamera(kinectCam);
	//depth image
	depthShortImage = cvCreateImage(cvSize(camWidth,camHeight),IPL_DEPTH_16U,1);
	//BW image
	bwImage = cvCreateImage(cvSize(camWidth,camHeight),IPL_DEPTH_8U,1);
	//BGR image
	bgrImage = cvCreateImage(cvSize(camWidth,camHeight),IPL_DEPTH_8U,3);
	//RGB image
	rgbImage = cvCreateImage(cvSize(camWidth,camHeight),IPL_DEPTH_8U,3);
	//pseudo-color depth image
	depthColorImage = cvCreateImage(cvSize(camWidth,camHeight),IPL_DEPTH_8U,3);

	depthData = new USHORT[camWidth*camHeight];
	colorData = new BYTE[camWidth*camHeight*3];
	depthColorData = new DWORD[camWidth*camHeight];

	return b;
}

void ofxKinectCLNUI::setLowBound(double bound)
{
	lowBound = bound;
}

void ofxKinectCLNUI::setUpBound(double bound)
{
	upBound = bound;
}

bool ofxKinectCLNUI::getDepthBW()
{
	bool b = GetNUICameraDepthFrameRAW(kinectCam, depthData);//gray scale depth image
	
	#if 0

	for( int y=0; y<camHeight; y++ ) {
		USHORT* ptr = (USHORT*) (depthShortImage->imageData + y * depthShortImage->widthStep	);

		for( int x=0; x<camWidth; x++ ) {
			ptr[x] = depthData[y*camWidth+x];
		}
	}
	//we use cvInRangeS() to do the thresholding
	cvInRangeS( depthShortImage, cvScalar(lowBound), cvScalar(upBound), bwImage );
	
	#else
	
	cvSetData(depthShortImage, depthData, depthShortImage->widthStep);
	cvConvertScale(depthShortImage,bwImage,65535.0/255.0,0);

	#endif

	return b;
}

//http://nicolas.burrus.name/index.php/Research/KinectCalibration
float raw_depth_to_meters(int raw_depth)
{
  if (raw_depth < 2047)
  {
   return 1.0f / (raw_depth * -0.0030711016 + 3.3309495161);
  }
  return 0;
}

//http://goo.gl/tsxtm
void raw_depth_to_meters2(int raw_depth)
{
	for (size_t i=0; i<2048; i++) 
	{ 
		const float k1 = 1.1863; 
		const float k2 = 2842.5; 
		const float k3 = 0.1236; 
		const float depth = k3 * tanf(i/k2 + k1); 
	//	t_gamma[i] = depth; 
	} 
}





bool ofxKinectCLNUI::getDepthColorImage()
{
	bool b = GetNUICameraDepthFrameRGB32(kinectCam,depthColorData);//pseudo color depth image

	for( int y=0; y<camHeight; y++ ) {
		uchar* ptrC = (uchar*) (depthColorImage->imageData + y * depthColorImage->widthStep	);
		for( int x=0; x<camWidth; x++ ) {
			DWORD depth = depthColorData[y*camWidth+x];
			uchar r = depth>>16 & 0xff;
			uchar g = depth>>8 & 0xff;
			uchar b = depth & 0xff;
			ptrC[3*x] = b;
			ptrC[3*x+1] = g;
			ptrC[3*x+2] = r;
		}
	}
	return b;
}

bool ofxKinectCLNUI::getColorImage()
{
	bool b = GetNUICameraColorFrameRGB24(kinectCam, colorData);
	bgrImage->imageData = (char*)colorData;
	cvCvtColor(bgrImage,rgbImage,CV_BGR2RGB);
	return b;
}
int ofxKinectCLNUI::getCamWidth()
{
	return camWidth;
}

int ofxKinectCLNUI::getCamHeight()
{
	return camHeight;
}

ofxKinectCLNUI::~ofxKinectCLNUI()
{
	if (kinectCam != NULL)
	{
		StopNUICamera(kinectCam);
		DestroyNUICamera(kinectCam);
	}

	cvReleaseImage(&bgrImage);
	cvReleaseImage(&rgbImage);
	cvReleaseImage(&depthShortImage);
	cvReleaseImage(&depthColorImage);
	cvReleaseImage(&bwImage);

	delete[] depthData;
	delete[] colorData;
	delete[] depthColorData;
}