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
#define DEFAULT_PORT 27016

bool InitializeWindowsSockets();
int Connect();
int Publish(ARTICLE* vest);
char* TopicToLower(char* topic);

extern SOCKET connectSocket;
