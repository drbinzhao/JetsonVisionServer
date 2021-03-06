#include <opencv2/core.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <time.h>
#include <stdio.h>
#include <VisionTracker.h>
#include <opencv2/videoio/videoio.hpp>
#include <iostream>

cv::Mat DriverCamClass::m_Img2A;
cv::Mat DriverCamClass::m_Img2B;
cv::Mat *DriverCamClass::m_Img2ReadPtr;
cv::Mat *DriverCamClass::m_Img2WritePtr;
cv::VideoCapture *DriverCamClass::m_VideoCap2;
bool DriverCamClass::m_NewSecondaryImg;

cv::Mat DriverCamClass::m_Img3A;
cv::Mat DriverCamClass::m_Img3B;
cv::Mat *DriverCamClass::m_Img3ReadPtr;
cv::Mat *DriverCamClass::m_Img3WritePtr;
cv::VideoCapture *DriverCamClass::m_VideoCap3;
bool DriverCamClass::m_NewThirdImg;

pthread_mutex_t DriverCamClass::FRAMELOCKER = PTHREAD_MUTEX_INITIALIZER;

//cv::RNG rng(12345);
//cv::Scalar color = cv::Scalar( rng.uniform(0, 1), rng.uniform(0, 1), rng.uniform(254, 255) );

int hmin = 52; //46
int hmax = 96;
int smin=60;
int smax=255;
int vmin =50;
int vmax = 255;
int brightness = 10;
int exposure = 10;
int gain = 10;

bool Debug = false;



inline void VisionTrackerClass::draw_rotated_rect(cv::Mat& image, cv::RotatedRect rRect, cv::Scalar color)
{
   cv::Point2f vertices2f[4];
   cv::Point vertices[4];
   rRect.points(vertices2f);
   for(int i = 0; i < 4; ++i)
   {
        vertices[i] = vertices2f[i];
   }
    cv::line(image,vertices[0],vertices[1],color,2);
    cv::line(image,vertices[1],vertices[2],color,2);
    cv::line(image,vertices[2],vertices[3],color,2);
    cv::line(image,vertices[3],vertices[0],color,2);
}

inline void VisionTrackerClass::draw_cross_hair(cv::Mat& image,float cx, float cy,int len,int thick)
{
    const float GAP = 4;
    cv::line(image,cv::Point(cx,cy-len),cv::Point(cx,cy-GAP),cv::Scalar(255.0,255.0,255.0),thick);
    cv::line(image,cv::Point(cx,cy+GAP),cv::Point(cx,cy+len),cv::Scalar(255.0,255.0,255.0),thick);
    cv::line(image,cv::Point(cx-len,cy),cv::Point(cx-GAP,cy),cv::Scalar(255.0,255.0,255.0),thick);
    cv::line(image,cv::Point(cx+GAP,cy),cv::Point(cx+len,cy),cv::Scalar(255.0,255.0,255.0),thick);
}

inline void VisionTrackerClass::draw_aim_line(cv::Mat& image,float cx)
{
    cv::line(image,cv::Point(cx,0),cv::Point(cx,image.rows),cv::Scalar(255.0,255.0,255.0),1);
}
inline void VisionTrackerClass::draw_moving_target_aim_line(cv::Mat& image,float cx,float X_Offset,float Y_Offset)
{
    //XOffset is in degrees, Y Offset is in RPM
    const float FOV = 75.0f;
    const int linetrim = 50;

    float noffset = X_Offset/(0.5*FOV);
    float ncx = Pixel_X_To_Normalized_X(cx);
    float cxfinal = Normalized_X_To_Pixel_X(ncx+noffset);

    cv::line(image,cv::Point(cxfinal,linetrim),cv::Point(cxfinal,image.rows - linetrim),cv::Scalar(200.0,200.0,255.0),1);


    char buffer[128];
    sprintf(buffer, "dRPM: %.0f",m_MovingTargetY);
    cv::putText(image, buffer, cv::Point(0, image.rows - 20), cv::FONT_HERSHEY_SIMPLEX, 0.5,cv::Scalar(200.0,200.0,255.0),2);

    sprintf(buffer, "dAngle: %.0f",m_MovingTargetX);
    cv::putText(image, buffer, cv::Point(0, image.rows - 5), cv::FONT_HERSHEY_SIMPLEX, 0.5,cv::Scalar(200.0,200.0,255.0),2);

}
inline void VisionTrackerClass::draw_calibration_range(cv::Mat& image,float cxnear, float cxfar, float cynear,float cyfar)
{
    const float LEN = 4;
    const float GAP = 0;
    cv::line(image,cv::Point(cxnear-GAP-LEN,cynear),cv::Point(cxnear-GAP,cynear),cv::Scalar(128.0,128.0,255.0),1);
    cv::line(image,cv::Point(cxfar-GAP-LEN,cyfar),cv::Point(cxfar-GAP,cyfar),cv::Scalar(128.0,128.0,255.0),1);
    cv::line(image,cv::Point(cxnear-GAP,cynear),cv::Point(cxfar-GAP,cyfar),cv::Scalar(128.0,128.0,255.0),1);
}





