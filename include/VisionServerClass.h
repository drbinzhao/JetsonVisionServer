#ifndef MJPGSERVERCLASS_H
#define MJPGSERVERCLASS_H

#include "ServerClass.h"

//
// VisionServerClass
//
// Functions as both an Mjpg server and a server for sending the vision data to the RoboRio
// clients that connect with 'GET ... ' will be sent the mjpeg stream
// clients that connect and send 'VISION' will be able to send vision commands and recieve data
//

class VisionServerClass : public ServerClass
{
    public:
        VisionServerClass();
        virtual ~VisionServerClass();

        virtual void Process();

        // mJpeg interface - publish images to all clients trying to watch the mjpegfeed
        void Send_New_Image(const unsigned char * image_data, unsigned int image_data_size);

        // Vision data interface, accept commands from the robo-rio and send targetting data
        void Handle_Command(int client_socket,char * cmd);
        void Cmd_Shutdown(int client_socket,char * cmd);
        void Cmd_Get_Target(int client_socket,char * cmd);
        void Cmd_Set_Arm_Angle(int client_socket,char * cmd);
        void Cmd_Help(int client_socket, char * cmd);
        void Cmd_Video_Enable(int client_socket, char * cmd);
        void Set_Target(float x, float y){m_TargetX = x; m_TargetY = y;}
        float Get_Arm_Angle(void){return m_ArmAngle;}
        int Get_Video_Enable() {return m_VideoEnabled;}

    protected:

        void Send_Mjpg_Http_Header(int client_socket);
        virtual void Handle_New_Client_Connected(int client_socket);
        virtual void Handle_Client_Disconnected(int client_socket);
        virtual void Handle_Incoming_Message(int client_socket,char * msg);

    private:

        void Deliver_Next_Image_To_Clients(const unsigned char * image_data, unsigned int image_data_size);

        bool m_NewImageRecieved;
        int m_ImageDataSize;
        unsigned char m_ImageData[1920*1080*4];     // grossly oversized image buffer!
        std::set<int> m_ClientsReadyForImages;

        float m_ArmAngle;
        float m_TargetX;
        float m_TargetY;

        int m_VideoEnabled;
};

#endif // MJPGSERVERCLASS_H