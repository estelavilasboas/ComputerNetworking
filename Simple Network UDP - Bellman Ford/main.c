#include <stdio.h>        //printf
#include <string.h>       //memset
#include <stdlib.h>       //exit(0);
#include <pthread.h>      //thread
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <time.h>         //time
#include "dijkstra.c"

#define MaxNumberNodes 10
#define Timeout 1000
#define SendAfterTimeout 5

enum MessageType{
  ConfirmationMsg,
  DataMsg,
  DistanceMsg
};

typedef struct{
  int id;
  char IP[15];
  int port;
  int *nextNodes;
}Node;

typedef struct{
  Node nodes[MaxNumberNodes];
  int len;
}NodeList;

typedef struct{
  int id;
  int sourceId;
  int destId;
  char data[100];
  int type;
}Message;

Node newNode;
NodeList nodeList;
bool waitingConfirm = false;

typedef struct{
  int node;
  int distance;
}DistanceNode;

typedef struct{
  DistanceNode distances[MaxNumberNodes];
}DistanceNodeList;

void die(char *s){
  perror(s);
  exit(1);
}

int getPort(Message *msg){
  int port = -1;
  for(int i = 0; i!=nodeList.len; i++){
    if(nodeList.nodes[i].id == newNode.nextNodes[msg->destId])
      port = nodeList.nodes[i].port;
  }
  if(port == -1)
    printf("\n\t~ id NOT found :( ~\n");
  
  return port;
}

void keepWaitingConfirmation(clock_t clock1){
  clock_t clock2;
  while(waitingConfirm){
    clock2 = clock();
    if( ( (clock2 - clock1)*1000/CLOCKS_PER_SEC ) >= Timeout ){
      break;
    }
  }
  if(waitingConfirm == true)
    printf("\n\t~ TIMEOUT ~\n");
}

void *socketSend(void *data){
  int messageId=1, port=-1;

  // Set socket for thread
  struct sockaddr_in newSocket;
  int s, socketLength = sizeof(newSocket);
  char IP[15];
  
  while(1){
    Message *buffer = (Message *)malloc(sizeof(Message));
    Message *msg = (Message *)malloc(sizeof(Message));
    clock_t clock1;

    // Get destination node
    printf("\nSend a message to id: ");
    scanf("%d", &msg->destId);

    // Search port
    if( (port = getPort(msg)) == -1)
      continue;
    
    // Search IP
    for(int i = 0; i!=nodeList.len; i++){
      if(nodeList.nodes[i].id == newNode.nextNodes[msg->destId])
        strcpy(IP, nodeList.nodes[i].IP);
    }
    
    // Define/Create UDP socket
    if( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1 )
      die("socket");

    // Zero out the struture
    memset((char *) &newSocket, 0, sizeof(newSocket));
    newSocket.sin_family = AF_INET;
    newSocket.sin_port = htons(port);

    if(inet_aton(IP, &newSocket.sin_addr) == 0){
      fprintf(stderr, "inet_aton() failed\n");
      exit(1);
    }

    // Get and prepare the message
    printf("Enter message : ");
    scanf("%s", msg->data);
    msg->sourceId = newNode.id;
    msg->id = messageId;
    msg->type = DataMsg;

    printf("~Message will be send to router %d. Destination: router %d\n", newNode.nextNodes[msg->destId], msg->destId);

    // Send the message
    if(sendto(s, msg, sizeof(Message), 0, (struct sockaddr *) &newSocket, socketLength) == -1)
      die("sendto()");

    clock1 = clock();
    waitingConfirm = true;

    // Try to send the message again
    for(int i = 1; i <= SendAfterTimeout; i++){
      // Check clock/timeout
      keepWaitingConfirmation(clock1);
      if(waitingConfirm){
        printf("~Sending message again. %d(st/nd/rd/th) attempt", i+1);
        if(sendto(s, msg, sizeof(Message), 0, (struct sockaddr *) &newSocket, socketLength) == -1)
          die("sendto()");
      }
    }
  
    waitingConfirm = false;
    // Clear the buffer by filling null, it might have received data
    memset(buffer, '\0', sizeof(Message));
    messageId++;
  }

  close(s);
}


