#include "VisionServerClass.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <unistd.h>
#include <math.h>
#include "VisionTracker.h"


const char * SETTINGS_FILENAME = "/home/ubuntu/Documents/JetsonVisionServerSettings.txt";


// Good info here: http://answers.opencv.org/question/6976/display-iplimage-in-webbrowsers/
//http://stackoverflow.com/questions/33064955/how-to-create-a-http-mjpeg-streaming-server-with-qtcp-server-sockets
//bool imencode(const string& ext, InputArray img, vector<uchar>& buf, const vector<int>& params=vector<int>())

VisionServerClass::VisionServerClass() :
    m_NewImageRecieved(false),
    m_ImageDataSize(0),
    m_ArmAngle(0.0f),
    m_TargetX(0.0f),
    m_TargetY(0.0f),
    m_MjpegQuality(30),
    m_FlipImage(false),
    m_EqualizeImage(false)
{
    memset(m_ImageData,0,sizeof(m_ImageData));
    Load_Settings();
    Save_Settings();
}

VisionServerClass::~VisionServerClass()
{
    //dtor
}


void VisionServerClass::Save_Settings()
{
    FILE * f = fopen(SETTINGS_FILENAME,"w");
    if (f != NULL)
    {
        fprintf(f,"CrossHair %f %f %f\r\n",m_CrossHair.X,m_CrossHair.YNear,m_CrossHair.YFar);
        fprintf(f,"CrossHair2 %f %f %f\r\n",m_CrossHair2.X,m_CrossHair2.YNear,m_CrossHair2.YFar);
        fprintf(f,"MjpegQuality %d\r\n",m_MjpegQuality);
        fclose(f);
    }
}
void VisionServerClass::Load_Settings()
{
    FILE * f = fopen(SETTINGS_FILENAME,"r");
    if (f != NULL)
    {
        char line[1024];
        while (fgets(line, sizeof(line), f))
        {
            char *token = strtok(line," \r\n");

            // handle the different cases of settings
            if (strcmp(token,"CrossHair") == 0)
            {
                m_CrossHair.X = atof(strtok(NULL," \r\n"));
                m_CrossHair.YNear = atof(strtok(NULL," \r\n"));
                m_CrossHair.YFar = atof(strtok(NULL," \r\n"));
            }
            else if(strcmp(token,"CrossHair2") == 0)
            {
                m_CrossHair2.X = atof(strtok(NULL," \r\n"));
                m_CrossHair2.YNear = atof(strtok(NULL," \r\n"));
                m_CrossHair2.YFar = atof(strtok(NULL," \r\n"));
            }
            else if (strcmp(token,"MjpegQuality") == 0)
            {
                m_MjpegQuality = atoi(strtok(NULL," \r\n"));
            }
        }

        fclose(f);
        printf("Loaded Settings:\r\n");
        printf(" CrossHair: %f, %f, %f\r\n",m_CrossHair.X,m_CrossHair.YNear,m_CrossHair.YFar);
        printf(" MjpegQuality: %d\r\n",m_MjpegQuality);
    }
    else
    {
        char dir_name[1024];
        char * retval = getcwd(dir_name,sizeof(dir_name));
        printf("Failed to load settings file: %s\r\n",SETTINGS_FILENAME);
        if (retval != NULL)
        {
            printf(" Current directory: %s\r\n",dir_name);
        }
    }
}

void VisionServerClass::Send_Mjpg_Http_Header(int client_socket)
{
   Send_String(client_socket,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: multipart/x-mixed-replace;boundary=--myboundary\r\n"
        "Cache-Control: no-store\r\n"
        "Pragma: no-cache\r\n"
        "Connection: close\r\n"
        "\r\n");
}
void VisionServerClass::Handle_New_Client_Connected(int client_socket)
{
    LOG(("MjpgServerClass::Handle_New_Client_Connected %d\r\n",client_socket));
}

void VisionServerClass::Handle_Client_Disconnected(int client_socket)
{
    m_ClientsReadyForImages.erase(client_socket);
}

void VisionServerClass::Handle_Incoming_Message(int client_socket,char * msg)
{
    // print out the message
    LOG(("Incoming message: %s\r\n",msg));

    // if its an HTTP GET, send the header and start sending images
    //bool is_mjpeg_connection = (strncmp(msg,"GET / HTTP",10) == 0);  //smart dash sends 'GET /mjpg/video.mjpg...'
    bool is_mjpeg_connection = (strncmp(msg,"GET",3) == 0);

    if (is_mjpeg_connection)
    {
        LOG(("Recognized HTTP GET from client: %d\r\n",client_socket));
        Send_Mjpg_Http_Header(client_socket);
        m_ClientsReadyForImages.insert(client_socket);
    }

    // If this is a client that is watching the video feed, just ignore any messages from them
    if (m_ClientsReadyForImages.find(client_socket) != m_ClientsReadyForImages.end())
    {
        return;
    }

    // Otherwise, this is the Robo-rio or other computer wanting to interact with the Vision Data Server
    char * cmd = strtok(msg,"\n");
    while (cmd)
    {
        Handle_Command(client_socket,cmd);
        cmd = strtok(NULL,"\n");
    }
}
void VisionServerClass::Process()
{
    ServerClass::Process();

    // if we have new image data, pass it on to the clients
    if (m_NewImageRecieved)
    {
        Deliver_Next_Image_To_Clients(m_ImageData,m_ImageDataSize);
        m_NewImageRecieved = false;
    }
}

