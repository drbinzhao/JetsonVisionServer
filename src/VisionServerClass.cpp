#include "VisionServerClass.h"
#include "stdio.h"
#include "string.h"


// Good info here: http://answers.opencv.org/question/6976/display-iplimage-in-webbrowsers/
//http://stackoverflow.com/questions/33064955/how-to-create-a-http-mjpeg-streaming-server-with-qtcp-server-sockets
//bool imencode(const string& ext, InputArray img, vector<uchar>& buf, const vector<int>& params=vector<int>())

VisionServerClass::VisionServerClass() :
    m_NewImageRecieved(false),
    m_ImageDataSize(0),
    m_ArmAngle(0.0f),
    m_TargetX(0.0f),
    m_TargetY(0.0f),
    m_VideoEnabled(0)
{
    memset(m_ImageData,0,sizeof(m_ImageData));
}

VisionServerClass::~VisionServerClass()
{
    //dtor
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
    if (strncmp(msg,"GET / HTTP",10) == 0)
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
        LOG(("Sending image to socket: %d size: %d\r\n",client_sock,image_data_size));

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
    switch(cmd[0])
    {
        case '0':
            // Get target, send the latest data to this client
            Cmd_Get_Target(client_socket,cmd);
            break;
        case '1':
            // set the current arm angle
            Cmd_Set_Arm_Angle(client_socket, cmd);
            break;
        case '2':
            //enable and disable video box
            Cmd_Video_Enable(client_socket, cmd);
            break;
        default:
            // show options to the client
            Cmd_Help(client_socket,cmd);
            break;
    }
}

void VisionServerClass::Cmd_Help(int client_socket, char * cmd)
{
    Send_String(client_socket, cmd);
    Send_String(client_socket, "Command:\r\n");
    Send_String(client_socket," 0 - returns target x y\r\n");
    Send_String(client_socket," 1 <armangle> - set the arm angle\r\n");
    Send_String(client_socket," 2 <videoenable> - enable and disable video window\r\n");
    Send_String(client_socket," q - SHUTDOWN\r\n");
}
void VisionServerClass::Cmd_Shutdown(int client_socket,char * cmd)
{
    //system("pwd");
    //system("/home/team987/KillScript.sh");
}

void VisionServerClass::Cmd_Get_Target(int client_socket,char * cmd)
{
    char buf[1024];
    sprintf(buf,"0 %10.6g %10.6g\r\n",m_TargetX, m_TargetY);
    Send_String(client_socket,buf);
}

void VisionServerClass::Cmd_Set_Arm_Angle(int client_socket,char * cmd)
{
    float rads;
    sscanf(cmd,"1 %f",&rads);
    m_ArmAngle = rads;
}
void VisionServerClass::Cmd_Video_Enable(int client_socket, char * cmd)
{
    int value;
    sscanf(cmd,"2 %d",&value);
    m_VideoEnabled = value;
}

