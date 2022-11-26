// Author: Ashley Calton
// Class: CS344
// Date: 11/29/2021

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

/**
* Server code
* 1. Create a socket and listen for a connection to a client.
* 2. Upon connection, read a message and key from the client.
* 3. Use the key to encrypt the message,
* 4. Send encrypted message back to client.
*/

// Error function used for reporting issues
void error(const char* msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in* address,
    int portNumber) {

    // Clear out the address struct
    memset((char*)address, '\0', sizeof(*address));

    // The address should be network capable
    address->sin_family = AF_INET;
    // Store the port number
    address->sin_port = htons(portNumber);
    // Allow a client at any address to connect to this server
    address->sin_addr.s_addr = INADDR_ANY;
}

// Given a char, returns the integer value 0-25 corresponding to alphabetical order. Returns 26 for a space.
int convertChar(char currChar) {
    int returnInt;
    if (currChar == ' ') {
        returnInt = 26;
    }
    else {
        returnInt = currChar - 'A';
    }
    return returnInt;
}

// Given an int, returns the capital character corresponding to 0-25. Returns space for 26.
char convertInt(int currInt) {
    char returnChar;
    if (currInt == 26) {
        returnChar = ' ';
    }
    else {
        returnChar = currInt + 'A';
    }
    return returnChar;
}

// Encode a given message using a set of random letters created by keygen.c
char* encryptMessage(char* message, char* key) {
    // convert message to array of num
    int intMsgBuffer[strlen(message)];
    int i = 0;
    while (i < strlen(message)) {
        intMsgBuffer[i] = convertChar(message[i]);
        i++;
    }

    // convert key to array of num
    int intKeyBuffer[strlen(key)];
    i = 0;
    while (i < strlen(key)) {
        intKeyBuffer[i] = convertChar(key[i]);
        i++;
    }
    
    // add message nums and key nums together
    int intEncBuffer[strlen(message)];
    int addedNums;
    i = 0;
    while (i < strlen(message)) {
        addedNums = intMsgBuffer[i] + intKeyBuffer[i];
        if (addedNums > 26) {  // subtract 27 to keep in range of allowed char
            addedNums -= 27;
        }
        intEncBuffer[i] = addedNums;
        i++;
    }

    // convert added nums to chars
    char* encMessage = malloc(strlen(message));
    i = 0;
    while (i < strlen(message)) {
        encMessage[i] = convertInt(intEncBuffer[i]);
        i++;
    }

    // return encrypted message
    return encMessage;
}

int main(int argc, char* argv[]) {
    int connectionSocket, charsRead;
    char buffer[256];
    char messageSize[10];
    char keySize[10];
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t sizeOfClientInfo = sizeof(clientAddress);

    // Check usage & args
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s port\n", argv[0]);
        exit(1);
    }

    // Create the socket that will listen for connections
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0) {
        error("ERROR opening socket");
    }

    // Set up the address struct for the server socket
    setupAddressStruct(&serverAddress, atoi(argv[1]));

    // Associate the socket to the port
    if (bind(listenSocket,
        (struct sockaddr*)&serverAddress,
        sizeof(serverAddress)) < 0) {
        error("ERROR on binding");
    }

    // Start listening for connections. Allow up to 5 connections to queue up
    listen(listenSocket, 5);

    // Accept a connection, blocking if one is not available until one connects
    while (1) {
        // Accept the connection request which creates a connection socket
        connectionSocket = accept(listenSocket,
            (struct sockaddr*)&clientAddress,
            &sizeOfClientInfo);
        if (connectionSocket < 0) {
            error("ERROR on accept");
        }

        pid_t spawnPid = fork();

        // forking format from Exploration: Process API - Executing a New Program
        switch (spawnPid) {
        case -1:
            error("fork()");
            close(connectionSocket);  // close failed client
            break;

        case 0: {
            // Get the message size from client
            memset(messageSize, '\0', 10);
            charsRead = recv(connectionSocket, messageSize, 10, 0);
            if (charsRead < 0) {
                error("ERROR reading from socket");
            }
            //printf("SERVER: Message size I got is: \"%s\"\n", messageSize);

            // Send a Success message back to the client
            // handshake to enc_client
            charsRead = send(connectionSocket,
                "e", 2, 0);
            if (charsRead < 0) {
                error("ERROR writing to socket");
            }

            // Get the key size from client
            memset(keySize, '\0', 10);
            charsRead = recv(connectionSocket, keySize, 10, 0);
            if (charsRead < 0) {
                error("ERROR reading from socket");
            }
            //printf("SERVER: Key size I got is: \"%s\"\n", keySize);

            // Send a Success message back to the client
            charsRead = send(connectionSocket,
                "I am the server, and I got the key size", 44, 0);
            if (charsRead < 0) {
                error("ERROR writing to socket");
            }

            // Get the message from the client and display it
            int messageSizeInt = atoi(messageSize);
            char message[messageSizeInt];

            // Read the client's message from the socket
            int totalCharsRead = 0;
            while (totalCharsRead < messageSizeInt - 1) {
                charsRead = recv(connectionSocket, message, messageSizeInt - 1, 0);
                if (charsRead < 0) {
                    error("ERROR reading from socket");
                    break;
                }
                totalCharsRead += charsRead;
            }
            //printf("SERVER: I received this from the client: \"%s\"\n", message);

            // Send a Success message back to the client
            charsRead = send(connectionSocket,
                "I am the server, and I got your message", 39, 0);
            if (charsRead < 0) {
                error("ERROR writing to socket");
            }

            // Get the key from the client and display it
            int keySizeInt = atoi(keySize);
            char key[keySizeInt];

            // Read the client's key from the socket
            totalCharsRead = 0;
            while (totalCharsRead < keySizeInt - 1) {
                charsRead = recv(connectionSocket, key, keySizeInt - 1, 0);
                if (charsRead < 0) {
                    error("ERROR reading from socket");
                    break;
                }
                totalCharsRead += charsRead;
            }
            //printf("SERVER: I received this from the client: \"%s\"\n", key);

            // Send a Success message back to the client
            charsRead = send(connectionSocket,
                "I am the server, and I got your key", 39, 0);
            if (charsRead < 0) {
                error("ERROR writing to socket");
            }

            char* encryptedMessage = encryptMessage(message, key);
            // Send encrypted message back to the client
            int totalCharsWritten = 0;
            int charsWritten;
            while (totalCharsWritten < strlen(encryptedMessage)) {
                charsWritten = send(connectionSocket, encryptedMessage, strlen(encryptedMessage), 0);
                if (charsWritten < 0) {
                    error("SERVER: ERROR sending encrypted message to client");
                    break;
                }
                totalCharsWritten += charsWritten;
            }

            // Close the connection socket for this client
            close(connectionSocket);
            break;
        }
        default:
            close(connectionSocket);
            break;
        }

    }
    // Close the listening socket
    close(listenSocket);
    return 0;
}
