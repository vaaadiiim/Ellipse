#include "stdafx.h"
#include "WorkManager.h"
#include <iostream>

WorkManager::WorkManager(int iPort)
{
	WSAData wsaData;
	WORD DllVersion = MAKEWORD(2, 1);
	if (WSAStartup(DllVersion, &wsaData) != 0)
	{
		std::cout << "WinSock startup failed" << std::endl;
		return;
	}

	int addrlen = sizeof(addr);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(iPort);
	addr.sin_family = AF_INET; 

	sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	bind(sListen, (SOCKADDR*)&addr, sizeof(addr));
	listen(sListen, SOMAXCONN);
	InitializeCriticalSection(&csData);
	hConnecter = nullptr;
	hJober = nullptr;
	StartServer();
	StartJob();
}

WorkManager::~WorkManager()
{
	StopJob();
	StopServer();
	for (int i = 0; i < vWorkers.size(); i++)
		closesocket(vWorkers.at(i).s);
	WSACleanup();
}

void WorkManager::StartJob()
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

void WorkManager::StopJob()
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
	WorkManager* wmParam = (WorkManager*)param;
	WSANETWORKEVENTS events;
	WSAEVENT hCloseEvent = WSACreateEvent();
	WSAEVENT hReadEvent = WSACreateEvent();
	while (true)
	{
		if (TryEnterCriticalSection(&wmParam->csData))
		{
			if (wmParam->iJobThrState == 2)
			{
				wmParam->iJobThrState = 0;
				CloseHandle(wmParam->hJober);
				wmParam->hJober = nullptr;
				LeaveCriticalSection(&wmParam->csData);
				WSACloseEvent(hCloseEvent);
				WSACloseEvent(hReadEvent);
				ExitThread(0);
			}
			for (int i = 0; i < wmParam->vWorkers.size(); i++)
			{
				WSAEventSelect(wmParam->vWorkers.at(i).s, hCloseEvent, FD_CLOSE);
				if (WSAWaitForMultipleEvents(1, &hCloseEvent, FALSE, CLOSE_WAIT, FALSE) == WSA_WAIT_EVENT_0)
				{
					DWORD dwResult = WSAEnumNetworkEvents(wmParam->vWorkers.at(i).s, hCloseEvent, &events);
					if (events.lNetworkEvents & FD_CLOSE)
					{
						closesocket(wmParam->vWorkers.at(i).s);
						wmParam->vWorkers.erase(wmParam->vWorkers.begin() + i);
						std::cout << "Worker disconnected" << std::endl;
					}
				}
			}
			if (!wmParam->stTasks.empty())
			{
				for (auto& itw : wmParam->vWorkers)
				{
					if (!itw.bBusy)
					{
						wmParam->FillBufWithTask(wmParam->stTasks.top());
						if (send(itw.s, wmParam->buf, TASK_LEN, 0) == TASK_LEN)
						{
							itw.bBusy = true;
							wmParam->stTasks.pop();
							if (wmParam->stTasks.empty())
								break;
						}
					}
				}
			}
			for (int i = 0; i < wmParam->vWorkers.size(); i++)
			{
				WSAEventSelect(wmParam->vWorkers.at(i).s, hReadEvent, FD_READ);
				if (WSAWaitForMultipleEvents(1, &hReadEvent, FALSE, READ_WAIT, FALSE) == WSA_WAIT_EVENT_0)
				{
					DWORD dwResult = WSAEnumNetworkEvents(wmParam->vWorkers.at(i).s, hReadEvent, &events);
					if (events.lNetworkEvents & FD_READ)
					{
						ZeroMemory(&wmParam->buf, BUF_LEN);
						if (recv(wmParam->vWorkers.at(i).s, wmParam->buf, RES_LEN, 0) == RES_LEN)
						{
							wmParam->vWorkers.at(i).bBusy = false;
							if (wmParam->buf[0] == 'R')
							{
								double a = *((double*)&wmParam->buf[1]);
								double b = *((double*)&wmParam->buf[9]);
								double eps = *((double*)&wmParam->buf[17]);
								double s = *((double*)&wmParam->buf[25]);
								std::cout << "ellipse(" << a << ", " << b << ", " << eps << ") = " << s << std::endl;
							}
						}
						else
						{
							wmParam->vWorkers.at(i).bBusy = false;
							if (wmParam->buf[0] == 'E')
							{
								double a = *((double*)&wmParam->buf[1]);
								double b = *((double*)&wmParam->buf[9]);
								double eps = *((double*)&wmParam->buf[17]);
								std::cout << "ellipse(" << a << ", " << b << ", " << eps << ") has wrong parameters" << std::endl;
							}
						}
					}
				}
			}
			LeaveCriticalSection(&wmParam->csData);
			Sleep(20);
		}
	}
}

