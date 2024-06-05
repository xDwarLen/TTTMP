#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 512

// Function to print the Tic Tac Toe board
void printBoard(char board[3][3]) {
    std::cout << "  1 2 3\n";
    for (int i = 0; i < 3; ++i) {
        std::cout << i + 1 << " ";
        for (int j = 0; j < 3; ++j) {
            std::cout << board[i][j] << " ";
        }
        std::cout << "\n";
    }
}

// Function to handle receiving updates from the server
void receiveUpdates(SOCKET connectSocket) {
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    
    while (true) {
        int iResult = recv(connectSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            // Update the local board
            char board[3][3];
            memcpy(board, recvbuf, sizeof(board));
            
            // Print the updated board
            printBoard(board);
        }
    }
}

int main() {
    WSADATA wsaData;
    int iResult;

    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL,
        * ptr = NULL,
        hints;

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

    // Resolve the server address and port
    iResult = getaddrinfo("localhost", DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        std::cout << "getaddrinfo failed: " << iResult << "\n";
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            std::cout << "socket failed: " << WSAGetLastError() << "\n";
            WSACleanup();
            return 1;
        }

        // Connect to server
        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        std::cout << "Unable to connect to server!\n";
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to the server.\n";

    // Start a thread to receive updates from the server
    std::thread updateThread(receiveUpdates, ConnectSocket);

    // Game loop
    char move[2];
    while (true) {
        // Player's move
        int row, col;
        std::cout << "Enter your move (row column): \n";
        std::cin >> row >> col;

        // Send the move to the server
        move[0] = static_cast<char>(row + '0');
        move[1] = static_cast<char>(col + '0');
        iResult = send(ConnectSocket, move, sizeof(move), 0);
        if (iResult == SOCKET_ERROR) {
            std::cout << "send failed: " << WSAGetLastError() << "\n";
            closesocket(ConnectSocket);
            WSACleanup();
            return 1;
        }
    }

    // Join the update thread with the main thread
    updateThread.join();

    // Shutdown the connection
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        std::cout << "shutdown failed: " << WSAGetLastError() << "\n";
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Cleanup
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}
