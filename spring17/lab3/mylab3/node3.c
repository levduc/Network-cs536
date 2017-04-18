#include <stdio.h>
#define INFTY 999

extern struct rtpkt {
  int sourceid;       /* id of sending router sending this pkt */
  int destid;         /* id of router to which pkt being sent 
                         (must be an immediate neighbor) */
  int mincost[4];    /* min cost to node 0 ... 3 */
  };

extern int TRACE;
extern int YES;
extern int NO;
extern float clocktime; 

int connectcosts3[4] = { 7,  INFTY,  2, 0 };
int neighbor3[4] = { 7,  INFTY,  2, 0 };

struct distance_table 
{
  int costs[4][4];
} dt3;

/* students to write the following two routines, and maybe some others */

void rtinit3() 
{
  printf("[3] rtinit3 starts at %f.\n", clocktime);
  /*initalize costs*/
  int i,j;
  for(i = 0; i < 4 ; i++)
  {
    for(j = 0; j < 4; j++)
      dt3.costs[i][j] = INFTY;
  }
  /*add cost to neighbor*/
  dt3.costs[0][0] = 7;        //
  dt3.costs[1][1] = INFTY;      //   
  dt3.costs[2][2] = 2;        // 
  dt3.costs[3][3] = 0;        // 
  
  /*create a packet*/
  struct rtpkt vpacket;
  //get distance vector to neighbor
  for(i = 0; i < 4 ; i++)
  {
    vpacket.mincost[i] = connectcosts3[i];
  }
  /*sending vpacket*/
  vpacket.sourceid = 3;
  for(i = 0; i < 4 ; i++)
  {
    if(vpacket.sourceid == i)
      continue;
    //destination
    if(neighbor3[i] == INFTY)
      continue;
    vpacket.destid = i;
    printf("[3] rtinit3 sends routing packet to %d.\n", i);
    //send
    tolayer2(vpacket);
  }
  printdt3(&dt3);
}


void rtupdate3(rcvdpkt)
  struct rtpkt *rcvdpkt;
{
  printf("[3] rtupdate3 is called at %f.\n", clocktime);
  printf("[3] routing packet from %d.\n", rcvdpkt->sourceid);
  int srcID = rcvdpkt->sourceid;
  int isUpdate = 0;
  int i;
  /*updating table*/
  for (i = 0; i < 4; i++)
  {
    // ignore if cost is INFTY
    if(rcvdpkt->mincost[i] == INFTY)
      continue;
    // update
    dt3.costs[i][srcID] = neighbor3[srcID] + rcvdpkt->mincost[i];
    // // if connecting cost > bigger than new value. We update new connection cost
    if (connectcosts3[i] > dt3.costs[i][srcID])
    {
      connectcosts3[i] = dt3.costs[i][srcID];
      isUpdate = 1;
    }
  }
  if(isUpdate)
  {
    /*create a packet*/
    struct rtpkt vpacket;
    //get shortest distance to neighbor
    for(i = 0; i < 4 ; i++)
    {
      vpacket.mincost[i] = connectcosts3[i];
    }
    // if direct access
    /*sending distance vector*/
    //source
    vpacket.sourceid = 3;
    for(i = 0; i < 4 ; i++)
    {
      // don't send to itself
      if(vpacket.sourceid == i)
        continue;
      // don't send to unreachable node
      if(neighbor3[i] == INFTY)
        continue;
      //destination
      vpacket.destid = i;
      printf("[3] rtupdate3 sends routing packet to %d.\n", i);
      //send
      tolayer2(vpacket);
    }
  }
  printdt3(&dt3);
}


printdt3(dtptr)
  struct distance_table *dtptr;
  
{
  printf("             via     \n");
  printf("   D3 |    0     2 \n");
  printf("  ----|-----------\n");
  printf("     0|  %3d   %3d\n",dtptr->costs[0][0], dtptr->costs[0][2]);
  printf("dest 1|  %3d   %3d\n",dtptr->costs[1][0], dtptr->costs[1][2]);
  printf("     2|  %3d   %3d\n",dtptr->costs[2][0], dtptr->costs[2][2]);

}







