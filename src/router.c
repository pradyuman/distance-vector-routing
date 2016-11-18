#include <netdb.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

#include "ne.h"
#include "router.h"

#define ERR -1
#define NUM_FDS 4

struct sockaddr_in createClient(char*, char*);
int createTimer(int);
int setTimer(int, int);
int updateSockset(int*, fd_set*);
void initRouter(int, struct sockaddr_in);
void updateRoutes(int, int);
void sendUpdates(int, struct sockaddr_in);
struct addrinfo *getAddrInfo(char*, int);
int bindListener(struct addrinfo*);

FILE* logfile;
unsigned int id;
unsigned int numNbrs;
struct nbr_cost initState[MAX_ROUTERS];
int failureTimeouts[MAX_ROUTERS];

int main(int argc, char **argv) {
  if (argc < 5) {
    fprintf(stderr, "USAGE ./router <router id> <ne hostname> <ne UDP port> <router UDP port>\n");
    return 1;
  }

  id = strtol(argv[1], NULL, 10);

  // Setup logging
  char logfilename[12];
  sprintf(logfilename, "router%d.log", id);
  logfile = fopen(logfilename, "w");

  int server_fd = bindListener(getAddrInfo(argv[4], SOCK_DGRAM));
  if (server_fd < 0) {
    fprintf(stderr, "[main:18:socket] Failed to initialize socket (UDP)\n");
    return 2;
  }

  struct sockaddr_in neClient = createClient(argv[2], argv[3]);

  // Initialize Router
  initRouter(server_fd, neClient);

  int update_fd = createTimer(UPDATE_INTERVAL);
  int converge_fd = createTimer(CONVERGE_TIMEOUT);
  int seconds_fd = createTimer(1);

  int seconds = 0;
  fd_set sockset;
  int fds[NUM_FDS] = { server_fd, update_fd, converge_fd, seconds_fd };
  while (1) {
    select(updateSockset(fds, &sockset) + 1, &sockset, NULL, NULL, NULL);

    if (FD_ISSET(server_fd, &sockset))
      updateRoutes(server_fd, converge_fd);

    if (FD_ISSET(update_fd, &sockset)) {
      sendUpdates(server_fd, neClient);
      setTimer(update_fd, UPDATE_INTERVAL);
    }

    if (FD_ISSET(converge_fd, &sockset)) {
      fprintf(logfile, "%d:Converged\n", seconds);
      fflush(logfile);
      setTimer(converge_fd, 0);
    }

    if (FD_ISSET(seconds_fd, &sockset)) {
      seconds++;
      setTimer(seconds_fd, 1);
    }

    int i;
    for (i = 0; i < numNbrs; i++) {
      if (FD_ISSET(failureTimeouts[i], &sockset)) {
        UninstallRoutesOnNbrDeath(initState[i].nbr);
        PrintRoutes(logfile, id);
        setTimer(failureTimeouts[i], 0);
      }
    }
  }

  return 0;
}

struct sockaddr_in createClient(char *hostname, char *port) {
  struct sockaddr_in client;
  memset(&client, 0, sizeof(client));
  client.sin_family = AF_INET;
  client.sin_port = htons((unsigned short)strtol(port, NULL, 10));
  inet_aton(hostname, &client.sin_addr);

  return client;
}

int getIdIndex(int nbr_id) {
  int i;
  for (i = 0; i < numNbrs; i++) {
    if (initState[i].nbr == nbr_id)
      return i;
  }
  return ERR;
}

int createTimer(int sec) {
  // Use TFD_NONBLOCK instead of 0 for versions above 2.6.27
  int fd = timerfd_create(CLOCK_MONOTONIC, 0);
  if (fd < 0)
    return ERR;

  if (setTimer(fd, sec) < 0)
    return ERR;

  return fd;
}

int setTimer(int fd, int sec) {
  struct itimerspec timeout;

  timeout.it_value.tv_sec = sec;
  timeout.it_value.tv_nsec = 0;
  timeout.it_interval.tv_sec = 0;
  timeout.it_interval.tv_nsec = 0;

  if (timerfd_settime(fd, 0, &timeout, NULL)) {
    fprintf(stderr, "[setTimer:103] Failed to arm timer.\n");
    return ERR;
  }

  return 0;
}

