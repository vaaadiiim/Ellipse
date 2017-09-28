#include <winsock2.h>
#include <ws2tcpip.h>
#include  <string>


#pragma once

#define BUF_LEN 48
#define PING_LEN 5
#define PING_WAIT 1000
#define READ_WAIT 1
#define CLOSE_WAIT 1
#define TASK_LEN 25
#define RES_LEN 33


class Work
{
public:
	Work(std::string sIP, int iPort);
	~Work();
	void StartConnect();
	void StopConnect();
	void StartJob();
	void StopJob();
private:
	SOCKADDR_IN addr;
	SOCKET sConnection;
	WSAEVENT hSockEvent;
	WSANETWORKEVENTS events;
	char buf[BUF_LEN];
	CRITICAL_SECTION csData;
	void ParseTask(double* a, double* b, double* eps);
	void FillBufWithRes(double a, double b, double eps, double s);
	void FillBufWithErr(double a, double b, double eps);
	double CalculateArea(double a, double b, double epsilon);
private:
	HANDLE hConnecter;
	friend void ConnectThread(void* param);
	int iConnThrState;//0 - not working, 1 - working, 2 - to exit
private:
	HANDLE hJober;
	friend void JobThread(void* param);
	int iJobThrState;//0 - not working, 1 - working, 2 - to exit
};

