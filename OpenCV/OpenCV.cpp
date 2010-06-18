#include "OpenCV.h"

#ifdef _DEBUG
	#pragma comment(lib,"cv210d.lib")
	#pragma comment(lib,"cvaux210d.lib")
	#pragma comment(lib,"cxcore210d.lib")
	#pragma comment(lib,"highgui210d.lib")
#else
	#pragma comment(lib,"cv210.lib")
	#pragma comment(lib,"cvaux210.lib")
	#pragma comment(lib,"cxcore210.lib")
	#pragma comment(lib,"highgui210.lib")
#endif

 
void vCopyImageTo(CvArr* tiny_image, IplImage* big_image, const CvRect& region)
{
	CV_Assert(tiny_image && big_image);
	// Set the image ROI to display the current image
	cvSetImageROI(big_image, region);

	IplImage* t = (IplImage*)tiny_image;
	if (t->nChannels == 1 && big_image->nChannels == 3)
	{
		Ptr<IplImage> _tiny = cvCreateImage(cvGetSize(t), 8, 3);
		cvCvtColor(tiny_image, _tiny, CV_GRAY2BGR);
		cvResize(_tiny, big_image);
	}
	else
	{
		// Resize the input image and copy the it to the Single Big Image
		cvResize(tiny_image, big_image);
	}
	// Reset the ROI in order to display the next image
	cvResetImageROI(big_image);
}

void vDrawText(IplImage* img, int x,int y,char* str, CvScalar clr)
{
	static CvFont* font = NULL;

	if (!font)
	{
		font = new CvFont();
		cvInitFont(font,CV_FONT_VECTOR0,0.5,0.5, 0, 1);
	}
	cvPutText(img, str, cvPoint(x,y),font, clr); 
}


CvScalar default_colors[] = 
{
	{{255,128,0}},
	{{255,255,0}},
	{{0,0,255}},
	{{0,128,255}},
	{{0,255,255}},
	{{0,255,0}},
	{{255,0,0}},
	{{255,0,255}}
};

const int sizeOfColors = sizeof(default_colors)/sizeof(CvScalar);
CvScalar vDefaultColor(int idx){ return default_colors[idx%sizeOfColors];}

void HaarDetection::detect_object( IplImage* img, vector<CvRect>& regions)
{
	double scale = 1.5;
	regions.clear();

	Ptr<IplImage> gray = cvCreateImage( cvSize(img->width,img->height), 8, 1 );
	Ptr<IplImage> tiny = cvCreateImage( cvSize( cvRound (img->width/scale),
		cvRound (img->height/scale)), 8, 1 );

	vGrayScale(img, gray);
	cvResize( gray, tiny );
	cvEqualizeHist( tiny, tiny );
	cvClearMemStorage( storage );

	if( cascade )
	{
		double t = (double)cvGetTickCount();
		CvSeq* found = cvHaarDetectObjects( tiny, cascade, storage,
			1.1, 2, 0
			//|CV_HAAR_FIND_BIGGEST_OBJECT
			//|CV_HAAR_DO_ROUGH_SEARCH
			|CV_HAAR_DO_CANNY_PRUNING
			//|CV_HAAR_SCALE_IMAGE
			,
			cvSize(30, 30) );
		t = (double)cvGetTickCount() - t;
		//printf( "detection time = %gms\n", t/((double)cvGetTickFrequency()*1000.) );
		for(int i = 0; i < (found ? found->total : 0); i++ )
		{
			CvRect* r = (CvRect*)cvGetSeqElem( found, i );
			CvRect rr = cvRect(r->x*scale, r->y*scale, r->width*scale,r->height*scale);
			regions.push_back(rr);

			//CvPoint center;
			//CvScalar color = vDefaultColor(i);
			//int radius;
			//center.x = cvRound((r->x + r->width*0.5)*scale);
			//center.y = cvRound((r->y + r->height*0.5)*scale);
			//radius = cvRound((r->width + r->height)*0.25*scale);

			////
			//CvBox2D b;
			//b.angle = 0;
			//b.center.x = cvRound((r->x + r->width*0.5)*scale);
			//b.center.y = cvRound((r->y + r->height*0.5)*scale);
			//b.size = cvSize2D32f(r->width*scale, r->height*scale);
			//cvEllipseBox(img, b, color, 3, 8, 0);
			//cvCircle( img, center, radius, color, 3, 8, 0 );
			//let the main loop render it

			//{//fake
			//	face_detected = true;
			//	face_rect.x = center.x - radius;
			//	face_rect.y = center.y - radius;
			//	face_rect.width = radius;
			//	face_rect.height = radius;
			//}
		}
	}
}


char* get_time(bool full_length)
{
	static char str[MAX_PATH];
	time_t timep;
	tm *p;
	time(&timep);
	p = gmtime(&timep);

	if (full_length)
		sprintf(str, "%d-%d-%d__%d_%d_%d",
		1900+p->tm_year, 1+p->tm_mon, p->tm_mday,
		p->tm_hour, p->tm_min, p->tm_sec);
	else
		sprintf(str, "%d_%d_%d",
		p->tm_hour, p->tm_min, p->tm_sec);

	return str;
}

void feature_out(IplImage* img, IplImage* mask, int thresh)
{
	int w = img->width;
	int h = img->height;
	int step = img->widthStep;
	int channels = img->nChannels;
	uchar* data   = (uchar *)img->imageData;
	uchar* mdata   = (uchar *)mask->imageData;

	for(int i=0;i<h;i++) 
		for(int j=0;j<w;j++) 
			for(int k=0;k<channels;k++)
			{
				if (mdata[i*mask->widthStep+j] < thresh)
					data[i*step+j*channels+k] = 0;
			}

} 


