/*router.h*/

#ifndef ROUTER_H
#define ROUTER_H

  /*
   *  ECE 463 Introduction to Computer Networks LAB 2 HEADER FILE
   *
   *  File Name: router.h
   *
   *  Purpose: Defines routingtable manipulation functions. 
   *  Defines and Maintains the routing table and all route entries in routingtable.c
   */  


/* Routine Name    : InitRoutingTbl
 * INPUT ARGUMENTS : 1. (struct pkt_INIT_RESPONSE *) - The INIT_RESPONSE from Network Emulator
 *                   2. int - My router's id received from command line argument.
 * RETURN VALUE    : void
 * USAGE           : This routine is called after receiving the INIT_RESPONSE message from the Network Emulator. 
 *                   It initializes the routing table with the bootstrap neighbor information in INIT_RESPONSE.  
 *                   Also sets up a route to itself (self-route) with next_hop as itself and cost as 0.
 */
void InitRoutingTbl (struct pkt_INIT_RESPONSE *InitResponse, int myID);



/* Routine Name    : UpdateRoutes
 * INPUT ARGUMENTS : 1. (struct pkt_RT_UPDATE *) - The Route Update message from one of the neighbors of the router.
 *                   2. int - The direct cost to the neighbor who sent the update. 
 *                   3. int - My router's id received from command line argument.
 * RETURN VALUE    : int - Return 1 : if the routing table has changed on running the function.
 *                         Return 0 : Otherwise.
 * USAGE           : This routine is called after receiving the route update from any neighbor. 
 *                   The routing table is then updated after running the distance vector protocol. 
 *                   It installs any new route received, that is previously unknown. For known routes, 
 *                   it finds the shortest path using current cost and received cost. 
 *                   It also implements the forced update and split horizon rules. My router's id
 *                   that is passed as argument may be useful in applying split horizon rule.
 */
int UpdateRoutes(struct pkt_RT_UPDATE *RecvdUpdatePacket, int costToNbr, int myID);



/* Routine Name    : ConvertTabletoPkt
 * INPUT ARGUMENTS : 1. (struct pkt_RT_UPDATE *) - An empty pkt_RT_UPDATE structure
 *                   2. int - My router's id received from command line argument.
 * RETURN VALUE    : void
 * USAGE           : This routine fills the routing table into the empty struct pkt_RT_UPDATE. 
 *                   My router's id  is copied to the sender_id in pkt_RT_UPDATE. 
 *                   Note that the dest_id is not filled in this function. When this update message 
 *                   is sent to all neighbors of the router, the dest_id is filled.
 */
void ConvertTabletoPkt(struct pkt_RT_UPDATE *UpdatePacketToSend, int myID);



/* Routine Name    : PrintRoutes
 * INPUT ARGUMENTS : 1. (FILE *) - Pointer to the log file created in router.c, with a filename that uses MyRouter's id.
 *                   2. int - My router's id received from command line argument.
 * RETURN VALUE    : void
 * USAGE           : This routine prints the routing table to the log file 
 *                   according to the format and rules specified in the Handout.
 */
void PrintRoutes (FILE* Logfile, int myID);



/* Routine Name    : UninstallRoutesOnNbrDeath
 * INPUT ARGUMENTS : 1. int - The id of the inactive neighbor 
 *                   (one who didn't send Route Update for FAILURE_DETECTION seconds).
 *                   
 * RETURN VALUE    : void
 * USAGE           : This function is invoked when a nbr is found to be dead. The function checks all routes that
 *                   use this nbr as next hop, and changes the cost to INFINITY.
 */
void UninstallRoutesOnNbrDeath(int DeadNbr);


/* Variable      : struct route_entry routingTable[MAX_ROUTERS]
 * Variable Type : Array of type (struct route_entry)
 * USAGE         : Define as a Global Variable in routingtable.c.
 *                 The routingTable will be used by all the functions in routingtable.c.
 *                 #include ne.h in routingtable.c for definitions of struct route_entry and MAX_ROUTERS.
 */


/* Variable      : int NumRoutes
 * Variable Type : Integer
 * USAGE         : Define as a Global Variable in routingtable.c.
 *                 This variable holds the number of routes present in the routing table.
 *                 It is initialized on receiving INIT_RESPONSE from Network Emulator
 *                 and is updated in the UpdateRoutes() function, whenever the routingTable changes. 
 */
#endif


