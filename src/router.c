#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>

#include "ne.h"
#include "router.h"

int main(int argc, char **argv) {
  if (argc < 5) {
    fprintf(stderr, "USAGE ./router <router id> <ne hostname> <ne UDP port> <router UDP port>");
    return 1;
  }

  int ne = socket(AF_INET, SOCK_DGRAM, 0);
  if (ne < 0) {
    fprintf(stderr, "[main:65:bindListener] Failed to bind at port %s (UDP)\n", argv[3]);
    return 2;
  }

  char *eptr;
  int id = strtol(argv[1], &eptr, 10);
  struct pkt_INIT_REQUEST initReq;
  struct sockaddr_in neClient;

  memset(&neClient, 0, sizeof(neClient));
  neClient.sin_family = AF_INET;
  neClient.sin_port = htons((unsigned short)strtol(argv[3], &eptr, 10));
  inet_aton(argv[2], &neClient.sin_addr);
  socklen_t neSize = sizeof(neClient);

  initReq.router_id = htonl(id);
  sendto(ne, &initReq, sizeof(initReq), 0,
         (struct sockaddr *)&neClient, neSize);

  struct pkt_INIT_RESPONSE initRes;
  int initResSize = recvfrom(ne, &initRes, sizeof(initRes), 0, NULL, NULL);
  return 0;
}