//Just some convienience macros
#define CV_CVX_WHITE	CV_RGB(0xff,0xff,0xff)
#define CV_CVX_BLACK	CV_RGB(0x00,0x00,0x00)

void vFindBlobs(IplImage *src, vector<vBlob>& blobs, int morph_itr, bool poly1_hull0, float areaScale)
{
	static CvMemStorage*	mem_storage	= NULL; 	
	
	//CLEAN UP RAW MASK
	if (morph_itr > 0)
	{
		vOpen( src, morph_itr );
		vClose( src, morph_itr );
	}

	//FIND CONTOURS AROUND ONLY BIGGER REGIONS
	if( mem_storage==NULL ) mem_storage = cvCreateMemStorage(0);
	else cvClearMemStorage(mem_storage);
	blobs.clear();
	
	double minArea = src->height * src->width*areaScale;  //calculate area threshold

	CvSeq* first_contour = 0;
	cvFindContours(src,mem_storage,&first_contour, sizeof(CvContour),CV_RETR_CCOMP,CV_CHAIN_APPROX_SIMPLE);	

	for (CvSeq* d = first_contour; d != NULL; d=d->h_next)
	{
		bool isHole = false;
		CvSeq* c = d;
		while (c != NULL)
		{
			double area = fabs(cvContourArea( c ));
			if( area >= minArea)
			{
				CvSeq* contour;
				if(poly1_hull0) //Polygonal approximation of the segmentation
					contour = cvApproxPoly(c,sizeof(CvContour),mem_storage,CV_POLY_APPROX_DP, CVCONTOUR_APPROX_LEVEL,0);
				else //Convex Hull of the segmentation
					contour = cvConvexHull2(c,mem_storage,CV_CLOCKWISE,1);

				//Bounding rectangles around blobs			
				Rect box = cvBoundingRect(contour);
				Point center;
				center.x = box.x + box.width/2;
				center.y = box.y + box.height/2;
				vBlob blob(box, center, area);
				blob.isHole = isHole;
				
				for (int i=0;i<contour->total;i++)
					blob.pts.push_back(*((Point*)cvGetSeqElem(contour, i)));
				
				blobs.push_back(blob);

				cvDrawContours(src,contour,CV_CVX_WHITE,CV_CVX_WHITE,-1,CV_FILLED,8); //draw to src
			}//END if( area >= minArea)

			if (isHole)
				c = c->h_next;//洞的h_next是洞
			else
				c = c->v_next;//轮廓点v_next是洞
			isHole = true;
		}//END while (c != NULL)
	} 
}



void vFindBlobs(IplImage *src, int morph_itr, bool poly1_hull0, float areaScale)
{
	static CvMemStorage*	mem_storage	= NULL;

	//CLEAN UP RAW MASK
	if (morph_itr > 0)
	{
		vOpen( src, morph_itr );
		vClose( src, morph_itr );
	}

	//FIND CONTOURS AROUND ONLY BIGGER REGIONS
	if( mem_storage==NULL ) mem_storage = cvCreateMemStorage(0);
	else cvClearMemStorage(mem_storage);
	
	double minArea = src->height * src->width*areaScale;  //calculate area threshold

	CvContourScanner scanner = cvStartFindContours(src,mem_storage,sizeof(CvContour),CV_RETR_EXTERNAL,CV_CHAIN_APPROX_SIMPLE);

	CvSeq* c;
	while( c = cvFindNextContour( scanner ) )
	{
		double area = fabs(cvContourArea( c ));
		if( area >= minArea)
		{
			CvSeq* contour;
			if(poly1_hull0) //Polygonal approximation of the segmentation
				contour = cvApproxPoly(c,sizeof(CvContour),mem_storage,CV_POLY_APPROX_DP, CVCONTOUR_APPROX_LEVEL,0);
			else //Convex Hull of the segmentation
				contour = cvConvexHull2(c,mem_storage,CV_CLOCKWISE,1); 

			cvDrawContours(src,contour,CV_CVX_WHITE,CV_CVX_WHITE,-1,CV_FILLED,8); //draw to src
		}
	}
	cvEndFindContours( &scanner );	
}





// various tracking parameters (in seconds)
const double MHI_DURATION = 1;
const double MAX_TIME_DELTA = 0.5;
const double MIN_TIME_DELTA = 0.05;

