#include <opencv2/core.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <time.h>
#include <stdio.h>

// Find the OpenCV stuff in the cmake files:
// find . -type f -exec grep -H 'opencv' {} \;


void Track()
{
    cv::Mat img;
    cv::Mat imgHSV;
    cv::Mat imgthresh;
    cv::Mat imgDilated;
    cv::Mat imgContours;
    cv::vector<cv::vector<cv::Point> > contours;
    cv::vector<cv::vector<cv::Point> > goodcontours;
    cv::vector<cv::Vec4i> hierarchy;
    cv::RNG rng(12345);

    int c;

    cv::VideoCapture * cv_cap = new cv::VideoCapture(0);
    if (!cv_cap->isOpened())
    {
        return;
    }
    //cv_cap->set(CV_CAP_PROP_FORMAT,CV_8UC3);
    int w = cv_cap->get(CV_CAP_PROP_FRAME_WIDTH);
    int h = cv_cap->get(CV_CAP_PROP_FRAME_HEIGHT);
    printf("Camera initial size: %d x %d\r\n",w,h);

    cv_cap->set(CV_CAP_PROP_FRAME_WIDTH,320);
    cv_cap->set(CV_CAP_PROP_FRAME_HEIGHT,240);
    cv_cap->set(CV_CAP_PROP_BRIGHTNESS,0);
    cv_cap->set(CV_CAP_PROP_EXPOSURE,0);

    w = cv_cap->get(CV_CAP_PROP_FRAME_WIDTH);
    h = cv_cap->get(CV_CAP_PROP_FRAME_HEIGHT);
    printf("Camera opened: %d x %d\r\n",w,h);

    cv::namedWindow("Video",0);

    char buffer[10];
    double t1 = 0.0;

    for(;;)
    {
        double t = cv::getTickCount();
        bool got_frame = cv_cap->read(img);

        if (got_frame && (img.empty() == false))
        {
            int d = 0;
            cv::cvtColor(img,imgHSV,cv::COLOR_BGR2HSV);
            cv::inRange(imgHSV,cv::Scalar(40,167,193),cv::Scalar(119,255,255),imgthresh);
            cv::dilate(imgthresh, imgDilated, cv::Mat(), cv::Point(-1, -1), 2);
            cv::findContours( imgDilated, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0) );
            for( unsigned int i = 0; i < contours.size(); i++)
            {
                if(cv::contourArea(contours[i])<50)
                {
                    contours.erase(contours.begin() + i);
                    d++;
                }
                else
                {
                    //cv::Scalar color = cv::Scalar( rng.uniform(0, 1), rng.uniform(0, 1), rng.uniform(254, 255) );
                    cv::Scalar color = cv::Scalar( 0, 0, 255);
                    cv::drawContours( img, contours, i, color, 2, 8, hierarchy, 0, cv::Point(0, 0));
                }
            }
            if (t1 > 0.0)
            {
                sprintf(buffer, "d: %d, fps: %f",d,1/t1);
                cv::putText(img, buffer, cv::Point(0, 50), CV_FONT_HERSHEY_SIMPLEX, 1.5,cv::Scalar(0,255,0));
            }
            cv::imshow("Video",img);
        }
        c = cv::waitKey(1);
        if (c== 27)
            break;

        t1 = ((double)cv::getTickCount()-t)/cv::getTickFrequency();
    }

    delete cv_cap;
    cv_cap = NULL;
    cv::destroyWindow("Video");
}




void Test_OpenCV()
{
    Track();
}
