#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 512

char board[3][3] = { {' ', ' ', ' '}, {' ', ' ', ' '}, {' ', ' ', ' '} };
char currentTurn = 'X'; // X starts the game
bool gameStarted = false;
bool gameEnded = false;

// Function to print the Tic Tac Toe board
void printBoard() {
    std::cout << "  1 2 3\n";
    for (int i = 0; i < 3; ++i) {
        std::cout << i + 1 << " ";
        for (int j = 0; j < 3; ++j) {
            std::cout << board[i][j] << " ";
        }
        std::cout << "\n";
    }
}

// Function to check if someone wins
bool checkWin() {
    // Check rows and columns
    for (int i = 0; i < 3; ++i) {
        if (board[i][0] == board[i][1] && board[i][1] == board[i][2] && board[i][0] != ' ')
            return true;
        if (board[0][i] == board[1][i] && board[1][i] == board[2][i] && board[0][i] != ' ')
            return true;
    }

    // Check diagonals
    if ((board[0][0] == board[1][1] && board[1][1] == board[2][2] && board[0][0] != ' ') ||
        (board[0][2] == board[1][1] && board[1][1] == board[2][0] && board[0][2] != ' '))
        return true;

    return false;
}

// Function to check if the game ends in a draw
bool checkDraw() {
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            if (board[i][j] == ' ')
                return false;
    return true;
}

// Function to handle client communication
// Adjust handleClient signature to accept both client sockets
void handleClient(SOCKET clientSocket, SOCKET ClientSocket1, SOCKET ClientSocket2) {
    int iResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    while (true) {
        // Receive the move from the current player
        iResult = recv(clientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            int row = recvbuf[0] - '0';
            int col = recvbuf[1] - '0';

            // Update the board
            board[row][col] = (clientSocket == ClientSocket1) ? 'X' : 'O';

            // Check for win or draw
            if (checkWin()) {
                // Send win message to both clients
                char winMsg[3] = { 'W', (clientSocket == ClientSocket1) ? 'X' : 'O', '\0' };
                send(ClientSocket1, winMsg, sizeof(winMsg), 0);
                send(ClientSocket2, winMsg, sizeof(winMsg), 0);
                gameEnded = true;
                break;
            } else if (checkDraw()) {
                // Send draw message to both clients
                char drawMsg[2] = { 'D', '\0' };
                send(ClientSocket1, drawMsg, sizeof(drawMsg), 0);
                send(ClientSocket2, drawMsg, sizeof(drawMsg), 0);
                gameEnded = true;
                break;
            }

            // Send the updated board to both clients
            for (SOCKET client : {ClientSocket1, ClientSocket2}) {
                int sendResult = send(client, reinterpret_cast<char*>(board), sizeof(board), 0);
                if (sendResult == SOCKET_ERROR) {
                    std::cout << "send failed: " << WSAGetLastError() << "\n";
                    closesocket(client);
                    return;
                }
            }
        }
    }

    closesocket(clientSocket);
}


int main() {
    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket1 = INVALID_SOCKET;
    SOCKET ClientSocket2 = INVALID_SOCKET;

    struct addrinfo* result = NULL;
    struct addrinfo hints;

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cout << "WSAStartup failed: " << iResult << "\n";
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        std::cout << "getaddrinfo failed: " << iResult << "\n";
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for the server to listen for client connections
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        std::cout << "Error at socket(): " << WSAGetLastError() << "\n";
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Bind the socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        std::cout << "bind failed: " << WSAGetLastError() << "\n";
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    // Listen for client connections
    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        std::cout << "listen failed: " << WSAGetLastError() << "\n";
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Waiting for clients to connect...\n";

    // Accept the first client connection
    ClientSocket1 = accept(ListenSocket, NULL, NULL);
    if (ClientSocket1 == INVALID_SOCKET) {
        std::cout << "accept failed: " << WSAGetLastError() << "\n";
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Player X connected!\n";

    // Accept the second client connection
    ClientSocket2 = accept(ListenSocket, NULL, NULL);
    if (ClientSocket2 == INVALID_SOCKET) {
        std::cout << "accept failed: " << WSAGetLastError() << "\n";
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Player O connected!\n";

    // Set the game as started
    gameStarted = true;

    // Start a thread for each client to handle communication
    std::thread thread1(handleClient, ClientSocket1, ClientSocket1, ClientSocket2);
    std::thread thread2(handleClient, ClientSocket2, ClientSocket1, ClientSocket2);


    // Join the threads with the main thread
    thread1.join();
    thread2.join();

    // Clean up
    closesocket(ClientSocket1);
    closesocket(ClientSocket2);
    closesocket(ListenSocket);
    WSACleanup();

    return 0;
}