vector<vBlob>  update_mhi( IplImage* silh, IplImage* dst )
{
	// temporary images
	static IplImage *mhi = 0; // MHI
	static IplImage *orient = 0; // orientation
	static IplImage *mask = 0; // valid orientation mask
	static IplImage *segmask = 0; // motion segmentation map
	static CvMemStorage* storage = 0; // temporary storage

	double timestamp = (double)clock()/CLOCKS_PER_SEC; // get current time in seconds
	CvSize size = cvSize(silh->width,silh->height); // get current frame size
	CvSeq* seq;
	CvRect comp_rect;
	double count;
	double angle;
	CvPoint center;
	double magnitude;          
	CvScalar color;

	vector<vBlob> result;

	// allocate images at the beginning or
	// reallocate them if the frame size is changed
	if( !mhi || mhi->width != size.width || mhi->height != size.height ) {

		cvReleaseImage( &mhi );
		cvReleaseImage( &orient );
		cvReleaseImage( &segmask );
		cvReleaseImage( &mask );

		mhi = cvCreateImage( size, IPL_DEPTH_32F, 1 );
		cvZero( mhi ); // clear MHI at the beginning
		orient = cvCreateImage( size, IPL_DEPTH_32F, 1 );
		segmask = cvCreateImage( size, IPL_DEPTH_32F, 1 );
		mask = cvCreateImage( size, IPL_DEPTH_8U, 1 );
	}

	//    cvThreshold( silh, silh, diff_threshold, 1, CV_THRESH_BINARY ); // and threshold it
	cvUpdateMotionHistory( silh, mhi, timestamp, MHI_DURATION ); // update MHI

	// convert MHI to blue 8u image
	cvCvtScale( mhi, mask, 255./MHI_DURATION,
		(MHI_DURATION - timestamp)*255./MHI_DURATION );
	cvZero( dst );
	cvCvtPlaneToPix( mask, 0, 0, 0, dst );

	// calculate motion gradient orientation and valid orientation mask
	cvCalcMotionGradient( mhi, mask, orient, MAX_TIME_DELTA, MIN_TIME_DELTA, 3 );

	if( !storage )
		storage = cvCreateMemStorage(0);
	else
		cvClearMemStorage(storage);

	// segment motion: get sequence of motion components
	// segmask is marked motion components map. It is not used further
	seq = cvSegmentMotion( mhi, segmask, storage, timestamp, MAX_TIME_DELTA );

	// iterate through the motion components,
	// One more iteration (i == -1) corresponds to the whole image (global motion)
	for(int i = -1; i < seq->total; i++ ) {

		if( i < 0 ) { // case of the whole image
			comp_rect = cvRect( 0, 0, size.width, size.height );
			color = CV_RGB(255,255,255);
			magnitude = 50;
		}
		else { // i-th motion component
			comp_rect = ((CvConnectedComp*)cvGetSeqElem( seq, i ))->rect;
			if( comp_rect.width + comp_rect.height < 75 ) // reject very tiny components
				continue;
			color = CV_RGB(255,0,0);
			magnitude = 25;
		}

		// select component ROI
		cvSetImageROI( silh, comp_rect );
		cvSetImageROI( mhi, comp_rect );
		cvSetImageROI( orient, comp_rect );
		cvSetImageROI( mask, comp_rect );

		// calculate orientation
		angle = cvCalcGlobalOrientation( orient, mask, mhi, timestamp, MHI_DURATION);
		angle = 360.0 - angle;  // adjust for images with top-left origin

		count = cvNorm( silh, 0, CV_L1, 0 ); // calculate number of points within silhouette ROI

		cvResetImageROI( mhi );
		cvResetImageROI( orient );
		cvResetImageROI( mask );
		cvResetImageROI( silh );

		// check for the case of little motion
		if( count < comp_rect.width*comp_rect.height * 0.05 )
			continue;

		// draw a clock with arrow indicating the direction
		center = cvPoint( (comp_rect.x + comp_rect.width/2),
			(comp_rect.y + comp_rect.height/2) );

		cvCircle( dst, center, cvRound(magnitude*1.2), color, 3, CV_AA, 0 );
		cvLine( dst, center, cvPoint( cvRound( center.x + magnitude*cos(angle*CV_PI/180)),
			cvRound( center.y - magnitude*sin(angle*CV_PI/180))), color, 3, CV_AA, 0 );

		result.push_back(vBlob(comp_rect, center, angle));
	}

	return result;
}


VideoInput::VideoInput()
{
	_fps = 0;
	_capture = NULL; 
	_frame = NULL;
	_InputType = From_Count;
}

bool VideoInput::init(int cam_idx)
{
	_capture = cvCaptureFromCAM(CV_CAP_DSHOW+cam_idx);

	if( !_capture )
	{
		printf("Could not initialize camera #%d.\n", cam_idx);
		return false;
	}
	else
	{
		_InputType = From_Camera;
		printf("Loaded from camera #%d.\n", cam_idx);
		_post_init();
		return true;
	}
}

bool VideoInput::init(char* video_file)
{
	_frame = cvLoadImage(video_file);

	if (_frame)
	{
		printf("Loaded from image %s.\n", video_file);
		_InputType = From_Image;
	}
	else
	{
		_capture = cvCaptureFromAVI(video_file);
		if( _capture )
		{
			printf("Loaded from video %s.\n", video_file);
			_InputType = From_Video;
		}
		else
		{
			printf("Could not open file %s.\n", video_file);
			return false;
		}
	}
		
	_post_init();
	return true;
	
}

bool VideoInput::init(int argc, char** argv)
{
	if( argc == 1 || (argc == 2 && strlen(argv[1]) == 1 && ::isdigit(argv[1][0])))
		return init( argc == 2 ? argv[1][0] - '0' : 0 );
	else if( argc == 2 )
		return init( argv[1] );
	return false;
}

void VideoInput::wait(int t)
{
	if (_InputType == From_Image)
		return;
	for (int i=0;i<t;i++)
		cvQueryFrame(_capture);
}

IplImage* VideoInput::get_frame()
{
	if (_InputType != From_Image)
		_frame = cvQueryFrame(_capture);
	return _frame;
}

void VideoInput::_post_init()
{
	if (_InputType != From_Image)
	{
		_frame = get_frame();
		_fps = cvGetCaptureProperty(_capture, CV_CAP_PROP_FPS);
		_codec = cvGetCaptureProperty(_capture, CV_CAP_PROP_FOURCC);
	}
	if (_fps == 0)
		_fps = 30;

	_size.width = _frame->width;
	_size.height = _frame->height;
	_half_size.width  = _size.width/2;
	_half_size.height  = _size.height/2;
	_channel = _frame->nChannels;	

	printf("【输入参数】fps: %d, size: %d,%d\n", _fps, _size.width, _size.height);
}