void *socketReceive(void *data){
  struct sockaddr_in newSocket, otherSocket, redirectSocket;;
  int s, socketLength = sizeof(newSocket), receiveLength;

  // Create a UDP socket
  if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
      die("socket");

  memset((char *) &newSocket, 0, sizeof(newSocket));

  newSocket.sin_family = AF_INET;
  newSocket.sin_port = htons(newNode.port);
  newSocket.sin_addr.s_addr = htonl(INADDR_ANY);

  // Bind socket to port
  if( bind(s, (struct sockaddr*)&newSocket, sizeof(newSocket)) == -1)
    die("bind");
  
  while(1){
    Message *buffer = (Message *)malloc(sizeof(Message));
    fflush(stdout);
    memset(buffer, '\0', sizeof(Message));
    int port = -1;

    if((receiveLength = recvfrom(s, buffer, sizeof(Message), 0, (struct sockaddr *) &otherSocket, &socketLength)) == -1)
      die("recvfrom()");

    // Check if the received message is for this node
    if(buffer->destId == newNode.id){

      if(buffer->type == DataMsg){
        printf("\n\n\t~ Received message %d ~", buffer->sourceId);
        printf("\n\tSource: %d\tId: %d", buffer->sourceId, buffer->id);
        printf("\n\tData: %s\n", buffer->data);

      // Prepare the confirmation message
        buffer->destId = buffer->sourceId;
        buffer->sourceId = newNode.id;
        strcpy(buffer->data, "Message was received by router ");
        // It is a confirmation message now
        buffer->type = ConfirmationMsg;

        if( (port = getPort(buffer)) == -1)
          continue;
        otherSocket.sin_port = htons(port);

        // Send the confirmation message to the source
        if(sendto(s, buffer, sizeof(Message), 0, (struct sockaddr*) &otherSocket, socketLength) == -1)
          die("sendto()");

      }else{
        waitingConfirm = false;
        printf("\n\n\t\t~ Confirmation ~\n");
        printf("\t%s%d\n", buffer->data, buffer->sourceId);
      }

    }else{
      int s, port = -1;
      char IP[15];
      // Search port
      if( (port = getPort(buffer)) == -1)
        continue;

      // Search IP
      for(int i = 0; i!=nodeList.len; i++){
        if(nodeList.nodes[i].id == newNode.nextNodes[buffer->destId])
          strcpy(IP, nodeList.nodes[i].IP);
      }

      printf("\n~ Redirecting message %d:%d for router %d", buffer->id, buffer->sourceId, newNode.nextNodes[buffer->destId]);
      // Create a UDP socket
      if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
          die("socket");

      memset((char *) &redirectSocket, 0, sizeof(redirectSocket));
      redirectSocket.sin_family = AF_INET;
      redirectSocket.sin_port = htons(port);

      if(inet_aton(IP, &redirectSocket.sin_addr) == 0){
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
      } 

      if(sendto(s, buffer, sizeof(Message), 0, (struct sockaddr*) &redirectSocket, socketLength) == -1)
        die("sendto()");

      memset(buffer, '\0', sizeof(Message));
    }
  }

  close(s);
}

void readFile(){
  FILE *file;

  printf("Who am I? ");
  scanf("%d", &newNode.id);
  printf("Searching my IP...\n");

  if(( file = fopen("roteadores.config", "r")) == NULL){
    printf("\n\t~ roteadores.config: File could not be opened :( ~\n");
    exit(1);
  }

  int id, port;
  char IP[15];
  nodeList.len = 0;

  // Reading the file and saving nodes data.
  while(fscanf(file, "%d %d %s", &id, &port, IP) != EOF ){
    int i = nodeList.len++;
    nodeList.nodes[i].id = id;
    strcpy(nodeList.nodes[i].IP, IP);
    nodeList.nodes[i].port = port;
    nodeList.nodes[i].nextNodes = NULL;
  
    if(id == newNode.id){
      strcpy(newNode.IP, IP);
      newNode.port = port;
    }
  }

  if(strlen(newNode.IP) <= 0){
    printf("\n\t~ IP not found :( ~\n");
    exit(1);
  }

  printf("I am router %d, IP %s:%d\n", newNode.id, newNode.IP, newNode.port);
  fclose(file);
}

void readLinksFile(){
  FILE *file;
  newNode.nextNodes = malloc(sizeof(int)*MaxNumberNodes);

  if(( file = fopen("enlaces.config", "r")) == NULL){
    printf("\n\t~ enlaces.config: File could not be opened :( ~\n");
    exit(1);
  }

  int source, dest, weight;
  struct Graph* graph = createGraph(nodeList.len); 

  // Reading file and adding edges to graph
  while(fscanf(file, "%d %d %d", &source, &dest, &weight) != EOF ){
    addEdge(graph, source, dest, weight);
  }

  // Dijkstra will return all the paths we need
	dijkstra(graph, newNode.id, newNode.nextNodes);
  
  /*
  for(int i=0;i<nodeList.len;i++)
    printf("\nTo arrive %i, sendo to %d", i, newNode.nextNodes[i]);
  printf("\n");
  */

  fclose(file);
}

int main(void){
  pthread_t tSend, tReceive;

  readFile();
  readLinksFile();
  pthread_create(&tReceive, NULL, socketReceive, NULL);
  pthread_create(&tSend, NULL, socketSend, NULL);

  pthread_join(tReceive, NULL);
  pthread_join(tSend, NULL);
  printf("Threads returned\n");

  return 0;

}