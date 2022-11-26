// Author: Ashley Calton
// Class: CS344
// Date: 11/29/2021

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  // ssize_t
#include <sys/socket.h> // send(),recv()
#include <netdb.h>      // gethostbyname()
#include <sys/stat.h>

/**
* Client code
* 1. Create a socket and connect to the server specified in the command arugments.
* 2. Open encrypted message and key files and read them into buffers.
* 3. Send encrypted message and key to server.
* 4. Receive decoded message from client and print it to stdout.
*/

// Error function used for reporting issues
void error(const char* msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address,
    int portNumber,
    char* hostname) {

    // Clear out the address struct
    memset((char*)address, '\0', sizeof(*address));

    // The address should be network capable
    address->sin_family = AF_INET;
    // Store the port number
    address->sin_port = htons(portNumber);

    // Get the DNS entry for this host name
    struct hostent* hostInfo = gethostbyname("localhost");
    if (hostInfo == NULL) {
        fprintf(stderr, "CLIENT: ERROR, no such host\n");
        exit(0);
    }
    // Copy the first IP address from the DNS entry to sin_addr.s_addr
    memcpy((char*)&address->sin_addr.s_addr,
        hostInfo->h_addr_list[0],
        hostInfo->h_length);
}

long getFileSize(char* filepath) {
    FILE* inputFile = fopen(filepath, "r");
    fseek(inputFile, 0, SEEK_END);  // seek end to get num bytes of file text
    long fileBytes = ftell(inputFile);

    fclose(inputFile);
    return fileBytes;
}

char* readFile(char* filepath, long fileBytes) {
    FILE* inputFile = fopen(filepath, "r");
    char* fileBuffer = malloc(fileBytes + 1);

    int numRead = fread(fileBuffer, 1, fileBytes, inputFile);  // reads fileBytes amount of char from file
    if (numRead < fileBytes) {
        error("ERROR: could not read from file");
    }

    fclose(inputFile);
    return fileBuffer;
}

// Returns int 0-25 for capital char, 26 for space, and int values outside of 0-26 for different char.
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

// Returns 1 if all char are capital letters or spaces. Returns 0 if invalid char found.
int validChar(char str[]) {
    int i = 0;
    while (i < strlen(str)) {
        int charValue = convertChar(str[i]);  // convert char to num
        if (charValue < 0 || charValue > 26) {  // valid char returns 0-26
            return 0;
        }
        i++;
    }
    return 1;
}