VideoInput::~VideoInput()
{
	if (_capture != NULL)
		cvReleaseCapture( &_capture );
//	if (_frame != NULL)
//		cvReleaseImage(&_frame);
}



HaarDetection::HaarDetection()
{
	cascade = NULL;
	storage = NULL;  
}

HaarDetection::~HaarDetection()
{
	if (cascade) cvReleaseHaarClassifierCascade(&this->cascade);
	if (storage) cvReleaseMemStorage(&this->storage); 
}

bool HaarDetection::init(char* cascade_name)
{
	cascade = (CvHaarClassifierCascade*)cvLoad( cascade_name, 0, 0, 0 );
	storage = cvCreateMemStorage(0);

	return cascade && storage;
}


vBackCodeBook::vBackCodeBook()
{
	isLearning = true;
	model = 0;
	yuvImage = 0;
	ImaskCodeBook = 0;
	ImaskCodeBookCC = 0;
}

vBackCodeBook::~vBackCodeBook()
{
	release();
}

void vBackCodeBook::init(CvSize size)
{
	model = cvCreateBGCodeBookModel();
	//Set color thresholds to default values
	model->modMin[0] = model->modMin[1] = model->modMin[2] = 3;			
	model->modMax[0] = model->modMax[1] = model->modMax[2] = 10;			
	model->cbBounds[0] = model->cbBounds[1] = model->cbBounds[2] = 10;
	// CODEBOOK METHOD ALLOCATION
	yuvImage = cvCreateImage( size, IPL_DEPTH_8U, 3 );
	ImaskCodeBook = cvCreateImage( size, IPL_DEPTH_8U, 1 );
	ImaskCodeBookCC = cvCreateImage( size, IPL_DEPTH_8U, 1 );
	cvSet(ImaskCodeBook,cvScalar(255));
}

bool vBackCodeBook::learn(IplImage* image)
{
	if (isLearning)
	{
		cvCvtColor( image, yuvImage, CV_BGR2YCrCb );
		//This is where we build our background model
		cvBGCodeBookUpdate( model, yuvImage );
		return true;
	}
	else
		return false;
}

void vBackCodeBook::finish_learn()
{
	if (isLearning)
	{
		cvBGCodeBookClearStale( model, model->t/2 );
		isLearning = false;
	}
}

//!!!!never release the returned image
IplImage* vBackCodeBook::getForeground(IplImage* image)
{
	cvCvtColor( image, yuvImage, CV_BGR2YCrCb );
	// Find foreground by codebook method
	cvBGCodeBookDiff( model, yuvImage, ImaskCodeBook );
	// This part just to visualize bounding boxes and centers if desired
	cvCopy(ImaskCodeBook,ImaskCodeBookCC);	
	//cvSegmentFGMask( ImaskCodeBookCC );

	return ImaskCodeBookCC;
}


void vBackCodeBook::release()
{

}


void vHighPass(IplImage* src, IplImage* dst, int blurLevel/* = 10*/, int noiseLevel/* = 3*/)
{	
	if (blurLevel > 0 && noiseLevel > 0)
	{
		// create the unsharp mask using a linear average filter
		cvSmooth(src, dst, CV_BLUR, blurLevel*2+1);

		cvSub(src, dst, dst);

		// filter out the noise using a median filter
		cvSmooth(dst, dst, CV_MEDIAN, noiseLevel*2+1);
	}
}
 
void vGetPerspectiveMatrix(CvMat*& warp_matrix, cv::Point2f xsrcQuad[4], cv::Point2f xdstQuad[4])
{
	static CvPoint2D32f srcQuad[4]; 
	static CvPoint2D32f dstQuad[4]; 
	for (int i=0;i<4;i++)
	{
		srcQuad[i] = xsrcQuad[i];
		dstQuad[i] = xdstQuad[i];
	}

	if (warp_matrix == NULL)
		warp_matrix = cvCreateMat(3, 3, CV_32FC1);
	cvGetPerspectiveTransform(srcQuad, dstQuad, warp_matrix); 
}

void vPerspectiveTransform(const CvArr* src, CvArr* dst, cv::Point xsrcQuad[4], cv::Point xdstQuad[4])
{
	static CvPoint2D32f srcQuad[4]; 
	static CvPoint2D32f dstQuad[4]; 
	for (int i=0;i<4;i++)
	{
		srcQuad[i] = xsrcQuad[i];
		dstQuad[i] = xdstQuad[i];
	}

	static CvMat* warp_matrix = cvCreateMat(3, 3, CV_32FC1);
	cvGetPerspectiveTransform(srcQuad, dstQuad, warp_matrix);
	cvWarpPerspective(src, dst, warp_matrix);
}

