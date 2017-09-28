#include "stdafx.h"
#include "Work.h"
#include <iostream>
#include <sstream>

Work::Work(std::string sIP, int iPort)
{
	WSAData wsaData;
	WORD DllVersion = MAKEWORD(2, 1);
	if (WSAStartup(DllVersion, &wsaData) != 0) 
	{
		MessageBoxA(NULL, "Winsock startup failed", "Error", MB_OK | MB_ICONERROR);
		return;
	}
	int sizeofaddr = sizeof(addr);
	addr.sin_addr.s_addr = inet_addr(sIP.c_str());
	addr.sin_port = htons(iPort);
	addr.sin_family = AF_INET;
	sConnection = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	hSockEvent = WSACreateEvent();
	InitializeCriticalSection(&csData);
	StartConnect();
	hJober = nullptr;
}

Work::~Work()
{
	StopJob();
	StopConnect();
	WSACloseEvent(hSockEvent);
	closesocket(sConnection);
	WSACleanup();
}

void Work::StartConnect()
{
	while (true)
	{
		if (TryEnterCriticalSection(&csData))
		{
			if (hConnecter == nullptr)
			{
				iConnThrState = 1;
				hConnecter = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)ConnectThread, this, 0, nullptr);
			}
			LeaveCriticalSection(&csData);
			return;
		}
		else
			Sleep(20);
	}
}

void Work::StopConnect()
{
	while (true)
	{
		if (TryEnterCriticalSection(&csData))
		{
			if (iConnThrState == 0)
			{
				LeaveCriticalSection(&csData);
				return;
			}
			else if (iConnThrState == 1)
				iConnThrState = 2;
			LeaveCriticalSection(&csData);
		}
		else
			Sleep(20);
	}
}

void ConnectThread(void* param)
{
	Work* cParam = (Work*)param;
	while (true)
	{
		if (TryEnterCriticalSection(&cParam->csData))
		{
			if (cParam->iConnThrState == 2)
			{
				cParam->iConnThrState = 0;
				CloseHandle(cParam->hConnecter);
				cParam->hConnecter = 0;
				LeaveCriticalSection(&cParam->csData);
				ExitThread(0);
			}
			if (connect(cParam->sConnection, (SOCKADDR*)&cParam->addr, sizeof(cParam->addr)) != 0)
			{
				
				WSAEventSelect(cParam->sConnection, cParam->hSockEvent, FD_READ);
				if (WSAWaitForMultipleEvents(1, &cParam->hSockEvent, FALSE, PING_WAIT, FALSE) == WSA_WAIT_EVENT_0)
				{
					DWORD dwResult = WSAEnumNetworkEvents(cParam->sConnection, cParam->hSockEvent, &cParam->events);
					if (cParam->events.lNetworkEvents & FD_READ)
					{
						ZeroMemory(&cParam->buf, BUF_LEN);
						if (recv(cParam->sConnection, cParam->buf, BUF_LEN, 0) == PING_LEN)
						{
							ZeroMemory(&cParam->buf, BUF_LEN);
							if (send(cParam->sConnection, cParam->buf, 5, 0) == PING_LEN)
							{
								std::cout << "Sheduler connected" << std::endl;
								cParam->iConnThrState = 0;
								CloseHandle(cParam->hConnecter);
								cParam->hConnecter = 0;
								cParam->StartJob();
								LeaveCriticalSection(&cParam->csData);
								ExitThread(0);
							}
						}
					}
				}
				else
				{
					closesocket(cParam->sConnection);
					cParam->sConnection = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				}
			}
			LeaveCriticalSection(&cParam->csData);
			Sleep(20);
		}
	}
}

void Work::StartJob()
{
	while (true)
	{
		if (TryEnterCriticalSection(&csData))
		{
			if (hJober == nullptr)
			{
				iJobThrState = 1;
				hJober = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)JobThread, this, 0, nullptr);
			}
			LeaveCriticalSection(&csData);
			return;
		}
		else
			Sleep(20);
	}
}

void Work::StopJob()
{
	while (true)
	{
		if (TryEnterCriticalSection(&csData))
		{
			if (iJobThrState == 0)
			{
				LeaveCriticalSection(&csData);
				return;
			}
			else if (iJobThrState == 1)
				iJobThrState = 2;
			LeaveCriticalSection(&csData);
		}
		else
			Sleep(20);
	}
}

