#include <opencv2/core.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <time.h>
#include <stdio.h>
#include <VisionTracker.h>

//cv::RNG rng(12345);
//cv::Scalar color = cv::Scalar( rng.uniform(0, 1), rng.uniform(0, 1), rng.uniform(254, 255) );


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
        cv::line(image,vertices[i-1],vertices[i],color);
   }
}



VisionTrackerClass::VisionTrackerClass() :
    m_VideoCap(NULL),
    m_LastUpdateTime(0.0),
    m_TargetX(INVALID_TARGET),
    m_TargetY(INVALID_TARGET),
    m_NewImageProcessed(false)
{
}

VisionTrackerClass::~VisionTrackerClass()
{
}


void VisionTrackerClass::Init()
{
    m_VideoCap = new cv::VideoCapture(0);
    if (!m_VideoCap->isOpened())
    {
        return;
    }

    //cv_cap->set(CV_CAP_PROP_FORMAT,CV_8UC3);
    int w = m_VideoCap->get(CV_CAP_PROP_FRAME_WIDTH);
    int h = m_VideoCap->get(CV_CAP_PROP_FRAME_HEIGHT);
    printf("Camera initial size: %d x %d\r\n",w,h);

    m_VideoCap->set(CV_CAP_PROP_FRAME_WIDTH,320);
    m_VideoCap->set(CV_CAP_PROP_FRAME_HEIGHT,240);
    m_VideoCap->set(CV_CAP_PROP_BRIGHTNESS,0);
    m_VideoCap->set(CV_CAP_PROP_EXPOSURE,10);

    w = m_VideoCap->get(CV_CAP_PROP_FRAME_WIDTH);
    h = m_VideoCap->get(CV_CAP_PROP_FRAME_HEIGHT);
    printf("Camera opened: %d x %d\r\n",w,h);

    cv::namedWindow("Video",0);
    cv::waitKey(1);
}

void VisionTrackerClass::Shutdown()
{
    delete m_VideoCap;
    m_VideoCap = NULL;
    cv::destroyWindow("Video");
}

void VisionTrackerClass::Process()
{
    unsigned int i;
    bool got_frame = m_VideoCap->read(m_Img);

    if (got_frame && (m_Img.empty() == false))
    {
        int d = 0;
        m_TargetX = INVALID_TARGET;
        m_TargetY = INVALID_TARGET;
        m_NewImageProcessed = true;

        cv::cvtColor(m_Img,m_ImgHSV,cv::COLOR_BGR2HSV);
        cv::inRange(m_ImgHSV,cv::Scalar(40,167,193),cv::Scalar(119,255,255),m_Imgthresh);
        cv::dilate(m_Imgthresh, m_ImgDilated, cv::Mat(), cv::Point(-1, -1), 2);
        cv::findContours( m_ImgDilated, m_Contours, m_Hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0) );

        // Eliminate some 'false-positive' contours
        for( i = 0; i < m_Contours.size(); i++)
        {
            float area = cv::contourArea(m_Contours[i]);
            if(area < 50)
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
                cv::drawContours( m_Img, m_Contours, i, color, 2, 8, m_Hierarchy, 0, cv::Point(0, 0));
            }

            // Set Targetx, Targety based on the best contour we found
            cv::RotatedRect rect = cv::minAreaRect(m_Contours[best_contour]);
            m_TargetX = rect.center.x;
            m_TargetY = rect.center.y;

            // Draw the rect on our image
            draw_rotated_rect(m_Img,rect,cv::Scalar( 255, 0, 0));
        }


        // measure framerate of the overall system (from last time we updated to this time)
        double t = cv::getTickCount();
        double delta_t = (t-m_LastUpdateTime)/cv::getTickFrequency();
        m_LastUpdateTime = t;

        if (delta_t > 0.0)
        {
            char buffer[256];
            sprintf(buffer, "d: %d, fps: %f",d,1.0/delta_t);
            cv::putText(m_Img, buffer, cv::Point(0, 50), CV_FONT_HERSHEY_SIMPLEX, 1.5,cv::Scalar(0,255,0));
        }

        // display the image
        cv::imshow("Video",m_Img);
        cv::waitKey(1);
    }
}

void VisionTrackerClass::Get_Image(unsigned char ** data, unsigned int * byte_count)
{
    std::vector<int> params;
    params.push_back(CV_IMWRITE_JPEG_QUALITY);
    params.push_back(10);
    cv::imencode(".jpg", m_Img, m_JpegOutputBuffer, params);

    *data = &(m_JpegOutputBuffer[0]);
    *byte_count = m_JpegOutputBuffer.size();

    m_NewImageProcessed = false;
}


