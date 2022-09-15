#include <winsock2.h>//for socket()
#include <iostream>
#include <WS2tcpip.h>//for socketlen_t
#include <io.h>//for read()
#include <string.h>
#include <cstdint>
#include <sys/timeb.h>
#include <vector>
#include <cstdint>
#pragma comment(lib, "ws2_32.lib")

typedef struct sServerMessage
{
    char start[3];
    char msg_id[5];
    char end[3];
}sServerMessage;

typedef struct sClientMessage
{
    char start[3];
    //uint64_t count;
    char msg_id[4];
    //byte client_id;
    //uint64_t request_id;
    //char timestamp[30];
    /*
    byte aisle;
    byte level;
    byte mode;
    byte robot_state;
    byte load_state;
    int power_time;
    int operation_time;
    */
    char end[3];
}sClientMessage;

struct sDataHandlers
{
    sServerMessage sent_message;
    sClientMessage received_message;

    BYTE		data_buf[8000] = { 0xFF };
};

sDataHandlers comm;

void create_internal_value_server_request()
{
    strcpy_s(comm.sent_message.start, 3, "//");
    strcpy_s(comm.sent_message.msg_id, 5, "SVRQ");
    strcpy_s(comm.sent_message.end, 3, "##");
}

int main()
{
    //initialize winsock

    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor 

    u_long mode = 1;
    WSADATA wsaData;
    WORD ver = MAKEWORD(2, 2);
    int bytesRecieved;
    int wsOk = WSAStartup(ver, &wsaData);
    if (wsOk != 0)
    {
        std::cout << "Can't initialize winsock";
        return 1;
    }

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

    //create socket
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET)
    {
        std::cerr << "Can't create socket, Err #" << WSAGetLastError << std::endl;
        return 1;
    }

    //bind ip address and port to socket
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.S_un.S_addr = INADDR_ANY;
    address.sin_port = htons(54000);

    bind(server_socket, (struct sockaddr*)&address, sizeof(address));

    //start listening
    listen(server_socket, 2);

    // add the listener to the master set
    FD_SET(server_socket, &master);

    // keep track of the biggest file descriptor
    fdmax = server_socket; // so far, it's this one

    //communicating
    create_internal_value_server_request();
    sServerMessage* s_buffer; //create buffer of ServerMessage type
    s_buffer = (sServerMessage*)(&comm.data_buf); //initialize it

    //write the values from the interval variables into the buffer
    s_buffer->start[0] = comm.sent_message.start[0];
    s_buffer->start[1] = comm.sent_message.start[1];
    s_buffer->msg_id[0] = comm.sent_message.msg_id[0];
    s_buffer->msg_id[1] = comm.sent_message.msg_id[1];
    s_buffer->msg_id[2] = comm.sent_message.msg_id[2];
    s_buffer->msg_id[3] = comm.sent_message.msg_id[3];
    s_buffer->end[0] = comm.sent_message.end[0];
    s_buffer->end[1] = comm.sent_message.end[1];

    std::cout << "Server listening...\n";

    //connecting
    char buffer[sizeof(sClientMessage)];
    SOCKET client_sock;
    sClientMessage* c_buffer;
    while (true)
    {
        read_fds = master; // copy it

        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for (int i = 0; i <= fdmax; i++)
        {
            if (FD_ISSET(i, &read_fds))
            { // we got one!!
                if (i == server_socket)
                {
                    // handle new connections
                    sockaddr_in client;
                    int client_size = sizeof(client);
                    client_sock = accept(server_socket, (struct sockaddr*)&client, &client_size);

                    if (client_sock == -1)
                    {
                        perror("accept");
                    }
                    else
                    {
                        FD_SET(client_sock, &master); // add to master set
                        if (client_sock > fdmax)
                        {    // keep track of the max
                            fdmax = client_sock;
                        }
                        std::cout << "selectserver: new connection" << std::endl;
                    }
                    //send request message to client
                    if (send(client_sock, (char*)s_buffer, sizeof(sServerMessage), 0) == -1)
                    {
                        perror("send");
                    }
                }
                else
                {
                    // handle data from a client
                    if ((bytesRecieved = recv(i, buffer, sizeof(buffer), 0)) <= 0)
                    {
                        // got error or connection closed by client
                        if (bytesRecieved == 0)
                        {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);
                            closesocket(i);
                        }
                        else
                        {
                            perror("recv");
                        }
                        FD_CLR(i, &master); // remove from master set
                    }
                    else
                    {
                        if (send(i, buffer, bytesRecieved + 1, 0) == -1)
                        {
                            perror("error in sending...");
                        }
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    }

    //close server socket for listening
    closesocket(server_socket);

    //cleanup winsock
    WSACleanup();

    return 0;
}