VisionTrackerClass::VisionTrackerClass() :
    m_VideoCap(NULL),
    m_LastUpdateTime(0.0),
    m_ResolutionW(320.0f),
    m_ResolutionH(240.0f),
    m_TargetX(INVALID_TARGET),
    m_TargetY(INVALID_TARGET),
    m_TargetArea(0.0f),
    m_TargetContourIndex(0),
    m_CameraMode(CAM_FRONT),
    m_NewImageProcessed(false),
    m_FrameCounter(0),
    m_TimeElapsedSinceLastFrameGet(0.0f),
    m_FPSTimeElapsed(0.0f),
    m_FPS(0.0f),
    m_EqualizeImage(false)
{
    // Test coordinate conversion functions...
    /*
    float nx = Pixel_X_To_Normalized_X(320.0f);
    float ny = Pixel_Y_To_Normalized_Y(240.0f);
    float px = Normalized_X_To_Pixel_X(nx);
    float py = Normalized_Y_To_Pixel_Y(ny);

    nx = Pixel_X_To_Normalized_X(0.0f);
    ny = Pixel_Y_To_Normalized_Y(0.0f);
    px = Normalized_X_To_Pixel_X(nx);
    py = Normalized_Y_To_Pixel_Y(ny);
    */
}

VisionTrackerClass::~VisionTrackerClass(){
}


void VisionTrackerClass::Init()
{
    m_ResolutionW = 320;
    m_ResolutionH = 240;



    m_VideoCap = new cv::VideoCapture(0); //this line is where the HIGHGUI ERROR V4L/V4L2 VIDIOC_S_CROP error occurs first when this program is ran

    sleep(1);
    if (!m_VideoCap->isOpened())
    {
        return;
    }

    // Get the default res and try to set the resolution to what we want.
    int w = m_VideoCap->get(cv::CAP_PROP_FRAME_WIDTH);
    int h = m_VideoCap->get(cv::CAP_PROP_FRAME_HEIGHT);
    printf("Camera initial size: %d x %d\r\n",w,h);

    m_VideoCap->set(cv::CAP_PROP_FRAME_WIDTH,m_ResolutionW);
    m_VideoCap->set(cv::CAP_PROP_FRAME_HEIGHT,m_ResolutionH);
    m_VideoCap->set(cv::CAP_PROP_FPS, 150);
    //m_VideoCap->set(cv::CAP_PROP_BUFFERSIZE,1);

    // Store off the actual resolution the camera is running at
    m_ResolutionW = m_VideoCap->get(cv::CAP_PROP_FRAME_WIDTH);
    m_ResolutionH = m_VideoCap->get(cv::CAP_PROP_FRAME_HEIGHT);

    DriverCamClass::Init();
    printf("Camera opened: %d x %d\r\n",(int)m_ResolutionW,(int)m_ResolutionH);

    if(Debug == true)
    {
        cv::namedWindow("Video",0);
         cv::createTrackbar( "hmin:", "Video", &hmin, 255, NULL );
         cv::createTrackbar( "hmax:", "Video", &hmax, 255, NULL );
         cv::createTrackbar( "smin:", "Video", &smin, 255, NULL );
         cv::createTrackbar( "smax:", "Video", &smax, 255, NULL );
         cv::createTrackbar( "vmin:", "Video", &vmin, 255, NULL );
         cv::createTrackbar( "vmax:", "Video", &vmax, 255, NULL );
         cv::namedWindow("Thres", 0);
         cv::namedWindow("DIL", 0);
         cv::namedWindow("Video2",0);
     }
     cv::waitKey(1);

    pthread_t SecondThread;
    pthread_create(&SecondThread,NULL,&DriverCamClass::Process,NULL);
}

