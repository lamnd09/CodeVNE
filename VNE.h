// includes
#include <sstream>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <math.h>
#include <time.h>
#include <string.h>
#include "VNEGeneral.h"
#include "VNEOrder.h"
#include "VNEAlgorithm.h"
#include "AdmissionControl.h"
#include "VNECandidate.h"
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

class PLink {
	// Link data
public :
	int node1;		// One of end node of the link ;
	int node2;		// Another end node ; 
	int capacity;		// Capacity ; 
	int filledW;	// Mapped capacity ; 
	int embedVNID[MAXEMBEDLINK]; 	// List of VN ID that requires VLink which was embedded on this link ; 
	int embedVLink[MAXEMBEDLINK]; 	// List of VLink ID that is embedded ;
	int intfnum;	// # of links which are interfering with this link ;
		// Default Constructor - empty
	PLink(){
		filledW = 0;
	}
	// Copy Contructor
	PLink(const PLink& s){
		node1 = s.node1;
		node2 = s.node2;
		capacity = s.capacity;
		filledW = s.filledW;
		intfnum = s.intfnum;
		for(int i=0;i<MAXEMBEDLINK; i++){
			embedVNID[i] = s.embedVNID[i];
			embedVLink[i] = s.embedVLink[i];
		}
	}
	~PLink(){
	
	}
};


class PNode {
// Node data
public :
	int cpu;		
	int* Links;
	int NumLink;
	int filledW;
	int x;
	int y;
	int embedVNID[MAXEMBEDNODE];
	int embedVNode[MAXEMBEDNODE];
	// Default Contructor
	PNode(){
	}
	// Constructor with input : # of links which are connected with this node
	PNode(int numlink){
		filledW = 0;
		Links = new int[numlink]; 
		NumLink = numlink;
	}
	// Copy Constructor
	PNode(const PNode& s){
		cpu = s.cpu;
		NumLink = s.NumLink;
		filledW = s.filledW;
		x = s.x;
		y = s.y;
		for(int i=0;i<MAXEMBEDNODE;i++){
			embedVNID[i]=s.embedVNID[i];
			embedVNode[i]=s.embedVNode[i];
		}
		Links = new int[NumLink];
		for(int i=0; i<NumLink; i++) Links[i] = s.Links[i];
	}
	~PNode(){
		delete [] Links;
	}
};

class VLink{
	// Link data
public :
	int node1;		// ID of PNode which is one of end of VLink ; 
	int node2;		// ID of PNode which is another of end of VLink ; 
	int capacity;	// Requiring capacity
	int embeddedPath[MAXPATH];
	// Default Constructor
	VLink(){ }
	// Copy Constructor
	VLink(const VLink & v){
		node1 = v.node1;
		node2 = v.node2;
		capacity = v.capacity;
		for(int i=0;i<MAXPATH;i++)	embeddedPath[i] = v.embeddedPath[i];	 
	}
};

class VNode{
// Node data
public :
	int cpu;			// Requiring CPU 
	int* Links;			// List of Links which are connected with this VNode 
	int NumLink;		// # of Links which are connected with this VNode
	int embeddedPNID;
	// Default Constructor
	VNode() {}
	// Constructor    
	VNode(int numlink){
		Links = new int[numlink];
		NumLink = numlink;
	}
	// Copy Constructor
	VNode(const VNode& v){
		cpu = v.cpu;
		NumLink = v.NumLink;
		embeddedPNID = v.embeddedPNID;
		Links = new int[NumLink];
		for(int i=0;i<NumLink;i++)		Links[i] = v.Links[i];
	}
	~VNode(){
		delete[] Links;
	}
};


class PNet{
//Subnet 
public:
	PNode** Nodes;
	PLink** Links;		// Physical Links
	int NumNode;		// # of physical nodes 
	int NumLink;		// # of physical links
	int** map;			// adjacent matrix. int*-map[i] 
	// Default Constructor     
	PNet(){
	}
	// Constructor
	PNet(int numnode, int numlink){
		//initialize
		int i, j;
		NumNode = numnode;
		NumLink = numlink;
		Nodes = new PNode*[NumNode];
		Links = new PLink*[NumLink];
		map = new int*[NumNode];
		for( i=0;i<NumNode;i++){
			map[i]= new int[NumNode];
			for(j=0;j<NumNode;j++) map[i][j]=0;
		}
	}
	// Copy Constructor
	PNet(const PNet& s){
		int i,j;
		NumNode = s.NumNode;
		NumLink = s.NumLink;
		Nodes = new PNode*[NumNode];
		Links = new PLink*[NumLink];
		for(i=0;i<NumNode;i++)	Nodes[i] = new PNode(*(s.Nodes[i]));
	 	for(i=0;i<NumLink;i++)	Links[i] = new PLink(*(s.Links[i]));

		map = new int*[NumNode];
		for(i=0;i<NumNode;i++){
			map[i]=new int[NumNode];
			for(j=0;j<NumNode;j++)	map[i][j] = s.map[i][j];
		}
	}

	// Metric Function
	double embmetric(){
		double metric = 0.0;
		for(int i=0; i<NumLink; i++){
			if(NORMTYPE == 1) {			// (SUM, 1 - norm)
				metric += ((double)Links[i]->filledW / Links[i]->capacity) * Links[i]->intfnum;
			}
			else if(NORMTYPE > 0){		// (SUM, normtype - norm)
				metric += pow(((double)Links[i]->filledW / Links[i]->capacity) * Links[i]->intfnum, (double)NORMTYPE);
			}
			else {						// (MAX, inf norm)
				if(((double)Links[i]->filledW / Links[i]->capacity) * Links[i]->intfnum > metric)
					metric = ((double)Links[i]->filledW / Links[i]->capacity) * Links[i]->intfnum;		
			}
		}
		if(NORMTYPE !=1 && NORMTYPE >0)	metric = pow(metric, 1.0 /((double)NORMTYPE));
		return metric;
	}
	~PNet(){
		int i;

		for(i=0;i<NumNode;i++)
			delete Nodes[i];
		delete [] Nodes;

		for(i=0;i<NumLink;i++)
			delete Links[i];
		delete [] Links;
		for(i=0;i<NumNode;i++)
			delete [] map[i];
		delete [] map;
     }
};


