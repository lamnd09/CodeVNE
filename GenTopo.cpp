
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include "scenario.h"

using namespace std;

struct Node{
	int x;
	int y;
	double coverage;
	int cpu;
	bool OutLink;
	bool InLink;
	bool isConnected;
};

/*
 MakePN(int NumNode, int xrange, int yrange, int cvmin, int cvmax, int nwmin, int nwmax, int lwmin, int lwmax, bool isDirectional, int TypeTopo){

NumNode : # of nodes in physical network
xrange, yrange : length of x/y edge of physical network (rectangular)
cvmin, cvmax : range of range ; [cvmin, cvmax]
nwmin, nwmax : range of cpu (node) resource ; [nwmin, nwmax]
lwmin, lwmax : range of link weight [lwmin, lwmax]
isDirectional : directional or undirectional (true or false)
TypeTopo : Topology type (0: random, 1: grid)

Generate connected physical network topology according to inputs.
Store the physical network topology in "SN.txt"
*/ 

void MakePN(int NumNode, int xrange, int yrange, int cvmin, int cvmax, int nwmin, int nwmax, int lwmin, int lwmax, bool isDirectional, int TypeTopo){

// Check input validity
	if (NumNode > MAXSNODENUM){
		printf("MakePN() input error \n NumNode exeeds MAXSNODENUM \n");
		return ;
	}
	if (TypeTopo!=0 && TypeTopo!=1){
		printf("MakePN() input error \n TYPETOPO should be 0 (random) or 1 (grid)\n");
		return ;
	}


// Declare variables
	int i,j;				// loop and array indices
	int NumRow, NumCol;
// Variables for GRID
	int LenGrid_x,  LenGrid_y,  LenGrid;
	double GridCoverage;

// Declare Node
	Node * PNode;
	PNode = new Node[NumNode];
// Declare Link
	int ** CapLink;			// Capacity of Link
	CapLink = new int*[NumNode];
	for(i=0;i<NumNode;i++)	CapLink[i] = new int[NumNode];
	int NumLink;			//# of links

// Temporary array and variable for checking connectivity using BFS
	int * Queue;
	Queue = new int[NumNode];
	int top, bot, temp;
	bool isConnected;
    isConnected=false;
	while(!isConnected){
//initialize
		for(i=0;i<NumNode;i++){
			for(j=0;j<NumNode;j++){
				CapLink[i][j]=0;
			}
			// Initialize Node
			PNode[i].x = -1;
			PNode[i].y = -1;
			PNode[i].coverage = 0.0;
			PNode[i].cpu = 0;
			PNode[i].OutLink = false;
			PNode[i].InLink = false;
			PNode[i].isConnected = false;
		}

		switch (TypeTopo){
		// 1: Grid_2
			case 1:
			// Arrange physical nodes. - Grid
				for(NumCol=0;NumCol*NumCol<NumNode;NumCol++){}
				for(NumRow=0;NumCol*NumRow<NumNode;NumRow++){}
				LenGrid_x = xrange/(NumCol+1);
				LenGrid_y = yrange/(NumRow+1);
				LenGrid = (LenGrid_x < LenGrid_y ? LenGrid_x:LenGrid_y);
				GridCoverage = LenGrid * 1.1;
				for(i=0;i<NumRow;i++) {
					for(j=0;j<NumCol;j++){
						if ((i*NumCol+j)>=NumNode)	break;
						PNode[i*NumCol+j].x = (j+1) * LenGrid;
						PNode[i*NumCol+j].y = (i+1) * LenGrid;
					}
				}
			// Set physical node spec [range, cpu] - Grid
				for(i=0;i<NumNode;i++){
					PNode[i].cpu = (random(nwmax-nwmin)+nwmin);
					PNode[i].coverage = GridCoverage;
				}
				break;
		// 0: Random
			case 0:
			// Arrange physical nodes. - random
				for(i=0;i<NumNode;i++){
					while(true){
						PNode[i].x = random(xrange); 		// dicrete value [0, xrange-1]
						PNode[i].y = random(xrange); 		// dicrete value [0, yrange-1]
						for(j=0;j<i;j++){
							if((PNode[i].x == PNode[j].x) && (PNode[i].y == PNode[j].y))	break;
						}
						// there is no pair of overlapping nodes
						if (j==i) break;
					}
				}
			// Set physical node spec [range, cpu] - Random
				for(i=0;i<NumNode;i++){
					PNode[i].cpu = (random(nwmax-nwmin)+nwmin);
					PNode[i].coverage = random(cvmax-cvmin)+cvmin;
				}
				break;
			default :
				printf("Wrong Type!\n");
				return ;
				break;
		}

// Get links according to nodes' coverages
// and then set CapLink;
		NumLink=0;
		int square_nome;
		double distance;
		for(i=0;i<NumNode;i++){
	    	for(j=0;j<i;j++){
				square_nome = (PNode[i].x - PNode[j].x)*(PNode[i].x - PNode[j].x) + (PNode[i].y - PNode[j].y)*(PNode[i].y - PNode[j].y);
				distance = sqrt(square_nome);
			// Directional link
				if(isDirectional){
				// from i to j	
					if (PNode[i].coverage >= distance){
						CapLink[i][j]= random(lwmax-lwmin)+lwmin;		// Capacity is randomly generated
						// OR, power decreasing according to distance
						//CapLink[i][j]= (PNode[i].converage/(distance+0.01));		// Capacity is generated according to distance
					}
					// from j to i
					if (PNode[j].coverage>=distance){
						CapLink[j][i]=random(lwmax-lwmin)+lwmin;
						// OR, power decreasing according to distance
						//CapLink[j][i]= (PNode[j].converage/(distance+0.01));
					}			
				}
			// (Bi)Undirectional link
				else{
					if ((PNode[i].coverage>=distance) && (PNode[i].coverage>=distance)){
						CapLink[i][j]=random(lwmax-lwmin)+lwmin;
						CapLink[j][i]=CapLink[i][j];
						//CapLink[j][i]=random(lwmax-lwmin)+lwmin;		// Asymetric capacity is randomly generated
						// OR, power decreasing according to distance
						//CapLink[i][j]= (PNode[i].converage/(distance+0.01));		// Capacity is generated according to distance
						//CapLink[j][i]= (PNode[j].converage/(distance+0.01));		// Capacity is generated according to distance
					}
				}
			}
		}
		// Count # of Links
		NumLink=0;
		if(isDirectional){
			for(i=0;i<NumNode;i++) {
				for(j=0;j<NumNode;j++) {
					if(CapLink[i][j]>0) NumLink++;
				}
			}
		}
		else{
			for(i=0;i<NumNode;i++) {
				for(j=i+1;j<NumNode;j++) {
					if(CapLink[i][j]>0) NumLink++;
				}
			}
		}

		// Update Isolation status and Detect Isolated node

		isConnected = true;

	// Isolation check via BFS (Broad First Search)
		for(i=0;i<NumNode;i++)	Queue[i] = -1;
		top=1;
		bot=0;
		Queue[0]=0;
		PNode[0].isConnected = true;
		while(top!=bot){		//BFS
			temp = Queue[bot];	//dequeue
			Queue[bot] = -1;
		    bot = (bot+1)%NumNode;
			    for(j=0;j<NumNode;j++){
				if( (!PNode[j].isConnected) && ((CapLink[temp][j]>0)||(CapLink[j][temp]>0)) ){
					PNode[j].isConnected = true;
					Queue[top] = j;		//enqueue
					top=(top+1)%NumNode;
					}
			}
		}
	// Additionally, we need to check that every node has both in and out links.
		if(isDirectional){
			for(i=0;i<NumNode;i++){
				for(j=0;j<NumNode;j++){
					if(CapLink[i][j]>0){
						PNode[i].OutLink = true;
						PNode[j].InLink = true;
					}
				}
			}
			for(i=0;i<NumNode;i++){
				PNode[i].isConnected = PNode[i].isConnected && PNode[i].OutLink && PNode[i].InLink;
			}
		}
		for(i=0;i<NumNode;i++){
		    if(!PNode[i].isConnected){
				isConnected=false;
				break;
		    }
		}
	
    }
	double AvgCoverage = 0;
	double VarCoverage = 0;
	double AvgCPU = 0;
	double VarCPU = 0;
	double AvgCapacity = 0;
	double VarCapacity = 0;
	for(i=0;i<NumNode;i++){
		AvgCoverage += (double)PNode[i].coverage;
		VarCoverage += (double)PNode[i].coverage * (double)PNode[i].coverage;
		AvgCPU += (double)PNode[i].cpu;
		VarCPU += (double)PNode[i].cpu * (double)PNode[i].cpu;
	}

	for(i=0;i<NumNode;i++){
		for(j=i+1;j<NumNode;j++){
			AvgCapacity += (double)CapLink[i][j];
			VarCapacity += (double)CapLink[i][j] * (double)CapLink[i][j];
		}
	}
	AvgCoverage = AvgCoverage/(double)NumNode;
	AvgCPU = AvgCPU/(double)NumNode;
	AvgCapacity = AvgCapacity/(double)NumLink;
	VarCoverage = VarCoverage/(double)NumNode - AvgCoverage * AvgCoverage;
	VarCPU = VarCPU/(double)NumNode - AvgCPU * AvgCPU;
	VarCapacity = VarCapacity/(double)NumLink - AvgCapacity * AvgCapacity;

	// We are done, write the physical node topology.
    FILE* SN_f;			// output file 1 ; "SN.txt"
	FILE* NODE_f;		// output file 2 ; "node.txt"
	FILE* TOPO_f;		// output file 3 ; "topology_graph.txt"


	stringstream st;
	stringstream st1;
	stringstream st2;
	stringstream st3;
	stringstream st4;
	stringstream st5;
	stringstream st6;
	string name;
	string name1;
	string name2;
	string name3;
	string name4;
	string name5;
	string name6;

	if(TypeTopo == 0){		// Random
		st<<"SN00"<<cvmax<<".txt";
		st1<<"node00"<<cvmax<<".txt";
		st2<<"random"<<cvmax<<".dem";
		st3<<"random"<<cvmax<<".txt";
	}
	else if(TypeTopo == 1){	// Grid
		st<<"SN11"<<NumNode<<".txt";
		st1<<"node11"<<NumNode<<".txt";
		st2<<"grid"<<NumNode<<".dem";
		st3<<"grid"<<NumNode<<".txt";
	}
	st>>name;
	st1>>name1;
	st2>>name2;
	st3>>name3;

    SN_f = fopen(name.c_str(),"wt");

	fprintf(SN_f,"%d %d\n", NumNode, NumLink);
	for(i=0;i<NumNode;i++){
			fprintf(SN_f,"%d %d %d\n", PNode[i].x, PNode[i].y, PNode[i].cpu);
	}

	// Draw nodes
	NODE_f = fopen(name1.c_str(),"wt");
	for(i=0;i<NumNode;i++)	fprintf(NODE_f,"%d %d\n", PNode[i].x, PNode[i].y);


	// Draw links

	TOPO_f = fopen(name2.c_str(),"wt");
	fprintf(TOPO_f, "set size square\n");
	fprintf(TOPO_f, "set noautoscale\n");
	fprintf(TOPO_f, "set style data points\n");
	fprintf(TOPO_f, "set xrange [0:%d]\n",xrange);
	fprintf(TOPO_f, "set yrange [0:%d]\n",yrange);
	fprintf(TOPO_f, "set term postscript eps color solid \"Time-Roman\" 30\n");
	st4<<"";
	if(TypeTopo==0){
		st4<<"set output 'output00"<<cvmax<<".eps'";
	}
	else if(TypeTopo==1){
		st4<<"set output 'output11"<<NumNode<<".eps'";
	}
	name4 = st4.str();
	fprintf(TOPO_f, "%s\n", name4.c_str());
	for(i=0;i<NumNode;i++) {
		for(j=0;j<NumNode;j++) {
			if(CapLink[i][j]>0) {
				if(isDirectional) {
					fprintf(TOPO_f, "set arrow from %d,%d to %d,%d lt -1 lw 1.0\n", PNode[i].x, PNode[i].y, PNode[j].x, PNode[j].y);
				}
				else {
					fprintf(TOPO_f, "set arrow from %d,%d to %d,%d nohead lt -1 lw 1.0\n", PNode[i].x, PNode[i].y, PNode[j].x, PNode[j].y);
				}
				if(isDirectional) {
					fprintf(SN_f, "%d %d %d\n", i, j, CapLink[i][j]);
				}
				else {
					if (j>i)	fprintf(SN_f, "%d %d %d\n", i, j, CapLink[i][j]);
				}
			}
		}
	}
	st5<<"";
	st5<<"plot '"<<name1.c_str()<<"' notitle pt 30 ps 1.5\n";
	name5 = st5.str();
	fprintf(TOPO_f,"%s", name5.c_str());
	fclose(SN_f);
	fclose(NODE_f);
	fclose(TOPO_f);

	FILE* pipe;
	st6<<"";
	st6<<"gnuplot "<<name2.c_str();
	name6 = st6.str();
	pipe = popen(name6.c_str(), "w");		// process open, write over
	fclose(pipe);

	delete [] Queue;
	delete [] PNode;
	for(i=0;i<NumNode;i++)	delete [] CapLink[i];
	delete [] CapLink;

	FILE* spec;
	spec = 	fopen(name3.c_str(),"wt");

	printf("==========================================\n");	
	printf("Physical Network is generated in SN.txt\n");
	printf("# of Node : %d\n", NumNode);
	printf("# of Link : %d\n", NumLink);
	printf("Density   : %4f [links / node]\n", (double)NumLink/NumNode);
	printf("Area      : %d [m] x %d [m] (x,y)\n", xrange, yrange);
	switch (TypeTopo) {
		case 2:		// Grid_4
			printf("Topology  : Grid (%d x %d)\n", NumRow, NumCol);
			break;
		case 1:		// Grid_2
			printf("Topology  : Grid (%d x %d)\n", NumRow, NumCol);
			break;
		case 0:		// Random		
		default:
			printf("Topology  : Random\n");
			printf("Coverage  : %d - %d [m]\n", cvmin, cvmax);
			break;
	}
	printf("Coverage  : [E]:%4f [m] [Var]: %4f [m^2]\n", AvgCoverage, VarCoverage);
	printf("Node CPU  : %d - %d\n", nwmin, nwmax);
	printf(">Node CPU : [E]:%4f [Var]: %4f\n", AvgCPU, VarCPU);
	printf("Link Cap. : %d - %d\n", lwmin, lwmax);
	printf(">Node Cap.: [E]:%4f [Var]: %4f\n", AvgCapacity, VarCapacity);
//	printf("Direction : %s\n", (isDirectional ? "Directional":"Un-directional"));
	printf("==========================================\n");
	fprintf(spec,"==========================================\n");	
	fprintf(spec,"Physical Network is generated in SN.txt\n");
	fprintf(spec,"# of Node : %d\n", NumNode);
	fprintf(spec,"# of Link : %d\n", NumLink);
	fprintf(spec,"Density   : %4f [links / node]\n", (double)NumLink/NumNode);
	fprintf(spec,"Area      : %d [m] x %d [m] (x,y)\n", xrange, yrange);
	switch (TypeTopo) {
		case 2:		// Grid_4
			fprintf(spec,"Topology  : Grid (%d x %d)\n", NumRow, NumCol);
			break;
		case 1:		// Grid_2
			fprintf(spec,"Topology  : Grid (%d x %d)\n", NumRow, NumCol);
			break;
		case 0:		// Random		
		default:
			fprintf(spec,"Topology  : Random\n");
			fprintf(spec,"Coverage  : %d - %d [m]\n", cvmin, cvmax);
			break;
	}
	fprintf(spec,"Coverage  : [E]:%4f [m] [Var]: %4f [m^2]\n", AvgCoverage, VarCoverage);
	fprintf(spec,"Node CPU  : %d - %d\n", nwmin, nwmax);
	fprintf(spec,">Node CPU : [E]:%4f [Var]: %4f\n", AvgCPU, VarCPU);
	fprintf(spec,"Link Cap. : %d - %d\n", lwmin, lwmax);
	fprintf(spec,">Node Cap.: [E]:%4f [Var]: %4f\n", AvgCapacity, VarCapacity);
//	fprintf(spec,"Direction : %s\n", (isDirectional ? "Directional":"Un-directional"));
	fprintf(spec,"==========================================\n");
	fclose(spec);
	return;
}




