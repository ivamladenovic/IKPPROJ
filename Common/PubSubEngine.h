#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS //inet_ntoa zbog toga ide ovaj define
#include "HashTable.h"
#include "Queue.h"
#define DEFAULT_BUFLEN 524
#define SAFE_DELETE_HANDLE(a) if(a){CloseHandle(a);} 
#define MAX_QUEUE_SIZE 1000
#define PUBLISHER_PORT "27016"
#define SUBSCRIBER_PORT "27017"
//#define _WINSOCK_DEPRECATED_NO_WARNINGS //inet_ntoa zbog toga ide ovaj define
#pragma comment(lib, "ws2_32.lib") // ovo je zbog greske LINK 2019 unresolved external symbols _imp_WSA or _imp_socket ....

//GLOBALNE PROMENLJIVE
QUEUE queue;
CRITICAL_SECTION criticalSectionForQueue, criticalSectionForPublisher, criticalSectionForSubscribers, criticalSectionForListOfSubscribers;
HANDLE FinishSignal, Full, Empty, publisherSemafor, publisherFinishSemafor;
uticnica * publisherSockets = NULL;
uticnica * subscriberSockets = NULL;

subscribers *tabela[table_size];

HANDLE t1, t2, t3, t4, t5, t6;
DWORD thread1ID, thread2ID, thread3ID, thread4ID, thread5ID, thread6ID;
//int brojac = 0; sluzi za prikaz rednog broja poruke pristigle od publishera -> ThreadFunkcija4 <- tu se korisit
//END GLOBALNE PROMENLJIVE



void InitAllNecessaryCriticalSection() {
	InitializeCriticalSection(&criticalSectionForQueue);
	InitializeCriticalSection(&criticalSectionForPublisher);
	InitializeCriticalSection(&criticalSectionForSubscribers);
	InitializeCriticalSection(&criticalSectionForListOfSubscribers);
}

void DeleteAllNecessaryCriticalSection() {
	DeleteCriticalSection(&criticalSectionForQueue);
	DeleteCriticalSection(&criticalSectionForPublisher);
	DeleteCriticalSection(&criticalSectionForSubscribers);
	DeleteCriticalSection(&criticalSectionForListOfSubscribers);
}

bool InitializeWindowsSockets()
{
	WSADATA wsaData;
	// Initialize windows sockets library for this process
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup failed with error: %d\n", WSAGetLastError());
		return false;
	}
	return true;
}

void CreateAllSemaphores() {

	FinishSignal = CreateSemaphore(0, 0, 6, NULL);
	Full = CreateSemaphore(0, 0, MAX_QUEUE_SIZE, NULL);
	Empty = CreateSemaphore(0, MAX_QUEUE_SIZE, MAX_QUEUE_SIZE, NULL);
	publisherSemafor = CreateSemaphore(0, 0, 1, NULL);
	publisherFinishSemafor = CreateSemaphore(0, 0, 1, NULL);
}

void DeleteAllThreadsAndSemaphores() {
	SAFE_DELETE_HANDLE(t1);
	SAFE_DELETE_HANDLE(t2);
	SAFE_DELETE_HANDLE(t3);
	SAFE_DELETE_HANDLE(t4);
	SAFE_DELETE_HANDLE(t5);
	SAFE_DELETE_HANDLE(t6);
	SAFE_DELETE_HANDLE(FinishSignal);
	SAFE_DELETE_HANDLE(Empty);
	SAFE_DELETE_HANDLE(Full);
	SAFE_DELETE_HANDLE(publisherSemafor);
	SAFE_DELETE_HANDLE(publisherFinishSemafor);
}

char * TopicToLower(char* topic) {

	for (unsigned int i = 0; i < strlen(topic); i++) {
		topic[i] = tolower(topic[i]);
	}
	return topic;
}