void JobThread(void* param)
{
	Work* cParam = (Work*)param;
	while (true)
	{
		if (TryEnterCriticalSection(&cParam->csData))
		{
			if (cParam->iJobThrState == 2)
			{
				cParam->iJobThrState = 0;
				CloseHandle(cParam->hJober);
				cParam->hJober = nullptr;
				LeaveCriticalSection(&cParam->csData);
				ExitThread(0);
			}
			WSAEventSelect(cParam->sConnection, cParam->hSockEvent, FD_CLOSE);
			if (WSAWaitForMultipleEvents(1, &cParam->hSockEvent, FALSE, CLOSE_WAIT, FALSE) == WSA_WAIT_EVENT_0)
			{
				DWORD dwResult = WSAEnumNetworkEvents(cParam->sConnection, cParam->hSockEvent, &cParam->events);
				if (cParam->events.lNetworkEvents & FD_CLOSE)
				{
					std::cout << "Sheduler disconnected" << std::endl;
					closesocket(cParam->sConnection);
					cParam->iJobThrState = 0;
					CloseHandle(cParam->hJober);
					cParam->hJober = nullptr;
					cParam->StartConnect();
					LeaveCriticalSection(&cParam->csData);
					ExitThread(0);
				}
			}
			WSAEventSelect(cParam->sConnection, cParam->hSockEvent, FD_READ);
			if (WSAWaitForMultipleEvents(1, &cParam->hSockEvent, FALSE, READ_WAIT, FALSE) == WSA_WAIT_EVENT_0)
			{
				DWORD dwResult = WSAEnumNetworkEvents(cParam->sConnection, cParam->hSockEvent, &cParam->events);
				if (cParam->events.lNetworkEvents & FD_READ)
				{
					ZeroMemory(&cParam->buf, BUF_LEN);
					if (recv(cParam->sConnection, cParam->buf, TASK_LEN, 0) == TASK_LEN)
					{
						if (cParam->buf[0] == 'T')
						{
							double a = *((double*)&cParam->buf[1]);
							double b = *((double*)&cParam->buf[9]);
							double eps = *((double*)&cParam->buf[17]);
							if (a > 0 && b > 0 && eps > 0 && eps <= 1)
							{
								std::cout << "Calculating..." << std::endl;
								double s = cParam->CalculateArea(a, b, eps);
								std::cout << s << std::endl;
								cParam->FillBufWithRes(a, b, eps, s);
								send(cParam->sConnection, cParam->buf, RES_LEN, 0);
							}
							else
							{
								cParam->FillBufWithErr(a, b, eps);
								send(cParam->sConnection, cParam->buf, TASK_LEN, 0);
							}
						}
					}
				}
			}
			LeaveCriticalSection(&cParam->csData);
			Sleep(20);
		}
	}
}

void Work::FillBufWithRes(double a, double b, double eps, double s)
{
	ZeroMemory(&buf, BUF_LEN);
	char* pA = (char*)&a;
	char* pB = (char*)&b;
	char* pEps = (char*)&eps;
	char* pS = (char*)&s;
	buf[0] = 'R';
	for (int i = 0; i < sizeof(double); i++)
	{
		buf[1 + i] = pA[i];
		buf[9 + i] = pB[i];
		buf[17 + i] = pEps[i];
		buf[25 + i] = pS[i];
	}
}

void Work::FillBufWithErr(double a, double b, double eps)
{
	ZeroMemory(&buf, BUF_LEN);
	char* pA = (char*)&a;
	char* pB = (char*)&b;
	char* pEps = (char*)&eps;
	buf[0] = 'E';
	for (int i = 0; i < sizeof(double); i++)
	{
		buf[1 + i] = pA[i];
		buf[9 + i] = pB[i];
		buf[17 + i] = pEps[i];
	}
}

double Work::CalculateArea(double a, double b, double epsilon)
{
	double dbNumIter = 1 / (epsilon*epsilon);
	double x = 0;
	double y = 0;
	int iNumInEllips = 0;
	for (double i = 0; i < dbNumIter; i++)
	{
		x = (double)(rand()) / RAND_MAX * 2 * a - a;
		y = (double)(rand()) / RAND_MAX * 2 * b - b;
		if (((x*x) / (a*a)) + ((y*y) / (b*b)) <= 1)
			iNumInEllips++;
	}
	double dbS = 4 * a * b;
	return dbS*iNumInEllips / dbNumIter;
}

