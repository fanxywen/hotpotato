#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "potato.h"
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/time.h>
#include <algorithm>
#include <utility>
#include <errno.h>

#define MAXDATASIZE 2048
#define BACKLOG 20

int leftneigh_port;
unsigned short int rightneigh_port;
int self_fd;
int leftneigh_fd;
int rightneigh_fd;
int num_player;
int player_id;
int sockfd;
int start_player;
int num_hops;
int hopzero;
int leftneigh_id;
int rightneigh_id;


std::pair<int, int> leftneigh;
std::pair<int, int> rightneigh;


void sendall(int fd, char* buffer, int len){
  int total = 0;
  int num;
  while(total < len){
    num = send(fd, buffer, len - total, 0);
    buffer = buffer + num;
    total += num;
  }
}

void recvall(int fd, char* buffer, int len){
  int total = 0;
  int num;
  while(total < len){
    num = recv(fd, buffer, len - total, MSG_WAITALL);
    buffer = buffer + num;
    if(num == 0){
      perror("recv");
      exit(EXIT_FAILURE);
    }
    total += num;
  }
}

std::pair<int, int> & passPotato(){
  if(rand()%2 == 1){
    return rightneigh;
  }
  return leftneigh;
}

void * get_in_addr(struct sockaddr * sa){
  if(sa->sa_family == AF_INET){
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int connect_neighbour(int neigh_port, const char * hostname){
  //std::cout<<"neigh_port"<<neigh_port<<std::endl;
  //std::cout<<"check hostname "<<hostname<<std::endl;
  char temp[100];
  struct addrinfo neighbour, *serv, *p;
  int rv;
  int server_fd;
  int errnum;
   memset(&neighbour, 0, sizeof neighbour);
    neighbour.ai_family = AF_INET;
    neighbour.ai_socktype = SOCK_STREAM;
    neighbour.ai_flags = AI_CANONNAME;
  sprintf(temp, "%d", neigh_port);
 if ((rv = getaddrinfo(hostname, temp, &neighbour, &serv)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }
  for(p = serv; p != NULL; p = p->ai_next) {
        if ((server_fd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        if (connect(server_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(server_fd);
            perror("client: connect");
            continue;
        }

        break;
    }
   if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 0;
    }
   //printf("the socket im connecting to: %d\n", server_fd);
   return server_fd;
}

int main(int argc, char * argv[]){
    srand((unsigned int)time(NULL));
    int  num;
    int new_fd;
    char buffer[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    char client_ip[2048];
    int length;
    char numberbuffer[8];

    if (argc != 3) {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }
    
    /*------------------------connecting to ringmaster------------------------------*/
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;
    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

     freeaddrinfo(servinfo); // all done with this structure

      
      num = recv(sockfd, &player_id, sizeof(player_id), 0);
      //player_id = ntohs(player_id);
      strcpy(buffer, "player_id received");
      send(sockfd, buffer, sizeof(buffer), 0);

      num = recv(sockfd, &num_player, sizeof(num_player), 0);
      //num_player = ntohs(num_player);
      strcpy(buffer, "number of players received");
      send(sockfd, buffer, sizeof(buffer), 0);

            std::cout<<"Connected as player "<<player_id<<" out of "<<num_player<<" total player "<<std::endl;
     
      num = recv(sockfd, &start_player, sizeof(start_player), 0);
      //start_player = ntohs(start_player);
      strcpy(buffer, "start player received");
      send(sockfd, buffer, sizeof(buffer), 0);

      num = recv(sockfd, &num_hops, sizeof(num_hops), 0);
      //num_hops = ntohs(num_hops);
      //std::cout<<"recv num hops "<<num_hops<<std::endl;
      strcpy(buffer, "number of hops received");
      send(sockfd, buffer, sizeof(buffer), 0);
      if(player_id == 0){
        leftneigh_id = num_player - 1;
        rightneigh_id = player_id + 1;
      }else{
        if(player_id == num_player - 1){
          rightneigh_id = 0;
          leftneigh_id = player_id - 1;
        }else{
          rightneigh_id = player_id + 1;
          leftneigh_id = player_id - 1;
        }
      } 

      //std::cout<<"leftneigh_id "<<leftneigh_id<<" rightneigh_id "<<rightneigh_id<<std::endl;

      std::string client;
      char client_name[1024];
      num = recv(sockfd, client_name, sizeof(client_name), 0);
      client_name[num] = '\0';
      client.assign(client_name);

      // std::cout<<"clinet ip "<<client<<std::endl;
      strcpy(buffer, "hostname received");
      send(sockfd, buffer, sizeof(buffer), 0);
      

    /*-----------------------------turn player into server---------------------------------*/
   
      //    printf("turn myself into a server..\n");
    struct addrinfo host_info;
    struct addrinfo * host_list;
    int status;
    int self_fd;
    socklen_t sin_size;
    const char * hostname = NULL;

    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family = AF_INET;
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_flags = AI_PASSIVE;
    

    
	  //int port_num = maseterport + player_id;
    unsigned short int port_num = 0;
	  char temp[20];
	  sprintf(temp, "%d", port_num);
    if( (status = getaddrinfo(hostname, temp, &host_info, &host_list)) != 0){
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(EXIT_FAILURE);
    }
    // make a socket;
    if( (self_fd = socket(host_list->ai_family, host_list->ai_socktype, host_list->ai_protocol)) == -1){
    fprintf(stderr, "socket error: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
    }
    //printf("sock estabilshed %d\n", player_id);
	  int yes = 1;
  //lose "address already in use" error message;
  status = setsockopt(self_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  if(status == -1){
    perror("setsockpot");
    exit(EXIT_FAILURE);
  }
  

   // struct in_addr ipAddr= ((struct sockaddr_in*)(host_info.ai_addr))->sin_addr;
   // char ipstr[INET_ADDRSTRLEN];
   // inet_ntop(AF_INET, &ipAddr, ipstr, INET_ADDRSTRLEN);
   // std::cout<<"player ip "<<ipstr<<std::endl;
  //((struct sockaddr_in*)(host_list->ai_addr))->sin_addr.s_addr = INADDR_ANY;
  // bind it to INADDR_ANY and port specified as 0, so the system will automatically assign available port to it.

  ((struct sockaddr_in*)(host_list->ai_addr))->sin_addr.s_addr = htons(INADDR_ANY);

  if(bind(self_fd, host_list->ai_addr, host_list->ai_addrlen) == -1){
    perror("bind");
    exit(EXIT_FAILURE);
  }
  
  if(getsockname(self_fd, host_list->ai_addr, &(host_list->ai_addrlen)) == -1){
    perror("getsockname");
    exit(EXIT_FAILURE);
  }
  
  if(host_list->ai_family == AF_INET){
     port_num = ntohs(((struct sockaddr_in *)(host_list->ai_addr))->sin_port);
  }
    else{
      
  port_num = ntohs(((struct sockaddr_in6 *)(host_list->ai_addr))->sin6_port);
    }
 
  
  //  std::cout<<"system assigned port number "<<port_num<<std::endl;

  
  freeaddrinfo(host_list);
  if(host_list == NULL){
    fprintf(stderr, "server: bind fail\n");
    exit(EXIT_FAILURE);
  }

  if(listen(self_fd, BACKLOG) == -1){
    perror("listen");
    exit(EXIT_FAILURE);
  }
  //std::cout<<"im listening "<<std::endl;
  // send(sockfd, ipstr, sizeof(ipstr), MSG_WAITALL);

  // std::string client;
  // num = recv(sockfd, ipstr, sizeof(ipstr), MSG_WAITALL);
  // client.assign(ipstr);
  // std::cout<<"neighbour ip "<<client<<std::endl;
  
  port_num = htons(port_num);
  send(sockfd, &port_num, sizeof(port_num), 0);
 
  num = recv(sockfd, &rightneigh_port, sizeof(rightneigh_port), 0);
  rightneigh_port = ntohs(rightneigh_port);
  //  std::cout<<"number of bytes "<<num<<std::endl;
  //std::cout<<"port number"<<rightneigh_port<<std::endl; 
  

  strcpy(buffer, "server");
  send(sockfd, buffer, sizeof(buffer), 0);
  //std::cout<<"I am a server now "<<std::endl;

  num = recv(sockfd, buffer, sizeof(buffer), 0);
  buffer[num] = '\0';
  //  std::cout<<buffer<<std::endl;
  if(strcmp(buffer, "connect") == 0){
    //      std::cout<<buffer<<std::endl;
}
  
  if(num_hops == 0){
        //std::cout<<"end game"<<std::endl;
        close(sockfd);
        return EXIT_SUCCESS;
      }

  /*---------------------set up player connection------------------------------*/
  
  //  printf("start connecting neighbour...\n");

  if(player_id == 0){
     if((rightneigh_fd = connect_neighbour(rightneigh_port, client.c_str())) > 0){
      //printf("connect to neighbour player %d\n", player_id + 1);
    }
   struct sockaddr_storage their_addr;
   sin_size = sizeof(their_addr);
   leftneigh_fd = accept(self_fd, (struct sockaddr*)&their_addr, &sin_size);
  }else{
    //std::cout<<"I'm here trying to accept "<<std::endl;
   struct sockaddr_storage their_addr;
   sin_size = sizeof(their_addr);
   leftneigh_fd = accept(self_fd, (struct sockaddr*)&their_addr, &sin_size);
   //printf("accept connection: neighbour fd: %d\n", leftneigh_fd);
   if((rightneigh_fd = connect_neighbour(rightneigh_port, client.c_str())) > 0){
     //printf("connect to neighbour player %d\n", player_id + 1);
   }
   }

   //std::cout<<"rightneigh_fd "<<rightneigh_fd<<std::endl;
   //std::cout<<"leftneigh_fd "<<leftneigh_fd<<std::endl;

   leftneigh = std::make_pair(leftneigh_id, leftneigh_fd);
   rightneigh = std::make_pair(rightneigh_id, rightneigh_fd);

  std::string potato;
  std::string trace;
  
  /*--------------------start player receive potato and start game---------------*/
  
  if(player_id == start_player){
  //std::cout<<"I'm the one to start\n";
  num = recv(sockfd, &buffer, sizeof(buffer), 0);
  buffer[num] = '\0';
  potato.assign(buffer);
  //std::cout<<"potato "<<potato<<std::endl;
  num_hops = std::stoi(potato, nullptr, 10);
  

  std::string temp = std::to_string(player_id);
  trace = trace + temp + ",";
  --num_hops;

  /*-------------------vaildate hop number, then act accordingly------------------*/
  
  if(num_hops == 0){
  std::cout << "I'm it !" << std::endl;
  send(sockfd, trace.c_str(), sizeof(trace.c_str()), 0);
  }else{
    std::string t = std::to_string(num_hops);    
    std::pair<int, int> temp = passPotato();
    potato.clear();
    potato = t + "/" + trace;
    
    int size = potato.size() + 1;
//    size = htons(size);
    //std::cout<<"size "<<size<<std::endl;
    //std::cout<<"potato "<<potato<<std::endl;
    send(temp.second, &size, sizeof(size), 0);
    char cstring[potato.size() + 1];
    strcpy(cstring, potato.c_str());
    sendall(temp.second, cstring, sizeof(cstring));
    std::cout<<"sending potato to " << temp.first <<std::endl;
  }
  }

  
  /* ------------------select-----------------------------------------------------*/

  int fdmax;
  fd_set master;
  fd_set read_fds;
  struct timeval tv;
  tv.tv_sec = 2;
  tv.tv_usec = 500000;
  FD_ZERO(&master);
  FD_ZERO(&read_fds);
  FD_SET(sockfd, &master);
  FD_SET(leftneigh_fd, &master);
  FD_SET(rightneigh_fd, &master);
  fdmax = std::max(std::max(sockfd, leftneigh_fd), rightneigh_fd);

  int num_ready;
  int str_ready;
  
  /*------------------ loop ------------------------------------------------------*/

   while(1){
    read_fds = master;
    if(select(fdmax + 1, &read_fds, NULL, NULL, &tv) == -1){
      //perror("select")
      break;
  }

  for(int i = 0; i <= fdmax; ++i){
    if(FD_ISSET(i, &read_fds)){
    
    if(i == leftneigh_fd || i == rightneigh_fd){
      int length;
      num = recv(i, &length, sizeof(length), MSG_WAITALL);
      //std::cout<<"byte "<<num<<std::endl;
      //std::cout<<"the length "<<length<<std::endl;
      char potatobuf[length];
      //recv(i, potatobuf, length, MSG_WAITALL);
      recvall(i, potatobuf, length);
      potatobuf[length] = '\0';
      std::string pp(potatobuf);
      size_t pos = pp.find('/');
       int hops = stoi(pp.substr(0, pos), nullptr, 10);
      pp = pp.substr(pos+1);
      pp = pp + std::to_string(player_id) + ",";
      --hops;
      if(hops == 0){
        int leng = pp.size() + 1;
        send(sockfd, &leng, sizeof(leng), 0);
        char str[leng];
        strcpy(str, pp.c_str());
        //std::cout<<"trace "<<pp<<std::endl;
        sendall(sockfd, str, leng);
        std::cout<<"I'm it! "<<std::endl;
      }else{
        std::pair<int, int> temp = passPotato();
        std::string hh = std::to_string(hops);
        pp = hh + "/" + pp;
         int sz = pp.size() + 1;

      //  sz = htons(sz);
         //std::cout<<"potato size "<<sz<<std::endl;
        send(temp.second, &sz, sizeof(sz), 0);
        char strtemp[sz];
        strcpy(strtemp, pp.c_str());
       // std::cout<<"now potato "<<pp<<std::endl;
        //sendall(temp.second, strtemp, sizeof(strtemp));
        sendall(temp.second, strtemp, sz);
        std::cout<<"sending potato to player "<<temp.first<<std::endl;
      }
    }else{
      // recv(sockfd, buffer, sizeof(buffer), MSG_WAITALL);
            close(sockfd);
      break;
      break;
    }
    }
    }
      }
   //   close(sockfd);
       close(self_fd);
       close(rightneigh_id);
       close(leftneigh_fd);
    return 0;
}











