#include <netdb.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

#include "ne.h"
#include "router.h"

#define ERR -1

int getDirectCost(int);
int createPeriodicTimer(int);
struct sockaddr_in createNeClient(char*, char*);
int updateSockset(int*, fd_set*, int);
void initRouter(int, struct sockaddr_in);
void updateRoutes(int);
void sendUpdates(int, struct sockaddr_in);

unsigned int id;
unsigned int numNbrs;
struct nbr_cost initState[MAX_ROUTERS];

int main(int argc, char **argv) {
  if (argc < 5) {
    fprintf(stderr, "USAGE ./router <router id> <ne hostname> <ne UDP port> <router UDP port>\n");
    return 1;
  }

  id = strtol(argv[1], NULL, 10);

  // Setup logging
  char logfilename[12];
  sprintf(logfilename, "router%d.log", id);
  FILE* logfile = fopen(logfilename, "w");

  int ne_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (ne_fd < 0) {
    fprintf(stderr, "[main:18:socket] Failed to initialize socket (UDP)\n");
    return 2;
  }

  struct sockaddr_in neClient = createNeClient(argv[2], argv[3]);

  // Initialize Router
  initRouter(ne_fd, neClient);
  PrintRoutes(logfile, id);

  int update_fd = createPeriodicTimer(UPDATE_INTERVAL);
  //converge_fd = createPeriodicTimer(CONVERGE_TIMEOUT);

  fd_set sockset;
  int clients[2] = { ne_fd, update_fd };
  while (1) {
    select(updateSockset(clients, &sockset, 2) + 1, &sockset, NULL, NULL, NULL);

    if (FD_ISSET(ne_fd, &sockset))
      updateRoutes(ne_fd);
    else if (FD_ISSET(update_fd, &sockset))
      sendUpdates(ne_fd, neClient);

  }

  return 0;
}

int getDirectCost(int nbr_id) {
  int i;
  for (i = 0; i < numNbrs; i++) {
    if (initState[i].nbr == nbr_id)
      return initState[i].cost;
  }
  return ERR;
}

int createPeriodicTimer(int sec) {
  struct itimerspec timeout;

  // Use TFD_NONBLOCK instead of 0 for versions above 2.6.27
  int fd = timerfd_create(CLOCK_MONOTONIC, 0);
  if (fd < 0)
    return ERR;

  timeout.it_value.tv_sec = sec;
  timeout.it_value.tv_nsec = 0;
  timeout.it_interval.tv_sec = 0;
  timeout.it_interval.tv_nsec = 0;

  if (timerfd_settime(fd, 0, &timeout, NULL))
    fprintf(stderr, "[createPeriodicTimer:71] Failed to arm timer.\n");

  return fd;
}

int updateSockset(int *clients, fd_set *sockset, int len) {
  int i, maxfd = 0;
  FD_ZERO(sockset);
  for (i = 0; i < len; i++) {
    FD_SET(clients[i], sockset);
    if (clients[i] > maxfd)
      maxfd = clients[i];
  }
  return maxfd;
}

struct sockaddr_in createNeClient(char *neHostname, char *nePort) {
  struct sockaddr_in neClient;
  memset(&neClient, 0, sizeof(neClient));
  neClient.sin_family = AF_INET;
  neClient.sin_port = htons((unsigned short)strtol(nePort, NULL, 10));
  inet_aton(neHostname, &neClient.sin_addr);

  return neClient;
}

void initRouter(int ne_fd, struct sockaddr_in neClient) {
  struct pkt_INIT_REQUEST initReq;
  initReq.router_id = htonl(id);
  sendto(ne_fd, &initReq, sizeof(initReq), 0,
         (struct sockaddr *)&neClient, (socklen_t)sizeof(neClient));

  // Parse Initialization Response
  struct pkt_INIT_RESPONSE initRes;
  recvfrom(ne_fd, &initRes, sizeof(initRes), 0, NULL, NULL);
  ntoh_pkt_INIT_RESPONSE(&initRes);
  InitRoutingTbl(&initRes, id);

  // Initialize Router Globals
  int i;
  numNbrs = initRes.no_nbr;
  for (i = 0; i < numNbrs; i++)
    initState[i] = initRes.nbrcost[i];
}

void updateRoutes(int ne_fd) {
  struct pkt_RT_UPDATE updateRes;
  recvfrom(ne_fd, &updateRes, sizeof(updateRes), 0, NULL, NULL);
  ntoh_pkt_RT_UPDATE(&updateRes);
  UpdateRoutes(&updateRes,
               getDirectCost(updateRes.sender_id),
               id);
}

void sendUpdates(int ne_fd, struct sockaddr_in neClient) {
  int i;
  struct pkt_RT_UPDATE updatePkt;
  for (i = 0; i < numNbrs; i++) {
    updatePkt.dest_id = initState[i].nbr;
    ConvertTabletoPkt(&updatePkt, id);
    hton_pkt_RT_UPDATE(&updatePkt);
    sendto(ne_fd, &updatePkt, sizeof(updatePkt), 0,
           (struct sockaddr *)&neClient, (socklen_t)sizeof(neClient));
  }
}
