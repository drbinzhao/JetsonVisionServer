#include <opencv2/core.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <time.h>
#include <stdio.h>
#include <VisionTracker.h>
#include <opencv2/videoio/videoio.hpp>

//cv::RNG rng(12345);
//cv::Scalar color = cv::Scalar( rng.uniform(0, 1), rng.uniform(0, 1), rng.uniform(254, 255) );

int hmin=39;
int hmax = 96;
int smin=184;
int smax=255;
int vmin =75;
int vmax = 255;
int brightness = 10;
int exposure = 10;
int gain = 10;

bool Debug = false;

inline void draw_rotated_rect(cv::Mat& image, cv::RotatedRect rRect, cv::Scalar color = cv::Scalar(255.0, 255.0, 255.0) )
{
   cv::Point2f vertices2f[4];
   cv::Point vertices[4];
   rRect.points(vertices2f);
   for(int i = 0; i < 4; ++i)
   {
        vertices[i] = vertices2f[i];
   }
   for(int i = 1; i < 4; ++i)
   {
        cv::line(image,vertices[i-1],vertices[i],color,2);
   }
}

inline void draw_cross_hair(cv::Mat& image,float cx, float cy)
{
    const float LEN = 32;
    const float GAP = 4;
    cv::line(image,cv::Point(cx,cy-LEN),cv::Point(cx,cy-GAP),cv::Scalar(255.0,255.0,255.0),2);
    cv::line(image,cv::Point(cx,cy+GAP),cv::Point(cx,cy+LEN),cv::Scalar(255.0,255.0,255.0),2);
    cv::line(image,cv::Point(cx-LEN,cy),cv::Point(cx-GAP,cy),cv::Scalar(255.0,255.0,255.0),2);
    cv::line(image,cv::Point(cx+GAP,cy),cv::Point(cx+LEN,cy),cv::Scalar(255.0,255.0,255.0),2);
}





VisionTrackerClass::VisionTrackerClass() :
    m_VideoCap(NULL),
    m_LastUpdateTime(0.0),
    m_ResolutionW(320.0f),
    m_ResolutionH(240.0f),
    m_TargetX(INVALID_TARGET),
    m_TargetY(INVALID_TARGET),
    m_CrossHairX(0.0f),
    m_CrossHairY(0.0f),
    m_NewImageProcessed(false),
    m_FrameCounter(0),
    m_TimeElapsedSinceLastFrameGet(0.0f),
    m_FPSTimeElapsed(0.0f),
    m_FPS(0.0f),
    m_FlipImage(false)
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

VisionTrackerClass::~VisionTrackerClass()
{
}


void VisionTrackerClass::Init()
{
    m_ResolutionW = 320;
    m_ResolutionH = 240;

    m_VideoCap = new cv::VideoCapture(0); //this line is where the HIGHGUI ERROR V4L/V4L2 VIDIOC_S_CROP error occurs first when this program is ran
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
    m_VideoCap->set(cv::CAP_PROP_FPS, 125);

    // Store off the actual resolution the camera is running at
    m_ResolutionW = m_VideoCap->get(cv::CAP_PROP_FRAME_WIDTH);
    m_ResolutionH = m_VideoCap->get(cv::CAP_PROP_FRAME_HEIGHT);

    printf("Camera opened: %d x %d\r\n",(int)m_ResolutionW,(int)m_ResolutionH);

    // Tried to set some of the camera settings... not working yet
    //m_VideoCap->set(cv::CAP_PROP_BRIGHTNESS,0.2f);
    //m_VideoCap->set(cv::CAP_PROP_EXPOSURE,0.2f);
    //m_VideoCap->set(cv::CAP_PROP_AUTO_EXPOSURE,0);
    //m_VideoCap->set(cv::CAP_PROP_GAIN, 0.1f);

    cv::namedWindow("Video",0);

    if(Debug == true)
    {
         cv::createTrackbar( "hmin:", "Video", &hmin, 255, NULL );
         cv::createTrackbar( "hmax:", "Video", &hmax, 255, NULL );
         cv::createTrackbar( "smin:", "Video", &smin, 255, NULL );
         cv::createTrackbar( "smax:", "Video", &smax, 255, NULL );
         cv::createTrackbar( "vmin:", "Video", &vmin, 255, NULL );
         cv::createTrackbar( "vmax:", "Video", &vmax, 255, NULL );
         cv::namedWindow("Thres", 0);

         //cv::createTrackbar( "exposure:", "Video", &exposure, 255, NULL );
         //cv::createTrackbar( "brightness:", "Video", &brightness, 255, NULL );
         //cv::createTrackbar( "gain:", "Video", &gain, 255, NULL );
     }
     cv::waitKey(1);
}

void VisionTrackerClass::Shutdown()
{
    delete m_VideoCap;
    m_VideoCap = NULL;
    cv::destroyWindow("Video");
}