void VisionTrackerClass::Shutdown()
{
    delete m_VideoCap;
    m_VideoCap = NULL;
    DriverCamClass::Shutdown();
    cv::destroyWindow("Video");
    cv::destroyWindow("Video2");
    cv::destroyWindow("Thresh");
    cv::destroyWindow("DIL");
}

float VisionTrackerClass::Pixel_X_To_Normalized_X(float pixel_x)
{
    return (pixel_x - 0.5f*m_ResolutionW) / (0.5f*m_ResolutionW);
}

float VisionTrackerClass::Pixel_Y_To_Normalized_Y(float pixel_y)
{
    return -(pixel_y - 0.5f*m_ResolutionH) / (0.5f*m_ResolutionH);
}
float VisionTrackerClass::Pixel_Area_To_Normalized_Area(float pixel_area)
{
    float pixelRes = m_ResolutionH * m_ResolutionW;

    return pixel_area/pixelRes;
}

float VisionTrackerClass::Normalized_X_To_Pixel_X(float nx)
{
    return nx * (0.5f*m_ResolutionW) + 0.5f*m_ResolutionW;
}

float VisionTrackerClass::Normalized_Y_To_Pixel_Y(float ny)
{
    // ny goes from -1..+1
    // output goes from res_h to 0 (opposite sense)
    return (-ny) * 0.5f*m_ResolutionH + 0.5f*m_ResolutionH;
}
float VisionTrackerClass::Normalized_Area_To_Pixel_Area(float na)
{
    // na goes from 0..+1

    return (na) * (m_ResolutionH *m_ResolutionH);
}

