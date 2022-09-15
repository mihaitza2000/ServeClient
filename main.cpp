#include <winsock2.h>//for socket()
#include <iostream>
#include <WS2tcpip.h>//for socketlen_t
#include <io.h>//for read()
#include <string>
#pragma comment(lib, "ws2_32.lib")

struct client_message
{
	char start[3] = "//";
	uint64_t count[8];
	char msg_id[4] = "CST";
	byte client_id[1];
	uint64_t  request_id[8];
	char timestampp[30] = "2022-06-29T10:56:10.111+00:00";
	byte mode[1];
	int8_t temperature[1];
	byte sensor_state[1];
	char end[3] = "##";
};

struct server_message
{
	char start[2];
	uint64_t count;
	char msg_id[4];
	byte client_id;
	uint64_t request_id;
	char end[2];
};

client_message* s_data_client;
server_message* s_data_server;

int main()
{
	//initialize winsock
	std::string ipAddress = "127.0.0.1";
	WSADATA wsaData;
	WORD ver = MAKEWORD(2, 2);
	int wsOk = WSAStartup(ver, &wsaData);
	if (wsOk != 0)
	{
		std::cout << "Can't initialize winsock";
		return 1;
	}

	//create socket
	SOCKET client_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (client_sock == INVALID_SOCKET)
	{
		std::cerr << "Can't create socket, Err #" << WSAGetLastError << std::endl;
		return 1;
	}

	//bind ip address and port to socket
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(54000);
	inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

	//connecting
	int connResult = connect(client_sock, (sockaddr*)&hint, sizeof(hint));
	if (connResult == SOCKET_ERROR)
	{
		std::cerr << "Can't connect to server,  Err #" << WSAGetLastError << std::endl;
		closesocket(client_sock);
		WSACleanup();
		return 1;
	}

	//communicating
	char buffer[100];
	server_message* recieve_buffer;
	std::string user_input;
	while (true)
	{
		std::cout << "> ";
		std::getline(std::cin, user_input);
		if (user_input.size() > 0)
		{
			int sendResult = send(client_sock, user_input.c_str(), user_input.size() + 1, 0);
			if (sendResult != SOCKET_ERROR)
			{
				ZeroMemory(buffer, 100);
				int bytesRecieved = recv(client_sock, buffer, 100, 0);
				if (bytesRecieved > 0)
				{
					std::cout << "SERVER > " << std::string(buffer, 0, bytesRecieved) << std::endl;
				}
				recieve_buffer = (server_message*)buffer;
				s_data_server->start[0] = recieve_buffer->start[0];
				s_data_server->start[1] = recieve_buffer->start[1];
			}
		}
	}

	//close client socket
	closesocket(client_sock);

	//cleanup winsock
	WSACleanup();

	return 0;
}