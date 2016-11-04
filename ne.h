/*ne.h*/

#ifndef NE_H
#define NE_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#define MAX_ROUTERS 10 /* max # of routers in the system */
#define MAX_PATH_LEN (MAX_ROUTERS - 1)
#define PACKETSIZE 1024 /* assume packet size large enough to contain a full routing table */
#define INFINITY 999 /* Cost to a unreacheable destination router */
#define UPDATE_INTERVAL 1 /* router sends routing updates every 1 sec */
#define FAILURE_DETECTION (UPDATE_INTERVAL * 3) /* not receiving a nbr's update more than 3 update cycles = 3 * UPDATE_INTERVAL, consider it dead */
#define CONVERGE_TIMEOUT (UPDATE_INTERVAL * 5) /* if routing table is not changed after 5 update cycles, assume converged */

  /*
   *  ECE 463 Introduction to Computer Networks LAB 2 HEADER FILE
   *
   *  File Name: ne.h
   *
   *  Purpose: Defines packet formats and endian functions. 
   *
   */


struct pkt_INIT_REQUEST {
  unsigned int router_id; /* my id, used to retrieve neighbor database from ne */
};

struct nbr_cost {
  unsigned int nbr; /* neighbor id */
  unsigned int cost; /* cost to neighbor */
};

struct pkt_INIT_RESPONSE {
  unsigned int no_nbr; /* number of directly connected neighbors */
  struct nbr_cost nbrcost[MAX_ROUTERS]; /* array holding the cost to each neighbor */
};

struct route_entry {
  unsigned int dest_id; /* destination router id */
  unsigned int next_hop; /* next hop on the shortest path to dest_id */
  unsigned int cost; /* cost to desintation router */
#ifdef PATHVECTOR
  unsigned int path_len; /* length of loop-free path to dest_id, eg: with path R1 -> R2 -> R3, the length is 2 */
  unsigned int path[MAX_PATH_LEN]; /* array containing id's of routers along the path */
#endif
};

struct pkt_RT_UPDATE {
  unsigned int sender_id; /* id of router sending the message */
  unsigned int dest_id; /* id of neighbor router to which routing table is sent */
  unsigned int no_routes; /* number of routes in my routing table */
  struct route_entry route[MAX_ROUTERS]; /* array containing rows of routing table */
};

/* The following endian functions are to be implemented in endian.c */

/*
 *  This function converts struct pkt_RT_UPDATE
 *  from host to network byte order.
 */
void hton_pkt_RT_UPDATE (struct pkt_RT_UPDATE *);

/*
 *  This function converts struct pkt_RT_UPDATE
 *  from network to host byte order.
 */
void ntoh_pkt_RT_UPDATE (struct pkt_RT_UPDATE *);

/*
 *  This function converts struct pkt_INIT_RESPONSE
 *  from network to host byte order.
 */
void ntoh_pkt_INIT_RESPONSE (struct pkt_INIT_RESPONSE *);

#endif
