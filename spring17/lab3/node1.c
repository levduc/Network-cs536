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

int connectcosts1[4] = { 1,  0,  1, INFTY };
int neighbor1[4] = { 1,  0,  1, INFTY };

struct distance_table 
{
  int costs[4][4];
} dt1;


/* students to write the following two routines, and maybe some others */


rtinit1() 
{
  printf("[1] rtinit1 starts at %f.\n", clocktime);
  /*initalize costs*/
  int i,j;
  for(i = 0; i < 4 ; i++)
  {
    for(j = 0; j < 4; j++)
      dt1.costs[i][j] = INFTY;
  }
  /*add cost to neighbor*/
  dt1.costs[0][0] = 1;      //
  dt1.costs[1][1] = 0;      //   
  dt1.costs[2][2] = 1;      // 
  dt1.costs[3][3] = INFTY;    // 
  
  /*create a packet*/
  struct rtpkt vpacket;
  //get distance vector to neighbor
  for(i = 0; i < 4 ; i++)
  {
      vpacket.mincost[i] = dt1.costs[i][i];
  }
  /*sending vpacket*/
  vpacket.sourceid = 1;
  for(i = 0; i < 4 ; i++)
  {
    //destination
    if(vpacket.sourceid == i)
      continue;
    if(neighbor1[i] == INFTY)
      continue;
    vpacket.destid = i;
    //send
    printf("[1] rtinit1 send routing packet to %d.\n", clocktime, i);

    tolayer2(vpacket);
  }
  printdt1(&dt1);
}


rtupdate1(rcvdpkt)
  struct rtpkt *rcvdpkt;

{
  printf("[1] rtupdate1 is called at %f.\n", clocktime);
  printf("[1] routing packet from %d.\n", rcvdpkt->sourceid);
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
    dt1.costs[i][srcID] = neighbor1[srcID] + rcvdpkt->mincost[i];
    // if connecting cost > bigger than new value. We update new connection cost
    if (connectcosts1[i] > dt1.costs[i][srcID])
    {
      connectcosts1[i] = dt1.costs[i][srcID];
      isUpdate = 1;
    }
  }
  if (isUpdate)
  {
    /* code */
    /*create a packet*/
    struct rtpkt vpacket;
    //get shortest distance to neighbor
    for(i = 0; i < 4 ; i++)
    {
      vpacket.mincost[i] = connectcosts1[i];
    }

    /*sending distance vector*/
    //source
    vpacket.sourceid = 1;
    for(i = 0; i < 4 ; i++)
    {
      // don't send to itself
      if(vpacket.sourceid == i)
        continue;
      // don't send to unreachable node
      if(neighbor1[i] == INFTY)
        continue;
      //destination
      vpacket.destid = i;
      printf("[1] rtupdate1 sends routing packet to %d.\n", i);
      //send
      tolayer2(vpacket);
    }
  }
  printdt1(&dt1);
}


printdt1(dtptr)
  struct distance_table *dtptr;
  
{
  printf("             via   \n");
  printf("   D1 |    0     2 \n");
  printf("  ----|-----------\n");
  printf("     0|  %3d   %3d\n",dtptr->costs[0][0], dtptr->costs[0][2]);
  printf("dest 2|  %3d   %3d\n",dtptr->costs[2][0], dtptr->costs[2][2]);
  printf("     3|  %3d   %3d\n",dtptr->costs[3][0], dtptr->costs[3][2]);

}



linkhandler1(linkid, newcost)   
int linkid, newcost;   
/* called when cost from 1 to linkid changes from current value to newcost*/
/* You can leave this routine empty if you're an undergrad. If you want */
/* to use this routine, you'll need to change the value of the LINKCHANGE */
/* constant definition in prog3.c from 0 to 1 */
	
{
  printf("[1] linkhandler1 is called at %f.\n", clocktime);
  printf("[1] cost of link between %d and %d is now %d.\n", 1 ,linkid,newcost);
  int min = INFTY;
  int i,j;
  for (i=0;i<4;i++) 
  {
    dt1.costs[i][linkid] = dt1.costs[i][linkid] - neighbor1[linkid] + newcost;
  }
  neighbor1[linkid] = newcost;    

  /* Recompute the new costs of the shortest paths to each node */
  for (i=0;i<4;i++) 
  {
    /* Find the lowest cost in each row */
    for (j=0;j<4;j++) {
      if (dt1.costs[i][j] < min ) 
      {
        min = dt1.costs[i][j];
      }
    }
    connectcosts1[i] = min;
    min = INFTY;
  }
  
  printdt1(&dt1);
  
  /* Send out routing packets to neighbors */
  /*create a packet*/
  struct rtpkt vpacket;
  //get shortest distance to neighbor
  for(i = 0; i < 4 ; i++)
  {
    vpacket.mincost[i] = connectcosts1[i];
  }
  /*sending distance vector*/
  //source
  vpacket.sourceid = 1;
  for(i = 0; i < 4 ; i++)
  {
    // don't send to itself
    if(vpacket.sourceid == i)
      continue;
    // don't send to unreachable node
    if(neighbor1[i] == INFTY)
      continue;
    printf("[1] linkhandler1 sends routing packet to %d.\n", i);
    //destination
    vpacket.destid = i;
    //send
    tolayer2(vpacket);
  }
}


