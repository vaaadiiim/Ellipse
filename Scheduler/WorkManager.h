#include <winsock2.h>
#include <ws2tcpip.h>
#include <stack>
#include <vector>
#include <string>

#pragma once

#define BUF_LEN 48
#define PING_LEN 5
#define PING_WAIT 1000
#define READ_WAIT 1
#define CLOSE_WAIT 1
#define TASK_LEN 25
#define RES_LEN 33

struct Task
{
	double a;
	double b;
	double eps;
};

struct Worker
{
	SOCKET s;
	BOOL bBusy;
};

class WorkManager
{
public:
	WorkManager(int iPort);
	~WorkManager();
	void StartServer();
	void StopServer();
	void AddTask(double a, double b, double eps);
private:
	void StartJob();
	void StopJob();
	void FillBufWithTask(Task t);

private:
	SOCKADDR_IN addr;
	SOCKET sListen;
	char buf[BUF_LEN];
	HANDLE hConnecter;
	friend void ConnectThread(void* param);
	int iConnThrState;//0 - not working, 1 - working, 2 - to exit
	HANDLE hJober;
	friend void JobThread(void* param);
	int iJobThrState;//0 - not working, 1 - working, 2 - to exit
	CRITICAL_SECTION csData;
	std::vector<Worker> vWorkers;
	std::stack<Task> stTasks;
};

