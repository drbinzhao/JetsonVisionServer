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

    bool New_Image_Processed() { return m_NewImageProcessed; }
    void Get_Image(unsigned char ** data, unsigned int * byte_count);

protected:

    cv::VideoCapture * m_VideoCap;
    //CvCapture *m_VideoCap;
    cv::Mat m_Img;
    cv::Mat m_ImgHSV;
    cv::Mat m_Imgthresh;
    cv::Mat m_ImgDilated;
    cv::Mat m_ImgContours;
    cv::vector<cv::vector<cv::Point> > m_Contours;
    cv::vector<cv::vector<cv::Point> > m_Goodcontours;
    cv::vector<cv::Vec4i> m_Hierarchy;
    std::vector<uchar> m_JpegOutputBuffer;
    double m_LastUpdateTime;

    float m_TargetX;
    float m_TargetY;
    bool m_NewImageProcessed;

};