CvFGDStatModelParams cvFGDStatModelParams()
{
	CvFGDStatModelParams p;
    p.Lc = CV_BGFG_FGD_LC;			/* Quantized levels per 'color' component. Power of two, typically 32, 64 or 128.				*/
    p.N1c = CV_BGFG_FGD_N1C;			/* Number of color vectors used to model normal background color variation at a given pixel.			*/
    p.N2c = CV_BGFG_FGD_N2C;			/* Number of color vectors retained at given pixel.  Must be > N1c, typically ~ 5/3 of N1c.			*/
				/* Used to allow the first N1c vectors to adapt over time to changing background.				*/

    p.Lcc = CV_BGFG_FGD_LCC;			/* Quantized levels per 'color co-occurrence' component.  Power of two, typically 16, 32 or 64.			*/
    p.N1cc = CV_BGFG_FGD_N1CC;		/* Number of color co-occurrence vectors used to model normal background color variation at a given pixel.	*/
    p.N2cc = CV_BGFG_FGD_N2CC;		/* Number of color co-occurrence vectors retained at given pixel.  Must be > N1cc, typically ~ 5/3 of N1cc.	*/
				/* Used to allow the first N1cc vectors to adapt over time to changing background.				*/

    p.is_obj_without_holes;/* If TRUE we ignore holes within foreground blobs. Defaults to TRUE.						*/
    p.perform_morphing;	/* Number of erode-dilate-erode foreground-blob cleanup iterations.						*/
				/* These erase one-pixel junk blobs and merge almost-touching blobs. Default value is 1.			*/

    p.alpha1 = CV_BGFG_FGD_ALPHA_1;		/* How quickly we forget old background pixel values seen.  Typically set to 0.1  				*/
    p.alpha2 = CV_BGFG_FGD_ALPHA_2;		/* "Controls speed of feature learning". Depends on T. Typical value circa 0.005. 				*/
    p.alpha3 = CV_BGFG_FGD_ALPHA_3;		/* Alternate to alpha2, used (e.g.) for quicker initial convergence. Typical value 0.1.				*/

    p.delta = CV_BGFG_FGD_DELTA;		/* Affects color and color co-occurrence quantization, typically set to 2.					*/
    p.T = CV_BGFG_FGD_T;			/* "A percentage value which determines when new features can be recognized as new background." (Typically 0.9).*/
    p.minArea = CV_BGFG_FGD_MINAREA;		/* Discard foreground blobs whose bounding box is tinyer than this threshold.					*/

	return  p;
}

void vBackFGDStat::init(IplImage* initial, void* param)
{
	CvFGDStatModelParams* p = (CvFGDStatModelParams*)param;
	bg_model = cvCreateFGDStatModel( initial, p );
}

void vBackGaussian::init(IplImage* initial, void* param)
{
	CvGaussBGStatModelParams* p = (CvGaussBGStatModelParams*)param;
	bg_model = cvCreateGaussianBGModel( initial, p );
}

void vBackGrayDiff::init(IplImage* initial, void* param/* = NULL*/){
	cv::Size size = cvGetSize(initial);

	grayFrame.release();
	grayFrame = cvCreateImage(size, 8, 1);
	grayBg.release();
	grayBg = cvCreateImage(size, 8, 1);
	grayDiff.release();
	grayDiff = cvCreateImage(size, 8, 1);

	vGrayScale(initial, grayBg);
}

void vBackGrayDiff::update(IplImage* image, int mode/* = 0*/){
	vGrayScale(image, grayFrame);
	if (mode == DETECT_BOTH)
		cvAbsDiff(grayFrame, grayBg, grayDiff);
	else if (mode == DETECT_DARK)
		cvSub(grayBg, grayFrame, grayDiff);
	else if (mode == DETECT_BRIGHT)
		cvSub(grayFrame, grayBg, grayDiff);
	vThresh(grayDiff, thresh);
}

void vPolyLine(IplImage* dst, vector<Point>& pts, CvScalar clr, int thick)
{
	int n = pts.size();
	if (n > 1)
	{
		int k =0;
		for (;k<n-1;k++)
		{
			cvLine(dst, pts[k], pts[k+1], clr, thick);
		}
		cvLine(dst, pts[k], pts[0], clr, thick);
	}
}

static void draw_edge( IplImage* img, CvSubdiv2DEdge edge, CvScalar color )
{
	CvSubdiv2DPoint* org_pt = cvSubdiv2DEdgeOrg(edge);
	CvSubdiv2DPoint* dst_pt = cvSubdiv2DEdgeDst(edge);

	if( org_pt && dst_pt )
	{
		CvPoint2D32f org = org_pt->pt;
		CvPoint2D32f dst = dst_pt->pt;

		CvPoint iorg = cvPoint( cvRound( org.x ), cvRound( org.y ));
		CvPoint idst = cvPoint( cvRound( dst.x ), cvRound( dst.y ));

		cvLine( img, iorg, idst, color, 1, CV_AA, 0 );
	}
}


static void draw_facet( CvSubdiv2D * subdiv, IplImage * dst, IplImage * src, CvSubdiv2DEdge edge, bool drawLine )
{
	CvSubdiv2DEdge e = edge;
	int i, count = 0;
	vector<CvPoint> buf;

	// count number of edges in facet 
	do
	{
		count++;
		e = cvSubdiv2DGetEdge( e, CV_NEXT_AROUND_LEFT );
	}
	while( e != edge && count < subdiv->quad_edges * 4 );

	// gather points
	e = edge;
	for( i = 0; i < count; i++ )
	{
		CvSubdiv2DPoint *pt = cvSubdiv2DEdgeOrg( e );

		if( !pt )
			break;
		assert( fabs( pt->pt.x ) < 10000 && fabs( pt->pt.y ) < 10000 );		
		buf.push_back(cvPoint( cvRound( pt->pt.x ), cvRound( pt->pt.y )));

		e = cvSubdiv2DGetEdge( e, CV_NEXT_AROUND_LEFT );		
	}

	if( i == count )
	{
		CvSubdiv2DPoint *pt = cvSubdiv2DEdgeDst( cvSubdiv2DRotateEdge( edge, 1 ));
		if (!pt) 
			pt = cvSubdiv2DEdgeOrg( cvSubdiv2DRotateEdge( edge, 0 ));
		CvPoint ip = cvPoint( cvRound( pt->pt.x ), cvRound( pt->pt.y ));
		CvScalar color = {{0,0,0,0}};

		//printf("count = %d, (%d,%d)\n", ip.x, ip.y );

		if( 0 <= ip.x && ip.x < src->width && 0 <= ip.y && ip.y < src->height )
		{
			uchar *ptr = (uchar*)(src->imageData + ip.y * src->widthStep + ip.x * 3);
			color = CV_RGB( ptr[2], ptr[1], ptr[0] );
		}

		cvFillConvexPoly( dst, &buf[0], count, color );

		if (drawLine)
		{
			for (i = 1;i<count;i++)
			{
				cvDrawLine(dst, buf[i], buf[i-1], CV_RGB(30,30,30),1);
			}
		}
	} 
}