class VNet{
//Subnet and Virnet
public :
	int startT;
	int RqT;
	int endT;
	int delayed;
	int NumNode;
	int NumLink;
	int revenue;	// revenue of this VNet Req.
	bool isEmbedded;
	VNode** Nodes;
	VLink** Links;
	int** map;
	// Default Constructor
	VNet(){
		isEmbedded = false;
		delayed = 0;
	}
	// Constructor
	VNet(int numnode, int numlink, int sT, int rT){
		// initialize
		int i, j;
		Nodes = new VNode*[numnode];
		Links = new VLink*[numlink];
		map = new int*[numnode];
		for(i=0;i<numnode;i++){
			map[i]= new int[numnode];
			for( j=0;j<numnode;j++)	map[i][j]=0;
		}
		NumNode = numnode;
		NumLink = numlink;
		startT = sT;
		RqT = rT;
		isEmbedded = false;
		delayed = 0;
	}

	// Copy Constructor
	VNet(const VNet& v){
		int i,j;
		startT = v.startT;
		RqT = v.RqT;
		endT=v.endT;
		delayed = v.delayed;
		NumNode = v.NumNode;
		NumLink = v.NumLink;
		revenue = v.revenue;
		isEmbedded = v.isEmbedded;
		Nodes = new VNode*[NumNode];
		Links = new VLink*[NumLink];
		for(i=0;i<NumNode;i++)	Nodes[i] = new VNode(*(v.Nodes[i]));
		for(i=0;i<NumLink;i++)	Links[i] = new VLink(*(v.Links[i]));
		map=new int*[NumNode];
		for(i=0;i<NumNode;i++){
			map[i]=new int[NumNode];
			for(j=0;j<NumNode;j++)	map[i][j]=v.map[i][j];
		}
	}
	~VNet(){
		int i;
		for(i=0;i<NumNode;i++)	delete Nodes[i];
		delete [] Nodes;
	 	for(i=0;i<NumLink;i++)	delete Links[i];
		delete [] Links;
		for(i=0;i<NumNode;i++)	delete [] map[i];
		delete [] map;
	}
};


class MWIS{

public:
	int* IS;
	double weight;
	int vnum;
	// Default Constructor
	MWIS(){
		IS = NULL;
		weight =-1;
		vnum =0;
	}
	// Constructor
    MWIS(int vnum_, int* is, double *w){
		vnum = vnum_;
		IS = new int[vnum];
		weight = 0;
		for(int i=0;i<vnum;i++){
			IS[i] = is[i];
			weight += is[i]*w[i];
		}
	}
	// Copy Constructor
	MWIS(const MWIS &m){
		vnum = m.vnum;
		IS = new int[vnum];
		for(int i=0;i<vnum;i++){
			IS[i] = m.IS[i];
		}
		weight = m.weight;
	}
    ~MWIS(){
		delete[] IS;
    }
};


void ReadSN(char* order);		// void MakeSN();
void MakeVNs(char* order);
void MakeOriginCG();
void Calcspaths();
	void SpathfromRoot(int root);		//  subroutine for Calcspaths()
		double Distance(int n1, int n2, double* weightedlength);	// subroutine for SpathfromRoot()
		int getSNlink(int node1, int node2);
void ShortestTest(int source, int dest);				// Shortest Debugging subroutine
void Unmap(int VID);
void NodeMapOrder(int VID, int* output);
	void BFS(int **mappt, int numnode, int root, int** inout);	// NodeMapOrder()의 subroutine
void SNodeOrder(int* output );
//void SNcpy(PNet* to, PNet* from);
//void VNcpy(VNet* to, VNet* from);
void NEmbedTo(int VID, int from, int to);
void EEmbedTo(int VID, int from, int* to);

bool Embed_SUM_PBGON(int VID, int NID, int* embedArr);		// Embedding based on SUM NORM
bool Embed_SUM_PBGOFF(int VID, int NID, int* embedArr);		// Embedding based on SUM NORM
bool Embed_MAX_Weight_PBGON(int VID, int NID, int* embedArr);		// Embedding based on MAX NORM
	bool PBGcheck(const PNet& PSN);						// Embed1 & 2() subroutine

bool Necessary_PBG(const PNet& FSN);
bool FeasCheck_SUF(const PNet& FSN);
bool FeasCheck_MWIS_NEC(const PNet& FSN);
bool FeasCheck_MWIS(const PNet& FSN);
	void BFSsub(int** wcg, int numnode, int root, int r, int** subneigh, int* vsub);
	//	void BFS(int **mappt, int numnode, int root, int** inout);	// BFSsub()의 subroutine
	double AMWIS(int** subneigh, int* vsub, double* qlength, int numlink, int* mwis); 
		void MWISFIND(int** grph, int* mwis, int* alive, double* qlength, MWIS& mw1, int vnum);
int VNRevenue(VNet* VN);
int VNLinkAccept(VNet* VN);
int VNNodeAccept(VNet* VN);
int VNCapAccept(VNet* VN);
int VNCpuAccept(VNet* VN);



bool compembed(int VID);
bool compembed(int VID, int rootnode);
	void Findsnode(int* found, int rootnode, int num);