SOCKET InitializeListenSocket(const char* port) {

	SOCKET listenSocket = INVALID_SOCKET;
	// Prepare address information structures
	addrinfo *resultingAddress = NULL;
	addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;       // IPv4 address
	hints.ai_socktype = SOCK_STREAM; // Provide reliable data streaming
	hints.ai_protocol = IPPROTO_TCP; // Use TCP protocol
	hints.ai_flags = AI_PASSIVE;     // 

	// Resolve the server address and port
	int iResult = getaddrinfo(NULL, port, &hints, &resultingAddress);
	if (iResult != 0)
	{
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}


	// Create a SOCKET for connecting to server
						//		IPv4 address famly|stream socket | TCP protocol
	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (listenSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(resultingAddress);

		WSACleanup();
		return INVALID_SOCKET;
	}

	// Setup the TCP listening socket - bind port number and local address 
	// to socket
	iResult = bind(listenSocket, resultingAddress->ai_addr, (int)resultingAddress->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(resultingAddress);
		WSACleanup();
		return INVALID_SOCKET;
	}

	// Since we don't need resultingAddress any more, free it
	freeaddrinfo(resultingAddress);
	//stavi u neblokirjauci rezim
	unsigned long mode = 1;
	iResult = ioctlsocket(listenSocket, FIONBIO, &mode);
	if (iResult != NO_ERROR) {
		printf("ioctlsocket failed with error: %ld\n", iResult);
		return INVALID_SOCKET;
	}
	return listenSocket;
}

