#ifndef MJPGSERVERCLASS_H
#define MJPGSERVERCLASS_H

#include "ServerClass.h"
#include "CrossHairClass.h"

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

        void Cmd_Help(int client_socket, const char * params);
        void Cmd_Shutdown(int client_socket,const char * params);

        void Cmd_Get_Target(int client_socket,const char * params);
        void Cmd_Set_Arm_Angle(int client_socket,const char * params);
        void Cmd_Set_Mjpeg_Quality(int client_socket, const char * params);
        void Cmd_Set_Cross_Hair(int client_socket,const char * params);
        void Cmd_Get_Cross_Hair(int client_socket,const char * params);
        void Cmd_Equalize_Image(int client_socket, const char * params);
        void Cmd_Camera_Mode(int client_socket, const char * params);


        void Set_Target(float x, float y,float area){m_TargetX = x; m_TargetY = y;m_TargetArea = area;}
        float Get_Arm_Angle(void){return m_ArmAngle;}
        int Get_Mjpeg_Quality() {return m_MjpegQuality; }
        bool Get_Equalize_Image() { return m_EqualizeImage; }
        CrossHairClass Get_Cross_Hair(){return m_CrossHair;}


        CameraMode Get_Camera_Mode(){ return m_CameraMode; }
    protected:

        void Send_Mjpg_Http_Header(int client_socket);
        virtual void Handle_New_Client_Connected(int client_socket);
        virtual void Handle_Client_Disconnected(int client_socket);
        virtual void Handle_Incoming_Message(int client_socket,char * msg);

        void Save_Settings();
        void Load_Settings();

    private:

        void Deliver_Next_Image_To_Clients(const unsigned char * image_data, unsigned int image_data_size);

        bool m_NewImageRecieved;
        int m_ImageDataSize;
        unsigned char m_ImageData[1920*1080*4];     // grossly oversized image buffer!
        std::set<int> m_ClientsReadyForImages;

        float m_ArmAngle;
        float m_TargetX;
        float m_TargetY;
        float m_TargetArea;
        CrossHairClass m_CrossHair;

        int m_MjpegQuality;
        bool m_EqualizeImage;
        CameraMode m_CameraMode;

};

#endif // MJPGSERVERCLASS_H