void WorkManager::StartServer()
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

void WorkManager::StopServer()
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

void WorkManager::AddTask(double a, double b, double eps)
{
	while (true)
	{
		if (TryEnterCriticalSection(&csData))
		{
			Task t;
			t.a = a;
			t.b = b;
			t.eps = eps;
			stTasks.push(t);
			LeaveCriticalSection(&csData);
			return;
		}
		else
			Sleep(20);
	}
}

void WorkManager::FillBufWithTask(Task t)
{
	ZeroMemory(&buf, BUF_LEN);
	char* pA = (char*)&t.a;
	char* pB = (char*)&t.b;
	char* pEps = (char*)&t.eps;
	buf[0] = 'T';
	for (int i = 0; i < sizeof(double); i++)
	{
		buf[1 + i] = pA[i];
		buf[9 + i] = pB[i];
		buf[17 + i] = pEps[i];
	}
}

void ConnectThread(void* param)
{
	WorkManager* wmParam = (WorkManager*)param;
	WSANETWORKEVENTS events;
	WSAEVENT hReadEvent = WSACreateEvent();
	WSAEVENT hAccEvent = WSACreateEvent();
	while (true)
	{
		if (TryEnterCriticalSection(&wmParam->csData))
		{
			if (wmParam->iConnThrState == 2)
			{
				wmParam->iConnThrState = 0;
				CloseHandle(wmParam->hConnecter);
				wmParam->hConnecter = nullptr;
				LeaveCriticalSection(&wmParam->csData);
				WSACloseEvent(hReadEvent);
				WSACloseEvent(hAccEvent);
				ExitThread(0);
			}
			WSAEventSelect(wmParam->sListen, hAccEvent, FD_ACCEPT);
			if (WSAWaitForMultipleEvents(1, &hAccEvent, FALSE, 1, FALSE) == WSA_WAIT_EVENT_0)
			{
				DWORD dwResult = WSAEnumNetworkEvents(wmParam->sListen, hAccEvent, &events);
				if ((events.lNetworkEvents & FD_ACCEPT) && (events.iErrorCode[FD_ACCEPT_BIT] == 0))
				{
					int addrlen = sizeof(wmParam->addr);
					SOCKET newConnection = accept(wmParam->sListen, (SOCKADDR*)&wmParam->addr, &addrlen);
					ZeroMemory(&wmParam->buf, BUF_LEN);
					if (send(newConnection, wmParam->buf, 5, 0) == 5)
					{
						WSAEventSelect(newConnection, hReadEvent, FD_READ);
						if (WSAWaitForMultipleEvents(1, &hReadEvent, FALSE, 1000, FALSE) == WSA_WAIT_EVENT_0)
						{
							DWORD dwResult = WSAEnumNetworkEvents(newConnection, hReadEvent, &events);
							if (events.lNetworkEvents & FD_READ)
							{
								ZeroMemory(&wmParam->buf, BUF_LEN);
								if (recv(newConnection, wmParam->buf, BUF_LEN, 0) == 5)
								{
									Worker w;
									w.s = newConnection;
									w.bBusy = false;
									wmParam->vWorkers.push_back(w);
									std::cout << "Worker connected" << std::endl;
								}	
								else
									closesocket(newConnection);
							}
						}
					}
					
				}
			}
			LeaveCriticalSection(&wmParam->csData);
			Sleep(20);
		}
	}
}


