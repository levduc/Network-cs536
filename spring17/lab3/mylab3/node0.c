#include <stdio.h>
#define INFTY 999

extern struct rtpkt 
{
  int sourceid;       /* id of sending router sending this pkt */
  int destid;         /* id of router to which pkt being sent 
                         (must be an immediate neighbor) */
  int mincost[4];    /* min cost to node 0 ... 3 */
};

extern int TRACE;
extern int YES;
extern int NO;

extern float clocktime;

int connectcosts0[4] = { 0,  1,  3, 7 };
int neighbor0[4] = { 0,  1,  3, 7 };

struct distance_table 
{
  int costs[4][4];
} dt0;


/* students to write the following two routines, and maybe some others */

void rtinit0() 
{

  printf("[0] rtinit0 starts at %f.\n", clocktime);
  /*initalize costs*/
  int i,j;
  for(i = 0; i < 4 ; i++)
  {
    for(j = 0; j < 4; j++)
    {
      dt0.costs[i][j] = INFTY;
    }
  }

  /* add cost to neighbor i via j*/
  dt0.costs[0][0] = 0;    //  0 via 0 
  dt0.costs[1][1] = 1;    //  1 via 1
  dt0.costs[2][2] = 3;    //  2 via 2
  dt0.costs[3][3] = 7;    //  3 via 3
  /* print table*/
  printdt0(&dt0);

  /*create a packet*/
  struct rtpkt vpacket;
  //get shortest distance to neighbor
  for(i = 0; i < 4 ; i++)
  {
    vpacket.mincost[i] = connectcosts0[i];
  }

  /*sending distance vector*/
  //source
  vpacket.sourceid = 0;
  for(i = 0; i < 4 ; i++)
  {
    // don't send to itself
    if(vpacket.sourceid == i)
      continue;
    // don't send to unreachable node
    if(neighbor0[i] == INFTY)
      continue;
    //destination
    vpacket.destid = i;
    printf("[0] rtinit0 sends routing packet to %d.\n", i);
    //send
    tolayer2(vpacket);
  }
}


void rtupdate0(rcvdpkt)
  struct rtpkt *rcvdpkt;
{
  printf("[0] rtupdate0 is called at %f.\n", clocktime);
  printf("[0] routing packet from %d.\n", rcvdpkt->sourceid);
  int srcID = rcvdpkt->sourceid;
  int isUpdate = 0;
  int i;
  /*updating table*/
  for (i = 0; i < 4; i++)
  {
    // ignore if cost is INFTY
    if(rcvdpkt->mincost[i] == INFTY)
      continue;
    if(i == srcID)
      continue;

    dt0.costs[i][srcID] = neighbor0[srcID] + rcvdpkt->mincost[i];
    // if connecting cost > bigger than new value. We update new connection cost
    if (connectcosts0[i] > dt0.costs[i][srcID])
    {
      connectcosts0[i] = dt0.costs[i][srcID];
      isUpdate = 1;
    }
  }
  printdt0(&dt0);

  if(isUpdate)
  {
    /*create a packet*/
    struct rtpkt vpacket;
    //get shortest distance to neighbor
    for(i = 0; i < 4 ; i++)
    {
      vpacket.mincost[i] = connectcosts0[i];
    }
    /*sending distance vector*/
    //source
    vpacket.sourceid = 0;
    for(i = 0; i < 4 ; i++)
    {
      // don't send to itself
      if(vpacket.sourceid == i)
        continue;
      // don't send to unreachable node
      if(neighbor0[i] == INFTY)
        continue;
      printf("[0] rtupdate0 sends routing packet to %d.\n", i);
      //destination
      vpacket.destid = i;
      //send
      tolayer2(vpacket);
    }
  }
}


printdt0(dtptr)
  struct distance_table *dtptr;
  
{
  printf("                via     \n");
  printf("   D0 |    1     2    3 \n");
  printf("  ----|-----------------\n");
  printf("     1|  %3d   %3d   %3d\n",dtptr->costs[1][1],
	 dtptr->costs[1][2],dtptr->costs[1][3]);
  printf("dest 2|  %3d   %3d   %3d\n",dtptr->costs[2][1],
	 dtptr->costs[2][2],dtptr->costs[2][3]);
  printf("     3|  %3d   %3d   %3d\n",dtptr->costs[3][1],
	 dtptr->costs[3][2],dtptr->costs[3][3]);
}

linkhandler0(linkid, newcost)   
  int linkid, newcost;

/* called when cost from 0 to linkid changes from current value to newcost*/
/* You can leave this routine empty if you're an undergrad. If you want */
/* to use this routine, you'll need to change the value of the LINKCHANGE */
/* constant definition in prog3.c from 0 to 1 */
	
{
  printf("[0] linkhandler0 is called at %f.\n", clocktime);
  printf("[0] cost of link between %d and %d is now %d.\n", 0 ,linkid,newcost);
  int min = INFTY;
  int i,j;
  for (i=0;i<4;i++) 
  {
    dt0.costs[i][linkid] = dt0.costs[i][linkid] - neighbor0[linkid] + newcost;
  }
  neighbor0[linkid] = newcost;    
  
  /* Recompute the new costs*/
  for (i=0;i<4;i++) {
    /* Find the lowest cost in each row */
    for (j=0;j<4;j++) {
      if (dt0.costs[i][j] < min ) 
      {
        min = dt0.costs[i][j];
      }
    }
    connectcosts0[i] = min;
    min = INFTY;
  }
  
  printdt0(&dt0);

  struct rtpkt vpacket;
  //get shortest distance to neighbor
  for(i = 0; i < 4 ; i++)
  {
    vpacket.mincost[i] = connectcosts0[i];
  }
  /*sending distance vector*/
  //source
  vpacket.sourceid = 0;
  for(i = 0; i < 4 ; i++)
  {
    // don't send to itself
    if(vpacket.sourceid == i)
      continue;
    // don't send to unreachable node
    if(neighbor0[i] == INFTY)
      continue;
    printf("[0] rtupdate0 sends routing packet to %d.\n", i);
    //destination
    vpacket.destid = i;
    //send
    tolayer2(vpacket);
  }

}

