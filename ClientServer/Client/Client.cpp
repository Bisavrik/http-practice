#define WIN32_LEAN_AND_MEAN 

#include <ws2tcpip.h> 
#include <windows.h> 
#include <iostream> 
#include <string> 

#pragma comment(lib, "Ws2_32.lib") 

using namespace std;

#define SERVER_IP "127.0.0.1" 
#define SERVER_PORT 8888 
#define DEFAULT_BUFLEN 4096 

SOCKET client_socket;
string nickname; 

void setColor(int colorCode) 
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), colorCode);
}

DWORD WINAPI Receiver(void* param) 
{
    char buffer[DEFAULT_BUFLEN]; 

    while (true) 
    {
        sockaddr_in server;
        int serverLen = sizeof(server);
        int bytesReceived = recvfrom(client_socket, buffer, DEFAULT_BUFLEN, 0, (sockaddr*)&server, &serverLen);
        if (bytesReceived <= 0) continue; 

        buffer[bytesReceived] = '\0';
        setColor(7);
        cout << buffer << endl; 
    }
    return 0;
}

BOOL ExitHandler(DWORD ctrlType)
{
    if (ctrlType == CTRL_CLOSE_EVENT || ctrlType == CTRL_C_EVENT) 
    {
        string exitCommand = "/exit " + nickname;
        sockaddr_in server;
        server.sin_family = AF_INET;
        inet_pton(AF_INET, SERVER_IP, &server.sin_addr);
        server.sin_port = htons(SERVER_PORT);
        sendto(client_socket, exitCommand.c_str(), exitCommand.size(), 0, (sockaddr*)&server, sizeof(server));
        return TRUE; 
    }
    return FALSE;
}

int main() 
{
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)ExitHandler, TRUE);

    cout << "Enter nickname: ";
    getline(cin, nickname);

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa); 

    client_socket = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in server;
    server.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &server.sin_addr); 
    server.sin_port = htons(SERVER_PORT);

    string joinMessage = "/join " + nickname;
    sendto(client_socket, joinMessage.c_str(), joinMessage.size(), 0, (sockaddr*)&server, sizeof(server));

    CreateThread(0, 0, Receiver, 0, 0, 0);

    while (true)
    {
        string message;
        getline(cin, message); 

        if (message == "/exit") 
        {
            string exitCommand = "/exit " + nickname;
            sendto(client_socket, exitCommand.c_str(), exitCommand.size(), 0, (sockaddr*)&server, sizeof(server));
            break; 
        }

        if (!message.empty())
        {
            string fullMessage = nickname + ": " + message;
            sendto(client_socket, fullMessage.c_str(), fullMessage.size(), 0, (sockaddr*)&server, sizeof(server));
        }
    }

    closesocket(client_socket);
    WSACleanup();
    return 0;
}