void vDrawDelaunay( CvSubdiv2D* subdiv,IplImage* src,IplImage * dst , bool drawLine)
{
	int i, total = subdiv->edges->total;

	cvCalcSubdivVoronoi2D( subdiv );

	for( i = 0; i < total; i++ )
	{
		CvQuadEdge2D *edge = (CvQuadEdge2D *) cvGetSetElem( subdiv->edges, i );

		if( edge && CV_IS_SET_ELEM( edge ))
		{
			CvSubdiv2DEdge e = (CvSubdiv2DEdge) edge;

			//	draw_edge( src, (CvSubdiv2DEdge)edge + 1, CV_RGB(0,0,0) );//voroni edge
			//	draw_edge( src, (CvSubdiv2DEdge)edge, CV_RGB(0,0,0) );//delaunay edge

			//e itslef
			draw_facet( subdiv, dst, src, cvSubdiv2DRotateEdge( e, 0 ), drawLine);
			//reversed e
			draw_facet( subdiv, dst, src, cvSubdiv2DRotateEdge( e, 2 ), drawLine);
		}
	}
}

void vDrawVoroni( CvSubdiv2D * subdiv, IplImage * src, IplImage * dst, bool drawLine )
{
	int i, total = subdiv->edges->total;

	cvCalcSubdivVoronoi2D( subdiv );

	//icvSet( dst, 255 );
	for( i = 0; i < total; i++ )
	{
		CvQuadEdge2D *edge = (CvQuadEdge2D *) cvGetSetElem( subdiv->edges, i );

		if( edge && CV_IS_SET_ELEM( edge ))
		{
			CvSubdiv2DEdge e = (CvSubdiv2DEdge) edge;

			// left
			draw_facet( subdiv, dst, src, cvSubdiv2DRotateEdge( e, 1 ), drawLine);
			// right
			draw_facet( subdiv, dst, src, cvSubdiv2DRotateEdge( e, 3 ), drawLine);
		}
	}
}



Subdiv::Subdiv(int w, int h)
{
	storage = cvCreateMemStorage();
	rect = cvRect(0, 0, w, h);

	subdiv = cvCreateSubdivDelaunay2D(rect, storage);
}

void Subdiv::insert(float x, float y)
{
	Point2f pt(x,y);
	cvSubdivDelaunay2DInsert(subdiv, pt);
}

void Subdiv::clear()
{
	cvClearMemStorage(storage);

	subdiv = cvCreateSubdivDelaunay2D(rect, storage);
}

void Subdiv::build()
{
	cvCalcSubdivVoronoi2D( subdiv );
}
 

void Subdiv::drawDelaunay( IplImage* src,IplImage * dst , bool drawLine)
{
	int i, total = subdiv->edges->total;

	build();

	for( i = 0; i < total; i++ )
	{
		CvQuadEdge2D *edge = (CvQuadEdge2D *) cvGetSetElem( subdiv->edges, i );

		if( edge && CV_IS_SET_ELEM( edge ))
		{
			CvSubdiv2DEdge e = (CvSubdiv2DEdge) edge;

			//	draw_edge( src, (CvSubdiv2DEdge)edge + 1, CV_RGB(0,0,0) );//voroni edge
			//	draw_edge( src, (CvSubdiv2DEdge)edge, CV_RGB(0,0,0) );//delaunay edge

			//e itslef
			draw_facet( subdiv, dst, src, cvSubdiv2DRotateEdge( e, 0 ), drawLine);
			//reversed e
			draw_facet( subdiv, dst, src, cvSubdiv2DRotateEdge( e, 2 ), drawLine);
		}
	}
}


void Subdiv::drawVoroni( IplImage * src, IplImage * dst, bool drawLine )
{
	int i, total = subdiv->edges->total;

	build();

	//icvSet( dst, 255 );
	for( i = 0; i < total; i++ )
	{
		CvQuadEdge2D *edge = (CvQuadEdge2D *) cvGetSetElem( subdiv->edges, i );

		if( edge && CV_IS_SET_ELEM( edge ))
		{
			CvSubdiv2DEdge e = (CvSubdiv2DEdge) edge;

			// left
			draw_facet( subdiv, dst, src, cvSubdiv2DRotateEdge( e, 1 ), drawLine);
			// right
			draw_facet( subdiv, dst, src, cvSubdiv2DRotateEdge( e, 3 ), drawLine);
		}
	}
}


void on_default(int )
{

}


