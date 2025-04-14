#include <winsock2.h> 
#include <iostream> 
#include <vector> 
#include <string> 
#include <ctime> 

#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996) 

using namespace std;

#define MAX_CLIENTS 100 
#define DEFAULT_BUFLEN 4096 
#define SERVER_PORT 8888 

struct Message 
{
    string nickname; 
    string content; 
    string timestamp; 
};

SOCKET server_socket; 
vector<Message> message_table; 
vector<sockaddr_in> clients; 

string getCurrentTime() 
{
    time_t now = time(0);
    tm* localtm = localtime(&now);
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", localtm);
    return string(buffer); 
}

void sendToAll(const string& message, const sockaddr_in* exclude = nullptr)
{
    for (const auto& client : clients) 
    { 
        if (exclude && memcmp(&client, exclude, sizeof(sockaddr_in)) == 0) continue;
        sendto(server_socket, message.c_str(), message.size(), 0, (sockaddr*)&client, sizeof(client));
    }
}

void sendTableToClient(const sockaddr_in& client) 
{
    for (const auto& msg : message_table)
    {
        string formatted = "[" + msg.timestamp + "] " + msg.nickname + ": " + msg.content;
        sendto(server_socket, formatted.c_str(), formatted.size(), 0, (sockaddr*)&client, sizeof(client));
    }
}

int main()
{
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa); 

    server_socket = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in server;
    server.sin_family = AF_INET; 
    server.sin_addr.s_addr = INADDR_ANY; 
    server.sin_port = htons(SERVER_PORT); 

    bind(server_socket, (sockaddr*)&server, sizeof(server));

    cout << "UDP server started on port " << SERVER_PORT << endl;

    char buffer[DEFAULT_BUFLEN];
    sockaddr_in client;
    int clientLen = sizeof(client);

    while (true) 
    {
        int bytesReceived = recvfrom(server_socket, buffer, DEFAULT_BUFLEN, 0, (sockaddr*)&client, &clientLen);
        if (bytesReceived <= 0) continue; 

        buffer[bytesReceived] = '\0';
        string message(buffer); 

        if (message.substr(0, 5) == "/join") 
        {
            string nickname = message.substr(6); 
            clients.push_back(client); 

            string joinMessage = "[SERVER]: " + nickname + " joined the chat.";
            Message joinMsg = { "SERVER", nickname + " joined the chat.", getCurrentTime() };
            message_table.push_back(joinMsg);

            sendToAll(joinMessage);

            sendTableToClient(client);
        }
        else if (message.substr(0, 5) == "/exit")
        {
            string nickname = message.substr(6); 
            for (auto it = clients.begin(); it != clients.end(); ++it) 
            {
                if (memcmp(&(*it), &client, sizeof(sockaddr_in)) == 0) 
                {
                    clients.erase(it);
                    break;
                }
            }
            string exitMessage = "[SERVER]: " + nickname + " left the chat.";
            Message exitMsg = { "SERVER", nickname + " left the chat.", getCurrentTime() };
            message_table.push_back(exitMsg);

            sendToAll(exitMessage);
        }
        else
        {
            size_t colonPos = message.find(": ");
            if (colonPos != string::npos) 
            {
                string nickname = message.substr(0, colonPos);
                string content = message.substr(colonPos + 2);
                string timestamp = getCurrentTime(); 

                Message newMsg = { nickname, content, timestamp };
                message_table.push_back(newMsg);

                string formattedMessage = "[" + timestamp + "] " + nickname + ": " + content;
                sendToAll(formattedMessage); 
            }
        }
    }

    WSACleanup();
    return 0;
}