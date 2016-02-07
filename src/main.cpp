#include "ServerClass.h"
#include "VisionServerClass.h"
#include "VisionTracker.h"
#include "OpenCVTest.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>


// notes:
// OpenCV video stream - source\samples\cpp\starter_video.cpp

VisionServerClass g_VisionServer;
VisionTrackerClass g_VisionTracker;




/*
//http://answers.opencv.org/question/6976/display-iplimage-in-webbrowsers/
// convert the image to JPEG ( in memory! )

std::vector<uchar>outbuf;
std::vector<int> params;
params.push_back(CV_IMWRITE_JPEG_QUALITY);
params.push_back(100);
cv::imencode(".jpg", frame, outbuf, params);
*/



void Load_File(const char * name,unsigned char ** out_data, unsigned int * out_data_size)
{
    unsigned char * data = NULL;
    unsigned int data_size = 0;

    FILE *f = fopen(name,"r");
    if (f != NULL)
    {
        // size of the file
        struct stat file_stats;
        stat(name,&file_stats);
        data_size = file_stats.st_size;

        // allocate memory and read in the bytes
        data = new unsigned char[data_size];
        memset(data,0,data_size);
        fread(data,1,data_size,f);
        fclose(f);
    }

    if (out_data != NULL)
    {
        *out_data = data;
    }
    if (out_data_size != NULL)
    {
        *out_data_size = data_size;
    }
}

void Test_Mjpg()
{

    unsigned int img1_size;
    unsigned char * img1_data;
    unsigned int img2_size;
    unsigned char * img2_data;

    // load two jpg files
    Load_File("WaterCandle.jpg",&img1_data, &img1_size);
    Load_File("Car.jpg",&img2_data, &img2_size);

    for (;;)
    {
        g_VisionServer.Send_New_Image(img1_data,img1_size);
        g_VisionServer.Process();
        usleep(500000);

        g_VisionServer.Send_New_Image(img2_data,img2_size);
        g_VisionServer.Process();
        usleep(500000);
    }
}

void Test_Vision_Tracker()
{
    for(;;)
    {
        g_VisionTracker.Process();
    }
}

int main(int argc,char ** argv)
{
    char dir[1024];
    getcwd(dir,sizeof(dir));
    printf("Current Directory: %s\r\n",dir);

    g_VisionServer.Init(9870);
    g_VisionTracker.Init();

    //Test_Mjpg();
    //Test_OpenCV();
    //Test_Vision_Tracker();

    while(1)
    {
        g_VisionServer.Process();
        g_VisionTracker.Process();

        // pass data between systems
        g_VisionServer.Set_Target(g_VisionTracker.Get_Target_X(),g_VisionTracker.Get_Target_Y());

        if (g_VisionTracker.New_Image_Processed())
        {
            unsigned char * data;
            unsigned int byte_count;
            g_VisionTracker.Get_Image(&data,&byte_count);
            g_VisionServer.Send_New_Image(data,byte_count);
        }
    }

    return 0;
}