int BrightnessAdjust(const IplImage* srcImg,
					 IplImage* dstImg,
					 float brightness)
{
	assert(srcImg != NULL);
	assert(dstImg != NULL);

	int h = srcImg->height;
	int w = srcImg->width;
	int n = srcImg->nChannels;

	int x,y,i;
	float val;
	for (i = 0; i < n; i++)//彩色图像需要处理3个通道，灰度图像这里可以删掉
	{
		for (y = 0; y < h; y++)
		{
			for (x = 0; x < w; x++)
			{

				val = ((uchar*)(srcImg->imageData + srcImg->widthStep*y))[x*n+i];
				val += brightness;
				//对灰度值的可能溢出进行处理
				if(val>255)	val=255;
				if(val<0) val=0;
				((uchar*)(dstImg->imageData + dstImg->widthStep*y))[x*n+i] = (uchar)val;
			}
		}
	}

	return 0;
}

int ContrastAdjust(const IplImage* srcImg,
				   IplImage* dstImg,
				   float nPercent)
{
	assert(srcImg != NULL);
	assert(dstImg != NULL);

	int h = srcImg->height;
	int w = srcImg->width;
	int n = srcImg->nChannels;

	int x,y,i;
	float val;
	for (i = 0; i < n; i++)//彩色图像需要处理3个通道，灰度图像这里可以删掉
	{
		for (y = 0; y < h; y++)
		{
			for (x = 0; x < w; x++)
			{

				val = ((uchar*)(srcImg->imageData + srcImg->widthStep*y))[x*n+i];
				val = 128 + (val - 128) * nPercent;
				//对灰度值的可能溢出进行处理
				if(val>255) val=255;
				if(val<0) val=0;
				((uchar*)(dstImg->imageData + dstImg->widthStep*y))[x*n+i] = (uchar)val;
			}
		}
	}
	return 0;
}

// Create a HSV image from the RGB image using the full 8-bits, since OpenCV only allows Hues up to 180 instead of 255.
// ref: "http://cs.haifa.ac.il/hagit/courses/ist/Lectures/Demos/ColorApplet2/t_convert.html"
// Remember to free the generated HSV image.
void convertRGBtoHSV(const IplImage *imageRGB, IplImage *imageHSV)
{
	float fR, fG, fB;
	float fH, fS, fV;
	const float FLOAT_TO_BYTE = 255.0f;
	const float BYTE_TO_FLOAT = 1.0f / FLOAT_TO_BYTE;

	// Create a blank HSV image
	//IplImage *imageHSV = cvCreateImage(cvGetSize(imageRGB), 8, 3);
	//if (!imageHSV || imageRGB->depth != 8 || imageRGB->nChannels != 3) {
	//	printf("ERROR in convertImageRGBtoHSV()! Bad input image.\n");
	//	exit(1);
	//}

	int h = imageRGB->height;		// Pixel height.
	int w = imageRGB->width;		// Pixel width.
	int rowSizeRGB = imageRGB->widthStep;	// Size of row in bytes, including extra padding.
	char *imRGB = imageRGB->imageData;	// Pointer to the start of the image pixels.
	int rowSizeHSV = imageHSV->widthStep;	// Size of row in bytes, including extra padding.
	char *imHSV = imageHSV->imageData;	// Pointer to the start of the image pixels.
	for (int y=0; y<h; y++) {
		for (int x=0; x<w; x++) {
			// Get the RGB pixel components. NOTE that OpenCV stores RGB pixels in B,G,R order.
			uchar *pRGB = (uchar*)(imRGB + y*rowSizeRGB + x*3);
			int bB = *(uchar*)(pRGB+0);	// Blue component
			int bG = *(uchar*)(pRGB+1);	// Green component
			int bR = *(uchar*)(pRGB+2);	// Red component

			// Convert from 8-bit integers to floats.
			fR = bR * BYTE_TO_FLOAT;
			fG = bG * BYTE_TO_FLOAT;
			fB = bB * BYTE_TO_FLOAT;

			// Convert from RGB to HSV, using float ranges 0.0 to 1.0.
			float fDelta;
			float fMin, fMax;
			int iMax;
			// Get the min and max, but use integer comparisons for slight speedup.
			if (bB < bG) {
				if (bB < bR) {
					fMin = fB;
					if (bR > bG) {
						iMax = bR;
						fMax = fR;
					}
					else {
						iMax = bG;
						fMax = fG;
					}
				}
				else {
					fMin = fR;
					fMax = fG;
					iMax = bG;
				}
			}
			else {
				if (bG < bR) {
					fMin = fG;
					if (bB > bR) {
						fMax = fB;
						iMax = bB;
					}
					else {
						fMax = fR;
						iMax = bR;
					}
				}
				else {
					fMin = fR;
					fMax = fB;
					iMax = bB;
				}
			}
			fDelta = fMax - fMin;
			fV = fMax;				// Value (Brightness).
			if (iMax != 0) {			// Make sure its not pure black.
				fS = fDelta / fMax;		// Saturation.
				float ANGLE_TO_UNIT = 1.0f / (6.0f * fDelta);	// Make the Hues between 0.0 to 1.0 instead of 6.0
				if (iMax == bR) {		// between yellow and magenta.
					fH = (fG - fB) * ANGLE_TO_UNIT;
				}
				else if (iMax == bG) {		// between cyan and yellow.
					fH = (2.0f/6.0f) + ( fB - fR ) * ANGLE_TO_UNIT;
				}
				else {				// between magenta and cyan.
					fH = (4.0f/6.0f) + ( fR - fG ) * ANGLE_TO_UNIT;
				}
				// Wrap outlier Hues around the circle.
				if (fH < 0.0f)
					fH += 1.0f;
				if (fH >= 1.0f)
					fH -= 1.0f;
			}
			else {
				// color is pure Black.
				fS = 0;
				fH = 0;	// undefined hue
			}

			// Convert from floats to 8-bit integers.
			int bH = (int)(0.5f + fH * 255.0f);
			int bS = (int)(0.5f + fS * 255.0f);
			int bV = (int)(0.5f + fV * 255.0f);

			// Clip the values to make sure it fits within the 8bits.
			if (bH > 255)
				bH = 255;
			if (bH < 0)
				bH = 0;
			if (bS > 255)
				bS = 255;
			if (bS < 0)
				bS = 0;
			if (bV > 255)
				bV = 255;
			if (bV < 0)
				bV = 0;

			// Set the HSV pixel components.
			uchar *pHSV = (uchar*)(imHSV + y*rowSizeHSV + x*3);
			*(pHSV+0) = bH;		// H component
			*(pHSV+1) = bS;		// S component
			*(pHSV+2) = bV;		// V component
		}
	}
}


