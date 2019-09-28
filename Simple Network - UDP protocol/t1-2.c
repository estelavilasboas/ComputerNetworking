#include<stdio.h>  //printf
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include<pthread.h> //thread
#include<arpa/inet.h>
#include<sys/socket.h>

#define SERVER "127.0.0.1"
#define BufferMaxLength 512

void die(char *s){
  perror(s);
  exit(1);
}

void *socketSend(void *data){
  int PORT = 8000;
  //Instanciando socket para a thread
  struct sockaddr_in newSocket;
  int s, socketLength = sizeof(newSocket);
  char buffer[BufferMaxLength];
  char message[100];

  // IPPROTO_UDP -> definindo que é o protocolo UDP, create a UDP socket
  if( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1 )
    die("socket");

  //Inicializa - zero out the struture
  memset((char *) &newSocket, 0, sizeof(newSocket));
  newSocket.sin_family = AF_INET;
  newSocket.sin_port = htons(PORT);

  if(inet_aton(SERVER, &newSocket.sin_addr) == 0){
    fprintf(stderr, "inet_aton() failed\n");
    exit(1);
  }

  while(1){
    printf("Enter message : ");
    scanf("%s", message);

    //send the message
    if(sendto(s, message, strlen(message), 0, (struct sockaddr *) &newSocket, socketLength) == -1)
      die("sendto()");

    //receive a reply and print it.
    //clear the buffer by filling null,
    //it might have previously received data
    memset(buffer, '\0', BufferMaxLength);
    //try to receive some data, this is a blocking call
    if (recvfrom(s, buffer, BufferMaxLength, 0, (struct sockaddr *) &newSocket, &socketLength) == -1)
      die("recvfrom()");

    puts(buffer);
  }

  close(s);
}

void *socketReceive(void *data){
  int PORT = 8001;
  struct sockaddr_in newSocket, otherSocket;
  int s, socketLength = sizeof(newSocket), receiveLength;
  char buffer[BufferMaxLength];

  //create a UDP socket
  if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
      die("socket");

  memset((char *) &newSocket, 0, sizeof(newSocket));

  newSocket.sin_family = AF_INET;
  newSocket.sin_port = htons(PORT);
  newSocket.sin_addr.s_addr = htonl(INADDR_ANY);

  //bind socket to port
  if( bind(s, (struct sockaddr*)&newSocket, sizeof(newSocket)) == -1)
    die("bind");

  while(1){
    //printf("Waiting for data...")
    fflush(stdout);
    memset(buffer, '\0', BufferMaxLength);

    if((receiveLength = recvfrom(s, buffer, BufferMaxLength, 0, (struct sockaddr *) &otherSocket, &socketLength)) == -1)
      die("recvfrom()");

    printf("Received \n");
    printf("Data: %s\n", buffer);

    if(sendto(s, buffer, receiveLength, 0, (struct sockaddr*) &otherSocket, socketLength) == -1)
      die("sendto()");
  }

  close(s);
}

/*void serverUDP(){
  
}*/

int main(void){
  pthread_t tSend, tReceive;
  pthread_create(&tSend, NULL, socketSend, NULL);
  pthread_create(&tReceive, NULL, socketReceive, NULL);

  pthread_join(tSend, NULL);
  pthread_join(tReceive, NULL);
  printf("Threads returned\n");

  return 0;

}