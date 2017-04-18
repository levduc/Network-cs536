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

int connectcosts2[4] = { 3,  1,  0, 2 };
int neighbor2[4] = { 3,  1,  0, 2 };

struct distance_table 
{
  int costs[4][4];
} dt2;


/* students to write the following two routines, and maybe some others */

void rtinit2() 
{
  printf("[2] rtinit2 starts at %f.\n", clocktime);
  /*initalize costs*/
  int i,j;
  for(i = 0; i < 4 ; i++)
  {
    for(j = 0; j < 4; j++)
      dt2.costs[i][j] = INFTY;
  }
  /*add cost to neighbor*/
  dt2.costs[0][0] = 3;      //
  dt2.costs[1][1] = 1;      //   
  dt2.costs[2][2] = 0;      // 
  dt2.costs[3][3] = 2;      //
  /*create a packet*/
  struct rtpkt vpacket;
  //get distance vector to neighbor
  for(i = 0; i < 4 ; i++)
  {
      vpacket.mincost[i] = dt2.costs[i][i];
  }
  /*sending vpacket*/
  vpacket.sourceid = 2;
  for(i = 0; i < 4 ; i++)
  {
    if(vpacket.sourceid == i)
      continue;
    if(neighbor2[i] == INFTY)
      continue;
    //destination
    vpacket.destid = i;
    printf("[2] rtinit2 sends routing packet to %d.\n", i);
    //send
    tolayer2(vpacket);
  }
  printdt2(&dt2);
}


void rtupdate2(rcvdpkt)
  struct rtpkt *rcvdpkt;
  
{
  printf("[2] rtupdate2 is called at %f.\n", clocktime);
  printf("[2] routing packet from %d.\n", rcvdpkt->sourceid);
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
    dt2.costs[i][srcID] = neighbor2[srcID] + rcvdpkt->mincost[i];
    // if connecting cost > bigger than new value. We update new connection cost
    if (connectcosts2[i] > dt2.costs[i][srcID])
    {
      connectcosts2[i] = dt2.costs[i][srcID];
      isUpdate = 1;
    }
  }
  // if update send
  if(isUpdate)
  {
    /*create a packet*/
    struct rtpkt vpacket;
    //get shortest distance to neighbor
    for(i = 0; i < 4 ; i++)
    {
      vpacket.mincost[i] = connectcosts2[i];
    }

    /*sending distance vector*/
    //source
    vpacket.sourceid = 2;
    for(i = 0; i < 4 ; i++)
    {
      // don't send to itself
      if(vpacket.sourceid == i)
        continue;
      // don't send to unreachable node
      if(neighbor2[i] == INFTY)
        continue;
      //destination
      vpacket.destid = i;
      //send
      printf("[2] rtupdate2 sends routing packet to %d.\n", i);
      tolayer2(vpacket);
    }
  }
  printdt2(&dt2); 
}


printdt2(dtptr)
  struct distance_table *dtptr;
  
{
  printf("                via     \n");
  printf("   D2 |    0     1    3 \n");
  printf("  ----|-----------------\n");
  printf("     0|  %3d   %3d   %3d\n",dtptr->costs[0][0],
	 dtptr->costs[0][1],dtptr->costs[0][3]);
  printf("dest 1|  %3d   %3d   %3d\n",dtptr->costs[1][0],
	 dtptr->costs[1][1],dtptr->costs[1][3]);
  printf("     3|  %3d   %3d   %3d\n",dtptr->costs[3][0],
	 dtptr->costs[3][1],dtptr->costs[3][3]);
}







