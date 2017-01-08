

#if 0

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>

#define WIN32_MEAN_AND_LEAN
#include <winsock2.h>
#include <windows.h>




using namespace std;

class ROTException
{
public:
    ROTException() :
         m_pMessage("") {}
    virtual ~ROTException() {}
    ROTException(const char *pMessage) :
         m_pMessage(pMessage) {}
    const char * what() { return m_pMessage; }
private:
    const char *m_pMessage;
};
//---
int length;
char * buffer;

const char HEAD_RESPONSE[] =
{
    "HTTP Code: 200 OK\r\n"
    "Content-Type: multipart/x-mixed-replace;boundary=myboundary\r\n"
	"--myboundary\r\n"
};

const char HEAD_RESPONSE_PER_IMAGE[] =
{
	"Content-Type: image/jpeg\r\n"
	"Content-Length: 69214\r\n\r\n"

};

const char SEPARATOR[] =
{
    "\r\n"
	"--myboundary\r\n"
};
//---

const int  REQ_WINSOCK_VER   = 2;	// Minimum winsock version required
const int  DEFAULT_PORT      = 1234;// Listening port
const int  TEMP_BUFFER_SIZE  = 128;

string GetHostDescription(const sockaddr_in &sockAddr)
{
	ostringstream stream;
	stream << inet_ntoa(sockAddr.sin_addr) << ":" << ntohs(sockAddr.sin_port);
	return stream.str();
}

void SetServerSockAddr(sockaddr_in *pSockAddr, int portNumber)
{
	// Set family, port and find IP
	pSockAddr->sin_family = AF_INET;
	pSockAddr->sin_port = htons(portNumber);
	pSockAddr->sin_addr.S_un.S_addr = INADDR_ANY;
}


void HandleConnection(SOCKET hClientSocket, const sockaddr_in &sockAddr)
{
	// Print description (IP:port) of connected client
	cout << "Connected with " << GetHostDescription(sockAddr) << ".\n";

	char tempBuffer[TEMP_BUFFER_SIZE];

	// Read data
	while(true)
	{
		int retval;
		retval = recv(hClientSocket, tempBuffer, sizeof(tempBuffer), 0);
		if (retval==0)
		{
			break; // Connection has been closed
		}
		else if (retval==SOCKET_ERROR)
		{
			throw ROTException("socket error while receiving.");
		}
		else
		{
		    /////////////////////////////////////////////////////
		    //
		    //This is the part that sends response back to client
		    //
		    /////////////////////////////////////////////////////
            //Send header to client
            //---------------------
            /*
            HTTP Code: 200 OK
            Content-Type: multipart/x-mixed-replace; boundary=myboundary
            --myboundary
            */
			if (send(hClientSocket, HEAD_RESPONSE,sizeof(HEAD_RESPONSE), 0)==SOCKET_ERROR)
				throw ROTException("socket error while sending header");


            //Main loop
            //---------
            while(1){

                /*
                Read image data, just used a single image
                to keep things simple [Binary format]
                */
                ifstream is;
                is.open ("lena.jpg", ios::binary);
                // get length of file:
                is.seekg (0, ios::end);
                length = is.tellg();
                is.seekg (0, ios::beg);
                // allocate memory:
                buffer = new char [length];
                // read data as a block:
                is.read (buffer,length);


                //Send per image header to client
                //-------------------------------
                /*
                Content-Type: image/jpeg
                */
                if (send(hClientSocket, HEAD_RESPONSE_PER_IMAGE,sizeof(HEAD_RESPONSE_PER_IMAGE), 0)==SOCKET_ERROR)
                throw ROTException("socket error while sending header per image");
                //Sleep(500);
                //Send image data
                //---------------
                if (send(hClientSocket, buffer, length, 0)==SOCKET_ERROR)
				throw ROTException("socket error while sending image data");
                //Sleep(500);
                //Add a separator after image data
                //--------------------------------
                /*
                --myboundary
                */
                if (send(hClientSocket, SEPARATOR,sizeof(SEPARATOR), 0)==SOCKET_ERROR)
				throw ROTException("socket error while sending separator");

                //Close img file
                //--------------
                is.close();
                Sleep(500);
            //////////////////////////////////////////////////////
		    //end of loop
		    //////////////////////////////////////////////////////
            }

		}
	}
	cout << "Connection closed.\n";
}

bool RunServer(int portNumber)
{
	SOCKET 		hSocket = INVALID_SOCKET,
				hClientSocket = INVALID_SOCKET;
	bool		bSuccess = true;
	sockaddr_in	sockAddr = {0};

	try
	{
		// Create socket
		cout << "Creating socket... ";
		if ((hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
			throw ROTException("could not create socket.");
		cout << "created.\n";

		// Bind socket
		cout << "Binding socket... ";
		SetServerSockAddr(&sockAddr, portNumber);
		if (bind(hSocket, reinterpret_cast<sockaddr*>(&sockAddr), sizeof(sockAddr))!=0)
			throw ROTException("could not bind socket.");
		cout << "bound.\n";

		// Put socket in listening mode
		cout << "Putting socket in listening mode... ";
		if (listen(hSocket, SOMAXCONN)!=0)
			throw ROTException("could not put socket in listening mode.");
		cout << "done.\n";

		// Wait for connection
		cout << "Waiting for incoming connection... ";

		sockaddr_in clientSockAddr;
		int			clientSockSize = sizeof(clientSockAddr);

		// Accept connection:
		hClientSocket = accept(hSocket,
						 reinterpret_cast<sockaddr*>(&clientSockAddr),
						 &clientSockSize);

		// Check if accept succeeded
		if (hClientSocket==INVALID_SOCKET)
			throw ROTException("accept function failed.");
		cout << "accepted.\n";

		// Wait for and accept a connection:
		HandleConnection(hClientSocket, clientSockAddr);

	}
	catch(ROTException e)
	{
		cerr << "\nError: " << e.what() << endl;
		bSuccess = false;
	}

	if (hSocket!=INVALID_SOCKET)
		closesocket(hSocket);

	if (hClientSocket!=INVALID_SOCKET)
		closesocket(hClientSocket);

	return bSuccess;
}

int main(int argc, char* argv[])
{
	int iRet = 1;
	WSADATA wsaData;

	cout << "Initializing winsock... ";

	if (WSAStartup(MAKEWORD(REQ_WINSOCK_VER,0), &wsaData)==0)
	{
		// Check if major version is at least REQ_WINSOCK_VER
		if (LOBYTE(wsaData.wVersion) >= REQ_WINSOCK_VER)
		{
			cout << "initialized.\n";

			int port = DEFAULT_PORT;
			if (argc > 1)
				port = atoi(argv[1]);
			iRet = !RunServer(port);
		}
		else
		{
			cerr << "required version not supported!";
		}

		cout << "Cleaning up winsock... ";

		// Cleanup winsock
		if (WSACleanup()!=0)
		{
			cerr << "cleanup failed!\n";
			iRet = 1;
		}
		cout << "done.\n";
	}
	else
	{
		cerr << "startup failed!\n";
	}
	return iRet;
}

#endif
