#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <limits>
#include <string>
#include <thread>

using namespace std;

#define BUFFER_SIZE 200

bool game_over = false;


void receiveData(SOCKET sock, char* buffer, size_t bufferSize)
{
    int valread = recv(sock, buffer, bufferSize, 0);
    if (valread == SOCKET_ERROR)
    {
        std::cerr << "Recv error: " << WSAGetLastError() << std::endl;
    }
    else
    {
        buffer[valread] = '\0';
    }
}

void sendData(SOCKET sock, const char* message)
{
    if (send(sock, message, strlen(message), 0) == SOCKET_ERROR)
    {
        std::cerr << "Send error: " << WSAGetLastError() << std::endl;
    }
}

void ReceiveUpdates(SOCKET socket)
{
    char buffer[BUFFER_SIZE];
    while (!game_over)
    {
        receiveData(socket, buffer, BUFFER_SIZE);
        std::cout << buffer;

        std::string message(buffer);
        if (message == "Game Ended.") {
            game_over = true;
            std::cout << "Game ended. Disconnecting from the server.\n";
            closesocket(socket);
            break;
        }
    }
}

void PlayGame(SOCKET socket){
    std::thread receiver(ReceiveUpdates, socket);
    while(!game_over){
        std::string buffer;
        std::getline(std::cin, buffer);
        sendData(socket, buffer.c_str());
    }
    receiver.join();
}

void ClientStart(const char* serverIp, int port)
{
//void runClient(const char* serverIp, int port)
    WSAData wsaData;
    int wsaerr;
    WORD wVersionRequested = MAKEWORD(2, 2);
    wsaerr = WSAStartup(wVersionRequested, &wsaData);

    if (wsaerr != 0)
    {
        std::cout << "The Winsock dll not found!" << std::endl;
        return;
    }
    else
    {
        std::cout << "The Winsock dll found" << std::endl;
        std::cout << "The status: " << wsaData.szSystemStatus << std::endl;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET)
    {
        std::cout << "Error at socket(): " << WSAGetLastError() << std::endl;
        WSACleanup();
        return;
    }
    else
    {
        std::cout << "Socket is OK!" << std::endl;
    }

    sockaddr_in clientService;
    clientService.sin_family = AF_INET;
    clientService.sin_addr.s_addr = inet_addr(serverIp);
    clientService.sin_port = htons(port);

    if (connect(clientSocket, reinterpret_cast<SOCKADDR*>(&clientService), sizeof(clientService)) == SOCKET_ERROR)
    {
        std::cout << "Client: connect() - Failed to connect: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return;
    }
    else
    {
        std::cout << "Client: Connect() is OK!" << std::endl;
        std::cout << "Client: Can start sending and receiving data..." << std::endl;
    }
    PlayGame(clientSocket);
}



int main()
{
    while(true)
    {
        const char* serverIp = "127.0.0.1";
        int port = 55555;
        cout << "Selamat datang di alfamaret, selamat belanja" << endl;
        cout << "Berikut adalah perintah yang bisa anda lakukan ! " << endl;
        cout << "1. Sambung ke Server" << endl;
        cout << "2. Keluar"<< endl;
        cout << "Silahkan masukkan perintah yang ingin anda lakukan : ";
        int clientConsoleCommand;
        cin >> clientConsoleCommand;
        cout << endl;
        switch(clientConsoleCommand)
        {
        case 1:
            cout << "Terasmbung ke server dengan IP 127.0.0.1 dan Port 55555" << endl;
            ClientStart(serverIp, port);
            break;
        case 2:
            cout << "Terima kasih sudah berbelanja di alfamaret" << endl;
            return 0;
            break;
        default:
            cout << "Invalid Input " << endl;
        }
    }
}
