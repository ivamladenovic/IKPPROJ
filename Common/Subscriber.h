#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include "Structs.h"
#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_BUFLEN 524
#define DEFAULT_PORT 27017
#define SAFE_DELETE_HANDLE(a) if(a){CloseHandle(a);} 

bool InitializeWindowsSockets();
DWORD WINAPI FunkcijaThread1(LPVOID param);
void Subscribe(ARTICLE* zahtev);
int Connect();

// Globalne promenljive
extern HANDLE signalKraja, signalKrajaNiti;
extern DWORD thread1ID;
extern CRITICAL_SECTION criticalSectionForSTDIO;
extern SOCKET connectSocket;
