#include "ne.h"
#include "router.h"

int numRoutes;
struct route_entry routingTable[MAX_ROUTERS];

void updateEntry(int i, int dest_id, int next_hop, int cost) {
  routingTable[i].dest_id = dest_id;
  routingTable[i].next_hop = next_hop;
  routingTable[i].cost = cost;
}

void InitRoutingTbl(struct pkt_INIT_RESPONSE *res, int myID) {
  numRoutes = res->no_nbr + 1;

  // Initialize Self Routes
  routingTable[0].dest_id = myID;
  routingTable[0].next_hop = myID;
  routingTable[0].cost = 0;

  int i;
  for (i = 1; i < numRoutes; i++) {
    updateEntry(i, res->nbrcost[i-1].nbr, res->nbrcost[i-1].nbr, res->nbrcost[i-1].cost);
  }
}

int UpdateRoutes(struct pkt_RT_UPDATE *p, int costToNbr, int myID) {
  int i, j;
  for (i = 0; i < p->no_routes; i++) {
    int hit = 0;
    int nr_dest_id = p->route[i].dest_id;
    int nr_next_hop = p->route[i].next_hop;
    int nr_cost = p->route[i].cost;
    for (j = 0; j < numRoutes; j++) {
      if (nr_dest_id == routingTable[j].dest_id) {
        int nr_total_cost = costToNbr + nr_cost;
        if (routingTable[j].next_hop == p->sender_id
            || (routingTable[j].cost > nr_total_cost && nr_next_hop != myID)) {
          updateEntry(j, nr_dest_id, p->sender_id, nr_total_cost);
        }
        hit = 1; break;
      }
    }

    if (!hit) {
      numRoutes++;
      updateEntry(j, p->route[i].dest_id, p->sender_id, costToNbr + p->route[i].cost);
    }
  }
  return 0;
}

void ConvertTabletoPkt(struct pkt_RT_UPDATE *UpdatePacketToSend, int myID) {
  UpdatePacketToSend->sender_id = myID;
  UpdatePacketToSend->no_routes = numRoutes;

  int i;
  for (i = 0; i < numRoutes; i++) {
    UpdatePacketToSend->route[i] = routingTable[i];
  }
}