void VisionServerClass::Send_New_Image(const unsigned char * image_data, unsigned int image_data_size)
{
    if (image_data_size < sizeof(m_ImageData))
    {
        memcpy(m_ImageData,image_data,image_data_size);
        m_ImageDataSize = image_data_size;
    }
    m_NewImageRecieved = true;
}


void VisionServerClass::Deliver_Next_Image_To_Clients(const unsigned char * image_data, unsigned int image_data_size)
{
    std::set<int>::iterator it;
    for (it = m_ClientsReadyForImages.begin(); it != m_ClientsReadyForImages.end(); ++it)
    {
        int client_sock = *it;
        LOG_SEND(("Sending image to socket: %d size: %d\r\n",client_sock,image_data_size));

        static char tmp[2048];
        sprintf(tmp,"--myboundary\r\n"
            "Content-Type: image/jpeg\r\n"
            "Content-length: %d\r\n"
            "\r\n",image_data_size);

        Send_String(client_sock,tmp);
        Send_Binary_Data(client_sock,(char*)image_data,image_data_size);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
//
//  Vision Data System, accept commands from the robo-rio, send data back
//  Numbered commands.
//
//////////////////////////////////////////////////////////////////////////////////////////

void VisionServerClass::Handle_Command(int client_socket,char * cmd)
{
    // Super Simple command parsing... first character is the command, remaining are arguments
    const char * params = "";
    if (strlen(cmd) > 2) { params = cmd + 2; }

    switch(cmd[0])
    {
        case '0':
            // Get target, send the latest data to this client
            Cmd_Get_Target(client_socket,params);
            break;
        case '1':
            // set the current arm angle
            Cmd_Set_Arm_Angle(client_socket, params);
            break;
        case '2':
            // set image flipping
            Cmd_Flip_Image(client_socket, params);
            break;
        case '3':
            // set mjpeg quality
            Cmd_Set_Mjpeg_Quality(client_socket, params);
            break;

        case '4':
            // Set targetting calibration
            Cmd_Set_Cross_Hair(client_socket,params);
            break;
        case '5':
            //Get cross hair
            Cmd_Get_Cross_Hair(client_socket,params);
            break;
         case '6':
            // set image flipping
            Cmd_Equalize_Image(client_socket, params);
            break;
        case 'q':
            Cmd_Shutdown(client_socket, params);
            break;
        default:
            // show options to the client
            Cmd_Help(client_socket,params);
            break;
    }
}

void VisionServerClass::Cmd_Help(int client_socket, const char * params)
{
    Send_String(client_socket, params);
    Send_String(client_socket, "Command:\r\n");
    Send_String(client_socket," 0 - returns target x y\r\n");
    Send_String(client_socket," 1 <armangle> - set the arm angle\r\n");
    Send_String(client_socket," 2 <flip image> - flip image (0 or 1)\r\n");
    Send_String(client_socket," 3 <mjpg quality> - set mjpeg quality (0-100)\r\n");
    Send_String(client_socket," 4 set crosshair coords to current target \r\n");
    Send_String(client_socket," 5 get crosshair\r\n");
    Send_String(client_socket," 6 <equalize image> - equalize image (0 or 1)\r\n");
    Send_String(client_socket," q - SHUTDOWN\r\n");
}

void VisionServerClass::Cmd_Shutdown(int client_socket,const char * params)
{
    int success;
    success = system("~/Killscript.sh");
    if (success == 0)
    {
        printf("system call failed.");
    }
}

void VisionServerClass::Cmd_Get_Target(int client_socket,const char * params)
{
    char buf[1024];
    sprintf(buf,"0 %.4f %.4f %.4f\r\n",m_TargetX, m_TargetY, m_TargetArea);
    Send_String(client_socket,buf);
}

void VisionServerClass::Cmd_Set_Arm_Angle(int client_socket,const char * params)
{
    float rads;
    sscanf(params,"%f",&rads);
    m_ArmAngle = rads;
}

void VisionServerClass::Cmd_Flip_Image(int client_socket,const char *params)
{
    int value;
    sscanf(params,"%d",&value);
    m_FlipImage = (value != 0);
}

void VisionServerClass::Cmd_Set_Mjpeg_Quality(int client_socket, const char * params)
{
    int value;
    sscanf(params,"%d",&value);
    m_MjpegQuality = value;
    Save_Settings();
}

void VisionServerClass::Cmd_Set_Cross_Hair(int client_socket,const char * params)
{
    if ((fabs(m_TargetX) < 1.0f) && (fabs(m_TargetY) < 1.0f))
    {
        if (m_FlipImage)
        {
            m_CrossHair2.Update_Calibration(m_TargetX,m_TargetY,m_TargetArea);
        }
        else
        {
            m_CrossHair.Update_Calibration(m_TargetX,m_TargetY,m_TargetArea);
        }

        Save_Settings();
    }
}

void VisionServerClass::Cmd_Get_Cross_Hair(int client_socket,const char * params)
{
    char buf[1024];

    // pick the right cross hair depending on if we're shooting backwards.
    CrossHairClass c = m_CrossHair;
    if (m_FlipImage)
    {
        c = m_CrossHair2;
    }

    float x = c.X;
    float y = c.Get_Average_Y();
    if (m_TargetX != INVALID_TARGET)
    {
        y = c.Get_Y(m_TargetArea);
    }
    sprintf(buf,"5 %.4f %.4f\r\n",x, y);
    Send_String(client_socket,buf);
}

void VisionServerClass::Cmd_Equalize_Image(int client_socket,const char *params)
{
    int value;
    sscanf(params,"%d",&value);
    m_EqualizeImage = (value != 0);
}
