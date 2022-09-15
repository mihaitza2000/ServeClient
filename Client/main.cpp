#include <winsock2.h>//for socket()
#include <iostream>
#include <WS2tcpip.h>//for socketlen_t
#include <io.h>//for read()
#include <string>
#include <time.h>
#include <sys/timeb.h>
#include <Windows.h>
#pragma comment(lib, "ws2_32.lib")
#define DELAY_TIME 2


typedef struct sServerMessage
{
	char start[3];
	char msg_id[5];
	uint64_t request_id;
	char end[3];
}sServerMessage;

typedef struct sData
{
	byte aisle;
	byte level;
	byte mode;
	byte robot_state;
	byte load_state;
	int power_time;
	int operation_time;
}sData;

typedef struct sClientMessage
{
	char start[3];
	uint64_t count;
	char msg_id[4];
	uint64_t request_id;
	char timestamp[30];
	sData data[sizeof(sData)];
	char end[3];
}sClientMessage;

struct sDataHandlers
{
	sClientMessage sent_message;
	sServerMessage recieved_message;

	BYTE		data_buf[8000] = { 0xFF };
};

sDataHandlers comm;

void create_internal_value_client_request()
{
	strcpy_s(comm.sent_message.start, 3, "//");
	strcpy_s(comm.sent_message.msg_id, 4, "CST");
	strcpy_s(comm.sent_message.end, 3, "##");
}

std::string correct(int value)
{
	if (value / 10 == 0)
	{
		return "0" + std::to_string(value);
	}
	return std::to_string(value);
}

std::string getSign(struct tm local_time, struct tm gmt_time)
{
	if (local_time.tm_hour >= gmt_time.tm_hour-10)
	{
		if (local_time.tm_min >= gmt_time.tm_min)
		{
			return "+";
		}
		else
		{
			return "-";
		}
	}
	else
	{
		return "-";
	}
}

void getTime(std::string &time_stamp)
{
	//clear old timestamp
	time_stamp = "";

	//to get time in ms
	SYSTEMTIME lt;
	GetLocalTime(&lt);
	time_t now = time(NULL);
	struct tm local_time, gmt_time;
	localtime_s(&local_time, &now);
	gmtime_s(&gmt_time, &now);

	time_stamp += std::to_string(local_time.tm_year + 1900) + "-" +
	correct(local_time.tm_mon + 1) + "-" +
	correct(local_time.tm_mday) + "T" +
	correct(local_time.tm_hour) + ":" +
	correct(local_time.tm_min) + ":" +
	correct(local_time.tm_sec) + ":" +
	std::to_string(lt.wMilliseconds) +
	getSign(local_time, gmt_time) +
	correct(local_time.tm_hour - gmt_time.tm_hour + 10) + ":" +
	correct(local_time.tm_min - gmt_time.tm_min);
}

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
	sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = htons(54000);
	inet_pton(AF_INET, ipAddress.c_str(), &address.sin_addr);
	//address.sin_addr.S_un.S_addr = INADDR_ANY;

	//connecting
	int connResult = connect(client_sock, (sockaddr*)&address, sizeof(address));
	if (connResult == SOCKET_ERROR)
	{
		std::cerr << "Can't connect to server,  Err #" << WSAGetLastError << std::endl;
		closesocket(client_sock);
		WSACleanup();
		return 1;
	}
	std::string time_stamp;
	//communicating
	create_internal_value_client_request();
	sClientMessage* c_buffer; //create buffer of ServerMessage type
	c_buffer = (sClientMessage*)(&comm.data_buf); //initialize it

	//write the values from the interval variables into the buffer
	strcpy_s(c_buffer->start, comm.sent_message.start);
	strcpy_s(c_buffer->msg_id, comm.sent_message.msg_id);
	c_buffer->count = comm.sent_message.count;
	strcpy_s(c_buffer->end, comm.sent_message.end);

	char buffer[sizeof(sServerMessage)];
	recv(client_sock, buffer, sizeof(sServerMessage), 0);

	sServerMessage* s_buffer = (sServerMessage*)&buffer;

	if (strncmp(s_buffer->start, "//", 2) != 0 || strncmp(s_buffer->msg_id, "SVRQ", 4) != 0 || strncmp(s_buffer->end, "##", 2) != 0)
	{
		std::cout << "Wrong server !";
		closesocket(client_sock);
	}
	else
	{ 
		c_buffer->request_id = s_buffer->request_id;
		SYSTEMTIME lt;
		GetLocalTime(&lt);
		int start_clock = lt.wSecond, stop_clock;
		while (true)
		{
			getTime(time_stamp);
			strcpy_s(c_buffer->timestamp, time_stamp.c_str());
			c_buffer->data->aisle = rand() % 2 + 1;
			c_buffer->data->level = rand() % 12 + 1;
			c_buffer->data->mode = rand() % 4 + 1;
			c_buffer->data->robot_state = rand() % 4 + 1;
			c_buffer->data->load_state = rand() % 5;
			c_buffer->data->operation_time = 30 + rand() % 10;
			c_buffer->data->power_time = 2 + rand() % 2;

			GetLocalTime(&lt);
			stop_clock = lt.wSecond;
			if (stop_clock > start_clock)
			{
				if (stop_clock - start_clock > DELAY_TIME)
				{
					c_buffer->count += 1;
					send(client_sock, (char*)c_buffer, sizeof(sClientMessage), 0);
					std::cout << "Message send" << std::endl;
					start_clock = stop_clock;
				}
			}
			else if (stop_clock < start_clock)
			{
				if (stop_clock + 60 - start_clock > DELAY_TIME)
				{
					c_buffer->count += 1;
					send(client_sock, (char*)c_buffer, sizeof(sClientMessage), 0);
					std::cout << "Message send" << std::endl;
					start_clock = stop_clock;
				}
			}
			if (c_buffer->count == 20)
			{
				break;
			}

		}
		//close client socket
		closesocket(client_sock);
	}

	//cleanup winsock
	WSACleanup();

	return 0;
}