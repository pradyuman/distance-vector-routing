#include <stdio.h>
#include "ne.h"
#include "router.h"

int numRoutes;
struct route_entry routingTable[MAX_ROUTERS];

void updateEntry(int i, int dest_id, int next_hop, int cost) {
  routingTable[i].dest_id = dest_id;
  routingTable[i].next_hop = next_hop;
  if (cost <= INFINITY)
    routingTable[i].cost = cost;
}

void InitRoutingTbl(struct pkt_INIT_RESPONSE *res, int myID) {
  numRoutes = res->no_nbr + 1;

  // Initialize Self Routes
  updateEntry(0, myID, myID, 0);

  int i;
  for (i = 1; i < numRoutes; i++)
    updateEntry(i, res->nbrcost[i-1].nbr, res->nbrcost[i-1].nbr, res->nbrcost[i-1].cost);
}

int UpdateRoutes(struct pkt_RT_UPDATE *p, int costToNbr, int myID) {
  int i, j, changed = 0;
  for (i = 0; i < p->no_routes; i++) {
    int nr_dest_id = p->route[i].dest_id;
    int nr_next_hop = p->route[i].next_hop;
    int nr_cost = p->route[i].cost;

    // Find id in routing table
    for (j = 0; j < numRoutes; j++) {
      if (nr_dest_id == routingTable[j].dest_id) {
        int nr_total_cost = nr_cost == INFINITY ? nr_cost : costToNbr + nr_cost;
        int lessCost = routingTable[j].cost > nr_total_cost;
        int differentCost = routingTable[j].cost != nr_total_cost;
        int splitHorizon = nr_next_hop != myID;
        int forcedUpdate = routingTable[j].next_hop == p->sender_id && differentCost;
        if (forcedUpdate || (lessCost && splitHorizon)) {
          updateEntry(j, nr_dest_id, p->sender_id, nr_total_cost);
          changed = 1;
        }
        break;
      }
    }

    // Not in routing table
    if (j == numRoutes) {
      numRoutes++;
      updateEntry(j, p->route[i].dest_id, p->sender_id, costToNbr + p->route[i].cost);
      changed = 1;
    }
  }

  return changed;
}

void ConvertTabletoPkt(struct pkt_RT_UPDATE *UpdatePacketToSend, int myID) {
  UpdatePacketToSend->sender_id = myID;
  UpdatePacketToSend->no_routes = numRoutes;

  int i;
  for (i = 0; i < numRoutes; i++)
    UpdatePacketToSend->route[i] = routingTable[i];
}

void PrintRoutes(FILE* Logfile, int myID) {
  fprintf(Logfile, "Routing Table:\n");

  int i;
  for (i = 0; i < numRoutes; i++) {
    fprintf(Logfile,"R%d -> R%d: R%d, %d\n", myID,
            routingTable[i].dest_id,
            routingTable[i].next_hop,
            routingTable[i].cost);
  }

  fflush(Logfile);
}

void UninstallRoutesOnNbrDeath(int deadNbr) {
  int i;
  for (i = 0; i < numRoutes; i++) {
    if (routingTable[i].next_hop == deadNbr)
      routingTable[i].cost = INFINITY;
  }
}