// poisson distribution
int poisson(int poimean){
	int poi = 0;
	double u, p, f;  
	f = p = exp(-1*(double)poimean); 
	u = (double)rand()/RAND_MAX;
	while (f <= u){
		p *= ((double)poimean/(poi+1.0));
		f += p;
		poi++;
    }
    return poi;
}

//exponential random distribution
int exponential(int expmean){
	int exp;
	double u;
    u = (double)rand()/RAND_MAX;
    exp = (int)-expmean*log(1-u); 
    return exp;
}


/*
 MakeVNs(int NumVN, int MaxNumNode, int nwmin, int nwmax, int lwmin, int lwmax, int order, bool isDirectional, int vType)

NumVN : # of VNs which will be generated.
MaxNumNode : Max. # of nodes in a VN
nwmin, nwmax : range of virtual node cpu resource ; [nwmin, nwmax]
lwmin, lwmax : range of virtual link capacity [lwmin, lwmax]
isDirectional : directional or undirectional (true or false)
flag :

Generate connected virtual network requests according to inputs.

Store the sequence of virtual network requests in "VN[order].txt"

*/ 
void MakeVNs(int NumVN, int MaxNumNode, int nwmin, int nwmax, int lwmin, int lwmax, int order, bool isDirectional, int vType){ //NumVN 만큼의 가상 네트워크 생성

	if (MaxNumNode>MAXVNODENUM){
		printf("MAXVNUMNODE error!\n");
		return;
	}
	if (vType != 0 && vType != 1 && vType != 2){
		printf("Virtual network topology type error!\n");
		return;
	}

	int i,j,q;

	int NumVNode;			//# of virtual nodes
	int NumLink;			//# of virtual links

	int TotNumVNode=0;
	int TotNumLink=0;

// Declare virtual nodes and links
	Node VNode[MAXVNODENUM];
	int CapLink[MAXVNODENUM][MAXVNODENUM];

// Temporary array and variable for checking connectivity using BFS
	int Queue[MAXVNODENUM];
	int top, bot, temp;
	bool isConnected;
	int IndexVN;
	int sT, rT;			// sT: Arrival Time, rT: duration
	int poi; 			// poisson variable
	int PrLink;
	int count = 1;

    int level;
    int lv1num;
    
	stringstream st;
	string name;

	if(vType == 0){		// Random
		st<<"VN00"<<POIMEAN<<EXPMEAN<<order<<".txt";
		st>>name;	
	}
	else if(vType == 1){// tree
		st<<"VN11"<<POIMEAN<<EXPMEAN<<order<<".txt";
		st>>name;
	}
	else{				// star
		st<<"VN22"<<POIMEAN<<EXPMEAN<<order<<".txt";
		st>>name;
	}

	st>>name;
	FILE* f;
	f = fopen(name.c_str(),"wt");
	
	q=0;
	poi = poisson(POIMEAN);
	while(poi==0){
		q++;
		poi=poisson(POIMEAN);
    }
    for(IndexVN=0;IndexVN<NumVN;IndexVN++){
		isConnected = false;
		while(!isConnected){
	    	for(i=0;i<MAXVNODENUM;i++){
				VNode[i].cpu = 0;
				VNode[i].isConnected = false;
				for(j=0;j<MAXVNODENUM;j++){
					CapLink[i][j] = 0;
				}
	    	}
			NumVNode = 0;
			top=0;
			bot=0;
			sT=0;
			rT=0;
			temp=0;
			NumLink = 0;
			// Generate virtual nodes first.
			NumVNode = random(MaxNumNode-MINVNODENUM+1) + MINVNODENUM;
			for(i=0;i<MAXVNODENUM;i++){
				VNode[i].isConnected = false;
				VNode[i].InLink = false;
				VNode[i].OutLink = false;
				Queue[i] = 0;
			}
			for(i =0; i<NumVNode; i++)
				VNode[i].cpu = (random(nwmax-nwmin)+nwmin); 

			NumLink = 0;

			if(vType == 0){
			// Generate Virtual network request
				PrLink = random(MAXPRVNLINK - MINPRVNLINK) + MINPRVNLINK;
				for(i=0;i<NumVNode; i++){
					for(j=i+1;j<NumVNode;j++){
						if(isDirectional){
							if(random(99)<PrLink){    // With PrLink(%), generate link from i to j;
								CapLink[i][j] = random(lwmax-lwmin)+lwmin;
								NumLink++;
							}
							if(random(99)<PrLink){    // With PrLink(%), generate link from j to i;
								CapLink[j][i] = random(lwmax-lwmin)+lwmin;
								NumLink++;
							}

						}
						else{
							if(random(99)<PrLink){    // With PrLink(%), generate link from j to i;
								CapLink[i][j] = random(lwmax-lwmin)+lwmin;
								CapLink[j][i] = CapLink[i][j];
								NumLink++;
							}
						}
					}
				}

			// Check connectivity of the Virtual network request via BFS
				isConnected = true;
				for(i=0; i<NumVNode; i++)	Queue[i] = -1;
				for(i=0; i<NumVNode; i++)	VNode[i].isConnected = false;

				VNode[0].isConnected = true;
				Queue[0] = 0;
				top = 1;
				bot = 0;
	
				while(top!=bot){				// BFS
					temp=Queue[bot];			// dequeue
					Queue[bot]=-1;
					bot=(bot+1)%NumVNode;
					for( j=0;j<NumVNode;j++){
						if( (!VNode[j].isConnected)&& ((CapLink[temp][j]>0)||(CapLink[j][temp]>0))){
							VNode[j].isConnected = true;
							Queue[top] = j;		//enqueue
							top=(top+1)%NumVNode;
						}
					}
				}
			// Additionally, we need to check that every node has both in and out links.
				if(isDirectional){
					for(i=0;i<NumVNode;i++){
						for(j=0;j<NumVNode;j++){
							if(CapLink[i][j]>0){
								VNode[i].OutLink = true;
								VNode[j].InLink = true;
							}
						}
					}
					for(i=0;i<NumVNode;i++){
						VNode[i].isConnected = VNode[i].isConnected && VNode[i].OutLink && VNode[i].InLink;
					}
				}
				for(i=0;i<NumVNode;i++){
					if(!VNode[i].isConnected){
						isConnected=false;
						break;
					}
				}
			}
			else if(vType == 1){
				if(NumVNode>=7){
					level = 3;
				}
				else{
					j=random(3)+4;    
					if(NumVNode <= j){
						level = 2;
					}
					else{
						level = 3;
					}
				}
	
				if(level==2){
					for( i = 1; i<NumVNode; i++){                                                            
						CapLink[0][i] = random(lwmax-lwmin)+lwmin;
						NumLink++;
					}
				}
				else{
					if(NumVNode<=7){
						j=random(3)+5;
						if(NumVNode<=j)
							lv1num=2;
						else
							lv1num=3;
					}
					else{
						j=random(3)+8;
						if(NumVNode<=j)
							lv1num=3;
						else
							lv1num=4;
					}
			
					for(i=1;i<=lv1num;i++){
						CapLink[0][i] = random(lwmax-lwmin)+lwmin;
						NumLink++;
					}
					for(i = lv1num+1;i<NumVNode;i++){
						j=random(lv1num-1)+1;
						CapLink[j][i] = random(lwmax-lwmin)+lwmin;
						NumLink++;
					}
				}
				isConnected=true;
			}
			else{
				for(i=1;i<NumVNode;i++){                                                            
					CapLink[0][i] = random(lwmax-lwmin)+lwmin;
					NumLink++;
				}
				isConnected=true;
			}
		}


		sT = random(TWIN)+q*TWIN;
		rT = (exponential(EXPMEAN-1)+1)*TWIN;
	   
		fprintf(f, "%c%c\n",'%','%');
		fprintf(f, "%d %d %d %d\n", NumVNode, NumLink, sT, rT);
		for(i=0;i<NumVNode;i++){
			fprintf(f, "%d \n", VNode[i].cpu); 
		}
		
		for(i=0;i<NumVNode;i++){
			for(j=0;j<NumVNode;j++){
				if( CapLink[i][j]>0){
					if(isDirectional){
						fprintf(f, "%d %d %d\n", i, j, CapLink[i][j]);
					}
					else{
						if(j>i)
							fprintf(f, "%d %d %d\n", i, j, CapLink[i][j]);
					}
				}
			}
		}
		if(count==poi) {		
			count=1;
			poi = poisson(POIMEAN);
			while(poi==0){
				poi=poisson(POIMEAN);
				q++;
			}
			q++;
		}
		else
			count++;

/* Show Density
printf("%f %d %d %d\n", (double)NumLink/NumVNode, NumLink, NumVNode, PrLink);
*/
		TotNumVNode+=NumVNode;
		TotNumLink+=NumLink;
	}
    fclose(f);
	printf("==========================================\n");	
	printf("Virtual Network Requests are generated in VN%d.txt\n", order);
	printf("# of Req.s: %d\n", NumVN);
	printf("# of Node : %d - %d\n", 1, MaxNumNode);
	printf("VNode CPU : %d - %d\n", nwmin, nwmax);
	printf("VLink Cap.: %d - %d\n", lwmin, lwmax);
	printf("Link Pr.  : %d - %d percents\n", MINPRVNLINK, MAXPRVNLINK);
	printf("Avg.Dense : %4f [links / node]\n", (double)TotNumLink/TotNumVNode);
//	printf("Direction : %s\n", (isDirectional ? "Directional":"Un-directional"));
	printf("==========================================\n");
}

int main() {
	randomize();
	bool isDirectional = false;
    MakePN(SNODENUM, XRANGE, YRANGE, CVWMIN, CVWMAX, SNWMIN, SNWMAX, SLWMIN, SLWMAX, isDirectional, TYPETOPO);
    for(int i=0; i<10; i++){
		MakeVNs(VNNUM, MAXVNODENUM, VNWMIN, VNWMAX, VLWMIN, VLWMAX, i, isDirectional, VTYPETOPO);
	}
    return 0;
}

