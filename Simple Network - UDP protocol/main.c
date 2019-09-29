#include <stdio.h>        //printf
#include <string.h>       //memset
#include <stdlib.h>       //exit(0);
#include <pthread.h>      //thread
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>         //time
#include "dijkstra.c"

//arrumar SERVER, BufferMaxLength
typedef struct{
  int id;
  char IP[15];
  int port;
}Node;

typedef struct{
  Node nodes[99];
  int len;
}NodeList;

typedef struct{
  int id;
  int sourceId;
  int destId;
  char data[100];
}Message;

//#define BufferMaxLength 512
Node newNode;
NodeList nodeList;

void die(char *s){
  perror(s);
  exit(1);
}

void *socketSend(void *data){
  int messageId=1, port=-1;
  //Instanciando socket para a thread
  struct sockaddr_in newSocket;
  int s, socketLength = sizeof(newSocket);
  
  while(1){
    Message *buffer = (Message *)malloc(sizeof(Message));
    Message *msg = (Message *)malloc(sizeof(Message));

    printf("\nSend a message to id: ");
    scanf("%d", &msg->destId);

    for(int i = 0; i!=nodeList.len; i++){
      if(nodeList.nodes[i].id == msg->destId)
        port = nodeList.nodes[i].port;
    }

    if(port == -1){
      printf("\n\t~ id NOT found :( ~\n");
      continue;
    }
    
    // IPPROTO_UDP -> definindo que é o protocolo UDP, create a UDP socket
    if( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1 )
      die("socket");

    //Inicializa - zero out the struture
    memset((char *) &newSocket, 0, sizeof(newSocket));
    newSocket.sin_family = AF_INET;
    newSocket.sin_port = htons(port);

    if(inet_aton(newNode.IP, &newSocket.sin_addr) == 0){
      fprintf(stderr, "inet_aton() failed\n");
      exit(1);
    }

    printf("Enter message : ");
    scanf("%s", msg->data);
    msg->sourceId = newNode.id;
    msg->id = messageId;

    //send the message
    if(sendto(s, msg, sizeof(Message), 0, (struct sockaddr *) &newSocket, socketLength) == -1)
      die("sendto()");

    //receive a reply and print it.
    //clear the buffer by filling null,
    //it might have previously received data
    memset(buffer, '\0', sizeof(Message));
    //try to receive some data, this is a blocking call
    if (recvfrom(s, buffer, sizeof(Message), 0, (struct sockaddr *) &newSocket, &socketLength) == -1)
      die("recvfrom()");

    messageId++;
    printf("\n\t~ %s%d ~\n", buffer->data, buffer->sourceId);
    //puts(buffer);
  }

  close(s);
}


void *socketReceive(void *data){
  struct sockaddr_in newSocket, otherSocket;
  int s, socketLength = sizeof(newSocket), receiveLength;

  //create a UDP socket
  if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
      die("socket");

  memset((char *) &newSocket, 0, sizeof(newSocket));

  newSocket.sin_family = AF_INET;
  newSocket.sin_port = htons(newNode.port);
  newSocket.sin_addr.s_addr = htonl(INADDR_ANY);

  //bind socket to port
  if( bind(s, (struct sockaddr*)&newSocket, sizeof(newSocket)) == -1)
    die("bind");
  
  while(1){
    Message *buffer = (Message *)malloc(sizeof(Message));

    fflush(stdout);
    memset(buffer, '\0', sizeof(Message));

    if((receiveLength = recvfrom(s, buffer, sizeof(Message), 0, (struct sockaddr *) &otherSocket, &socketLength)) == -1)
      die("recvfrom()");

    if(buffer->destId == newNode.id){
      printf("\n\n\t~ Received message %d from node %d ~\n", buffer->id, buffer->sourceId);
      printf("\t\tData: %s\n", buffer->data);

      buffer->destId = buffer->sourceId;
      buffer->sourceId = newNode.id;
      strcpy(buffer->data, "Confirmation: Message was received by node ");

      if(sendto(s, buffer, sizeof(Message), 0, (struct sockaddr*) &otherSocket, socketLength) == -1)
        die("sendto()");

      //printf("\nSend a message to id: ");

    }else{
      printf("ERROU");
    }

    
  }

  close(s);
}

void readFile(){
  FILE *file;
  printf("Searching my IP...\n");

  if(( file = fopen("roteadores.config", "r")) == NULL){
    printf("\n\t~ roteadores.config: File could not be opened :( ~\n");
    exit(1);
  }

  int id, port;
  char IP[15];
  nodeList.len = 0;

  while(fscanf(file, "%d %d %s", &id, &port, IP) != EOF ){
    int i = nodeList.len++;
    nodeList.nodes[i].id = id;
    strcpy(nodeList.nodes[i].IP, IP);
    nodeList.nodes[i].port = port;
  
    if(id == newNode.id){
      strcpy(newNode.IP, IP);
      newNode.port = port;
    }
  }

  if(strlen(newNode.IP) <= 0){
    printf("\n\t~ IP not found :( ~\n");
    exit(1);
  }

  printf("I am node %d, IP %s:%d\n", newNode.id, newNode.IP, newNode.port);
  fclose(file);
}

void readLinksFile(){
  FILE *file;

  if(( file = fopen("enlaces.config", "r")) == NULL){
    printf("\n\t~ enlaces.config: File could not be opened :( ~\n");
    exit(1);
  }

  int source, dest, weight;
  graph_t *g = calloc(1, sizeof (graph_t));

  while(fscanf(file, "%d %d %d", &source, &dest, &weight) != EOF ){
    add_edge(g, source, dest, weight);
  }
  //dijkstra(g, 0, dest);
  //print_paths(g);
/*
  int target = 4;
  printf("\nTo arrive %d, send to %d first\n", target, next_id(g, target));
*/

  fclose(file);
}

int main(void){
  pthread_t tSend, tReceive;

  printf("Who am I? ");
  scanf("%d", &newNode.id);

  readFile();
  readLinksFile();
  pthread_create(&tReceive, NULL, socketReceive, NULL);
  pthread_create(&tSend, NULL, socketSend, NULL);

  pthread_join(tReceive, NULL);
  pthread_join(tSend, NULL);
  printf("Threads returned\n");

  return 0;

}