#include <opencv2/core.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>


#define INVALID_TARGET -1000.0f

class VisionTrackerClass
{
public:

    VisionTrackerClass();
    ~VisionTrackerClass();

    void Init();
    void Shutdown();
    void Process();

    float Get_Target_X() { return m_TargetX; }
    float Get_Target_Y() { return m_TargetY; }

    void Set_Cross_Hair_X(float x) { m_CrossHairX = x; }
    float Get_Cross_Hair_X(void) { return m_CrossHairX; }
    void Set_Cross_Hair_Y(float y) { m_CrossHairY = y; }
    float Get_Cross_Hair_Y(void) { return m_CrossHairY; }

    bool New_Image_Processed() { return m_NewImageProcessed; }
    void Get_Image(unsigned char ** data, unsigned int * byte_count,int quality);
    float Get_Time_Elapsed_Since_Last_Frame_Get() { return m_TimeElapsedSinceLastFrameGet; }
    void Set_Flip_Image(bool flip) { m_FlipImage = flip; }

protected:

    void Print_FPS_On_Image(cv::Mat & img);

    float Pixel_X_To_Normalized_X(float pixel_x);
    float Pixel_Y_To_Normalized_Y(float pixel_y);
    float Normalized_X_To_Pixel_X(float nx);
    float Normalized_Y_To_Pixel_Y(float ny);

    cv::VideoCapture * m_VideoCap;
    //CvCapture *m_VideoCap;
    cv::Mat m_Img;
    cv::Mat m_FlippedImg;
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
    float m_CrossHairX;
    float m_CrossHairY;

    bool m_NewImageProcessed;
    unsigned int m_FrameCounter;
    float m_TimeElapsedSinceLastFrameGet;
    float m_FPSTimeElapsed;
    float m_FPS;
    bool m_FlipImage;

};