float VisionTrackerClass::Pixel_X_To_Normalized_X(float pixel_x)
{
    return (pixel_x - 0.5f*m_ResolutionW) / (0.5f*m_ResolutionW);
}

float VisionTrackerClass::Pixel_Y_To_Normalized_Y(float pixel_y)
{
    return -(pixel_y - 0.5f*m_ResolutionH) / (0.5f*m_ResolutionH);
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

void VisionTrackerClass::Process()
{
    if (Debug)
    {
        //m_VideoCap->set(cv::CAP_PROP_BRIGHTNESS,(float)(brightness)/255.0f);
        //m_VideoCap->set(cv::CAP_PROP_EXPOSURE,(float)(exposure)/255.0f);
        //m_VideoCap->set(cv::CAP_PROP_GAIN,(float)(gain)/255.0f);
    }

    unsigned int i;
    bool got_frame = m_VideoCap->read(m_Img);
    //m_Img=cv::cvarrToMat(cvQueryFrame(m_VideoCap));
    if (got_frame && (m_Img.empty() == false))
    {
        int d = 0;

        m_TargetX = INVALID_TARGET;
        m_TargetY = INVALID_TARGET;
        m_NewImageProcessed = true;

        cv::cvtColor(m_Img,m_ImgHSV,cv::COLOR_BGR2HSV);
       // cv::inRange(m_ImgHSV,cv::Scalar(40,167,193),cv::Scalar(119,255,255),m_Imgthresh);
        cv::inRange(m_ImgHSV,cv::Scalar(hmin,smin,vmin),cv::Scalar(hmax,smax,vmax),m_Imgthresh);
        cv::dilate(m_Imgthresh, m_ImgDilated, cv::Mat(), cv::Point(-1, -1), 2);
        cv::findContours( m_ImgDilated, m_Contours, m_Hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0) );

        draw_cross_hair(m_Img,Normalized_X_To_Pixel_X(m_CrossHairX),Normalized_Y_To_Pixel_Y(m_CrossHairY));

        // Eliminate some 'false-positive' contours
        for( i = 0; i < m_Contours.size(); i++)
        {
            float area = cv::contourArea(m_Contours[i]);
            if(area < 500)
            {
                m_Contours.erase(m_Contours.begin() + i);
                d++;
            }
        }

        // If we have contours left, try to pick the best target
        if (m_Contours.size() > 0)
        {
            int best_contour = 0;
            int best_area = 0.0f;

            for( i = 0; i < m_Contours.size(); i++)
            {
                float area = cv::contourArea(m_Contours[i]);
                if (area > best_area)
                {
                    best_area = area;
                    best_contour = i;
                }
                cv::Scalar color = cv::Scalar( 0, 0, 255);
                cv::drawContours( m_Img, m_Contours, i, color, 1, 4, m_Hierarchy, 0, cv::Point(0, 0));
            }

            // Set Targetx, Targety based on the best contour we found
            cv::RotatedRect rect = cv::minAreaRect(m_Contours[best_contour]);
            float target_pixel_x = rect.center.x;
            float target_pixel_y = rect.center.y;

            // compute the normalized target position (resolution independent)
            m_TargetX = Pixel_X_To_Normalized_X(target_pixel_x);
            m_TargetY = Pixel_Y_To_Normalized_Y(target_pixel_y);

            // Draw the rect on our image
            draw_rotated_rect(m_Img,rect,cv::Scalar(255, 255, 0));
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
            printf("fps: %f\r\n",m_FPS);
        }

        cv::imshow("Video",m_Img);

        // display the image on the desktop
        if(Debug)
        {
            cv::imshow("Video",m_Img);
            cv::imshow("Thres",m_Imgthresh);
        }
        cv::waitKey(1);
    }
}

void VisionTrackerClass::Print_FPS_On_Image(cv::Mat & img)
{
    char buffer[128];
    sprintf(buffer, "fps: %f",m_FPS);
    cv::putText(img, buffer, cv::Point(0, 15), cv::FONT_HERSHEY_SIMPLEX, 0.5,cv::Scalar(0,255,0),2);
}

void VisionTrackerClass::Get_Image(unsigned char ** data, unsigned int * byte_count,int quality)
{
    std::vector<int> params;
    params.push_back(cv::IMWRITE_JPEG_QUALITY);
    params.push_back(quality);

    if (m_FlipImage)
    {
        #define FLIP_X 0
        #define FLIP_Y 1
        cv::flip(m_Img,m_FlippedImg,FLIP_X);
        Print_FPS_On_Image(m_FlippedImg);
        cv::imencode(".jpg", m_FlippedImg, m_JpegOutputBuffer, params);
    }
    else
    {
        Print_FPS_On_Image(m_Img);
        cv::imencode(".jpg", m_Img, m_JpegOutputBuffer, params);
    }

    *data = &(m_JpegOutputBuffer[0]);
    *byte_count = m_JpegOutputBuffer.size();

    m_NewImageProcessed = false;
    m_TimeElapsedSinceLastFrameGet = 0.0f;
}