//argv[1] = encrypted message file
//argv[2] = file made by keygen
//argv[3] = port
int main(int argc, char* argv[]) {
    int socketFD, portNumber, charsWritten, charsRead;
    struct sockaddr_in serverAddress;
    // Check usage & args
    if (argc < 3) {
        fprintf(stderr, "USAGE: %s hostname port\n", argv[0]);
        exit(0);
    }

    // Create a socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD < 0) {
        error("CLIENT: ERROR opening socket");
    }

    // Set up the server address struct
    setupAddressStruct(&serverAddress, atoi(argv[3]), argv[1]);

    // Connect to server
    if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        error("CLIENT: ERROR connecting");
    }

    // read the two files
    long messageSize = getFileSize(argv[1]);
    long keySize = getFileSize(argv[2]);
    char* message = readFile(argv[1], messageSize);
    message[strcspn(message, "\n")] = '\0';  // remove \n from end
    char* key = readFile(argv[2], keySize);
    key[strcspn(key, "\n")] = '\0';

    // verify key not shorter than message
    if (keySize < messageSize) {
        close(socketFD);
        error("ERROR: key is too short");
    }

    // verify files have proper char
    char* messageCopy = calloc(strlen(message), sizeof(char));
    strcpy(messageCopy, message);
    int messageValid = validChar(messageCopy);
    if (messageValid == 0) {
        error("enc_client error : input contains bad characters");
    }
    char* keyCopy = calloc(strlen(key), sizeof(char));
    strcpy(keyCopy, key);
    int keyValid = validChar(keyCopy);
    if (keyValid == 0) {
        error("enc_client error : key contains bad characters");
    }

    // Send num of message bytes to server (including \0)
    int totalCharsWritten = 0;
    char messageSizeStr[10];
    sprintf(messageSizeStr, "%ld", messageSize);  // str of messageSize
    while (totalCharsWritten < strlen(messageSizeStr)) {
        charsWritten = send(socketFD, messageSizeStr, strlen(messageSizeStr), 0);
        if (charsWritten < 0) {
            error("CLIENT: ERROR sending message size to server");
            break;
        }
        totalCharsWritten += charsWritten;
    }

    // Get confirmation message from server
    // Handshake from dec_server
    char returnMessage[256];
    charsRead = recv(socketFD, returnMessage, sizeof(returnMessage) - 1, 0);
    if (charsRead < 0) {
        error("CLIENT: ERROR reading from socket");
    }
    if (returnMessage[0] != 'd') {
        close(socketFD);
        fprintf(stderr, "CLIENT: Attempted connection to invalid server on port %d\n", atoi(argv[3]));
        exit(2);
    }
    //printf("CLIENT: I received this from the server: \"%s\"\n", returnMessage);

    // Send num of key bytes to server
    totalCharsWritten = 0;
    char keySizeStr[10];
    sprintf(keySizeStr, "%ld", keySize);  // str of keySize
    while (totalCharsWritten < strlen(keySizeStr)) {
        charsWritten = send(socketFD, keySizeStr, strlen(keySizeStr), 0);
        if (charsWritten < 0) {
            error("CLIENT: ERROR sending key size to server");
            break;
        }
        totalCharsWritten += charsWritten;
    }

    // Get confirmation message from server
    char returnKey[256];
    charsRead = recv(socketFD, returnKey, sizeof(returnKey) - 1, 0);
    if (charsRead < 0) {
        error("CLIENT: ERROR reading from socket");
    }
    //printf("CLIENT: I received this from the server: \"%s\"\n", returnKey);

    // Send message to server
    // Write to the server - 1000 bytes at a time
    totalCharsWritten = 0;
    while (totalCharsWritten < strlen(message)) {  // send until entire message sent
        charsWritten = send(socketFD, message, strlen(message), 0);
        if (charsWritten < 0) {
            error("CLIENT: ERROR sending message to server");
            break;
        }
        totalCharsWritten += charsWritten;
    }

    // Get confirmation message from server
    char gotMessage[256];
    charsRead = recv(socketFD, gotMessage, sizeof(gotMessage) - 1, 0);
    if (charsRead < 0) {
        error("CLIENT: ERROR reading from socket");
    }
    //printf("CLIENT: I received this from the server: \"%s\"\n", gotMessage);

    // Send key to server
    totalCharsWritten = 0;
    while (totalCharsWritten < strlen(key)) {  // send until entire key sent
        charsWritten = send(socketFD, key, strlen(key), 0);
        if (charsWritten < 0) {
            error("CLIENT: ERROR sending key to server");
            break;
        }
        totalCharsWritten += charsWritten;
    }

    // Get confirmation message from server
    char gotKey[256];
    charsRead = recv(socketFD, gotKey, sizeof(gotKey) - 1, 0);
    if (charsRead < 0) {
        error("CLIENT: ERROR reading from socket");
    }
    //printf("CLIENT: I received this from the server: \"%s\"\n", gotKey);

    char decMessage[strlen(message)];
    int totalCharsRead = 0;
    while (totalCharsRead < strlen(message)) {
        charsRead = recv(socketFD, decMessage, sizeof(decMessage), 0);
        if (charsRead < 0) {
            error("CLIENT: ERROR receiving decrypted message from server");
            break;
        }
        totalCharsRead += charsRead;
    }
    printf("%s\n", decMessage);
    fflush(stdout);    

    // Close the socket
    close(socketFD);
    return 0;
}