// Create an RGB image from the HSV image using the full 8-bits, since OpenCV only allows Hues up to 180 instead of 255.
// ref: "http://cs.haifa.ac.il/hagit/courses/ist/Lectures/Demos/ColorApplet2/t_convert.html"
// Remember to free the generated RGB image.
void convertHSVtoRGB(const IplImage *imageHSV, IplImage *imageRGB)
{
	float fH, fS, fV;
	float fR, fG, fB;
	const float FLOAT_TO_BYTE = 255.0f;
	const float BYTE_TO_FLOAT = 1.0f / FLOAT_TO_BYTE;

	// Create a blank RGB image
	//IplImage *imageRGB = cvCreateImage(cvGetSize(imageHSV), 8, 3);
	//if (!imageRGB || imageHSV->depth != 8 || imageHSV->nChannels != 3) {
	//	printf("ERROR in convertImageHSVtoRGB()! Bad input image.\n");
	//	exit(1);
	//}

	int h = imageHSV->height;			// Pixel height.
	int w = imageHSV->width;			// Pixel width.
	int rowSizeHSV = imageHSV->widthStep;		// Size of row in bytes, including extra padding.
	char *imHSV = imageHSV->imageData;		// Pointer to the start of the image pixels.
	int rowSizeRGB = imageRGB->widthStep;		// Size of row in bytes, including extra padding.
	char *imRGB = imageRGB->imageData;		// Pointer to the start of the image pixels.
	for (int y=0; y<h; y++) {
		for (int x=0; x<w; x++) {
			// Get the HSV pixel components
			uchar *pHSV = (uchar*)(imHSV + y*rowSizeHSV + x*3);
			int bH = *(uchar*)(pHSV+0);	// H component
			int bS = *(uchar*)(pHSV+1);	// S component
			int bV = *(uchar*)(pHSV+2);	// V component

			// Convert from 8-bit integers to floats
			fH = (float)bH * BYTE_TO_FLOAT;
			fS = (float)bS * BYTE_TO_FLOAT;
			fV = (float)bV * BYTE_TO_FLOAT;

			// Convert from HSV to RGB, using float ranges 0.0 to 1.0
			int iI;
			float fI, fF, p, q, t;

			if( bS == 0 ) {
				// achromatic (grey)
				fR = fG = fB = fV;
			}
			else {
				// If Hue == 1.0, then wrap it around the circle to 0.0
				if (fH >= 1.0f)
					fH = 0.0f;

				fH *= 6.0;			// sector 0 to 5
				fI = floor( fH );		// integer part of h (0,1,2,3,4,5 or 6)
				iI = (int) fH;			//		"		"		"		"
				fF = fH - fI;			// factorial part of h (0 to 1)

				p = fV * ( 1.0f - fS );
				q = fV * ( 1.0f - fS * fF );
				t = fV * ( 1.0f - fS * ( 1.0f - fF ) );

				switch( iI ) {
					case 0:
						fR = fV;
						fG = t;
						fB = p;
						break;
					case 1:
						fR = q;
						fG = fV;
						fB = p;
						break;
					case 2:
						fR = p;
						fG = fV;
						fB = t;
						break;
					case 3:
						fR = p;
						fG = q;
						fB = fV;
						break;
					case 4:
						fR = t;
						fG = p;
						fB = fV;
						break;
					default:		// case 5 (or 6):
						fR = fV;
						fG = p;
						fB = q;
						break;
				}
			}

			// Convert from floats to 8-bit integers
			int bR = (int)(fR * FLOAT_TO_BYTE);
			int bG = (int)(fG * FLOAT_TO_BYTE);
			int bB = (int)(fB * FLOAT_TO_BYTE);

			// Clip the values to make sure it fits within the 8bits.
			if (bR > 255)
				bR = 255;
			if (bR < 0)
				bR = 0;
			if (bG > 255)
				bG = 255;
			if (bG < 0)
				bG = 0;
			if (bB > 255)
				bB = 255;
			if (bB < 0)
				bB = 0;

			// Set the RGB pixel components. NOTE that OpenCV stores RGB pixels in B,G,R order.
			uchar *pRGB = (uchar*)(imRGB + y*rowSizeRGB + x*3);
			*(pRGB+0) = bB;		// B component
			*(pRGB+1) = bG;		// G component
			*(pRGB+2) = bR;		// R component
		}
	}
}

