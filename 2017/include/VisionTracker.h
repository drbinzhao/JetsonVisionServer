#include <opencv2/core.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "CrossHairClass.h"
#include "pthread.h"
#include <unistd.h>
#define INVALID_TARGET -2.0f

class DriverCamClass
{
    public:
    static cv::Mat m_Img2A;
    static cv::Mat m_Img2B;
    static cv::Mat *m_Img2ReadPtr;
    static cv::Mat *m_Img2WritePtr;
    static cv::VideoCapture * m_VideoCap2;
    static bool m_NewSecondaryImg;
    static pthread_mutex_t FRAMELOCKER;
    static void Init()
    {
        m_Img2ReadPtr = &m_Img2A;
        m_Img2WritePtr = &m_Img2B;
        m_VideoCap2 = new cv::VideoCapture(1);

        if(m_VideoCap2->isOpened())
        {
            m_VideoCap2->set(cv::CAP_PROP_FRAME_WIDTH,320);
            m_VideoCap2->set(cv::CAP_PROP_FRAME_HEIGHT,240);
            m_VideoCap2->set(cv::CAP_PROP_FPS, 15);
        }

    }
    static void GetSecondaryImg(cv::Mat& FillThis)
    {
        if(m_NewSecondaryImg)
        {
            pthread_mutex_lock(&FRAMELOCKER);
            FillThis =  *m_Img2ReadPtr;
            m_NewSecondaryImg = false;
            pthread_mutex_unlock(&FRAMELOCKER);
        }
    }
    static void Capture()
    {
        m_VideoCap2->read(*m_Img2WritePtr);

        pthread_mutex_lock(&FRAMELOCKER);
        cv::Mat* Tmp = m_Img2ReadPtr;
        m_Img2ReadPtr = m_Img2WritePtr;
        m_Img2WritePtr = Tmp;
        m_NewSecondaryImg = true;
        pthread_mutex_unlock(&FRAMELOCKER);
    }

    static void* Process(void* arg)
    {
        while(1)
        {
            Capture();
            usleep(100000); //.1s
        }
        return NULL;
    }
    static void Shutdown()
    {
        delete m_VideoCap2;
        m_VideoCap2 = NULL;
    }
};
class VisionTrackerClass
{
public:

    VisionTrackerClass();
    ~VisionTrackerClass();

    void Init();
    void Shutdown();
    cv::Mat* GetSecondaryImg();
    void SetSecondaryImg();
    void Process();

    float Get_Target_X() { return m_TargetX; }
    float Get_Target_Y() { return m_TargetY; }
    float Get_Target_Area() { return m_TargetArea; }

    void Set_Cross_Hair(CrossHairClass c) { m_CrossHair = c; }

    bool New_Image_Processed() { return m_NewImageProcessed; }
    void Get_Image(unsigned char ** data, unsigned int * byte_count,int quality);
    float Get_Time_Elapsed_Since_Last_Frame_Get() { return m_TimeElapsedSinceLastFrameGet; }
    void Set_Equalize_Image(bool eq) { m_EqualizeImage = eq; }

   static void* ProcessThread2(void* arg);
protected:

    void Print_FPS_On_Image(cv::Mat & img);

    float Pixel_X_To_Normalized_X(float pixel_x);
    float Pixel_Y_To_Normalized_Y(float pixel_y);
    float Pixel_Area_To_Normalized_Area(float pixel_area);
    float Normalized_X_To_Pixel_X(float nx);
    float Normalized_Y_To_Pixel_Y(float ny);
    float Normalized_Area_To_Pixel_Area(float na);


    cv::VideoCapture * m_VideoCap;
    cv::Mat m_Img;
    cv::Mat m_Img2;

    cv::Mat m_TmpImg;
    cv::Mat m_ImgHSV;
    cv::Mat m_Imgthresh;
    cv::Mat m_ImgDilated;
    cv::Mat m_ImgContours;
    std::vector<std::vector<cv::Point> > m_Contours;
    std::vector<std::vector<cv::Point> > m_Goodcontours;
    std::vector<cv::Vec4i> m_Hierarchy;
    std::vector<uchar> m_JpegOutputBuffer;
    double m_LastUpdateTime;

    float m_ResolutionW;
    float m_ResolutionH;

    float m_TargetX;
    float m_TargetY;
    float m_TargetArea;
    int m_TargetContourIndex;

    CrossHairClass m_CrossHair;

    bool m_NewImageProcessed;
    unsigned int m_FrameCounter;
    float m_TimeElapsedSinceLastFrameGet;
    float m_FPSTimeElapsed;
    float m_FPS;
    bool m_EqualizeImage;

};