DWORD WINAPI FunkcijaThread1(LPVOID param) {
	
	SOCKET listenSocketPublisher = *(SOCKET *)param;
	SOCKET acceptedSocketPublisher = INVALID_SOCKET;
	unsigned long mode = 1;
	int lastIndex = 0;

	if (InitializeWindowsSockets() == false)
	{
		// we won't log anything since it will be logged
		// by InitializeWindowsSockets() function
		return 1;
	}

	fd_set readfds;
	timeval timeVal;
	timeVal.tv_sec = 1;
	timeVal.tv_usec = 0;





	while (WaitForSingleObject(FinishSignal, 500) == WAIT_TIMEOUT) {

		FD_ZERO(&readfds);
		FD_SET(listenSocketPublisher, &readfds);

		// ako je listen -> prihvati zahtev klijenta 
		int selectResult = select(0, &readfds, NULL, NULL, &timeVal);
		if (selectResult == SOCKET_ERROR) {

			printf("SELECT PUBLISHERA (LISTEN SOCKET) FAILED WITH ERROR: %d\n", WSAGetLastError());
			closesocket(listenSocketPublisher);
			WSACleanup();
			return 1;

		}
		else if (selectResult == 0) {
			// vreme zadato u timeVal isteklo a pri tome se nije desio
			//ni jedan dogadjaj ni na jednom socketu -> nastavi dalje da slusas
			continue;
		}
		else {
			// desio se dogadjaj na listen socketu
			acceptedSocketPublisher = INVALID_SOCKET;
			sockaddr_in clientAddr;
			int clinetAddrSize = (sizeof(struct sockaddr_in));

			acceptedSocketPublisher = accept(listenSocketPublisher, (struct sockaddr*)&clientAddr, &clinetAddrSize);

			if (acceptedSocketPublisher == INVALID_SOCKET) {

				if (WSAGetLastError() == WSAECONNRESET) {
					printf("accept failed, beacuse timeout for client request has expired.\n");
				}
				
			}
			else {
				//uspesno prihvacen klijent postavi socket u neblokirajuci rezim
				if (ioctlsocket(acceptedSocketPublisher, FIONBIO, &mode) != 0) {
					printf("ioctlsocket failed with error.");
					continue;
				}
				else {

					//dodati u listu ovaj accpeted socket;
					EnterCriticalSection(&criticalSectionForPublisher);
					Add(&publisherSockets, acceptedSocketPublisher);
					LeaveCriticalSection(&criticalSectionForPublisher);
					ReleaseSemaphore(publisherSemafor, 1, NULL); //obavesti THREAD 3 da moze da prolazi korz acceptedSockete publishera i da proverava da li se desio neki dogadjaj na nekom od njih
					lastIndex++;
					printf("New Publisher request accpeted (%d). Client address: %s : %d\n", lastIndex, inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
				}
			}
		}

	}

	//pusti i thread 3 da zavrsi mozda je bas u trenutku zaglavila u wait-u za ovaj semafor
	ReleaseSemaphore(publisherFinishSemafor, 1, NULL);

	// cleanup
	closesocket(listenSocketPublisher);
	
	WSACleanup();

	return 0;
}


DWORD WINAPI FunkcijaThread2(LPVOID param) {
	SOCKET listenSocketSubscriber = *(SOCKET*)param;
	SOCKET acceptedSocketSubscriber = INVALID_SOCKET;
	short lastIndexSub = 0;
	int IResultSubscriber;
	char recvbuf[DEFAULT_BUFLEN];

	fd_set readfds;
	timeval timeVal;
	timeVal.tv_sec = 1;
	timeVal.tv_usec = 0;
	unsigned long mode = 1;

	uticnica* trenutni = NULL;

	if (InitializeWindowsSockets() == false) return 1;

	while (WaitForSingleObject(FinishSignal, 1000) == WAIT_TIMEOUT) {
		trenutni = subscriberSockets;
		FD_ZERO(&readfds);
		FD_SET(listenSocketSubscriber, &readfds);

		while (trenutni != NULL) {
			FD_SET(trenutni->acceptedSocket, &readfds);
			trenutni = trenutni->next;
		}

		int selectResult = select(0, &readfds, NULL, NULL, &timeVal);
		if (selectResult == SOCKET_ERROR) {
			printf("SELECT SUBSCRIBERA FAILED WITH ERROR: %d\n", WSAGetLastError());
		}
		else if (selectResult == 0) {
			continue;
		}
		else {
			if (FD_ISSET(listenSocketSubscriber, &readfds)) {
				sockaddr_in clientAddr;
				int clinetAddrSize = sizeof(struct sockaddr_in);
				acceptedSocketSubscriber = accept(listenSocketSubscriber, (struct sockaddr*)&clientAddr, &clinetAddrSize);

				if (acceptedSocketSubscriber != INVALID_SOCKET) {
					if (ioctlsocket(acceptedSocketSubscriber, FIONBIO, &mode) == 0) {
						Add(&subscriberSockets, acceptedSocketSubscriber);
						lastIndexSub++;
						printf("New Subscriber request accepted (%d). Address: %s:%d\n", lastIndexSub, inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
					}
				}
			}
			else {
				trenutni = subscriberSockets;
				while (trenutni != NULL) {
					if (FD_ISSET(trenutni->acceptedSocket, &readfds)) {
						IResultSubscriber = recv(trenutni->acceptedSocket, recvbuf, DEFAULT_BUFLEN, 0);
						if (IResultSubscriber > 0) {
							ARTICLE zahtev;
							memcpy(&zahtev, recvbuf, sizeof(ARTICLE));

							char* key = zahtev.isLocationBased ? zahtev.location : zahtev.topic;
							strcpy_s(key, strlen(key) + 1, TopicToLower(key));

							printf("Message received from subscriber for %s: %s\n", zahtev.isLocationBased ? "location" : "topic", key);
							subscribers* temp = FindSubscriberInTable(tabela, key, zahtev.isLocationBased);
							//subscribers* temp = FindSubscriberInTable(tabela, key);
							if (temp == NULL) {
								subscribers* novi = CreateSubscriber(key,key);
								if (novi) {
									EnterCriticalSection(&criticalSectionForSubscribers);
									if (Add(&novi->acceptedSocketsForTopic, trenutni->acceptedSocket)) {
										AddToTable(tabela, novi);
										printf("New topic/location and subscriber added.\n");
									}
									LeaveCriticalSection(&criticalSectionForSubscribers);
								}
							}
							else {
								EnterCriticalSection(&criticalSectionForSubscribers);
								if (!FindInList(&temp->acceptedSocketsForTopic, trenutni->acceptedSocket)) {
									Add(&temp->acceptedSocketsForTopic, trenutni->acceptedSocket);
									printf("Subscriber added to existing topic/location.\n");
								}
								else {
									printf("Subscriber already subscribed to this topic/location.\n");
								}
								LeaveCriticalSection(&criticalSectionForSubscribers);
							}
							trenutni = trenutni->next;
						}
						else if (IResultSubscriber == 0 || IResultSubscriber == SOCKET_ERROR) {
							printf("Subscriber disconnected or error occurred.\n");
							closesocket(trenutni->acceptedSocket);

							EnterCriticalSection(&criticalSectionForSubscribers);
							DeleteSubscriberFromListOfSubscribers(tabela, trenutni->acceptedSocket);
							LeaveCriticalSection(&criticalSectionForSubscribers);

							uticnica* zaBrisanje = trenutni;
							trenutni = trenutni->next;
							Remove(&subscriberSockets, zaBrisanje->acceptedSocket);
							lastIndexSub--;
						}
					}
					else {
						trenutni = trenutni->next;
					}
				}
			}
		}
	}

	closesocket(listenSocketSubscriber);
	WSACleanup();
	return 0;
}

DWORD WINAPI FunkcijaThread3(LPVOID param) {
	
	int iResult;
	
	char recvbuf[DEFAULT_BUFLEN];

	fd_set readfds;
	timeval timeVal;
	timeVal.tv_sec = 1;
	timeVal.tv_usec = 0;
	ARTICLE *vest;
	uticnica * trenutni = NULL;
	bool finish = false;
	if (InitializeWindowsSockets() == false)
	{
		// we won't log anything since it will be logged
		// by InitializeWindowsSockets() function
		return 1;
	}

	const int broj_semafora = 2;
	HANDLE semafori[broj_semafora] = { publisherSemafor, publisherFinishSemafor };

	while (WaitForSingleObject(FinishSignal, 500) == WAIT_TIMEOUT) {

		trenutni = publisherSockets; // mora ovde zbog publisherSockets jer je globalna promenljiva pa da svaki put imamo one koje je dodala Thread 1 
		while (trenutni == NULL) {
			// bilo je nekoliko publishera pa su se svi odjavili i sad opet blokiraj nit dok se bar 1 publisher ne prijavi
			//WaitForSingleObject(publisherSemafor, INFINITE);
			if (WaitForMultipleObjects(broj_semafora, semafori, FALSE, INFINITE) == WAIT_OBJECT_0 + 1) {
				finish = true;
				break;
			}
			trenutni = publisherSockets; //uzmi i te nove koji su se naknadno prijavili.
		}

		if (finish) break;
		
		FD_ZERO(&readfds);

		EnterCriticalSection(&criticalSectionForPublisher);
		while (trenutni != NULL)
		{
			FD_SET(trenutni->acceptedSocket, &readfds);
			trenutni = trenutni->next;
		}
		LeaveCriticalSection(&criticalSectionForPublisher);


		int selectResult = select(0, &readfds, NULL, NULL, &timeVal);

		if (selectResult == SOCKET_ERROR) {

			printf("SELECT LISTE ACCEPTED SOCKETA PUBLISHERA FAILED WITH ERROR: %d\n", WSAGetLastError());
			//printf("SELECT nad listom accepted socketa publishera nije uspeo, verovatno je prazna lista\n");

		}
		else if (selectResult == 0) {
			// vreme zadato u timeVal isteklo a pri tome se nije desio
			//ni jedan dogadjaj ni na jednom socketu -> nastavi dalje da slusas
			continue;
		}
		else {
			EnterCriticalSection(&criticalSectionForPublisher);
			trenutni = publisherSockets; // vracanje trenutni da pokazuje opet na pocetak liste;
			while (trenutni != NULL)
			{
				if (FD_ISSET(trenutni->acceptedSocket, &readfds)) {


					// Receive data until the client shuts down the connection
					iResult = recv(trenutni->acceptedSocket, recvbuf, (int)(sizeof(ARTICLE)), 0);
					if (iResult > 0)
					{
						vest = (ARTICLE*)recvbuf;
						//Ovde ce da ceka semafor empty (skinuo sam sa queueu nije vise pun, mozes da ubacis na queue)
						WaitForSingleObject(Empty, INFINITE);
						EnterCriticalSection(&criticalSectionForQueue);

						/*printf("Naslov: %s\n", vest->topic);
						printf("Tekst: %s \n\n", vest->text);*/

						Enqueue(&queue, *vest);
						//ShowQueue(&queue);
						LeaveCriticalSection(&criticalSectionForQueue);
						trenutni = trenutni->next; // promena na sledeci socekt za sledecu iteraciju
						ReleaseSemaphore(Full, 1, NULL);
					}
					else if (iResult == 0)
					{
						// connection was closed gracefully
						printf("Connection with publisher closed.\n");
						closesocket(trenutni->acceptedSocket);

						uticnica * zaBrisanje = trenutni;
						trenutni = trenutni->next;  // ovo mora ovde da se odradi, jer da sam porosledio da obrise trenutnog izgubili bi svaki socket u listi posle trenutnog
						Remove(&publisherSockets, zaBrisanje->acceptedSocket);

					}
					else
					{
						// there was an error during recv
						// ovde ce uci kada se klijent konzolna app ugasi na X u gornjem desnom uglu
						printf("publisher recv failed with error: %d\n", WSAGetLastError());
						closesocket(trenutni->acceptedSocket);

						uticnica * zaBrisanje = trenutni;
						trenutni = trenutni->next;  // ovo mora ovde da se odradi, jer da sam porosledio da obrise trenutnog izgubili bi svaki socket u listi posle trenutnog
						Remove(&publisherSockets, zaBrisanje->acceptedSocket);


					}

				}
				else trenutni = trenutni->next;

			}
			LeaveCriticalSection(&criticalSectionForPublisher);
		}


	}

	//THREAD 3
	//ZatvoriSveSocketeZaListu(publisherSockets); pozvacemo u main-u
	WSACleanup();
	return 0;
}

DWORD WINAPI FunkcijaThread4(LPVOID param) {
	int iResult;
	const int broj_semafora = 2;
	HANDLE semafori[broj_semafora] = { FinishSignal, Full };
	ARTICLE vest;
	int identifikator = (int)param;

	while (WaitForMultipleObjects(broj_semafora, semafori, FALSE, INFINITE) == WAIT_OBJECT_0 + 1) {
		EnterCriticalSection(&criticalSectionForQueue);
		printf("\tIdentifikator: %d\n", identifikator);
		if (Dequeue(&queue, &vest)) {
			printf("SKINUTO SA QUEUE:\n");
			printf("Topic/Location: %s \n", vest.isLocationBased ? vest.location : vest.topic);
			printf("Message: %s\n\n", vest.text);
		}
		else {
			printf("Nije uspelo skidanje sa queue\n");
		}
		LeaveCriticalSection(&criticalSectionForQueue);

		ReleaseSemaphore(Empty, 1, NULL);

		// Odredi da li šalješ po topicu ili lokaciji
		char* key = vest.isLocationBased ? vest.location : vest.topic;

		uticnica* listaPretplacenih = NULL;
		EnterCriticalSection(&criticalSectionForSubscribers);
		subscribers* temp = FindSubscriberInTable(tabela, key,key);
		LeaveCriticalSection(&criticalSectionForSubscribers);

		if (temp != NULL) {
			EnterCriticalSection(&criticalSectionForSubscribers);
			listaPretplacenih = temp->acceptedSocketsForTopic;

			while (listaPretplacenih != NULL) {
				iResult = send(listaPretplacenih->acceptedSocket, (char*)&vest, sizeof(ARTICLE), 0);
				if (iResult == SOCKET_ERROR) {
					printf("Greska pri slanju subscriberu.\n");
				}
				listaPretplacenih = listaPretplacenih->next;
			}
			LeaveCriticalSection(&criticalSectionForSubscribers);
		}
	}
	return 0;
}




void CreateAllThreads(SOCKET *listenSocketPublisher, SOCKET* listenSocketSubscriber) {

	t1 = CreateThread(NULL, 0, &FunkcijaThread1, listenSocketPublisher, 0, &thread1ID);
	t2 = CreateThread(NULL, 0, &FunkcijaThread2, listenSocketSubscriber, 0, &thread2ID);
	t3 = CreateThread(NULL, 0, &FunkcijaThread3, NULL, 0, &thread3ID);

	t4 = CreateThread(NULL, 0, &FunkcijaThread4, (LPVOID)0, 0, &thread4ID);
	t5 = CreateThread(NULL, 0, &FunkcijaThread4, (LPVOID)1, 0, &thread5ID);
	t6 = CreateThread(NULL, 0, &FunkcijaThread4, (LPVOID)2, 0, &thread6ID);
}

