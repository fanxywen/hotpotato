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

#define BACKLOG 20
#define MAXDATASIZE 2048

// //get sockaddr, IPV4 or IPV6

// void * get_in_addr(struct sockaddr * sa){
//   if(sa->sa_family == AF_INET){
//     return &(((struct sockaddr_in *)sa)->sin_addr);
//   }
//   return &(((struct sockaddr_in6 *)sa)->sin6_addr);
// }
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
    num = recv(fd, buffer, len-total, MSG_WAITALL);
    buffer = buffer + num;
    if(num == 0){
      perror("recv");
      exit(EXIT_FAILURE);
    }
    total += num;
  }
}

int main(int argc, char * argv[]){
  //srand( (unsigned int) time(NULL) + player_id );
  if(argc != 4){
    fprintf(stderr, "usage: ringmaster\n");
    return EXIT_FAILURE;
  }
   int num_player = atoi(argv[2]);
  if(num_player <= 1){
    fprintf(stderr, "num_player invalid\n");
    return EXIT_FAILURE;
  }
   int num_hops = atoi(argv[3]);
  if(num_hops < 0 || num_hops > 512){
    fprintf(stderr, "num_hops invalid\n");
    return EXIT_FAILURE;
  }
  printf("Potato Ringmaster\n");
  printf("players = %d\n", num_player);
  printf("hops = %d\n", num_hops);

  std::string potato;
  
  srand((unsigned int)time(NULL));
   int start_player = rand() % num_player;  

  char buffer[MAXDATASIZE];
  int status, num;
  int sockfd, new_fd;
  struct addrinfo host_info;
  struct addrinfo * host_list;
  const char* port = argv[1];
  int port_num = atoi(port);
  const char* hostname = NULL;
  socklen_t sin_size;
  struct sigaction sa;
  char s[INET6_ADDRSTRLEN];
  
 /*----------------------set up ringmaster ----------------------------------*/
  //addrinfo setup
  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family = AF_INET;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags = AI_PASSIVE;
  //host_info.ai_flags = AI_CANONNAME;
  //load up address struct
  if( (status = getaddrinfo(hostname, port, &host_info, &host_list)) != 0){
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(EXIT_FAILURE);
  }
  // make a socket;
  if( (sockfd = socket(host_list->ai_family, host_list->ai_socktype, host_list->ai_protocol)) == -1){
    fprintf(stderr, "socket error: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  //printf("ringmaster fd: %d\n", sockfd);
  int yes = 1;
  //lose "address already in use" error message;
  status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  if(status == -1){
    perror("setsockpot");
    exit(EXIT_FAILURE);
  }

  ((struct sockaddr_in*)(host_list->ai_addr))->sin_addr.s_addr = htons(INADDR_ANY);

  // bind it to INADDR_ANY
// struct sockaddr_in serveraddr;
// memset(&serveraddr, 0, sizeof(serveraddr));
// serveraddr.sin_family = AF_INET;
// serveraddr.sin_port = htons(port_num);
// serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);      
      /* receive multicast */
// if (bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
//         printf("recv bind()");
//         exit(EXIT_FAILURE);
// }
  if(bind(sockfd, host_list->ai_addr, host_list->ai_addrlen) == -1){
    fprintf(stderr, "bind() error: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }     
  freeaddrinfo(host_list);
  if(host_list == NULL){
    fprintf(stderr, "server: bind fail\n");
    exit(EXIT_FAILURE);
  }
  if(listen(sockfd, BACKLOG) == -1){
    perror("listen");
    exit(EXIT_FAILURE);
  }

  /*-----------------------process player connection request -----------------------------------*/

  char client_ip[1024];
  std::string client;
  char client_service[20];
  int player_fd[num_player];  //keep track of all players' file descriptor
  int count = 0;
  std::vector<std::string> names;
  while(1){
    struct sockaddr_storage their_addr;
    sin_size = sizeof(their_addr);
    new_fd = accept(sockfd, (struct sockaddr*)&their_addr, &sin_size);
    player_fd[count] = new_fd;
    if(new_fd == -1){
      perror("accept");
      continue;
    }else{

    getnameinfo((struct sockaddr*)&their_addr, sizeof(their_addr), client_ip, sizeof(client_ip), client_service, sizeof(client_service), 0);
    //    std::cout<<"clinet host name "<<client_ip<<std::endl;
    client.assign(client_ip);
    names.push_back(client);
    client.clear();
    printf("player %d is ready to play\n", count);

    //sprintf(buffer, "%d", count);
    //count = htons(count);
    if(send(player_fd[count], &count, sizeof(count), 0) == -1){
      perror("error sending player_id");
      exit(1);
    }
    //count = ntohs(count);
    num = recv(player_fd[count], buffer, sizeof(buffer), 0);
    //sprintf(buffer, "%d", num_player);
    //num_player = htons(num_player);
    if(send(player_fd[count], &num_player, sizeof(num_player), 0) == -1){
      perror("error sending num_player");
      exit(1);
    }
    //num_player = ntohs(num_player);

     num = recv(player_fd[count], buffer, sizeof(buffer), 0);
     //start_player = htons(start_player);
    if(send(player_fd[count], &start_player, sizeof(start_player), 0) == -1){
      perror("error sending start_player");
      exit(1);
    }
    //start_player = ntohs(start_player);
     num = recv(player_fd[count], buffer, sizeof(buffer), 0);
    //std::cout<<"num hops im sending.. "<<num_hops<<std::endl;

    //num_hops = htons(num_hops);
    if(send(player_fd[count], &num_hops, sizeof(num_hops), 0) == -1){
      perror("error sending number of hops");
      exit(1);
    }
    //num_hops = ntohs(num_hops);
     num = recv(player_fd[count], buffer, sizeof(buffer), 0);
    ++count;
    if(count == num_player){
      break;
    }
  }
  }
  count = 0;

  // for(int i = 0; i < names.size(); i++){
  //   std::cout<<names[i]<<std::endl;
  // }
  // char ipstr[INET_ADDRSTRLEN];
  // for(int i = 0; i < num_player; i++){
  //   recv(player_fd[i], ipstr, sizeof(ipstr), 0);
  //   std::string temp(ipstr);
  //   names.push_back(temp);
   
  // }

 for(int i = 0; i < num_player; i++){
  if(i != num_player - 1){
    std::string name = names[i + 1];
    send(player_fd[i], name.c_str(), sizeof(name), 0);

    num = recv(player_fd[i], buffer, sizeof(buffer), 0);
  }else{
    std::string name = names[0];
    send(player_fd[i], name.c_str(), sizeof(name), 0);
     num = recv(player_fd[i], buffer, sizeof(buffer), 0);
  }
 }

  /*--------------------------check num hops-----------------------------------------*/

  if(num_hops == 0){
    close(sockfd);
    for(int i = 0; i < num_player; i++){
      close(player_fd[i]);
    }
    return EXIT_SUCCESS;
  }

  /*-----------------------make sure all players become a server, able to start between player connection----------------------*/
 
 //receive player port number;
 std::vector< unsigned short int> port_list;

 count = 0;
 while(1){
   unsigned short int port_num;
   recv(player_fd[count], &port_num, sizeof(port_num), 0);
   port_num = ntohs(port_num);
   ++count;
   port_list.push_back(port_num);
   if(count == num_player){
    break;
   }
 }
 count = 0;
 // while(1){
 //    recv(player_fd[count], buffer, sizeof(buffer), 0);
 //    if(strcmp(buffer, "server") == 0){
 //      if(count != num_player - 1){
 //      unsigned short int num = port_list[count + 1];
 //      send(player_fd[count], &num, sizeof(num), 0);
 //      num = recv(player_fd[count], buffer, sizeof(buffer), 0);
 //       count++;
 //     }else{
 //      unsigned short int num = port_list[0];
 //      send(player_fd[count], &num, sizeof(num), 0);
 //      num = recv(player_fd[count], buffer, sizeof(buffer), 0);
 //       count++;
 //     }
 //    }
 //    if(count == num_player){
 //      break;
 //    }
 //  }
 while(1){
  if(count != num_player - 1){
      unsigned short int port = port_list[count + 1];
     port = htons(port);
      send(player_fd[count], &port, sizeof(port), 0);
       count++;
     }else{
      unsigned short int port = port_list[0];
      port = htons(port);
      send(player_fd[count], &port, sizeof(port), 0);
      
       count++;
     }
     if(count == num_player){
      break;
     }
 }
  count = 0;

  while(1){
    num = recv(player_fd[count], buffer, sizeof(buffer), 0);
    if(strcmp(buffer, "server") == 0){
      count++;
    }
    if(count == num_player){
      break;
    }
  }
  count = 0;
  while(1){
    strcpy(buffer, "connect");
    send(player_fd[count], buffer, sizeof(buffer), 0);
   // std::cout<<"send connect "<<std::endl;
    // num = recv(player_fd[count], buffer, sizeof(buffer), 0);
    count++;
    if(count == num_player){
      break;
    }
  }

 /*---------------------send out potato to start player ------------------------------------------------*/
  
  potato = std::to_string(num_hops);
  send(player_fd[start_player], potato.c_str(), sizeof(potato), 0);
  // num = recv(player_fd[start_player], buffer, sizeof(buffer), 0);
    
  std::cout<<"Ready to start the game, sending potato to player "<<start_player<<std::endl;

  /*---------------------set up select------------------------------*/
  
  int fdmax;
  fd_set master;
  fd_set read_fds;
  struct timeval tv;
  tv.tv_sec = 2;
  tv.tv_usec = 500000;
  FD_ZERO(&master);
  FD_ZERO(&read_fds);
  //  FD_SET(sockfd, &master);
  fdmax = sockfd;
  for(int i = 0; i < num_player; ++i){
    FD_SET(player_fd[i], &master);
    if(player_fd[i] > fdmax) fdmax = player_fd[i];
  }

  /*---------------------main loop------------------------------*/
  
  while(1){
  read_fds = master;
  if(select(fdmax + 1, &read_fds, NULL, NULL, &tv) == -1){
    //perror("select");
    //    exit(EXIT_FAILURE);
    break;
  }
  for(int i = 0; i <= fdmax; ++i){
    if(FD_ISSET(i, &read_fds)){
      int length;
      recv(i, &length, sizeof(length), MSG_WAITALL);
      // int length = atoi(numberbuffer);
      //length = ntohs(length);
      char potatobuffer[length];
      recv(i, potatobuffer, length, MSG_WAITALL); 
      std::cout<<"Trace of potato: "<<std::endl;
      std::cout<<potatobuffer<<std::endl;
      for(int i = 0; i < num_player; i++){
          send(player_fd[i], "end", 4, 0);
	            close(player_fd[i]);
      }
      break;
      break;
    }
  }
}
  close(sockfd);
  /*  for(int i = 0; i < num_player; i++){
    close(player_fd[i]);
    }*/
  return EXIT_SUCCESS;
}
