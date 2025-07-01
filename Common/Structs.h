#pragma once
#include <WinSock2.h>
#define max_topic 21

//Lista
typedef struct uticnica {
	SOCKET acceptedSocket;
	struct uticnica *next;
}UTICNICA;
//End Lista

//Queue
typedef struct artikal {
	bool isLocationBased;              // true = lokacija, false = topic
	char topic[max_topic];            // koristi se ako je isLocationBased == false
	char location[max_topic];         // koristi se ako je isLocationBased == true
	char text[491];
} ARTICLE;

typedef struct queue {
	struct node *front;
	struct node *back;
} QUEUE;

typedef struct node {
	struct artikal data;
	struct node *next;
} NODE;
// End queue

//HashTable 
typedef struct subscribers {
	bool isLocationBased;
	char topic[max_topic];            // ili lokacija, zavisi
	char location[max_topic];
	uticnica* acceptedSocketsForTopic;
	struct subscribers* next;
} subscribers;
//End HashTable
