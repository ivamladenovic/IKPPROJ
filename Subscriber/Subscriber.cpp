#include "../Common/Subscriber.h"

HANDLE signalKraja = NULL;
HANDLE signalKrajaNiti = NULL;
DWORD thread1ID;
CRITICAL_SECTION criticalSectionForSTDIO;
SOCKET connectSocket = INVALID_SOCKET;

bool InitializeWindowsSockets() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return false;
    }
    return true;
}

int Connect() {
    connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (connectSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port = htons(DEFAULT_PORT);

    if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        printf("Unable to connect to server.\n");
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }
    return 0;
}

void Subscribe(ARTICLE* zahtev) {
    int iResult = send(connectSocket, (char*)zahtev, sizeof(ARTICLE), 0);
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
    }
    else {
        printf("Pretplata poslata.\n");
    }
}

DWORD WINAPI FunkcijaThread1(LPVOID param) {
    SOCKET connectSocket = *(SOCKET*)param;

    while (WaitForSingleObject(signalKrajaNiti, 500) == WAIT_TIMEOUT) {
        if (_kbhit()) {
            EnterCriticalSection(&criticalSectionForSTDIO);
            char ch = _getch();
            LeaveCriticalSection(&criticalSectionForSTDIO);

            if (ch == 'x') {
                EnterCriticalSection(&criticalSectionForSTDIO);

                ARTICLE noviZahtev;
                memset(&noviZahtev, 0, sizeof(ARTICLE));

                int izbor;
                printf("Zelite da se pretplatite na:\n1. Topic\n2. Lokaciju\nIzbor: ");
                scanf_s("%d", &izbor);
                getchar();

                if (izbor == 1) {
                    noviZahtev.isLocationBased = false;
                    int izborTopica = 0;
                    do {
                        printf("Izaberite jedan od ponu?enih topic-a:\n");
                        printf("1. jacina\n2. napon\n3. snaga\nIzbor: ");
                        scanf_s("%d", &izborTopica);
                        getchar();
                        if (izborTopica < 1 || izborTopica > 3)
                            printf("? Neispravan unos. Pokusajte ponovo.\n\n");
                    } while (izborTopica < 1 || izborTopica > 3);

                    switch (izborTopica) {
                    case 1: strcpy_s(noviZahtev.topic, "jacina"); break;
                    case 2: strcpy_s(noviZahtev.topic, "napon"); break;
                    case 3: strcpy_s(noviZahtev.topic, "snaga"); break;
                    }
                }
                else {
                    noviZahtev.isLocationBased = true;
                    printf("Unesite naziv lokacije: ");
                    gets_s(noviZahtev.location, sizeof(noviZahtev.location));
                }

                Subscribe(&noviZahtev);
                LeaveCriticalSection(&criticalSectionForSTDIO);
            }
            else if (ch == 'q') {
                ReleaseSemaphore(signalKraja, 1, NULL);
                break;
            }
        }
    }

    closesocket(connectSocket);
    WSACleanup();
    return 0;
}

int __cdecl main(int argc, char** argv) {
    int iResult;
    char recvbuf[DEFAULT_BUFLEN];

    if (!InitializeWindowsSockets())
        return 1;

    if (Connect())
        return 1;

    ARTICLE subscriptionRequest;
    memset(&subscriptionRequest, 0, sizeof(ARTICLE));

    char izbor[10];
    printf("Da li zelite da se pretplatite na TOPIC ili na LOKACIJU? (topic/lokacija): ");
    gets_s(izbor, sizeof(izbor));

    if (_stricmp(izbor, "lokacija") == 0) {
        subscriptionRequest.isLocationBased = true;
        printf("Unesite LOKACIJU na koju se zelite pretplatiti: ");
        gets_s(subscriptionRequest.location, sizeof(subscriptionRequest.location));
    }
    else {
        subscriptionRequest.isLocationBased = false;

        int izborTopica = 0;
        do {
            printf("Izaberite jedan od ponu?enih topic-a:\n");
            printf("1. jacina\n2. napon\n3. snaga\nIzbor: ");
            scanf_s("%d", &izborTopica);
            getchar();
            if (izborTopica < 1 || izborTopica > 3)
                printf("? Neispravan unos. Pokusajte ponovo.\n\n");
        } while (izborTopica < 1 || izborTopica > 3);

        switch (izborTopica) {
        case 1: strcpy_s(subscriptionRequest.topic, "jacina"); break;
        case 2: strcpy_s(subscriptionRequest.topic, "napon"); break;
        case 3: strcpy_s(subscriptionRequest.topic, "snaga"); break;
        }
    }

    Subscribe(&subscriptionRequest);

    InitializeCriticalSection(&criticalSectionForSTDIO);
    signalKrajaNiti = CreateSemaphore(0, 0, 1, NULL);
    signalKraja = CreateSemaphore(0, 0, 1, NULL);
    HANDLE t1 = CreateThread(NULL, 0, &FunkcijaThread1, &connectSocket, 0, &thread1ID);

    int brojac = 1;
    ARTICLE* vest;

    do {
        iResult = recv(connectSocket, recvbuf, sizeof(ARTICLE), 0);
        if (iResult > 0) {
            vest = (ARTICLE*)recvbuf;
            EnterCriticalSection(&criticalSectionForSTDIO);
            printf("PORUKA BROJ: %d\n", brojac++);
            if (vest->isLocationBased)
                printf("Lokacija: %s\n", vest->location);
            else
                printf("Naslov: %s\n", vest->topic);
            printf("Tekst: %s \n", vest->text);
            LeaveCriticalSection(&criticalSectionForSTDIO);
        }
        else if (iResult == 0) {
            EnterCriticalSection(&criticalSectionForSTDIO);
            printf("Connection with PubSubEngine closed.\n");
            LeaveCriticalSection(&criticalSectionForSTDIO);
            closesocket(connectSocket);
            ReleaseSemaphore(signalKrajaNiti, 1, NULL);
            break;
        }
        else {
            EnterCriticalSection(&criticalSectionForSTDIO);
            printf("PubSubEngine finished. Closing socket....\n");
            LeaveCriticalSection(&criticalSectionForSTDIO);
            closesocket(connectSocket);
            ReleaseSemaphore(signalKrajaNiti, 1, NULL);
            break;
        }
    } while (WaitForSingleObject(signalKraja, 200) == WAIT_TIMEOUT);

    if (t1)
        WaitForSingleObject(t1, INFINITE);

    SAFE_DELETE_HANDLE(t1);
    SAFE_DELETE_HANDLE(signalKraja);
    SAFE_DELETE_HANDLE(signalKrajaNiti);
    DeleteCriticalSection(&criticalSectionForSTDIO);
    closesocket(connectSocket);
    WSACleanup();

    return 0;
}
