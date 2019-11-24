/* notes:
FIX IT
OLD CODE
TEST
TO DO
desativar enlaces com base no timestamp
printar nós desativados
*/

#include <stdio.h>        //printf
#include <string.h>       //memset
#include <stdlib.h>       //exit(0);
#include <pthread.h>      //thread
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <time.h>         //time

#define MaxNumberNodes 10
#define Timeout 1000
#define SendAfterTimeout 5

enum MessageType{
  ConfirmationMsg,
  DataMsg,
  DistanceMsg
};

typedef struct{
  int node;
  int distance;
}DistanceNode;

typedef struct{
  int id;
  char IP[15];
  int port;
  int *nextNodes; // OLD CODE
  DistanceNode *distanceVector;
  int distanceVectorLen;
  bool active;
  clock_t timestamp;
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
bool vectorUpdated = false;
bool sendVectorTimeout = false;
char* vectorToString();
int stringToVector(char *string);

void die(char *s){
  perror(s);
  exit(1);
}

int getPort(Message *msg){ // OLD CODE
  int port = -1;
  for(int i = 0; i!=nodeList.len; i++){
    if(nodeList.nodes[i].id == newNode.nextNodes[msg->destId])
      port = nodeList.nodes[i].port;
  }

  if(port == -1)
    printf("\n\t~ id NOT found :( ~\n");
  
  return port;
}

void nodesTimeout(int i){
  clock_t clock1;
  clock1 = clock();
  if( ((clock1 - nodeList.nodes[i].timestamp)*1000/CLOCKS_PER_SEC) >= 3*Timeout ){
    nodeList.nodes[i].active = false;
    //printf("\n\n~ Router %d is not active", nodeList.nodes[i].id);
  }
}

int getDistance(int id){
  for(int i=0; i<newNode.distanceVectorLen; i++){
    if(id == newNode.distanceVector[i].node)
      return newNode.distanceVector[i].distance;
  }
}

// get node position in newNode's distance vector
int getPositionDistVector(int id){
  for(int i=0; i<newNode.distanceVectorLen; i++){
    if(id == newNode.distanceVector[i].node)
      return i;
  }
  return -1;
}

// get node position in nodeList
int getPositionNodeList(int id){
  for(int i=0; i<nodeList.len; i++){
    if(id == nodeList.nodes[i].id)
      return i;
  }
  return -1;
}

void addDistanceVector(int target, int weight){
  int position = newNode.distanceVectorLen++;
  newNode.distanceVector[position].node = target;
  newNode.distanceVector[position].distance = weight;
}

void showDistanceVector() {
  printf("\t(Node, Distance) = [");
  for(int i=0; i<newNode.distanceVectorLen; i++){
    printf(" ( %d, %d ) ", newNode.distanceVector[i].node, newNode.distanceVector[i].distance);
  }
  printf("]");
}

// Bellman-Ford result analysis section:
// get destination position in the nodeList
// then we can get ports and IPs
int getDestinationPosition(int distance, int destId){
  int position = 0;

  // check all neighbours
  for( ; position<nodeList.len; position++){
    if(nodeList.nodes[position].id == destId){
      // find newNode position in neighbour's distance vector
      int newNodepos = 0;
      for( ; newNodepos < nodeList.nodes[position].distanceVectorLen; newNodepos++){
        if(newNode.id == nodeList.nodes[position].distanceVector[newNodepos].node)
          break;
      }

      // maybe neighbour's distance vector was never received
      if(nodeList.nodes[position].active == false){
        return -1;
      }
        
      // check distance of newNode to neighbour
      if(distance == nodeList.nodes[position].distanceVector[newNodepos].distance)
        return position;

    }else{
      int i = 0;
      bool foundDestination = false;
      // find position of destination node in neighbour's distance vetor
      for( ; i < nodeList.nodes[position].distanceVectorLen; i++){
        if(destId == nodeList.nodes[position].distanceVector[i].node){
          foundDestination = true;
          break;
        }
        // if i = nodeList.nodes[position].distanceVectorLen-1: destination not found
      }

      if(!foundDestination)
        continue;

      // neighbour's distance vector
      int nbDistanceVector = nodeList.nodes[position].id;
      // distance of neighbour to destination
      int compareDistance = nodeList.nodes[position].distanceVector[i].distance;
      // distance of newNode to neighbour
      int distanceToNb = getDistance(nbDistanceVector);

      if(compareDistance+distanceToNb == distance)
        return position;
    }
  }
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

// received vector and neighbour id
void updateVector(int nbPosition, int nbId){
  int linkDistance = getDistance(nbId);
  int receivedVectorLen = nodeList.nodes[nbPosition].distanceVectorLen;
  DistanceNode *receivedVector = nodeList.nodes[nbPosition].distanceVector;
  bool updated = false;

  for(int i=0; i < receivedVectorLen; i++){
    if(receivedVector[i].node == newNode.id)
      continue;

    else if(receivedVector[i].distance == 0)
      continue;

    else{
      // find position in newNode's distance vector
      int position = getPositionDistVector(receivedVector[i].node);

      // newNode didn't knew a node 
      if(position == -1){
        newNode.distanceVector[newNode.distanceVectorLen].node = receivedVector[i].node;
        newNode.distanceVector[newNode.distanceVectorLen].distance = receivedVector[i].distance + linkDistance;
        newNode.distanceVectorLen++;
        updated = true;
        continue;
      }

      if( (receivedVector[i].distance + linkDistance) < (newNode.distanceVector[position].distance) ){
        newNode.distanceVector[position].distance = receivedVector[i].distance + linkDistance;
        updated = true;
      }
    }
  }

  if(updated){
    vectorUpdated = true;
    printf("\n\n\t~ Distance vector updated ~\n");
    showDistanceVector();
  }
}


void *socketSendVector(){
  int port = -1;
  struct sockaddr_in newSocket;
  int s, len = nodeList.len, socketLength = sizeof(newSocket);
  char IP[15];

  // create message
  Message *msg = (Message *)malloc(sizeof(Message));
  clock_t clock2;

  while(1){
    // handling timeout
    clock2 = clock();
    
    if( ( (clock2 - newNode.timestamp)*1000/CLOCKS_PER_SEC ) >= Timeout ){
      sendVectorTimeout = true;
    }

    // if newNode vector was updated, send again
    if( vectorUpdated || sendVectorTimeout ){
      strcpy(msg->data, vectorToString());
      //printf("\n Sending distance vector: %s", msg->data);

      for(int i=0; i < len; i++){
        nodesTimeout(i);

        msg->destId = nodeList.nodes[i].id;
        msg->type = DistanceMsg;
        msg->sourceId = newNode.id;

        // Define/Create UDP socket
        if( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1 )
          die("socket");
        
        port = nodeList.nodes[i].port;
        strcpy(IP, nodeList.nodes[i].IP);

        // Zero out the struture
        memset((char *) &newSocket, 0, sizeof(newSocket));
        newSocket.sin_family = AF_INET;
        newSocket.sin_port = htons(port);

        if(inet_aton(IP, &newSocket.sin_addr) == 0){
          fprintf(stderr, "inet_aton() failed\n");
          exit(1);
        }

        // Send the message
        if(sendto(s, msg, sizeof(Message), 0, (struct sockaddr *) &newSocket, socketLength) == -1)
          die("sendto()");
      }

      newNode.timestamp = clock();
      sendVectorTimeout = false;
      vectorUpdated = false;

      // Reset message
      memset(msg, '\0', sizeof(Message));
    }
  }

  free(msg);
  close(s);
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

    // set destination node
    printf("\nSend a message to id: ");
    scanf("%d", &msg->destId);

    // destination position in nodeList
    // if destination node is not a neighbour, the function returns the next node
    int destPosition = getDestinationPosition(getDistance(msg->destId), msg->destId);
    
    if(destPosition == -1)
      printf("~ destination unreachable");

    else{
      port = nodeList.nodes[destPosition].port;
      strcpy(IP, nodeList.nodes[destPosition].IP);

      // Search port OLD CODE
      //if( (port = getPort(msg)) == -1)
      //  continue;
      
      // Search IP  OLD CODE
      //for(int i = 0; i!=nodeList.len; i++){
      //  if(nodeList.nodes[i].id == newNode.nextNodes[msg->destId])
      //    strcpy(IP, nodeList.nodes[i].IP);
      //}
      
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

      //msg->data = vectorToString();
      //msg->type = DistanceMsg;// TEST
      printf("~Message will be send to router %d. Destination: router %d\n", nodeList.nodes[destPosition].id, msg->destId);

      // Send the message
      if(sendto(s, msg, sizeof(Message), 0, (struct sockaddr *) &newSocket, socketLength) == -1)
        die("sendto()");

      clock1 = clock();
      waitingConfirm = true;

      // Try to send the message again (timeout)
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

        // destination position in nodeList
        // if destination node is not a neighbour, the function returns the next node
        int positionNodeList = getDestinationPosition(getDistance(buffer->destId), buffer->destId);
        char IP[15];

        port = nodeList.nodes[positionNodeList].port;
        strcpy(IP, nodeList.nodes[positionNodeList].IP);

        /*if( (port = getPort(buffer)) == -1) // OLD CODE
          continue;*/

        otherSocket.sin_port = htons(port);

        // Send the confirmation message to the source
        if(sendto(s, buffer, sizeof(Message), 0, (struct sockaddr*) &otherSocket, socketLength) == -1)
          die("sendto()");

      }else if(buffer->type == DistanceMsg){
        // Received a distance vector from a neighbour
        //printf("\n%s\n", buffer->data); // TEST

        // return position in newNode's distance vector 
        int position = stringToVector(buffer->data);
        // update newNode's distance vector
        updateVector(position, buffer->sourceId);

        position = getPositionNodeList(buffer->sourceId);
        nodeList.nodes[position].timestamp = clock();
        nodeList.nodes[position].active = true;

      }else{
        waitingConfirm = false;
        printf("\n\n\t\t~ Confirmation ~\n");
        printf("\t%s%d\n", buffer->data, buffer->sourceId);
      }

    }else{
      int s, port = -1;
      char IP[15];

      // destination position in nodeList
      // if destination node is not a neighbour, the function returns the next node
      int positionNodeList = getDestinationPosition(getDistance(buffer->destId), buffer->destId);
      
      if(positionNodeList == -1)
        printf("\n\n~ Cannot redirect message: destination unreachable");

      else{
        port = nodeList.nodes[positionNodeList].port;
        strcpy(IP, nodeList.nodes[positionNodeList].IP);

        /*// Search port OLD CODE
        if( (port = getPort(buffer)) == -1)
          continue;*/

        /*// Search IP OLD CODE
        for(int i = 0; i!=nodeList.len; i++){
          if(nodeList.nodes[i].id == newNode.nextNodes[buffer->destId])
            strcpy(IP, nodeList.nodes[i].IP);
        }*/

        printf("\n\n~ Redirecting message: %d to router %d", buffer->sourceId, nodeList.nodes[positionNodeList].id);
        // Create a UDP socketnewNode.nextNodes[buffer->destId]
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
  }

  close(s);
}

char* vectorToString(){
  char* string = malloc(sizeof(char)*(MaxNumberNodes*2));
  for(int i=0; i<newNode.distanceVectorLen; i++){
    int target = newNode.distanceVector[i].node;
    int weight = newNode.distanceVector[i].distance;

    char auxTarget[2], auxWeight[2];
    sprintf(auxTarget, "%d", target);
    sprintf(auxWeight, "%d", weight);

    strcat(string, auxTarget);
    strcat(string, ":");
    strcat(string, auxWeight);
    strcat(string, " ");
  }
  return string;
}

int stringToVector(char* string){
  DistanceNode *distanceVector = malloc(sizeof(DistanceNode)*MaxNumberNodes);
  char *item = malloc(strlen(string));
  char *stringCopy = malloc(strlen(string));
  int owner = -1, position = 0, node=0, distance=0;
  int i = 0;

  strcpy(stringCopy, string);

  while(stringCopy!=NULL){
    //example: "1:0 0:3"
    strcpy(item, strtok(stringCopy, " "));   // "1:0"
    strtok(item, ":");                       // "1"
    node = atoi(item);
    item = strtok(NULL, ":");                // "0"
    distance = atoi(item);

    strcpy(stringCopy, string);           // "1:0 0:3"
    strtok(stringCopy, " ");              // "1:0"
    for(int jump=0; jump < i+1; jump++)
      stringCopy = strtok(NULL, " ");     // "0:3"

    if(distance == 0){
      owner = node;
      for( ; position < nodeList.len; position++){
        if(owner == nodeList.nodes[position].id)
          break;
      }
    }
    // insert values into distanceVector
    distanceVector[i].node = node;
    distanceVector[i].distance = distance;
    i++;

    //printf("\n%d | %d | %s | %d\n", node, distance, stringCopy, distanceVector[i-1].distance); // TEST
  }
  
  if(owner != -1){
    // insert distance vector into node distance vector
    nodeList.nodes[position].distanceVector = NULL;
    nodeList.nodes[position].distanceVector = distanceVector;
    nodeList.nodes[position].distanceVectorLen = i;
    nodeList.nodes[position].active = true;
    nodeList.nodes[position].timestamp = clock();
    nodeList.nodes[position].distanceVectorLen = i;

    /*
    printf("\n\t~ Received distance vetor from %d ~\n", nodeList.nodes[position].id);
    printf("\t(Node, Distance) = [");
    for(int j=0; j<nodeList.nodes[position].distanceVectorLen; j++){
      printf(" ( %d, %d ) ", nodeList.nodes[position].distanceVector[j].node, nodeList.nodes[position].distanceVector[j].distance);
    }
    printf("]");
    */
  }

  return position;
}

bool checkLink(int nodeId){ // é usado?
  for(int i=0; i<newNode.distanceVectorLen; i++){
    if(nodeId == newNode.distanceVector[i].node && newNode.distanceVector[i].distance != 0)
      return true;
  }
  return false;
}

void readFile(){
  FILE *file;

  //printf("Who am I? "); OLD CODE
  //scanf("%d", &newNode.id); OLD CODE
  printf("\nSearching my IP...\n");

  if(( file = fopen("roteadores.config", "r")) == NULL){
    printf("\n\t~ roteadores.config: File could not be opened :( ~\n");
    exit(1);
  }

  int id, port;
  char IP[15];
  
  nodeList.len = newNode.distanceVectorLen-1;
  int position = 0;

  // Reading the file and saving nodes data.
  while(fscanf(file, "%d %d %s", &id, &port, IP) != EOF ){
    if(id == newNode.id){
      // get newNode atributes
      strcpy(newNode.IP, IP);
      newNode.port = port;

    } else if(checkLink(id) && position<nodeList.len){
      nodeList.nodes[position].id = id;
      strcpy(nodeList.nodes[position].IP, IP);
      nodeList.nodes[position].port = port;
      //nodeList.nodes[position].nextNodes = NULL; // OLD CODE
      nodeList.nodes[position].active = false;
      position++;
      //printf("id: %d  %s:%d", nodeList.nodes[position].id, nodeList.nodes[position].IP, nodeList.nodes[position].port); // TEST
    }
  }

  if(strlen(newNode.IP) <= 0){
    printf("\n\t~ IP not found :( ~\n");
    exit(1);
  }

  printf("\tI am router %d, IP %s:%d\n", newNode.id, newNode.IP, newNode.port);
  fclose(file);
}

void readLinksFile(){
  FILE *file;
  newNode.nextNodes = malloc(sizeof(int)*MaxNumberNodes); // OLD CODE

  // initialize distance vector
  newNode.distanceVector = malloc(sizeof(DistanceNode)*MaxNumberNodes);
  newNode.distanceVectorLen = 0;

  if(( file = fopen("enlaces.config", "r")) == NULL){
    printf("\n\t~ enlaces.config: File could not be opened :( ~\n");
    exit(1);
  }

  printf("Who am I? ");
  scanf("%d", &newNode.id);
  printf("Checking links...\n");

  int source, dest, weight;
  //struct Graph* graph = createGraph(nodeList.len); OLD CODE

  // Add itself to distance vector
  addDistanceVector(newNode.id, 0);

  // Reading file and adding edges to graph
  while(fscanf(file, "%d %d %d", &source, &dest, &weight) != EOF ){
    //addEdge(graph, source, dest, weight); OLD CODE

    if(source == newNode.id)
      addDistanceVector(dest, weight);
    else if(dest == newNode.id)
      addDistanceVector(source, weight);
  }

  // Dijkstra will return all the paths we need OLD CODE
	//dijkstra(graph, newNode.id, newNode.nextNodes); OLD CODE
  
  /*
  for(int i=0;i<nodeList.len;i++)
    printf("\nTo arrive %i, sendo to %d", i, newNode.nextNodes[i]);
  printf("\n");
  */

  showDistanceVector();
  vectorUpdated = true;

  fclose(file);
}

int main(void){
  pthread_t tSend, tReceive, tDistanceVector;

  readLinksFile();
  readFile();
  //readLinksFile(); OLD CODE
  pthread_create(&tDistanceVector, NULL, socketSendVector, NULL);
  pthread_create(&tReceive, NULL, socketReceive, NULL);
  pthread_create(&tSend, NULL, socketSend, NULL);
  
  pthread_join(tReceive, NULL);
  pthread_join(tSend, NULL);
  pthread_join(tDistanceVector, NULL);
  printf("Threads returned\n");

  return 0;

}