void VisionTrackerClass::Process()
{

    bool got_frame = m_VideoCap->read(m_Img);
    bool got_frame2 = false;
    if (got_frame && (m_Img.empty() == false))
    {
        m_TargetX = INVALID_TARGET;
        m_TargetY = INVALID_TARGET;
        m_NewImageProcessed = true;

        // When we're using the 'EqualizeImage' feature, we aren't tracking
        if (m_EqualizeImage)
        {
            const float CONTRAST_MULT = 16.0f;
            m_Img.convertTo(m_TmpImg,-1,CONTRAST_MULT,0.0);
            m_TmpImg = m_Img;
            //cv::cvtColor(m_Img,m_TmpImg,cv::COLOR_BGR2GRAY);
            //cv::equalizeHist(m_TmpImg, m_Img);
        }
        else
        {
            cv::cvtColor(m_Img,m_ImgHSV,cv::COLOR_BGR2HSV);

           // cv::inRange(m_ImgHSV,cv::Scalar(40,167,193),cv::Scalar(119,255,255),m_Imgthresh);
            cv::inRange(m_ImgHSV,cv::Scalar(hmin,smin,vmin),cv::Scalar(hmax,smax,vmax),m_Imgthresh);
            cv::findContours( m_Imgthresh, m_Contours, m_Hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0) );

            if (m_Contours.size() > 0)
            {
                int best_contour = -1;
                int best_area = -100000.0f;

                for( unsigned int i = 0; i < m_Contours.size(); i++)
                {
                    float area = Pixel_Area_To_Normalized_Area(cv::contourArea(m_Contours[i]));

                    // compute convex hull area
                    std::vector<cv::Point> hull;
                    cv::convexHull(m_Contours[i],hull);
                    float convex_area = Pixel_Area_To_Normalized_Area(cv::contourArea(hull));

                    //convexity will be 1.0 for a convex shape and very low for a concave shape
                    float convexity = area / convex_area;

                    cv::Rect bounding_rect = cv::boundingRect(m_Contours[i]);
                    float aspect_ratio = (float)bounding_rect.width/(float)bounding_rect.height;

                    if((area > 10.0f/(320.0f*240.0f))&&(aspect_ratio > 1.0f) /*&& (convexity < 0.5f)*/)
                    {
                        // If we had a good target last frame, slightly prefer to aim at it...
                        if ((m_TargetX != INVALID_TARGET) && (m_TargetY != INVALID_TARGET))
                        {
                            cv::RotatedRect candidate_rect = cv::minAreaRect(m_Contours[i]);
                            float center_x = Pixel_X_To_Normalized_X(candidate_rect.center.x);
                            float center_y = Pixel_Y_To_Normalized_Y(candidate_rect.center.y + (-0.5f * candidate_rect.size.height));
                            float delta_x = center_x - m_TargetX;
                            float delta_y = center_y - m_TargetY;
                            float dist = sqrt((delta_x*delta_x) + (delta_y * delta_y));

                            // essentially penalize each target based on how far from the last
                            // target it was.  20*dist means a target 1/2 screen away loses 20pixels of area
                            // if it is more than 20 pixels of area bigger than the last target, it will still
                            // get chosen and then we'll stick to it.
                            area -= 20.0f * dist;
                        }

                        if (area > best_area)
                        {
                            best_area = area;
                            best_contour = i;
                        }

                        cv::Scalar color = cv::Scalar( 0, 0, 255);
                        cv::drawContours( m_Img, m_Contours, i, color, 1, 4, m_Hierarchy, 0, cv::Point(0, 0));
                    }
                }
                // Set Targetx, Targety based on the best contour we found
                if (best_contour != -1)
                {
                    cv::RotatedRect rect = cv::minAreaRect(m_Contours[best_contour]);

                    // use the dimension of the rectangle to try to target
                    // the top edge of the rect.  This handles the cases where
                    // the two vision targets sometimes merge and sometimes dont
                    float h = rect.size.height;
                    if (h > rect.size.width)
                    {
                       h = rect.size.width;  // in this case, width is actually "height"
                    }
                    float target_pixel_x = rect.center.x;
                    float target_pixel_y = rect.center.y - 0.5f*h;
                    float target_pixel_area = rect.size.area();

                    // compute the normalized target position (resolution independent)
                    m_TargetX = Pixel_X_To_Normalized_X(target_pixel_x);
                    m_TargetY = Pixel_Y_To_Normalized_Y(target_pixel_y);
                    m_TargetArea = Pixel_Area_To_Normalized_Area(target_pixel_area);
                    m_TargetContourIndex = best_contour;

                    // Draw the rect on our image
                    draw_rotated_rect(m_Img,rect,cv::Scalar(255, 255, 0));
                }
                m_Contours.clear();
            }

            CrossHairClass c = m_CrossHair;
            float cx = c.Get_Average_X();
            float cxn = c.XNear;
            float cxf = c.XFar;
            float cyn = c.Get_Y_Near();
            float cyf = c.Get_Y_Far();
            if (m_TargetX != INVALID_TARGET)
            {
                cx = c.Get_X(m_TargetY);
                draw_cross_hair(m_Img,Normalized_X_To_Pixel_X(m_TargetX),Normalized_Y_To_Pixel_Y(m_TargetY),12,1);
            }
            draw_aim_line(m_Img,Normalized_X_To_Pixel_X(cx));
            draw_moving_target_aim_line(m_Img,Normalized_X_To_Pixel_X(cx),m_MovingTargetX,m_MovingTargetY);
            draw_calibration_range(m_Img,
              Normalized_X_To_Pixel_X(cxn),
              Normalized_X_To_Pixel_X(cxf),
              Normalized_Y_To_Pixel_Y(cyn),
              Normalized_Y_To_Pixel_Y(cyf));
        }

        // measure framerate of the overall system (from last time we updated to this time)
        double t = cv::getTickCount();
        double delta_t = (t-m_LastUpdateTime)/cv::getTickFrequency();

        m_FrameCounter++;
        m_TimeElapsedSinceLastFrameGet += (float)delta_t;
        m_FPSTimeElapsed += (float)delta_t;
        m_LastUpdateTime = t;

        // averaged fps for the past 'n' frames
        if ((m_FrameCounter % 32) == 0)
        {
            m_FPS = 32.0f / m_FPSTimeElapsed;
            m_FPSTimeElapsed = 0.0f;
        }

        // every 256 frames,print fps
        if ((m_FrameCounter & 0xFF) == 0)
        {
            //printf("fps: %f\r\n",m_FPS);
        }

        // display the image on the desktop
        if(Debug)
        {
            if(got_frame)
            {
                cv::imshow("Video",m_Img);
            }
            if(got_frame2)
            {
         //       cv::imshow("Video2",m_Img2);
            }
            cv::imshow("Thres",m_Imgthresh);
            cv::imshow("DIL",m_ImgDilated);

        }
        cv::waitKey(1);
    }
}

