#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#pragma comment(lib, "Ws2_32.lib")

using namespace std;

SOCKET serverSocket = INVALID_SOCKET;
SOCKET acceptSocket = INVALID_SOCKET;

SOCKET player1Socket;
SOCKET player2Socket;

char board[3][3] = {{' ', ' ', ' '}, {' ', ' ', ' '}, {' ', ' ', ' '}};
bool gameover = false;
int playerTurn = 0;
int turn = 1;
bool player1Notifiy;
bool player2Notifiy;


void receiveData(SOCKET client_socket, char* buffer, size_t bufferSize)
{
    int valread = recv(client_socket, buffer, bufferSize, 0);
    if (valread == SOCKET_ERROR)
    {
        std::cerr << "Recv error: " << WSAGetLastError() << std::endl;
    }
    else
    {
        buffer[valread] = '\0';
        cout << "Received data are = " << buffer << std::endl;
    }
}

void sendData(SOCKET client_socket, const char* message)
{
    if (send(client_socket, message, strlen(message), 0) == SOCKET_ERROR)
    {
        std::cerr << "Send error: " << WSAGetLastError() << std::endl;
    }
    else
    {
        //cout << "Data sent to the client " << endl;
    }
}

void displayBoard(SOCKET client_socket)
{
    std::string boardState = "Current Board:\n";
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            boardState += board[i][j];
            if (j < 2)
            {
                boardState += " | ";
            }
        }
        boardState += "\n";
        if (i < 2)
        {
            boardState += "--+---+--\n";
        }
    }
    sendData(client_socket, boardState.c_str());
}
bool checkWin(char player)
{
    // Check Horizontal vertical
    for (int i = 0; i < 3; ++i)
    {
        if ((board[i][0] == player && board[i][1] == player && board[i][2] == player) ||
                (board[0][i] == player && board[1][i] == player && board[2][i] == player))
        {
            return true;
        }
    }
    // Check diagonals
    if ((board[0][0] == player && board[1][1] == player && board[2][2] == player) ||
            (board[0][2] == player && board[1][1] == player && board[2][0] == player))
    {
        return true;
    }
    return false;
}



bool checkDraw()
{
    return !checkWin('X') && !checkWin('O') && turn == 9;
}

void UpdateBothBoard(SOCKET socket1, SOCKET socket2 ){
    displayBoard(socket1);
    displayBoard(socket2);
}

void HandleClient(SOCKET socket,int playerID)
{
    char buffer[200];
    const char* request = "Your turn.";
    SOCKET oppositeSocket;
    if(playerID == 1)
    {
        oppositeSocket = player2Socket;
    }
    else if (playerID == 2)
    {
        oppositeSocket = player1Socket;
    }
    while(!gameover)
    {
        if (playerTurn == playerID)
        {
            if(playerID == 1 && player1Notifiy == true)
            {
                player1Notifiy = false;
            }
            else if(playerID == 2 && player2Notifiy == true)
            {
                player2Notifiy = false;
            }
//            displayBoard(socket);
            UpdateBothBoard(player1Socket, player2Socket);
            sendData(socket, "Giliran anda. Masukkan baris yang ingin di isi(0, 1, 2): ");
            receiveData(socket, buffer, 200);
            int row = atoi(buffer);

            sendData(socket, "Masukkan Kolom yang ingin di isi (0, 1, 2): ");
            receiveData(socket, buffer, 200);
            int col = atoi(buffer);

            // Validate client input / Validate Client Move
            if (row < 0 || row >= 3 || col < 0 || col >= 3 || board[row][col] != ' ')
            {
                sendData(socket, "Invalid move. Try again.\n");
                continue;
            }
            board[row][col] = (playerID == 1) ? 'X' : 'O';
            ++turn;
            if (checkWin((playerID == 1) ? 'X' : 'O'))
            {
                cout << "Inside check win " << endl;
                displayBoard(socket);
                if(playerID == 1)
                {
                    displayBoard(oppositeSocket);
                    sendData(oppositeSocket, "Player 1 Win the game \n");
                    sendData(oppositeSocket, "Game Ended.");
                    sendData(socket, "Game Ended.");

                }
                else if(playerID == 2)
                {
                    displayBoard(oppositeSocket);
                    sendData(oppositeSocket, "Player 2 Win The game \n");
                    sendData(oppositeSocket, "Game Ended.");
                    sendData(socket, "Game Ended.");
                }
                gameover = true;
                continue;
            }
            if (checkDraw())
            {
                displayBoard(socket);
                sendData(socket, "It's a draw!\n");
                sendData(oppositeSocket, "It's a draw!\n");
                sendData(socket, "Game Ended.");
                sendData(oppositeSocket, "Game Ended.");
                gameover = true;
                continue;
            }
            playerTurn = (playerID == 1) ? 2 : 1; // Switch turn

        }
        else
        {
            if(playerID == 1 && player1Notifiy == false)
            {
                sendData(socket,"Its Opposite player turn ... \n");
                player1Notifiy = true;
            }
            else if(playerID == 2 && player2Notifiy == false)
            {
                sendData(socket,"Its Opposite player turn ... \n");
                player2Notifiy = true;
            }
        }
    }
}

