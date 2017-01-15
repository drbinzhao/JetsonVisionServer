#include "ServerClass.h"
#include "VisionServerClass.h"
#include "VisionTracker.h"
//#include "OpenCVTest.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>
#include <string.h>


// notes:
// OpenCV video stream - source\samples\cpp\starter_video.cpp

VisionServerClass g_VisionServer;
VisionTrackerClass g_VisionTracker;

const float MJPEG_TARGET_FRAME_TIME = 1.0f/15.0f;


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
        int bytes_read = fread(data,1,data_size,f);
        if (bytes_read != (int)data_size)
        {
            printf("Reading file %s expected %d bytes but got %d\r\n",name,data_size,bytes_read);
        }
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

    sleep(2);
    printf("987 Jetson Vision Server...\r\n");
    char buf[1024];
    char * dir = getcwd(buf,sizeof(buf));
    if (dir != NULL)
    {
        printf("Current Directory: %s\r\n",dir);
    }

    // Print out the arguments for Debugging
    printf("%d args\r\n",argc);
    for (int i=0; i<argc; ++i)
    {
        printf("arg[%d] %s\r\n",i,argv[i]);
    }

    // First argument after the program name is the number of seconds to delay
    if (argc > 1)
    {
        int delay = atoi(argv[1]);
        if (delay > 0) {
            printf("delaying startup for %d seconds...\r\n",delay);
            sleep(delay);
        }
    }

    printf("Starting up!\r\n");
    g_VisionServer.Init(5800);
    g_VisionTracker.Init();

    //Test_Mjpg();
    //Test_OpenCV();
    //Test_Vision_Tracker();

    while(1)
    {
        g_VisionServer.Process();
        g_VisionTracker.Process();

        // pass data between systems
        g_VisionServer.Set_Target(g_VisionTracker.Get_Target_X(),g_VisionTracker.Get_Target_Y(),g_VisionTracker.Get_Target_Area());
        g_VisionTracker.Set_Cross_Hair(g_VisionServer.Get_Cross_Hair());
        g_VisionTracker.Set_Equalize_Image(g_VisionServer.Get_Equalize_Image());

        if (g_VisionTracker.New_Image_Processed())
        {
            // limit the framerate of the mjpeg stream
            if (g_VisionTracker.Get_Time_Elapsed_Since_Last_Frame_Get() > MJPEG_TARGET_FRAME_TIME)
            {
                unsigned char * data;
                unsigned int byte_count;
                g_VisionTracker.Get_Image(&data,&byte_count,g_VisionServer.Get_Mjpeg_Quality());
                g_VisionServer.Send_New_Image(data,byte_count);
            }
        }
    }
    return 0;
}
