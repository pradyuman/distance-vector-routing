  /*
   *  ECE 463 Introduction to Computer Networks LAB 2 SOURCE FILE
   *
   *  File Name: unit-test.c
   *
   *  Purpose: Contains Unit test cases to test the functions in routingtable.c 
   *
   */


#include "ne.h"
#include "router.h"

#define MyAssert(X,Y) ASSERT(__FILE__,__FUNCTION__,__LINE__,X,Y)

void ASSERT(char *file,const char *func,int line,int x,char *y) {
    if(x == 0) {
       printf("Assert failed:file %s, function %s, line %i\n",file,func,line);
       printf("**%s**\n",y);
       exit(1);
    }
}

struct pkt_INIT_RESPONSE nbrs;
int MyRouterId = 0;

int TestInitRT() {

    int i;
    int nbr1 = 999;
    int nbr2 = 999;
    struct pkt_RT_UPDATE resultpkt;
    nbrs.no_nbr = 2;
    nbrs.nbrcost[0].nbr = 1;
    nbrs.nbrcost[0].cost = 4;
    nbrs.nbrcost[1].nbr = 2;
    nbrs.nbrcost[1].cost = 3;
    
    InitRoutingTbl (&nbrs, MyRouterId);
    ConvertTabletoPkt(&resultpkt, MyRouterId);
    MyAssert(resultpkt.no_routes==3,"Incorrect number of routes after initializing the routing table");
    for(i=0; i<resultpkt.no_routes; i++) {
        if(resultpkt.route[i].dest_id == 1) {
           nbr1 = i;
        }
        if(resultpkt.route[i].dest_id == 2) {
           nbr2 = i;
        }
    }
    MyAssert(nbr1!=999,"A Neighbor not found after initializing the routing table");
    MyAssert((resultpkt.route[nbr1].next_hop==1 && resultpkt.route[nbr1].cost==4),"Incorrect next hop or cost to a neighbor after initializing the routing table");
    MyAssert(nbr2!=999,"A Neighbor not found after initializing the routing table");
    MyAssert((resultpkt.route[nbr2].next_hop==2 && resultpkt.route[nbr2].cost==3),"Incorrect next hop or cost to a neighbor after initializing the routing table");
    return 0;
}


int TestNewRoute() {

    int i;
    int nbr = 999;

    struct pkt_RT_UPDATE updpkt, resultpkt; 
 
    updpkt.sender_id = 1;
    updpkt.dest_id = 0;
    updpkt.no_routes = 1;
    updpkt.route[0].dest_id = 4;
    updpkt.route[0].next_hop = 1;
    updpkt.route[0].cost = 5;
    UpdateRoutes(&updpkt, nbrs.nbrcost[0].cost, MyRouterId);
    ConvertTabletoPkt(&resultpkt, MyRouterId);
    MyAssert(resultpkt.no_routes==4,"Incorrect number of routes after adding a new destination");
    for(i=0; i<resultpkt.no_routes; i++) {
        if(resultpkt.route[i].dest_id == 4) {   
           nbr = i;
        }
    }
    MyAssert(nbr!=999,"Router didn't add route to a new destination that was previously unknown");
    MyAssert((resultpkt.route[nbr].next_hop==1 && resultpkt.route[nbr].cost==9),"Incorrect next hop or cost to a newly added destination");
    return 0;
}

int TestDVUpdate() {

    int i;
    int nbr = 999;
    struct pkt_RT_UPDATE updpkt, resultpkt;   

    updpkt.sender_id = 2;
    updpkt.dest_id = 0;
    updpkt.no_routes = 1;
    updpkt.route[0].dest_id = 4;
    updpkt.route[0].next_hop = 2;
    updpkt.route[0].cost = 2;
    UpdateRoutes(&updpkt, nbrs.nbrcost[1].cost, MyRouterId);
    ConvertTabletoPkt(&resultpkt, MyRouterId);
    MyAssert(resultpkt.no_routes==4,"Incorrect number of routes while testing shortest path calculation");
    for(i=0; i<resultpkt.no_routes; i++) {
        if(resultpkt.route[i].dest_id ==4) {
           nbr = i;
        }
    }
    MyAssert(nbr!=999,"A destination got removed on finding another shortest path to that destination");
    MyAssert((resultpkt.route[nbr].next_hop==2 && resultpkt.route[nbr].cost==5),"Incorrect next hop or cost to a destination on finding another shortest path");
    return 0;
}

int TestForcedUpd() {

    int i;
    int nbr = 999;
    struct pkt_RT_UPDATE updpkt, resultpkt;   

    updpkt.sender_id = 2;
    updpkt.dest_id = 0;
    updpkt.no_routes = 1;
    updpkt.route[0].dest_id = 4;
    updpkt.route[0].next_hop = 2;
    updpkt.route[0].cost = 3;
    UpdateRoutes(&updpkt, nbrs.nbrcost[1].cost, MyRouterId);
    ConvertTabletoPkt(&resultpkt, MyRouterId);
    MyAssert(resultpkt.no_routes==4,"Incorrect number of routes while testing forced update");
    for(i=0; i<resultpkt.no_routes; i++) {
        if(resultpkt.route[i].dest_id ==4) {
           nbr = i;
        }
    }
    MyAssert(nbr!=999,"A destination got removed while testing forced update");
    MyAssert((resultpkt.route[nbr].next_hop==2 && resultpkt.route[nbr].cost==6),"Incorrect next hop or cost to a destination while testing forced update rule");
    return 0;
}

int TestSplitHorizon() {

    int i;
    int nbr = 999;
    struct pkt_RT_UPDATE updpkt, resultpkt;   

    updpkt.sender_id = 2;
    updpkt.dest_id = 0;
    updpkt.no_routes = 1;
    updpkt.route[0].dest_id = 1;
    updpkt.route[0].next_hop = 0;
    updpkt.route[0].cost = 0;
    UpdateRoutes(&updpkt, nbrs.nbrcost[1].cost, MyRouterId);
    ConvertTabletoPkt(&resultpkt, MyRouterId);
    MyAssert(resultpkt.no_routes==4,"Incorrect number of routes while testing split horizon");
    for(i=0; i<resultpkt.no_routes; i++) {
        if(resultpkt.route[i].dest_id ==1) {
           nbr = i;
        }
    }
    MyAssert(nbr!=999,"A neighbor got removed while testing split horizon");
    MyAssert((resultpkt.route[nbr].next_hop==1 && resultpkt.route[nbr].cost==4),"Incorrect next hop or cost to a neighbor while testing split horizon rule");
    return 0;
}



int main (int argc, char *argv[])
{

//Testing Initialization of Routing Table

    TestInitRT();
    printf("Test Case 1: PASS Initialized routing table\n");

//Testing New Route Update
    
    TestNewRoute();
    printf("Test Case 2: PASS Added new route to routing table\n");
   
//Testing Distance Vector Calculation

    TestDVUpdate();
    printf("Test Case 3: PASS Updated new shortest path\n");

//Testing Forced Update Rule

    TestForcedUpd();
    printf("Test Case 4: PASS Forced update rule taken care\n");

//Testing Split Horizon Rule

    TestSplitHorizon();
    printf("Test Case 5: PASS Split horizon rule taken care\n");

return 0;

}