SOCKET AcceptConnection(SOCKET socket)
{
    SOCKET newSocket = accept(socket, NULL, NULL);
    if (newSocket == INVALID_SOCKET)
    {
        std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
        exit(EXIT_FAILURE);
    }
    return newSocket;
}


bool ServerStart(const char* ipAddress, int port)
{
    WSAData wsaData;
    int wsaerr;
    WORD wVersionRequested = MAKEWORD(2,2);
    wsaerr = WSAStartup(wVersionRequested, &wsaData);
    // Check for initialization success
    if (wsaerr != 0)
    {
        std::cout << "The Winsock dll not found!" << std::endl;
        return 0;
    }
    else
    {
        std::cout << "The Winsock dll found" << std::endl;
        std::cout << "The status: " << wsaData.szSystemStatus << std::endl;
    }
//    return 0;
    // Create a socket
//    SOCKET serverSocket;
    serverSocket = INVALID_SOCKET;
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // Check for socket creation success
    if (serverSocket == INVALID_SOCKET)
    {
        std::cout << "Error at socket(): " << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
    }
    else
    {
        std::cout << "Socket is OK!" << std::endl;
    }
    // Bind the socket to an IP address and port number
    sockaddr_in service;
    service.sin_family = AF_INET;
//    service.sin_addr.s_addr = inet_addr("127.0.0.1");  // Replace with your desired IP address
    service.sin_addr.s_addr = inet_addr(ipAddress);
    service.sin_port = htons(port);  // Choose a port number

    // Use the bind function
    if (bind(serverSocket, reinterpret_cast<SOCKADDR*>(&service), sizeof(service)) == SOCKET_ERROR)
    {
        std::cout << "bind() failed: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return false;
    }
    else
    {
        std::cout << "bind() is OK!" << std::endl;
    }
    // Listen for incoming connections
    if (listen(serverSocket, 1) == SOCKET_ERROR)
    {
        std::cout << "listen(): Error listening on socket: " << WSAGetLastError() << std::endl;
//        closesocket(serverSocket);
//        WSACleanup();
//        return false;
    }
    else
    {
        std::cout << "listen() is OK! I'm waiting for new connections..." << std::endl;
    }

    player1Socket = AcceptConnection(serverSocket);
    cout << "First Player Connected , Waiting for Second player ..." << endl;
    sendData(player1Socket, "Anda Terdaftar Sebagai Player 1 \n");

    player2Socket = AcceptConnection(serverSocket);
    cout << "Second Player Connected, Game Starting..." << endl;
    sendData(player2Socket, "Anda Terdaftar Sebagai Player 2 \n");
    playerTurn = 1;

    std::thread t1(HandleClient, player1Socket, 1);
    std::thread t2(HandleClient, player2Socket, 2);

    t1.join();
    t2.join();

}


void ServerStop()
{
    if(acceptSocket != INVALID_SOCKET)
    {
        shutdown(acceptSocket, SD_BOTH);
        closesocket(acceptSocket);
    }
    if(serverSocket != INVALID_SOCKET)
    {
        closesocket(serverSocket);
    }
    WSACleanup();
    cout << "Server Stopped and cleaned up." << endl;
}


int main()
{
    const char* ipAddress = "127.0.0.1";
    int port = 55555;

    while(true)
    {
        cout << "Selamat datang di Indomaret, selamat belanja" << endl;
        cout << "Berikut adalah perintah yang bisa anda lakukan ! " << endl;
        cout << "1. Mulai Server" << endl;
        cout << "2. Stop Server"<< endl;
        cout << "3. Keluar"<< endl;
        cout << "Silahkan masukkan perintah yang ingin anda lakukan : ";
        int serverConsoleCommand;
        cin >> serverConsoleCommand;

        switch (serverConsoleCommand)
        {
        case 1:
            cout << "Server dimulai dengan IP 127.0.0.1 dan port 55555" << endl;
            ServerStart(ipAddress, port);
            break;
        case 2:
            cout << "Anda memilih Perintah nomor 2" << endl;
            break;
        case 3:
            cout << "Anda memilih perintah nomor 3" << endl;
            cout << "Terima kasih sudah berbelanja di indomaret" << endl;
            return 0;
            break;
        default:
            cout << "anda memasukkan input yang tidak ada di dalam list!" << endl;
        }

    }
}