int updateSockset(int *clients, fd_set *sockset) {
  int i, maxfd = 0;
  FD_ZERO(sockset);

  for (i = 0; i < NUM_FDS; i++) {
    FD_SET(clients[i], sockset);
    if (clients[i] > maxfd)
      maxfd = clients[i];
  }

  for (i = 0; i < numNbrs; i++) {
    FD_SET(failureTimeouts[i], sockset);
    if (failureTimeouts[i] > maxfd)
      maxfd = failureTimeouts[i];
  }

  return maxfd;
}

void initRouter(int server_fd, struct sockaddr_in neClient) {
  struct pkt_INIT_REQUEST initReq;
  initReq.router_id = htonl(id);
  sendto(server_fd, &initReq, sizeof(initReq), 0,
         (struct sockaddr *)&neClient, (socklen_t)sizeof(neClient));

  // Parse Initialization Response
  struct pkt_INIT_RESPONSE initRes;
  recvfrom(server_fd, &initRes, sizeof(initRes), 0, NULL, NULL);
  ntoh_pkt_INIT_RESPONSE(&initRes);
  InitRoutingTbl(&initRes, id);

  // Initialize Router Globals
  int i;
  numNbrs = initRes.no_nbr;
  for (i = 0; i < numNbrs; i++) {
    initState[i] = initRes.nbrcost[i];
    failureTimeouts[i] = createTimer(FAILURE_DETECTION);
  }

  PrintRoutes(logfile, id);
}

void updateRoutes(int server_fd, int converge_fd) {
  struct pkt_RT_UPDATE updateRes;
  recvfrom(server_fd, &updateRes, sizeof(updateRes), 0, NULL, NULL);
  ntoh_pkt_RT_UPDATE(&updateRes);

  int i = getIdIndex(updateRes.sender_id);
  setTimer(failureTimeouts[i], FAILURE_DETECTION);
  if (UpdateRoutes(&updateRes, initState[i].cost, id)) {
    PrintRoutes(logfile, id);
    setTimer(converge_fd, CONVERGE_TIMEOUT);
  }
}

void sendUpdates(int server_fd, struct sockaddr_in neClient) {
  int i;
  struct pkt_RT_UPDATE updatePkt;

  for (i = 0; i < numNbrs; i++) {
    updatePkt.dest_id = initState[i].nbr;
    ConvertTabletoPkt(&updatePkt, id);
    hton_pkt_RT_UPDATE(&updatePkt);
    sendto(server_fd, &updatePkt, sizeof(updatePkt), 0,
           (struct sockaddr *)&neClient, (socklen_t)sizeof(neClient));
  }
}

// Get addr information (used to bindListener)
struct addrinfo *getAddrInfo(char *port, int socktype) {
  int r;
  struct addrinfo hints, *getaddrinfo_res;

  // Setup hints
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_socktype = socktype;
  if ((r = getaddrinfo(NULL, port, &hints, &getaddrinfo_res))) {
    fprintf(stderr, "[getAddrInfo:21:getaddrinfo] %s\n", gai_strerror(r));
    return NULL;
  }

  return getaddrinfo_res;
}

// Bind Listener
int bindListener(struct addrinfo *info) {
  if (info == NULL) return -1;

  int serverfd;
  for (;info != NULL; info = info->ai_next) {
    if ((serverfd = socket(info->ai_family,
                           info->ai_socktype,
                           info->ai_protocol)) < 0) {
      perror("[bindListener:35:socket]");
      continue;
    }

    int opt = 1;
    if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR,
                   &opt, sizeof(int)) < 0) {
      perror("[bindListener:43:setsockopt]");
      return -1;
    }

    if (bind(serverfd, info->ai_addr, info->ai_addrlen) < 0) {
      close(serverfd);
      perror("[bindListener:49:bind]");
      continue;
    }

    freeaddrinfo(info);
    return serverfd;
  }

  freeaddrinfo(info);
  return -1;
}

