#include "../Common/Publisher.h"

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

int Publish(ARTICLE* vest) {
    int iResult = send(connectSocket, (char*)vest, sizeof(ARTICLE), 0);
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(connectSocket);
        WSACleanup();
        return -1;
    }
    return iResult;
}

char* TopicToLower(char* topic) {
    for (unsigned int i = 0; i < strlen(topic); i++) {
        topic[i] = tolower(topic[i]);
    }
    return topic;
}

void RunSmallTest() {
    ARTICLE article;
    memset(&article, 0, sizeof(ARTICLE));

    printf("Pokrecem mali test (10 poruka)...\n");
    for (int i = 1; i <= 50; i++) {
        article.isLocationBased = false;
        strcpy_s(article.topic, "jacina");
        sprintf_s(article.text, "Mala test poruka broj %d", i);

        int res = Publish(&article);
        if (res == -1) break;
        printf("Poslato: %s - %s\n", article.topic, article.text);
    }
}

void RunLargeTest() {
    ARTICLE article;
    memset(&article, 0, sizeof(ARTICLE));

    printf("Pokrecem veliki test (10,000 poruka)...\n");
    for (int i = 1; i <= 10000; i++) {
        article.isLocationBased = false;

        // Rotiraj teme: jacina, napon, snaga
        switch (i % 3) {
        case 1: strcpy_s(article.topic, "jacina"); break;
        case 2: strcpy_s(article.topic, "napon"); break;
        case 0: strcpy_s(article.topic, "snaga"); break;
        }

        sprintf_s(article.text, "Velika test poruka broj %d", i);

        int res = Publish(&article);
        if (res == -1) break;
        if (i % 1000 == 0) printf("Poslato %d poruka...\n", i);
    }
    WSACleanup();

    printf("Veliki test zavrsen.\n");
}

int __cdecl main(int argc, char** argv) {
    if (!InitializeWindowsSockets()) return 1;
    if (Connect()) return 1;

    while (true) {
        int izborTesta = 0;

        printf("Izaberite opciju:\n");
        printf("1. Mali test (10 poruka)\n");
        printf("2. Veliki test (10,000 poruka)\n");
        printf("3. Nastavak rada (manualni unos poruka)\n");
        printf("Izbor: ");
        scanf_s("%d", &izborTesta);
        getchar(); // pojede enter

        if (izborTesta == 1) {
            RunSmallTest();

        }
        else if (izborTesta == 2) {
            RunLargeTest();
        }
        else if (izborTesta == 3) {
            break;  // izlaz iz testa i prelazak na ru?ni unos
        }
        else {
            printf("? Neispravan izbor, pokusajte ponovo.\n");
        }
    }

    // Nastavak rada: ru?ni unos i slanje poruka
    do {
        ARTICLE article;
        memset(&article, 0, sizeof(ARTICLE));

        char izbor[10];
        printf("Da li zelite da saljete poruku po TOPIC-u ili po LOKACIJI? (topic/lokacija): ");
        gets_s(izbor, sizeof(izbor));

        if (_stricmp(izbor, "lokacija") == 0) {
            article.isLocationBased = true;
            printf("Unesite LOKACIJU za vest: ");
            gets_s(article.location, sizeof(article.location));
        }
        else {
            article.isLocationBased = false;

            int izborTopica = 0;
            do {
                printf("Izaberite topic na koji zelite da posaljete poruku:\n");
                printf("1. jacina\n2. napon\n3. snaga\nIzbor: ");
                scanf_s("%d", &izborTopica);
                getchar();

                if (izborTopica < 1 || izborTopica > 3)
                    printf("? Neispravan unos. Pokusajte ponovo.\n");
            } while (izborTopica < 1 || izborTopica > 3);

            switch (izborTopica) {
            case 1: strcpy_s(article.topic, "jacina"); break;
            case 2: strcpy_s(article.topic, "napon"); break;
            case 3: strcpy_s(article.topic, "snaga"); break;
            }
        }

        printf("Unesite sada poruku: ");
        gets_s(article.text, sizeof(article.text));

        if (strcmp(article.text, "kraj") == 0 || strcmp(article.text, "exit") == 0)
            break;

        int iResult = Publish(&article);
        if (iResult == -1)
            break;

        printf("Bytes Sent: %d\n", iResult);

    } while (true);

    closesocket(connectSocket);
    WSACleanup();

    return 0;
}