void VisionTrackerClass::Print_FPS_On_Image(cv::Mat & img)
{
    char buffer[128];
    sprintf(buffer, "fps: %.0f",m_FPS);
    cv::putText(img, buffer, cv::Point(0, 15), cv::FONT_HERSHEY_SIMPLEX, 0.5,cv::Scalar(0,255,0),2);
}

void VisionTrackerClass::Get_Image(unsigned char ** data, unsigned int * byte_count,int quality)
{
    std::vector<int> params;
    params.push_back(cv::IMWRITE_JPEG_QUALITY);
    params.push_back(quality);

    Print_FPS_On_Image(m_Img);
    cv::Mat combine(240,640, CV_8UC3);//std::max(m_Img.size().height, m_Img2.size().height), m_Img.size().width + m_Img2.size().width, CV_8UC3);

    switch (m_CameraMode)
    {
    case CAM_FRONT :
        Build_Img_Front(combine);
        break;
    case CAM_BACK :
        Build_Img_Back(combine);
        break;
    }

    cv::imencode(".jpg", combine, m_JpegOutputBuffer, params);

    *data = &(m_JpegOutputBuffer[0]);
    *byte_count = m_JpegOutputBuffer.size();

    m_NewImageProcessed = false;
    m_TimeElapsedSinceLastFrameGet = 0.0f;

}
void VisionTrackerClass::Build_Img_Front(cv::Mat &Output)
{
    DriverCamClass::GetSecondaryImg(m_Img2);
    DriverCamClass::GetThirdImg(m_Img3);

    if(m_Img2.empty() == false)
    {
        cv::Mat left_roi(Output, cv::Rect(0, 0, m_Img2.size().width, m_Img2.size().height));
        m_Img2.copyTo(left_roi);
    }

    if(m_Img.empty() == false)
    {
        cv::Mat right_roi(Output, cv::Rect(m_Img2.size().width, 0, m_Img.size().width, m_Img.size().height));
        m_Img.copyTo(right_roi);
    }

    if(m_Img3.empty() == false)
    {
        cv::Size sz(0.2f*m_Img2.size().width,0.2f*m_Img2.size().height);
        cv::Mat pip_roi(Output, cv::Rect(10, 10, sz.width,sz.height));
        cv::resize(m_Img3,pip_roi,sz);
    }
}

void VisionTrackerClass::Build_Img_Back(cv::Mat &Output)
{
    DriverCamClass::GetSecondaryImg(m_Img2);
    DriverCamClass::GetThirdImg(m_Img3);

    if(m_Img3.empty() == false)
    {
        cv::Mat left_roi(Output, cv::Rect(0, 0, m_Img3.size().width, m_Img3.size().height));
        m_Img3.copyTo(left_roi);
    }

    if(m_Img.empty() == false)
    {
        cv::Mat right_roi(Output, cv::Rect(m_Img3.size().width, 0, m_Img.size().width, m_Img.size().height));
        m_Img.copyTo(right_roi);
    }
    // put the front image in a PIP on the back image
    if(m_Img2.empty() == false)
    {
        cv::Size sz(0.2f*m_Img2.size().width,0.2f*m_Img2.size().height);
        cv::Mat pip_roi(Output, cv::Rect(10, 10, sz.width, sz.height));
        cv::resize(m_Img2,pip_roi,sz);
    }

}
