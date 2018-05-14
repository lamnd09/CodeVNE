#include "VNE.h"
#include "VNEGeneral.h"
#include "VNEOrder.h"
#include "VNEAlgorithm.h"
#include "AdmissionControl.h"
#include "VNECandidate.h"
#include "scenario.h"
#include "iostream"
#include "fstream"
//#define MAXVNODENUM 50

using namespace std;

PNet* PN;			// physical network
PNet* bufPN;
PNet* tempPN;
PNet* nicePN;

VNet** VNs;			// virtual networks
VNet* tempVN;
VNet* niceVN;
VNet* bufVN;

int **Conflict;			// Conflict Graph

int ShortestPath[SNODENUM][SNODENUM][MAXPATH];		// shortest path
double distances[SNODENUM][SNODENUM];			// distance between two nodes

bool PBGfail = false;	// PBG graph or not

ofstream flog;		// log file

ofstream Rresult;
ofstream Tresult;

// Request Queue
class queue{
public :
  int* storage;		// queue
  int Qsize;		// queue max size
  int size;		// queue size, i.e., # items in queue
  int top;
  int bot;
  // Default Constructor
  queue(){}  
  // Constructor
  queue(int quesize){
	int i;
	storage = new int[quesize];
	for( i=0;i<quesize;i++)	storage[i] = -1;
	Qsize = quesize;
	top = 0;
	bot = 0;
	size = 0;
  }
  void enqueue(int ev){
	int i;
	if(size!=(top-bot+Qsize)%Qsize)	cout<<"error!!!"<<endl;
	//TODO:: enqueue
	if(size<Qsize-1){
	  storage[top]=ev;
	  top=(top+1)%Qsize;
	  size++;
	}
	else{
	  flog<<"Queue is full!"<<endl;
	  if(EXPANDWINQ == 0){
		storage[top] = ev;
		top=(top+1)%Qsize;
		storage[bot] = -1;
		bot=(bot+1)%Qsize;
		size = size;
	  }
	  else{
		int OldQsize = Qsize;
		int* ExpandStrg;
		Qsize = 2*OldQsize;
		ExpandStrg = new int[Qsize];
		for(i=0;i<OldQsize;i++)
		  ExpandStrg[i] = storage[(i+bot)%OldQsize];
		delete [] storage;
		storage = new int[Qsize];
		for(i=0;i<Qsize;i++)
		  storage[i] = -1;
		for(i=0;i<OldQsize;i++)
		  storage[i] = ExpandStrg[i];
		bot = 0;
		top = size;

		storage[top] = ev;
		top=(top+1)%Qsize;
		size++;
		delete [] ExpandStrg;
		ExpandStrg = NULL;
	  }
	}
	if(size!=(top-bot+Qsize)%Qsize)	cout<<"error!!!"<<endl;
  }  
  int dequeue(){
	//TODO:: dequeue
	int temp;
	if(size!=(top-bot+Qsize)%Qsize)	cout<<"error!!!"<<endl;
	if(size==0)
	  return -1;
	temp = storage[bot];
	storage[bot]=-1;
	bot=(bot+1)%Qsize;
	size--;
	if(size!=(top-bot+Qsize)%Qsize)	cout<<"error!!!"<<endl;
	return temp;            
  }
  void sort(int key){	//key = 1..6 : choice of key-metric for ordering, key = 0: service order
	//sort
	if(size!=(top-bot+Qsize)%Qsize)	cout<<"error!!!"<<endl;
	int i,j;
	double k;
	int* temp = new int[Qsize];
	double* worth = new double[Qsize];
	int index;
	double biggest;
	for(i=0;i<Qsize;i++)	temp[i]=storage[i];
	for(i=0;i<Qsize;i++){
	  if(i<size)
		storage[i] = temp[(bot+i)%Qsize];
	  else
		storage[i] = -1;
	}
	bot = 0;
	top = size;

	for(i=0;i<Qsize;i++)
	  worth[i] = -1;
	
	if(key==1){
	  for(i=0;i<size;i++)
		worth[i] = 0;
	}
	// 1 / density
	else if(key==2){  
	  for(i=0;i<size;i++)
		worth[i] = (double)(VNs[storage[i]]->NumNode)/(double)(VNs[storage[i]]->NumLink);
	  flog<<"top :"<<top<<"bot: "<<bot<<"size: "<<size<<endl;
	  for(i=0;i<Qsize;i++){
		flog<<"worth["<<i<<"] : "<<worth[i]<<"// storage["<<i<<"] : "<<storage[i]<<endl;
	  }
	  for(i=1;i<size;i++){
		biggest = worth[i-1];
		index = storage[i-1];
		for(j=i;j<size;j++){
		  if(biggest<worth[j]){
			worth[i-1] = worth[j];
			storage[i-1] = storage[j];
			worth[j] = biggest;
			storage[j] = index;
			biggest = worth[i-1];
			index = storage[i-1];
		  }
		}			
	  }
	}
	// Revenue decreasing order
	else if(key==3){
	  for(i=0;i<size;i++){
		worth[i] =0;
		for(j=0;j<VNs[storage[i]]->NumNode;j++){

		  worth[i] += VNs[storage[i]]->Nodes[j]->cpu;
		}
		for(j=0;j<VNs[storage[i]]->NumLink;j++){
		  worth[i] += ALPHA*VNs[storage[i]]->Links[j]->capacity;
		}
	  }
	  flog<<"top :"<<top<<"bot: "<<bot<<"size: "<<size<<endl;
	  for(i=0;i<Qsize;i++){
		flog<<"worth["<<i<<"] : "<<worth[i]<<"// storage["<<i<<"] : "<<storage[i]<<endl;
	  }



	  for(i=1;i<size;i++){
		biggest = worth[i-1];
		index = storage[i-1];
		for(j=i;j<size;j++){
		  if(biggest<worth[j]){
			worth[i-1] = worth[j];
			storage[i-1] = storage[j];
			worth[j] = biggest;
			storage[j] = index;
			biggest = worth[i-1];
			index = storage[i-1];
		  }
		}			
	  }
	}
	// Revenue * time
	else if(key==4){
	  for(i=0;i<size;i++){
		worth[i] =0;
		for(j=0;j<VNs[storage[i]]->NumNode;j++){
		  worth[i] += VNs[storage[i]]->Nodes[j]->cpu;
		}
		for(j=0;j<VNs[storage[i]]->NumLink;j++){
		  worth[i] += ALPHA*VNs[storage[i]]->Links[j]->capacity;
		}
		worth[i] = worth[i] * (double)(VNs[storage[i]]->RqT) /(double)(TWIN);
	  }
	  flog<<"top :"<<top<<"bot: "<<bot<<"size: "<<size<<endl;
	  for(i=0;i<Qsize;i++){
		flog<<"worth["<<i<<"] : "<<worth[i]<<"// storage["<<i<<"] : "<<storage[i]<<endl;
	  }

	  for(i=1;i<size;i++){
		biggest = worth[i-1];
		index = storage[i-1];
		for(j=i;j<size;j++){
		  if(biggest<worth[j]){
			worth[i-1] = worth[j];
			storage[i-1] = storage[j];
			worth[j] = biggest;
			storage[j] = index;
			biggest = worth[i-1];
			index = storage[i-1];
		  }
		}			
	  }
	}
	// Revenue/Density decreasing order
	else if(key==5){
	  for(i=0;i<size;i++){
		worth[i] =0;
		for(j=0;j<VNs[storage[i]]->NumNode;j++){
		  worth[i] += VNs[storage[i]]->Nodes[j]->cpu;
		}
		for(j=0;j<VNs[storage[i]]->NumLink;j++){
		  worth[i] += ALPHA*VNs[storage[i]]->Links[j]->capacity;
		}
		worth[i] = worth[i] *(double)(VNs[storage[i]]->NumNode) / (double)(VNs[storage[i]]->NumLink);
	  }
	  flog<<"top :"<<top<<"bot: "<<bot<<"size: "<<size<<endl;
	  for(i=0;i<Qsize;i++){
		flog<<"worth["<<i<<"] : "<<worth[i]<<"// storage["<<i<<"] : "<<storage[i]<<endl;
	  }

	  for(i=1;i<size;i++){
		biggest = worth[i-1];
		index = storage[i-1];
		for(j=i;j<size;j++){
		  if(biggest<worth[j]){
			worth[i-1] = worth[j];
			storage[i-1] = storage[j];
			worth[j] = biggest;
			storage[j] = index;
			biggest = worth[i-1];
			index = storage[i-1];
		  }
		}			
	  }
	}
	// Revenue*time/Density decreasing order
	else if(key==6){
	  for(i=0;i<size;i++){
		worth[i] =0;
		for(j=0;j<VNs[storage[i]]->NumNode;j++){
		  worth[i] += VNs[storage[i]]->Nodes[j]->cpu;
		}
		for(j=0;j<VNs[storage[i]]->NumLink;j++){
		  worth[i] += ALPHA*VNs[storage[i]]->Links[j]->capacity;
		}
		worth[i] = worth[i] * (double)(VNs[storage[i]]->RqT) /(double)(TWIN) *(double)(VNs[storage[i]]->NumNode) / (double)(VNs[storage[i]]->NumLink);
	  }
	  flog<<"top :"<<top<<"bot: "<<bot<<"size: "<<size<<endl;
	  for(i=0;i<Qsize;i++){
		flog<<"worth["<<i<<"] : "<<worth[i]<<"// storage["<<i<<"] : "<<storage[i]<<endl;
	  }

	  for(i=1;i<size;i++){
		biggest = worth[i-1];
		index = storage[i-1];
		for(j=i;j<size;j++){
		  if(biggest<worth[j]){
			worth[i-1] = worth[j];
			storage[i-1] = storage[j];
			worth[j] = biggest;
			storage[j] = index;
			biggest = worth[i-1];
			index = storage[i-1];
		  }
		}			
	  }
	}

	else if(key==0){ 
	  for(i=0;i<size;i++)
		worth[i] = VNs[storage[i]]->endT;
	  for(i=1;i<size;i++){
		biggest = worth[i-1];
		index = storage[i-1];
		for(j=i;j<size;j++){
		  if(biggest>worth[j]){			
			worth[i-1] = worth[j];
			storage[i-1] = storage[j];
			worth[j] = biggest;
			storage[j] = index;
			biggest = worth[i-1];
			index = storage[i-1];
		  }
		}			
	  }

	}


	if(key==0)
	  flog<<"EmbeddedQ Sort Result - "<<size<<" ("<<top<<" "<<bot<<")";
	else
	  flog<<"WindowQ Sort Result - "<<size<<" ("<<top<<" "<<bot<<")";

	for(i=0;i<Qsize;i++){
	  if(storage[i]!=-1){
		flog<<" ("<<i<<" | "<<storage[i]<<" | "<<worth[i]<<") ";
	  }
	}
	flog<<endl;

	delete [] temp;
	delete [] worth;
	if(size!=(top-bot+Qsize)%Qsize)	cout<<"error!!!"<<endl;
  }             
  bool isEmpty(){
	//isEmpty 1: empty, 0: not empty
	if(size!=(top-bot+Qsize)%Qsize)	cout<<"error!!!"<<endl;
	if(size==0)	return true;
	else 		return false;
  }

  ~queue(){
	delete [] storage;
  }
};



queue WindowQ(MAXWINQ);
queue EmbeddedQ(MAXEBEDVN);
int TotalVNSet = 0;
// FeasType = 1,  SUF
int SUF_O = 0;
int SUF_X = 0;

// FeasType = 2,  PBG, SUF
int PBG_O = 0;
int PBG_X = 0;
int PBG_O_SUF_X = 0;
int PBG_O_SUF_O = 0;

// FeasType = 3, PBG, MWIS
//	int PBG_O = 0;
//	int PBG_X = 0;
int PBG_O_MWIS_X = 0;
int PBG_O_MWIS_O = 0;

// FeasType = 4, PBG, Nec, MWIS
//	int PBG_O = 0;
//	int PBG_X = 0;
int PBG_O_NEC_X = 0;
int PBG_O_NEC_O = 0;
int PBG_O_NEC_O_MWIS_X = 0;
int PBG_O_NEC_O_MWIS_O = 0;

// FeasType = 5, PBG, SUF, MWIS
//	int PBG_O = 0;
//	int PBG_X = 0;
//	int PBG_O_SUF_X = 0;
//	int PBG_O_SUF_O = 0;
int PBG_O_SUF_X_MWIS_X = 0;
int PBG_O_SUF_X_MWIS_O = 0;


// FeasType = 6, PBG, SUF, Nec, MWIS
//	int PBG_O = 0;
//	int PBG_X = 0;
//	int PBG_O_SUF_X = 0;
//	int PBG_O_SUF_O = 0;
int PBG_O_SUF_X_NEC_X = 0;
int PBG_O_SUF_X_NEC_O = 0;
int PBG_O_SUF_X_NEC_O_MWIS_X = 0;
int PBG_O_SUF_X_NEC_O_MWIS_O = 0;




int main(int argc, char** argv){
  int i,j;
  int Time;
  int temp;
  int mapVnet;   //virtual network to be embedded
  bool mapped;
  int winnum;    //index of window
  bool feas;
  bool nodemapok;
  double embmetric;
  double EmbMetric[TRY];
  int nice =-1;
  int sort_index;
  int sort_temp;
  int windowqsize = 0;


  bool while1, while2;
  int VOrderArr[MAXVNODENUM];
  int SOrderArr[SNODENUM];
  int EmdArr[SNODENUM];

  // Statics Variables

  int TotRevenue = 0;
  int CurrentRevenue= 0;
  double AvRevenue = 0;			// TotRevenue / winnum

  int trycount = 0;
  int VNcount =0;
  int CurrentTry = 0;
  int CurrentVNcount =0;
  int acceptcount = 0;
  int CurrentAccept = 0;
  double accptrat = 0;			// acceptcount / trycount
  double Currentaccptrat = 0;		// (CurrentTry !=0) ? (CurrentAccept/CurrentTry) : 0;
	
  int LinkAccept =0;
  int CapAccept = 0;
  int CurrentLinkAccept = 0;
  int CurrentCapAccept = 0;

  int NodeAccept =0;
  int CpuAccept = 0;
  int CurrentNodeAccept = 0;
  int CurrentCpuAccept = 0;

  double AcceptDensity = 0;		// LinkAccept / NodeAccept
  double CurrentAcceptDensity = 0;	// CurrentLinkAccept / CurrentNodeAccept

  double CurrentAvgCpuRatio = 0;
  double CurrentAvgCapRatio = 0;

  double CurrentSqrCpuRatio = 0;
  double CurrentSqrCapRatio = 0;

  double CurrentVarCpuRatio = 0;
  double CurrentVarCapRatio = 0;

  double CurrentAvgCpuRatio_ON = 0;
  double CurrentAvgCapRatio_ON = 0;

  double CurrentSqrCpuRatio_ON = 0;
  double CurrentSqrCapRatio_ON = 0;

  double CurrentVarCpuRatio_ON = 0;
  double CurrentVarCapRatio_ON = 0;

  int CurrentOnLink = 0;		// How many phyical links are being used
  int CurrentOnNode = 0;		// How many phyical nodes are being used

	
  double TotCpuRatio =0;		// total busy time of nodes
  double TotCapRatio =0;		// total busy time of links


  double TotSqrCpuRatio = 0;
  double TotSqrCapRatio = 0;

  int TotOnLink = 0;
  int TotOnNode = 0;


  double AvgCpuRatio = 0;		// TotCpuRatio / (winnum * NumNode)
  double AvgCapRatio = 0;		// TotCapRatio / (winnum * NumLink)


  double AvgVarCpuRatio = 0;		// (TotSqrCpuRatio - TotCpuRatio * TotCpuRatio) / (winnum * NumNode)
  double AvgVarCapRatio = 0;		// (TotSqrCapRatio - TotCapRatio * TotCapRatio) / (winnum * NumNode)

  double AvgCpuRatio_ON = 0;		// TotCpuRatio / (TotOnNode)
  double AvgCapRatio_ON = 0;

  double AvgVarCpuRatio_ON = 0;		// (TotSqrCpuRatio_ON - TotCpuRatio_ON * TotCpuRatio_ON) / (TotOnNode)
  double AvgVarCapRatio_ON = 0;

  int AvgOnLink = 0;			// TotOnLink / winnum
  int AvgOnNode = 0;			// TotOnNode / winnum


  stringstream st;
  string resultfile;
  stringstream st1;
  string resultfile1;
  stringstream st2;
  string resultfile2;

  st<<"RealTimeResult_";
  st1<<"TotalResult_";
  st2<<"Log_";
  if(ALGTYPE_LENGTH==1){
	if(ALGTYPE_COUPLE==1){
	  st<<"Alg1";
	  st1<<"Alg1";
	  st2<<"Alg1";
	}
	else if(ALGTYPE_COUPLE==2){
	  st<<"Alg2";
	  st1<<"Alg2";
	  st2<<"Alg2";
	}
	else if(ALGTYPE_COUPLE==3){
	  st<<"Alg3";
	  st1<<"Alg3";
	  st2<<"Alg3";
	}
  }
  else if(ALGTYPE_LENGTH==2){
	if(ALGTYPE_COUPLE==1){
	  st<<"Alg4";
	  st1<<"Alg4";
	  st2<<"Alg4";
	}
	else if(ALGTYPE_COUPLE==2){
	  st<<"Alg5";
	  st1<<"Alg5";
	  st2<<"Alg5";
	}
	else if(ALGTYPE_COUPLE==3){
	  st<<"Alg6";
	  st1<<"Alg6";
	  st2<<"Alg6";
	}
  }

  if(FEASTYPE==1){
	st<<"_SUF";
	st1<<"_SUF";
	st2<<"_SUF";
  }
  else if(FEASTYPE==2){
	st<<"_PBG_SUF";
	st1<<"_PBG_SUF";
	st2<<"_PBG_SUF";
  }
  else if(FEASTYPE==3){
	st<<"_PBG_MWIS";
	st1<<"_PBG_MWIS";
	st2<<"_PBG_MWIS";
  }
  else if(FEASTYPE==4){
	st<<"_PBG_NEC_MWIS";
	st1<<"_PBG_NEC_MWIS";
	st2<<"_PBG_NEC_MWIS";
  }
  else if(FEASTYPE==5){
	st<<"_PBG_SUF_MWIS";
	st1<<"_PBG_SUF_MWIS";
	st2<<"_PBG_SUF_MWIS";
  }
  else if(FEASTYPE==6){
	st<<"_PBG_SUF_NEC_MWIS";
	st1<<"_PBG_SUF_NEC_MWIS";
	st2<<"_PBG_SUF_NEC_MWIS";
  }

  st<<"_"<<argv[1]<<"_"<<argv[2]<<".txt";
  st1<<"_"<<argv[1]<<"_"<<argv[2]<<".txt";
  st2<<"_"<<argv[1]<<"_"<<argv[2]<<".txt";

  st>>resultfile;
  st1>>resultfile1;
  st2>>resultfile2;


  Rresult.open(resultfile.c_str());
  Tresult.open(resultfile1.c_str());
  flog.open(resultfile2.c_str());

  //초기화
  ReadSN(argv[1]);
  cout<<"ReadSN() is DONE"<<endl;
  MakeVNs(argv[2]);
  cout<<"MakeVNs() is DONE"<<endl;
  MakeOriginCG();
  cout<<"MakeOriginCG() is DONE"<<endl;

  Calcspaths(); 
  cout<<"Calcspaths() is DONE"<<endl;
  //	ShortestTest(1, 20);

  bufPN = new PNet(*PN);
  tempPN = new PNet(*PN);
  nicePN = new PNet(*PN);
  tempVN = new VNet(*VNs[0]);
  niceVN = new VNet(*VNs[0]);
  bufVN = new VNet(*VNs[0]);
  cout<<"==Initializing is DONE=="<<endl;
  ////////////////////////////////////////////////////////////////
  Tresult<<"============================================"<<endl;
  Tresult<<" Physical Network"<<endl;
  Tresult<<"============================================"<<endl;
  Tresult<<"# of PNodes: "<<(PN->NumNode)<<endl;
  Tresult<<"# of PLinks: "<<(PN->NumLink)<<endl;
  Tresult<<"Area       : "<<XRANGE<<" [m] x "<<YRANGE<<" [m] (x,y)"<<endl;

  if(argv[1][0]=='4'&&argv[1][1]=='9'){
	Tresult<<"Topology   : Grid"<<endl;//("<<NumRow<<" x "<<NumCol<<")"<<endl;
  }
  else if(argv[1][0]=='1'&&argv[1][1]=='0'){
	Tresult<<"Topology   : Random"<<endl;
  }
  else if(argv[1][0]=='2'&&argv[1][1]=='0'){
	Tresult<<"Topology   : Random"<<endl;
  }
  else if(argv[1][0]=='3'&&argv[1][1]=='0'){
	Tresult<<"Topology   : Random"<<endl;
  }
  Tresult<<"Density    : "<<(double)(PN->NumLink)/(PN->NumNode)<<endl;
  Tresult<<"PNode CPU  : "<<SNWMIN<<" - "<<SNWMAX<<endl;
  Tresult<<"PLink Cap  : "<<SLWMIN<<" - "<<SLWMAX<<endl;
  Tresult<<"============================================"<<endl;
  Tresult<<" Virtual Network Requests"<<endl;
  Tresult<<"============================================"<<endl;
  Tresult<<"# of VNodes: "<<MINVNODENUM<<" - "<<MAXVNODENUM<<endl;
  Tresult<<"VNode CPU  : "<<VNWMIN<<" - "<<VNWMAX<<endl;
  Tresult<<"VLink Cap  : "<<VLWMIN<<" - "<<VLWMAX<<endl;
  Tresult<<"Link Pr.   : "<<MINPRVNLINK<<" - "<<MAXPRVNLINK<<" % "<<endl;
  Tresult<<"Arrival    : "<<POIMEAN<<"(Poisson)"<<endl;
  Tresult<<"Duration   : "<<EXPMEAN<<"(Exp. RV)"<<endl;
  Tresult<<"============================================"<<endl;
  Tresult<<" Algorithm Type"<<endl;
  Tresult<<"============================================"<<endl;
  if(ALGTYPE_COUPLE == 1){
	Tresult<<"COUPLE     : NON"<<endl;
  }
  else if(ALGTYPE_COUPLE == 2){
	Tresult<<"COUPLE     : MID (or Well)"<<endl;
  }
  else if(ALGTYPE_COUPLE == 3){
	Tresult<<"COUPLE     : FULL"<<endl;
  }
  if(ALGTYPE_LENGTH == 1){
	Tresult<<"INTERFERING: HOP Count"<<endl;
  }
  else if(ALGTYPE_LENGTH == 2){
	Tresult<<"INTERFERING: Weight"<<endl;
  }
  Tresult<<"# of Candi.: "<<TRY<<endl;
  if(NORMTYPE == 1){
	Tresult<<"Norm Type  : 1-norm(SUM)"<<endl;
  }
  else if(NORMTYPE > 1){
	Tresult<<"Norm Type  : "<<NORMTYPE<<"-norm"<<endl;
  }
  else if(NORMTYPE < 0){
	Tresult<<"Norm Type  : inf-norm(MAX)"<<endl;
  }
  if(VNORDER == 1){
	Tresult<<"VN Order   : Random (arrive)"<<endl;
  }
  else if(VNORDER == 2){
	Tresult<<"VN Order   : 1 / Density"<<endl;
  }
  else if(VNORDER == 3){
	Tresult<<"VN Order   : Revenue"<<endl;
  }
  else if(VNORDER == 4){
	Tresult<<"VN Order   : Revenue * time"<<endl;
  }
  else if(VNORDER == 5){
	Tresult<<"VN Order   : Revenue / Density"<<endl;
  }
  else if(VNORDER == 6){
	Tresult<<"VN Order   : Revenue * time / Density"<<endl;
  }



  Tresult<<" ALPHA     : "<<ALPHA<<endl;
  Tresult<<"============================================"<<endl;
  Tresult<<" Feasibility"<<endl;
  Tresult<<"============================================"<<endl;
  if(FEASTYPE == 1)
	Tresult<<"TYPE       : Sufficient"<<endl;
  else if(FEASTYPE == 2){
	Tresult<<"TYPE       : PBG + Sufficient"<<endl;
  }
  else if(FEASTYPE == 3){
	Tresult<<"TYPE       : PBG + MWIS"<<endl;
  }
  else if(FEASTYPE == 4){
	Tresult<<"TYPE       : PBG, Necessary, 1-"<<EPS<<"MWIS"<<endl;
	Tresult<<"PBG poly   : "<<P<<"*log(CGNnode*r^"<<Q<<")"<<endl;
  }
  else if(FEASTYPE == 5){
	Tresult<<"TYPE       : PBG, Sufficient, 1-"<<EPS<<"MWIS"<<endl;
	Tresult<<"PBG poly   : "<<P<<"*log(CGNnode*r^"<<Q<<")"<<endl;
  }
  else if(FEASTYPE == 6){
	Tresult<<"TYPE       : PBG, Sufficient, Necessary, 1-"<<EPS<<"MWIS"<<endl;
	Tresult<<"PBG poly   : "<<P<<"*log(CGNnode*r^"<<Q<<")"<<endl;
  }
  Tresult<<"============================================"<<endl;
  Tresult<<" Run Time Parameter"<<endl;
  Tresult<<"============================================"<<endl;
  Tresult<<"# of Wind. : "<<WINNUM<<endl;
  Tresult<<"MWIS SLOT  : "<<LONGSLOT<<endl;
  if(FORCECUT == 0)
	Tresult<<"FORCECUT   : No"<<endl;
  else if(FORCECUT == 1)
	Tresult<<"FORCECUT   : Yes"<<endl;


  Rresult<<"\n winnum CurrentRevenue TotalRevenue AverageRevenue ";
  Rresult<<"\n CurrentTry CurrentAccept TotalTry TotalAccept ";
  Rresult<<"\n CurrentAcceptRate TotalAcceptRate ";
  Rresult<<"\n CurrentLinkAccept CurrentNodeAccept LinkAccept NodeAccept ";
  Rresult<<"\n CurrentAcceptDensity AcceptDensity ";
  Rresult<<"\n CurrentCpuAccept CurrentCapAccept CpuAccept CapAccept ";
  Rresult<<"\n CurrentAvgCpuRatio CurrentAvgCapRatio CurrentVarCpuRatio CurrentVarCapRatio ";
  Rresult<<"\n CurrentAvgCpuRatio_ON CurrentAvgCapRatio_ON CurrentVarCpuRatio_ON CurrentVarCapRatio_ON ";
  Rresult<<"\n CurrentOnNode CurrentOnLink ";
  Rresult<<"\n TotCpuRatio TotCapRatio TotSqrCpuRatio TotSqrCapRatio ";
  Rresult<<"\n TotOnLink TotOnNode ";
  Rresult<<"\n AvgCpuRatio AvgCapRatio AvgVarCpuRatio AvgVarCapRatio ";
  Rresult<<"\n AvgCpuRatio_ON AvgCapRatio_ON AvgVarCpuRatio_ON AvgVarCapRatio_ON ";
  Rresult<<"\n AvgOnLink AvgOnNode ";
  Rresult<<"\n EmbeddedVNs"<<endl;

  ////////////////////////////////////////////////////////////////
  time_t St, End, runtime;
  time(&St);
  winnum = 1;
  while1 = true;
  Time = 0;
  while(while1){

	Time += TWIN;
	flog<<"Time Window "<<winnum<<" : "<<Time<<endl;
	// Enqueue VN req. arrived in the winnum into WindowQ
	for(i=0;i<VNNUM;i++){
	  temp = (VNs[i]->startT);
	  if(Time==TWIN && temp==0){
		WindowQ.enqueue(i);
		VNcount++;
	  }
	  if((temp>(Time-TWIN))&&(temp<=Time)){
		WindowQ.enqueue(i);
		VNcount++;
	  }
	}

	flog<<winnum<<" : Enqueuing VNs is DONE"<<endl;
	//while1 Break condition
	if(winnum>WINNUM)
	  break;	// break while1
	if(WindowQ.isEmpty() && EmbeddedQ.isEmpty()){
	  flog<<winnum<<" : Queue Empty -> checking left VNs "<<endl;
	  while1 = false;//break;
	  for(i=0;i<VNNUM;i++){
		if((VNs[i]->startT)>Time)	while1 = true;
	  }
	  if(!while1){
		//		flog<<winnum<<" : Embedding is finished"<<endl;
		continue;	// i.e., break;
	  }
	}
	//Window handling
	EmbeddedQ.sort(0);	// Service order
	//		flog<<winnum<<" : Handling 'going out' VNs "<<endl;
	for(i=0;i<EmbeddedQ.size;i++){
	  if(VNs[EmbeddedQ.storage[i]]->endT<=Time){
		temp = EmbeddedQ.dequeue();
		flog<<winnum<<" : "<<temp<<" is going out"<<endl;
		Unmap(temp);
	  }
	  else{
		break;
	  }
	}
	if(EmbeddedQ.size == 0)
	  flog<<winnum<<" : no going out"<<endl;

	//dequeue -> mapping 
	//Sort
	WindowQ.sort(VNORDER);
	flog<<winnum<<" : Start to Embedding"<<endl;
	while2 = true;
	windowqsize = WindowQ.size;
	while(while2){
	  mapVnet = WindowQ.dequeue();
	  windowqsize--;
	  if(mapVnet == -1){
		flog<<winnum<<" : WindowQ empty"<<endl;
		while2 = false;
		continue;		// i.e., break;
	  }
	  //mapping
	  mapped = false;
	  embmetric = 9999;
	  trycount++;
	  NodeMapOrder(mapVnet, VOrderArr);
	  flog<<winnum<<" : Node mapping order of VN"<<mapVnet<<" : ";
	  for(i=0;i<MAXVNODENUM;i++){
		flog<<" "<<VOrderArr[i];
	  }
	  flog<<endl;
	  SNodeOrder(SOrderArr);
	  flog<<winnum<<" : Node order of SN : ";
	  for(i=0;i<(PN->NumNode);i++)
		flog<<" "<<SOrderArr[i];
	  flog<<endl;

	  /////////////////////////////////////////////////////////////////////////////
	  if(ALGTYPE_COUPLE == 3){ 	// full couple
		/////////////////////////////////////////////////////////////////////////////
		delete tempVN;	// tempVn << VNs[mapVnet]
		tempVN = new VNet(*VNs[mapVnet]);
		for(j=0;j<TRY;j++){
		  EmbMetric[j] = -1;
		}
		// Calculate EmbMetric
		for(j=0;j<TRY;j++){
		  delete bufPN;
		  bufPN = new PNet(*PN);
		  delete VNs[mapVnet];
		  VNs[mapVnet] = new VNet(*tempVN);			

		  flog<<winnum<<" : Trying Embedding - "<<j<<"'th time"<<endl;

		  temp = SOrderArr[j];

		  for(i=0;i<SNODENUM;i++){
			EmdArr[i]=-1;
		  }
		  if((VNs[mapVnet]->Nodes[VOrderArr[0]]->cpu) > (bufPN->Nodes[SOrderArr[j]]->cpu) - (bufPN->Nodes[SOrderArr[j]]->filledW)){
			continue;
		  }
		  NEmbedTo(mapVnet, VOrderArr[0], SOrderArr[j]);

		  EmdArr[SOrderArr[j]]=VOrderArr[0];
		  for(i=1;i<(VNs[mapVnet]->NumNode);i++){
			//  flog<<winnum<<" : Node Embed VN: "<<mapVnet<<", Nodenum: "<<VOrderArr[i]<<endl;
			if(NORMTYPE == -1)	// MAX NORM
			  nodemapok = Embed_MAX_Weight_PBGON(mapVnet, VOrderArr[i], EmdArr);
			else{
			  if(FEASTYPE == 1){
				nodemapok = Embed_SUM_PBGOFF(mapVnet, VOrderArr[i], EmdArr);
			  }
			  else{
				nodemapok = Embed_SUM_PBGON(mapVnet, VOrderArr[i], EmdArr);
			  }
			}

			//    flog<<winnum<<" : Node Embedding Result is "<<nodemapok<<endl;
			if(!nodemapok)
			  i = MAXVNODENUM;
		  }
		  if(nodemapok){
			EmbMetric[j]= bufPN->embmetric();
		  }
		}

		// EmbMetric increasing order
		for(j=0;j<TRY;j++){
		  embmetric = EmbMetric[j];
		  sort_index = j;
		  for(i=j+1;i<TRY;i++){
			if(embmetric > EmbMetric[i] && (EmbMetric[i]!=-1)){
			  sort_index = i;
			  embmetric = EmbMetric[i];
			}
		  }
		  sort_temp = SOrderArr[sort_index];
		  SOrderArr[sort_index] = SOrderArr[j];
		  SOrderArr[j] = sort_temp;
		  EmbMetric[sort_index] = EmbMetric[j];
		  EmbMetric[j] = embmetric;

		}
		for(j=0;j<TRY;j++){
		  delete VNs[mapVnet];
		  VNs[mapVnet] = new VNet(*tempVN);
		  delete bufPN;
		  bufPN = new PNet(*PN);
		  flog<<winnum<<" : Trying Embedding - "<<j<<"'th time"<<endl;

		  temp = SOrderArr[j]; // check!!!
		  for(i=0;i<SNODENUM;i++)
			EmdArr[i]=-1;
		  if(VNs[mapVnet]->Nodes[VOrderArr[0]]->cpu > bufPN->Nodes[SOrderArr[j]]->cpu - bufPN->Nodes[SOrderArr[j]]->filledW) 
			continue;

		  NEmbedTo(mapVnet, VOrderArr[0], temp);
		  EmdArr[temp]=VOrderArr[0];

		  for(i=1;i<(VNs[mapVnet]->NumNode);i++){ 
			//  flog<<winnum<<" : Node Embed VN: "<<mapVnet<<", Nodenum: "<<VOrderArr[i]<<endl;
			if(NORMTYPE == -1)	// MAX NORM
			  nodemapok = Embed_MAX_Weight_PBGON(mapVnet, VOrderArr[i], EmdArr);
			else{
			  if(FEASTYPE == 1){
				nodemapok = Embed_SUM_PBGOFF(mapVnet, VOrderArr[i], EmdArr);
			  }
			  else{
				nodemapok = Embed_SUM_PBGON(mapVnet, VOrderArr[i], EmdArr);
			  }
			}
			//    flog<<winnum<<" : Node Embedding Result is "<<nodemapok<<endl;
			if(!nodemapok)
			  i = MAXVNODENUM;
		  }
		  feas = false;
		  if(nodemapok && (EmbMetric[i]!=-1)){
			if(FEASTYPE==1){		// Sufficient condition
			  feas = FeasCheck_SUF(*bufPN);
			  if(feas)	SUF_O++;
			  else		SUF_X++;
			}
			else if(FEASTYPE==2){	// PBG, SUF
			  feas = FeasCheck_SUF(*bufPN);
			  if(feas)	PBG_O_SUF_O++;
			  else		PBG_O_SUF_X++;
			}
			else if(FEASTYPE==3){	// PBG, MWIS
			  if(MWISTYPE == 1)
				feas = FeasCheck_MWIS(*bufPN);
			  if(MWISTYPE == 2)
				feas = FeasCheck_MWIS_NEC(*bufPN);
			  if(feas)	PBG_O_MWIS_O++;
			  else		PBG_O_MWIS_X++;
			}
			else if(FEASTYPE==4){	// PBG, NEC, MWIS
			  feas = Necessary_PBG(*bufPN);
			  if(feas){
				PBG_O_NEC_O++;				// Necessary suc
				if(MWISTYPE == 1)
				  feas = FeasCheck_MWIS(*bufPN);
				if(MWISTYPE == 2)
				  feas = FeasCheck_MWIS_NEC(*bufPN);
				if(feas)	PBG_O_NEC_O_MWIS_O++;	// MWIS suc
				else		PBG_O_NEC_O_MWIS_X++;	// MWIS fail
			  }
			  else{							// Necessary fail
				PBG_O_NEC_X ++;
			  }
			}
			else if(FEASTYPE==5){	// PBG, SUF, MWIS
			  feas = FeasCheck_SUF(*bufPN);
			  if(feas){
				PBG_O_SUF_O++;				// SUF suc
			  }
			  else{						// SUF fail
				PBG_O_SUF_X ++;
				if(MWISTYPE == 1)
				  feas = FeasCheck_MWIS(*bufPN);
				if(MWISTYPE == 2)
				  feas = FeasCheck_MWIS_NEC(*bufPN);
				if(feas)	PBG_O_SUF_X_MWIS_O++;	// MWIS pass
				else		PBG_O_SUF_X_MWIS_X++;	// MWIS fail
			  }
			}
			else if(FEASTYPE==6){	// PBG, SUF, NEC, MWIS
			  feas = FeasCheck_SUF(*bufPN);
			  if(feas){
				PBG_O_SUF_O++;				// SUF suc
			  }
			  else{							// SUF fail
				PBG_O_SUF_X ++;
				feas = Necessary_PBG(*bufPN);
				if(feas){// Necessary suc
				  PBG_O_SUF_X_NEC_O++;
				  if(MWISTYPE == 1)
					feas = FeasCheck_MWIS(*bufPN);
				  if(MWISTYPE == 2)
					feas = FeasCheck_MWIS_NEC(*bufPN);
				  if(feas)	PBG_O_SUF_X_NEC_O_MWIS_O++;	// MWIS suc
				  else		PBG_O_SUF_X_NEC_O_MWIS_X++;	// MWIS fail			
				}
				else
				  PBG_O_SUF_X_NEC_X++;	// Necessary fail
			  }
			}
			/////
			if(feas){
			  delete nicePN;
			  nicePN = new PNet(*bufPN);
			  VNs[mapVnet]->isEmbedded = true;
			  mapped =true;
			  nice = j;
			  delete niceVN;
			  niceVN = new VNet(*VNs[mapVnet]);


			  break;
			}
		  }
		}
		/////////////////////////////////////////////////////////////////////////////
	  }
	  /////////////////////////////////////////////////////////////////////////////
	  else if(ALGTYPE_COUPLE == 2){ 	// certain couple
		/////////////////////////////////////////////////////////////////////////////
		delete tempVN;
		tempVN = new VNet(*VNs[mapVnet]);
		for(j=0;j<TRY;j++){
		  EmbMetric[j] = -1;
		}
		for(j=0;j<TRY;j++){
		  delete VNs[mapVnet];
		  VNs[mapVnet] = new VNet(*tempVN);
		  delete bufPN;
		  bufPN = new PNet(*PN);
		  flog<<winnum<<" : Trying Embedding - "<<j<<"'th time"<<endl;
		  nodemapok = compembed(mapVnet,SOrderArr[j]);
		  if(nodemapok){
			EmbMetric[j]= bufPN->embmetric();
		  }
		}

		// EmbMetric
		for(j=0;j<TRY;j++){
		  embmetric = EmbMetric[j];
		  sort_index = j;
		  for(i=j+1;i<TRY;i++){
			if(embmetric > EmbMetric[i] && (EmbMetric[i]!=-1)){
			  sort_index = i;
			  embmetric = EmbMetric[i];
			}
		  }
		  sort_temp = SOrderArr[sort_index];
		  SOrderArr[sort_index] = SOrderArr[j];
		  SOrderArr[j] = sort_temp;
		  EmbMetric[sort_index] = EmbMetric[j];
		  EmbMetric[j] = embmetric;
		}

		for(j=0;j<TRY;j++){
		  delete VNs[mapVnet];
		  VNs[mapVnet] = new VNet(*tempVN);
		  delete bufPN;
		  bufPN = new PNet(*PN);

		  flog<<winnum<<" : Trying Embedding - "<<j<<"'th time"<<endl;
		  nodemapok = compembed(mapVnet,SOrderArr[j]);
		  feas = false;
		  if(nodemapok && (EmbMetric[j]!=-1)){
			/////
			if(FEASTYPE==1){		// Sufficient condition
			  feas = FeasCheck_SUF(*bufPN);
			  if(feas)	SUF_O++;
			  else		SUF_X++;
			}
			else if(FEASTYPE==2){	// PBG, SUF
			  feas = FeasCheck_SUF(*bufPN);
			  if(feas)	PBG_O_SUF_O++;
			  else		PBG_O_SUF_X++;
			}
			else if(FEASTYPE==3){	// PBG, MWIS
			  if(MWISTYPE == 1)
				feas = FeasCheck_MWIS(*bufPN);
			  if(MWISTYPE == 2)
				feas = FeasCheck_MWIS_NEC(*bufPN);
			  if(feas)	PBG_O_MWIS_O++;
			  else		PBG_O_MWIS_X++;
			}
			else if(FEASTYPE==4){	// PBG, NEC, MWIS
			  feas = Necessary_PBG(*bufPN);
			  if(feas){
				PBG_O_NEC_O++;				// Necessary suc
				if(MWISTYPE == 1)
				  feas = FeasCheck_MWIS(*bufPN);
				if(MWISTYPE == 2)
				  feas = FeasCheck_MWIS_NEC(*bufPN);
				if(feas)	PBG_O_NEC_O_MWIS_O++;	// MWIS suc
				else		PBG_O_NEC_O_MWIS_X++;	// MWIS fail
			  }
			  else{							// Necessary fail!
				PBG_O_NEC_X ++;
			  }
			}
			else if(FEASTYPE==5){	// PBG, SUF, MWIS
			  feas = FeasCheck_SUF(*bufPN);
			  if(feas){
				PBG_O_SUF_O++;				// SUF suc
			  }
			  else{							// SUF fail
				PBG_O_SUF_X ++;
				if(MWISTYPE == 1)
				  feas = FeasCheck_MWIS(*bufPN);
				if(MWISTYPE == 2)
				  feas = FeasCheck_MWIS_NEC(*bufPN);

				if(feas)	PBG_O_SUF_X_MWIS_O++;	// MWIS suc
				else		PBG_O_SUF_X_MWIS_X++;	// MWIS fail
			  }
			}
			else if(FEASTYPE==6){	// PBG, SUF, NEC, MWIS
			  feas = FeasCheck_SUF(*bufPN);
			  if(feas){
				PBG_O_SUF_O++;				// SUF suc
			  }
			  else{							// SUF fail
				PBG_O_SUF_X ++;
				feas = Necessary_PBG(*bufPN);
				if(feas){// Necessary suc
				  PBG_O_SUF_X_NEC_O++;
				  if(MWISTYPE == 1)
					feas = FeasCheck_MWIS(*bufPN);
				  if(MWISTYPE == 2)
					feas = FeasCheck_MWIS_NEC(*bufPN);

				  if(feas)	PBG_O_SUF_X_NEC_O_MWIS_O++;	// MWIS suc
				  else		PBG_O_SUF_X_NEC_O_MWIS_X++;	// MWIS fail			
				}
				else
				  PBG_O_SUF_X_NEC_X++;	// Necessary fail
			  }
			}
			/////
			if(feas){
			  delete nicePN;
			  nicePN = new PNet(*bufPN);

			  VNs[mapVnet]->isEmbedded = true;
			  mapped = true;
			  nice = j;

			  delete niceVN;
			  niceVN = new VNet(*VNs[mapVnet]);
			  break;
			}
		  }
		}



		/////////////////////////////////////////////////////////////////////////////
	  }
	  /////////////////////////////////////////////////////////////////////////////
	  else{ 	// non couple, ALGTYPE_COUPLE == 1
		/////////////////////////////////////////////////////////////////////////////
		delete tempVN;
		tempVN = new VNet(*VNs[mapVnet]);
		delete bufPN;
		bufPN = new PNet(*PN);

		nodemapok = compembed(mapVnet);
		feas = false;
		if(nodemapok){
		  /////
		  if(FEASTYPE==1){		// Sufficient condition
			feas = FeasCheck_SUF(*bufPN);
			if(feas)	SUF_O++;
			else		SUF_X++;
		  }
		  else if(FEASTYPE==2){	// PBG, SUF
			feas = FeasCheck_SUF(*bufPN);
			if(feas)	PBG_O_SUF_O++;
			else		PBG_O_SUF_X++;
		  }
		  else if(FEASTYPE==3){	// PBG, MWIS
			if(MWISTYPE == 1)
			  feas = FeasCheck_MWIS(*bufPN);
			if(MWISTYPE == 2)
			  feas = FeasCheck_MWIS_NEC(*bufPN);

			if(feas)	PBG_O_MWIS_O++;
			else		PBG_O_MWIS_X++;
		  }
		  else if(FEASTYPE==4){	// PBG, NEC, MWIS
			feas = Necessary_PBG(*bufPN);
			if(feas){
			  PBG_O_NEC_O++;				// Necessary suc
			  if(MWISTYPE == 1)
				feas = FeasCheck_MWIS(*bufPN);
			  if(MWISTYPE == 2)
				feas = FeasCheck_MWIS_NEC(*bufPN);

			  if(feas)	PBG_O_NEC_O_MWIS_O++;	// MWIS suc
			  else		PBG_O_NEC_O_MWIS_X++;	// MWIS fail
			}
			else{							// Necessary fail!
			  PBG_O_NEC_X ++;
			}
		  }
		  else if(FEASTYPE==5){	// PBG, SUF, MWIS
			feas = FeasCheck_SUF(*bufPN);
			if(feas){
			  PBG_O_SUF_O++;				// SUF suc
			}
			else{							// SUF fail
			  PBG_O_SUF_X ++;
			  if(MWISTYPE == 1)
				feas = FeasCheck_MWIS(*bufPN);
			  if(MWISTYPE == 2)
				feas = FeasCheck_MWIS_NEC(*bufPN);

			  if(feas)	PBG_O_SUF_X_MWIS_O++;	// MWIS suc
			  else		PBG_O_SUF_X_MWIS_X++;	// MWIS fail
			}
		  }
		  else if(FEASTYPE==6){	// PBG, SUF, NEC, MWIS
			feas = FeasCheck_SUF(*bufPN);
			if(feas){
			  PBG_O_SUF_O++;				// SUF suc
			}
			else{							// SUF fail
			  PBG_O_SUF_X ++;
			  feas = Necessary_PBG(*bufPN);
			  if(feas){// Necessary suc
				PBG_O_SUF_X_NEC_O++;
				if(MWISTYPE == 1)
				  feas = FeasCheck_MWIS(*bufPN);
				if(MWISTYPE == 2)
				  feas = FeasCheck_MWIS_NEC(*bufPN);

				if(feas)	PBG_O_SUF_X_NEC_O_MWIS_O++;	// MWIS suc
				else		PBG_O_SUF_X_NEC_O_MWIS_X++;	// MWIS fail			
			  }
			  else
				PBG_O_SUF_X_NEC_X++;	// Necessary fail
			}
		  }
		  /////
		  if(feas){
			delete nicePN;
			nicePN = new PNet(*bufPN);
			VNs[mapVnet]->isEmbedded = true;
			mapped =true;
			nice = j;
			delete niceVN;
			niceVN = new VNet(*VNs[mapVnet]);


		  }
		}
		/////////////////////////////////////////////////////////////////////////////
	  }
	  /////////////////////////////////////////////////////////////////////////////

	  if(mapped){
		flog<<nice<<"th embedding is applied"<<endl; 
		delete PN;
		PN = new PNet(*nicePN);
		delete VNs[mapVnet];
		VNs[mapVnet] = new VNet(*niceVN);
		//빠져나갈 시간 계산
		VNs[mapVnet]->endT=VNs[mapVnet]->RqT+Time;
		EmbeddedQ.enqueue(mapVnet);
		acceptcount++;
	  }
	  else{
		delete bufPN;
		bufPN = new PNet(*PN);
		delete VNs[mapVnet];
		VNs[mapVnet] = new VNet(*tempVN);
		VNs[mapVnet]->delayed = VNs[mapVnet]->delayed+1;
		if(MAXDELAY>VNs[mapVnet]->delayed)
		  WindowQ.enqueue(mapVnet);
	  }
	  if(WindowQ.isEmpty() || windowqsize <0){
		while2 = false;//break;
	  }
	}
	CurrentRevenue = 0;
	CurrentLinkAccept = 0;
	CurrentNodeAccept = 0;
	CurrentCapAccept = 0;
	CurrentCpuAccept = 0;
	for(i=EmbeddedQ.bot;i!=EmbeddedQ.top;i=(i+1)%EmbeddedQ.Qsize){
	  if(EmbeddedQ.storage[i]!=-1){
		CurrentRevenue += VNRevenue(VNs[EmbeddedQ.storage[i]]);
		CurrentLinkAccept += VNLinkAccept(VNs[EmbeddedQ.storage[i]]);
		CurrentNodeAccept += VNNodeAccept(VNs[EmbeddedQ.storage[i]]);
		CurrentCapAccept += VNCapAccept(VNs[EmbeddedQ.storage[i]]);
		CurrentCpuAccept += VNCpuAccept(VNs[EmbeddedQ.storage[i]]);
	  }
	}
	CurrentOnNode =0;
	CurrentAvgCpuRatio = 0;
	CurrentSqrCpuRatio = 0;
	double temptemp=0;
	for(i=0;i<(PN->NumNode);i++){
	  if(PN->Nodes[i]->filledW >0){
		CurrentOnNode++;
		temptemp = (double)(PN->Nodes[i]->filledW)/(PN->Nodes[i]->cpu);
		CurrentAvgCpuRatio += temptemp;
		CurrentSqrCpuRatio += temptemp*temptemp;
	  }
	}
	TotOnNode += CurrentOnNode;
	TotCpuRatio += CurrentAvgCpuRatio;
	TotSqrCpuRatio += CurrentSqrCpuRatio;
	CurrentAvgCpuRatio_ON = CurrentAvgCpuRatio/((double)CurrentOnNode);
	AvgCpuRatio_ON = TotCpuRatio/((double)TotOnNode);
	CurrentSqrCpuRatio_ON = CurrentSqrCpuRatio/((double)CurrentOnNode);
	CurrentAvgCpuRatio = CurrentAvgCpuRatio/((double)(PN->NumNode));
	AvgCpuRatio = TotCpuRatio / ((double)winnum * (double)(PN->NumNode));
	CurrentSqrCpuRatio = CurrentSqrCpuRatio/((double)(PN->NumNode));
	CurrentVarCpuRatio_ON = CurrentSqrCpuRatio_ON - CurrentAvgCpuRatio_ON*CurrentAvgCpuRatio_ON;
	CurrentVarCpuRatio = CurrentSqrCpuRatio - CurrentAvgCpuRatio*CurrentAvgCpuRatio;
	AvgVarCpuRatio = (TotSqrCpuRatio / ((double)winnum * (double)(PN->NumNode)) - AvgCpuRatio * AvgCpuRatio) ;
	AvgVarCpuRatio_ON = (TotSqrCpuRatio  / ((double)TotOnNode) - AvgCpuRatio_ON * AvgCpuRatio_ON);


	CurrentOnLink =0;
	CurrentAvgCapRatio = 0;
	CurrentSqrCapRatio = 0;
	for(i=0;i<(PN->NumLink);i++){
	  if(PN->Links[i]->filledW>0){
		temptemp = (double)(PN->Links[i]->filledW)/(PN->Links[i]->capacity);
		CurrentAvgCapRatio += temptemp;
		CurrentSqrCapRatio += temptemp*temptemp;
		CurrentOnLink++;
	  }
	}
	TotOnLink += CurrentOnLink;
	TotCapRatio += CurrentAvgCapRatio;
	TotSqrCapRatio += CurrentSqrCapRatio;
	CurrentAvgCapRatio_ON = CurrentAvgCapRatio/((double)CurrentOnLink);
	AvgCapRatio_ON = TotCapRatio/((double)TotOnLink);
	CurrentSqrCapRatio_ON = CurrentSqrCapRatio/((double)CurrentOnLink);
	CurrentAvgCapRatio = CurrentAvgCapRatio/((double)(PN->NumLink));
	AvgCapRatio = TotCapRatio / ((double)winnum * (double)(PN->NumLink));
	CurrentSqrCapRatio = CurrentSqrCapRatio/((double)(PN->NumLink));
	CurrentVarCapRatio_ON = CurrentSqrCapRatio_ON - CurrentAvgCapRatio_ON*CurrentAvgCapRatio_ON;
	CurrentVarCapRatio = CurrentSqrCapRatio - CurrentAvgCapRatio*CurrentAvgCapRatio;
	AvgVarCapRatio = (TotSqrCapRatio / ((double)winnum * (double)(PN->NumLink)) - AvgCapRatio * AvgCapRatio) ;
	AvgVarCapRatio_ON = (TotSqrCapRatio  / ((double)TotOnLink) - AvgCapRatio_ON * AvgCapRatio_ON);


	TotRevenue += CurrentRevenue;
	LinkAccept += CurrentLinkAccept;
	NodeAccept += CurrentNodeAccept;
	CapAccept += CurrentCapAccept;
	CpuAccept += CurrentCpuAccept;

	CurrentTry = trycount - CurrentTry;
	CurrentAccept = acceptcount - CurrentAccept;
	CurrentVNcount = VNcount - CurrentVNcount;


	////////////////////////////////////////////////////////////////////////////
	AvgOnLink = TotOnLink / winnum;
	AvgOnNode = TotOnNode / winnum;
	AvRevenue = (double)TotRevenue/winnum;
	accptrat = (VNcount !=0) ? (double)acceptcount/VNcount : 0;
	//accptrat = (VNcount !=0) ? (double)acceptcount : 0;
	Currentaccptrat = (CurrentVNcount !=0) ? (double)CurrentAccept/CurrentVNcount : 0;
	AcceptDensity = (NodeAccept !=0) ? (double)LinkAccept/NodeAccept : 0;
	CurrentAcceptDensity=(CurrentNodeAccept !=0) ? (double)CurrentLinkAccept/CurrentNodeAccept : 0;


	//		Rresult<<"winnum CurrentRevenue TotalRevenue AverageRevenue ";
	Rresult<<winnum<<" "<<CurrentRevenue<<" "<<TotRevenue<<" "<<AvRevenue<<" ";
	//		Rresult<<"CurrentTry CurrentAccept TotalTry TotalAccept ";
	Rresult<<CurrentTry<<" "<<CurrentAccept<<" "<<trycount<<" "<<acceptcount<<" ";
	//		Rresult<<"CurrentAcceptRate TotalAcceptRate ";
	Rresult<<Currentaccptrat<<" "<<accptrat<<" ";
	//		Rresult<<"CurrentLinkAccept CurrentNodeAccept LinkAccept NodeAccept ";
	Rresult<<CurrentLinkAccept<<" "<<CurrentNodeAccept<<" "<<LinkAccept<<" "<<NodeAccept<<" ";
	//		Rresult<<"CurrentAcceptDensity AcceptDensity ";
	Rresult<<CurrentAcceptDensity<<" "<<AcceptDensity<<" ";
	//		Rresult<<"CurrentCpuAccept CurrentCapAccept CpuAccept CapAccept ";
	Rresult<<CurrentCpuAccept<<" "<<CurrentCapAccept<<" "<<CpuAccept<<" "<<CapAccept<<" ";
	//		Rresult<<"CurrentAvgCpuRatio CurrentAvgCapRatio CurrentVarCpuRatio CurrentVarCapRatio ";
	Rresult<<CurrentAvgCpuRatio<<" "<<CurrentAvgCapRatio<<" "<<CurrentVarCpuRatio<<" "<<CurrentVarCapRatio<<" ";
	//		Rresult<<"CurrentAvgCpuRatio_ON CurrentAvgCapRatio_ON CurrentVarCpuRatio_ON CurrentVarCapRatio_ON ";
	Rresult<<CurrentAvgCpuRatio_ON<<" "<<CurrentAvgCapRatio_ON<<" "<<CurrentVarCpuRatio_ON<<" "<<CurrentVarCapRatio_ON<<" ";
	//		Rresult<<"CurrentOnNode CurrentOnLink ";
	Rresult<<CurrentOnNode<<" "<<CurrentOnLink<<" ";

	//		Rresult<<"TotCpuRatio TotCapRatio TotSqrCpuRatio TotSqrCapRatio ";
	Rresult<<TotCpuRatio<<" "<<TotCapRatio<<" "<<TotSqrCpuRatio<<" "<<TotSqrCapRatio<<" ";
	//		Rresult<<"TotOnLink TotOnNode ";
	Rresult<<TotOnLink<<" "<<TotOnNode<<" ";
	//		Rresult<<"AvgCpuRatio AvgCapRatio AvgVarCpuRatio AvgVarCapRatio ";
	Rresult<<AvgCpuRatio<<" "<<AvgCapRatio<<" "<<AvgVarCpuRatio<<" "<<AvgVarCapRatio<<" ";
	//		Rresult<<"AvgCpuRatio_ON AvgCapRatio_ON AvgVarCpuRatio_ON AvgVarCapRatio_ON ";
	Rresult<<AvgCpuRatio_ON<<" "<<AvgCapRatio_ON<<" "<<AvgVarCpuRatio_ON<<" "<<AvgVarCapRatio_ON<<" ";
	//		Rresult<<"AvgOnLink AvgOnNode<<endl;
	Rresult<<AvgOnLink<<" "<<AvgOnNode<<" ";
	//		Rresult<<"EmbeddedVNs"<<;
	Rresult<<EmbeddedQ.size<<" ";
	//		Rresult<<"WindowQ"<<endl;
	Rresult<<WindowQ.size<<endl;


	////////////////////////////////////////////////////////////////////////////
	CurrentTry = trycount;
	CurrentAccept = acceptcount;
	CurrentVNcount = VNcount;
	time(&End);
	runtime = End -St;
	
	cout<<winnum<<"/"<<WINNUM<<"\t"<<(double)winnum/(double)WINNUM * 100<<" %"<<"\tAvRev "<<AvRevenue<<"\t AvAccpRate "<<accptrat<<"["<<EmbeddedQ.size<<"]\t( "<<(int)((double)runtime*((double)WINNUM-(double)winnum)/((double)winnum))<<" )"<<endl;
	ofstream myfile1;	
	myfile1.open("Accept.txt",ios::app);
	myfile1<<Currentaccptrat<<endl;
	myfile1.close();
	winnum++;
  }

// print out Average Revene results to file - Lam	
	ofstream myfile;	
	myfile.open("AvRevenue.txt");
	myfile<<AvRevenue<<endl;
	myfile.close();

// print out Acceptance Ratio 

// close the file	 
	

  flog<<"End of Embedding"<<endl;

  //	cout<<"PBGfail : "<<PBGfail<<endl;

  cout<<"Simulation End"<<endl;

  time(&End);
  runtime = End -St; 

  Tresult<<"============================================"<<endl;
  Tresult<<" Outputs"<<endl;
  Tresult<<"============================================"<<endl;
  Tresult<<"Simualtion ended at winnum = "<<winnum<<endl;
  Tresult<<"RunTime    : "<<runtime<<" sec"<< endl;
  Tresult<<"Tried VNs  : "<<trycount<<" items"<< endl;
  Tresult<<"Avg.RunTime: "<<(double)runtime/trycount<<" sec"<<endl;
  Tresult<<"AvRevenue  : "<<AvRevenue<<endl;
  Tresult<<"Accp.Ratio : "<<accptrat<<" ( "<<acceptcount<<" / "<<trycount<<" )"<<endl;
  //Tresult<<"Accp.Ratio : "<<accptrat<<" ( "<<acceptcount<<")"<<endl;
	
  if(FEASTYPE == 1){		// FeasType = 1,  SUF
	Tresult<<"SUF_X      : "<<SUF_X<<endl;
	Tresult<<"SUF_O      : "<<SUF_O<<" (Embedded!)"<<endl;
	Tresult<<"SUF_O_Portion: "<<((double)SUF_O/(double)acceptcount)<<" (Embedded!)"<<endl;

  }
  else if(FEASTYPE == 2){	// FeasType = 2,  PBG, SUF
	//	Tresult<<"PBG_X      : "<<PBG_X<<endl;
	//	Tresult<<"PBG_O      : "<<PBG_O<<endl;
	Tresult<<"PBG_O_SUF_X: "<<PBG_O_SUF_X<<endl;
	Tresult<<"PBG_O_SUF_O: "<<PBG_O_SUF_O<<" (Embedded!)"<<endl;
	Tresult<<">PBG_O_SUF_O_Portion: "<<((double)PBG_O_SUF_O/(double)acceptcount)<<" (Embedded!)"<<endl;
  }

  else if(FEASTYPE == 3){	// FeasType = 3, PBG, MWIS
	//	Tresult<<"PBG_X      : "<<PBG_X<<endl;
	//	Tresult<<"PBG_O      : "<<PBG_O<<endl;
	Tresult<<"PBG_O_MWIS_X: "<<PBG_O_MWIS_X<<endl;
	Tresult<<"PBG_O_MWIS_O: "<<PBG_O_MWIS_O<<" (Embedded!)"<<endl;
	Tresult<<">PBG_O_MWIS_O_Portion: "<<((double)PBG_O_MWIS_O/(double)acceptcount)<<" (Embedded!)"<<endl;
  }
  else if(FEASTYPE == 4){	// FeasType = 4, PBG, Nec, MWIS
	//	Tresult<<"PBG_X      : "<<PBG_X<<endl;
	//	Tresult<<"PBG_O      : "<<PBG_O<<endl;
	Tresult<<"PBG_O_NEC_X: "<<PBG_O_NEC_X<<" (Saved!)"<<endl;
	Tresult<<">NEC_Saved_Rate: "<<((double)PBG_O_NEC_X/(double)(PBG_O_NEC_X+PBG_O_NEC_O))<<" (Saved!)"<<endl;
	Tresult<<"PBG_O_NEC_O: "<<PBG_O_NEC_O<<endl;
	Tresult<<"PBG_O_NEC_O_MWIS_X: "<<PBG_O_NEC_O_MWIS_X<<endl;
	Tresult<<"PBG_O_NEC_O_MWIS_O: "<<PBG_O_NEC_O_MWIS_O<<" (Embedded!)"<<endl;
	Tresult<<">PBG_O_NEC_O_MWIS_O_Portion: "<<((double)PBG_O_NEC_O_MWIS_O/(double)acceptcount)<<" (Embedded!)"<<endl;
  }
  else if(FEASTYPE == 5){	// FeasType = 5, PBG, SUF, MWIS
	//	Tresult<<"PBG_X      : "<<PBG_X<<endl;
	//	Tresult<<"PBG_O      : "<<PBG_O<<endl;
	Tresult<<"PBG_O_SUF_O: "<<PBG_O_SUF_O<<" (Embedded! & Saved!)"<<endl;
	Tresult<<">PBG_O_SUF_O_Portion: "<<((double)PBG_O_SUF_O/(double)acceptcount)<<" (Embedded!)"<<endl;
	Tresult<<">SUF_Saved_Rate: "<<((double)PBG_O_SUF_O/(double)(PBG_O_SUF_X+PBG_O_SUF_O))<<" (Saved!)"<<endl;
	Tresult<<"PBG_O_SUF_X: "<<PBG_O_SUF_X<<endl;
	Tresult<<"PBG_O_SUF_X_MWIS_X: "<<PBG_O_SUF_X_MWIS_X<<endl;
	Tresult<<"PBG_O_SUF_X_MWIS_O: "<<PBG_O_SUF_X_MWIS_O<<" (Embedded!)"<<endl;
	Tresult<<">PBG_O_SUF_X_MWIS_O_Portion: "<<((double)PBG_O_SUF_X_MWIS_O/(double)acceptcount)<<" (Embedded!)"<<endl;
  }
  else if(FEASTYPE == 6){	// FeasType = 6, PBG, SUF, Nec, MWIS
	//	Tresult<<"PBG_O      : "<<PBG_O<<endl;
	//	Tresult<<"PBG_X      : "<<PBG_X<<endl;
	Tresult<<"PBG_O_SUF_O: "<<PBG_O_SUF_O<<" (Embedded! & Saved!)"<<endl;
	Tresult<<">PBG_O_SUF_O_Portion: "<<((double)PBG_O_SUF_O/(double)acceptcount)<<" (Embedded!)"<<endl;
	Tresult<<">SUF_Saved_Rate: "<<((double)PBG_O_SUF_O/(double)(PBG_O_SUF_X+PBG_O_SUF_O))<<" (Saved!)"<<endl;
	Tresult<<"PBG_O_SUF_X: "<<PBG_O_SUF_X<<endl;
	Tresult<<"PBG_O_SUF_X_NEC_X: "<<PBG_O_SUF_X_NEC_X<<"( Saved!)"<<endl;
	Tresult<<">NEC_Saved_Rate: "<<((double)PBG_O_SUF_X_NEC_X/(double)(PBG_O_SUF_X_NEC_X+PBG_O_SUF_X_NEC_O))<<" (Saved!)"<<endl;
	Tresult<<"PBG_O_SUF_X_NEC_O: "<<PBG_O_SUF_X_NEC_O<<endl;
	Tresult<<"PBG_O_SUF_X_NEC_O_MWIS_X: "<<PBG_O_SUF_X_NEC_O_MWIS_X<<endl;
	Tresult<<"PBG_O_SUF_X_NEC_O_MWIS_O: "<<PBG_O_SUF_X_NEC_O_MWIS_O<<" ( Embedded!)"<<endl;
	Tresult<<">PBG_O_SUF_X_NEC_O_MWIS_O_Portion: "<<((double)PBG_O_SUF_X_NEC_O_MWIS_O/(double)acceptcount)<<" ( Embedded!)"<<endl;
  }

  cout<<"RunTime    : "<<runtime<<" sec"<< endl;
  cout<<"Tried VNs  : "<<trycount<<" VNRs"<< endl;
  cout<<"Avg.RunTime: "<<(double)runtime/trycount<<" sec"<<endl;
  cout<<"AvRevenue  : "<<AvRevenue<<endl;
  cout<<"Accp.Ratio : "<<accptrat<<endl;

  //////

  Tresult<<"winnum: "<<winnum<<"\nCurrentRevenue: "<<CurrentRevenue<<"\nTotalRevenue: "<<TotRevenue<<"\nAverageRevenue: "<<AvRevenue;
  Tresult<<"\nCurrentTry:"<<CurrentTry<<"\nCurrentAccept: "<<CurrentAccept<<"\nTotalTry: "<<trycount<<"\nTotalAccept: "<<acceptcount;
  Tresult<<"\nCurrentAcceptRate: "<<Currentaccptrat<<"\nTotalAcceptRate: "<<accptrat;
  Tresult<<"\nCurrentLinkAccept: "<<CurrentLinkAccept<<"\nCurrentNodeAccept: "<<CurrentNodeAccept<<"\nLinkAccept: "<<LinkAccept<<"\nNodeAccept: "<<NodeAccept;
  Tresult<<"\nCurrentAcceptDensity: "<<CurrentAcceptDensity<<"\nAcceptDensity: "<<AcceptDensity;
  Tresult<<"\nCurrentCpuAccept: "<<CurrentCpuAccept<<"\nCurrentCapAccept: "<<CurrentCapAccept<<"\nCpuAccept: "<<CpuAccept<<"\nCapAccept: "<<CapAccept;
  Tresult<<"\nCurrentAvgCpuRatio: "<<CurrentAvgCpuRatio<<"\nCurrentAvgCapRatio: "<<CurrentAvgCapRatio<<"\nCurrentVarCpuRatio: "<<CurrentVarCpuRatio<<"\nCurrentVarCapRatio: "<<CurrentVarCapRatio;
  Tresult<<"\nCurrentAvgCpuRatio_ON: "<<CurrentAvgCpuRatio_ON<<"\nCurrentAvgCapRatio_ON: "<<CurrentAvgCapRatio_ON<<"\nCurrentVarCpuRatio_ON: "<<CurrentVarCpuRatio_ON<<"\nCurrentVarCapRatio_ON: "<<CurrentVarCapRatio_ON;
  Tresult<<"\nCurrentOnNode: "<<CurrentOnNode<<"\nCurrentOnLink: "<<CurrentOnLink;
  Tresult<<"\nTotCpuRatio: "<<TotCpuRatio<<"\nTotCapRatio: "<<TotCapRatio<<"\nTotSqrCpuRatio: "<<TotSqrCpuRatio<<"\nTotSqrCapRatio: "<<TotSqrCapRatio;
  Tresult<<"\nTotOnLink: "<<TotOnLink<<"\nTotOnNode: "<<TotOnNode;
  Tresult<<"\nAvgCpuRatio: "<<AvgCpuRatio<<"\nAvgCapRatio: "<<AvgCapRatio<<"\nAvgVarCpuRatio: "<<AvgVarCpuRatio<<"\nAvgVarCapRatio: "<<AvgVarCapRatio;
  Tresult<<"\nAvgCpuRatio_ON: "<<AvgCpuRatio_ON<<"\nAvgCapRatio_ON: "<<AvgCapRatio_ON<<"\nAvgVarCpuRatio_ON: "<<AvgVarCpuRatio_ON<<"\nAvgVarCapRatio_ON: "<<AvgVarCapRatio_ON;
  Tresult<<"\nAvgOnLink: "<<AvgOnLink<<"\nAvgOnNode: "<<AvgOnNode<<endl;
  //////


  flog.close();
  Tresult.close();
  Rresult.close();


  delete tempVN;
  delete niceVN;
  delete bufVN;

  for(i=0;i<VNNUM;i++)
	delete VNs[i];
  delete VNs;

  for(i=0;i<PN->NumLink;i++)
	delete [] Conflict[i];
  delete [] Conflict;
  delete PN;
  delete bufPN;
  delete tempPN;
  delete nicePN;
}

//Functions

/*
  ReadSN()
  Read SN.txt (physical substrate network nodes and links)
  and Write input.txt (node position) and topology_vne.dem (gnuplot script)

*/
void ReadSN(char* order){
  int numnode;
  int numlink;
  int i, j, k, p, q;
  Node pnode[SNODENUM];
  int CapLink[SNODENUM][SNODENUM];

  ifstream snf;
  ofstream input;
  ofstream topol;

  stringstream st;
  string name;
  //    st<<"/home/js/INPUT/"<<"SN"<<order<<".txt";
  st<<"SN"<<order<<".txt";
  st>>name;
  ifstream vnf;
  snf.open(name.c_str());



  input.open("input.txt");
  topol.open("topology_vne.dem");
	
  // initialize PNode and linkwt
  for( i=0;i<SNODENUM;i++){
	for( j=0;j<SNODENUM;j++)	CapLink[i][j] = 0;
	pnode[i].x = -1;
	pnode[i].y = -1;
	pnode[i].cpu = 0;
  } 
  flog<<"   ReadSN() Initializing is DONE"<<endl;
  //	flog<<"Succeed to Make Substrate Network -> Save in class SN"<<endl;

  snf>>numnode;
  snf>>numlink;
  flog<<"# of nodes in SN: "<<numnode<<endl;
  cout<<"# of nodes in SN: "<<numnode<<endl;
  flog<<"# of links in SN: "<<numlink<<endl;
  cout<<"# of links in SN: "<<numlink<<endl;

  // Plug numnode and numlink in PN structure
  PN = new PNet(numnode, numlink);
    
  // Read node position (x,y) and cpu
  for(i=0;i<numnode;i++)	snf>>(pnode[i].x)>>(pnode[i].y)>>(pnode[i].cpu);
    
  // Read link and store links in temporary array
  for( i=0;i<numlink;i++){
	int node1, node2,wt;
	snf>>node1>>node2>>wt;
	CapLink[node1][node2]=wt;
  }
  flog<<"   ReadSN() Reading is DONE"<<endl;

  // Plug Link data from the temporary array to PN structure
  k=0;
  for(i=0;i<numnode;i++){             
	for( j=i+1;j<numnode;j++){
	  if( CapLink[i][j]>0){
		PN->Links[k] = new PLink();
		// Connect node data and link data
		PN->Links[k]->node1 = i;
		PN->Links[k]->node2 = j;
		PN->Links[k]->filledW = 0;
		PN->Links[k]->capacity = CapLink[i][j];
		PN->Links[k]->intfnum = 0;
		for(p=0;p<MAXEMBEDLINK;p++){
		  PN->Links[k]->embedVNID[p] = -1;
		  PN->Links[k]->embedVLink[p] = -1;
		}
		CapLink[i][j] = k+1;
		CapLink[j][i] = k+1;
		PN->map[i][j] = 1;
		PN->map[j][i] = 1;
		k++;
	  }
	}
  }
  // Plug Node data from the temporary array to PN structure
  int test_=0;
  for(i=0;i<numnode;i++){
	k=0;
	for(j=0;j<numnode;j++)
	  if(CapLink[i][j]>0)
		k++;
	PN->Nodes[i] = new PNode(k);
	PN->Nodes[i]->x = pnode[i].x;
	PN->Nodes[i]->y = pnode[i].y;
	PN->Nodes[i]->cpu = pnode[i].cpu;
	PN->Nodes[i]->filledW = 0;
	for(p=0;p<MAXEMBEDNODE;p++){
	  PN->Nodes[i]->embedVNID[p] = -1;
	  PN->Nodes[i]->embedVNode[p] = -1;
	}
	p=0;
	// Connect node data and link data
	for(j=0;j<numnode && p<k;j++){
	  if(CapLink[i][j]>0){
		PN->Nodes[i]->Links[p] = CapLink[i][j]-1;
		p++;
	  }
	}
	PN->Nodes[i]->NumLink = p;
  }
  flog<<"   ReadSN() Plugging is DONE"<<endl;

  // Draw physical substrate network
  for(i=0;i<numnode;i++)
	input<<(PN->Nodes[i]->x)<<" "<<(PN->Nodes[i]->y)<<endl; 
  topol<<"set size square"<<endl;
  topol<<"set noautoscale"<<endl;
  topol<<"set style data points"<<endl;
  topol<<"set xrange [0:"<<XRANGE<<"]"<<endl;
  topol<<"set yrange [0:"<<(YRANGE+20)<<"]"<<endl;
  topol<<"set term postscript eps color solid \"Time-Roman\" 30"<<endl;

  stringstream st1;
  string resultfile1;
  st1<<order;
  st1>>resultfile1;

  topol<<"set output 'output_"<<resultfile1<<".eps'"<<endl;

  for(i=0;i<numlink;i++){
	topol<<"set arrow from "<<(PN->Nodes[(PN->Links[i]->node1)]->x)<<","<<(PN->Nodes[(PN->Links[i]->node1)]->y)<<" to "<<(PN->Nodes[(PN->Links[i]->node2)]->x)<<","<<(PN->Nodes[(PN->Links[i]->node2)]->y)<<" nohead lt -1 lw 1.0"<<endl;	
  }
  topol<<"plot 'input.txt' title \"Nodes\" pt 30 ps 1.5"<<endl;
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  flog<<"ReadSN() is DONE"<<endl;
  //    cout<<"MAKE SN COMPLETE" << endl;
  input.close();
  topol.close();
  snf.close();
  FILE* pipe;
  pipe = popen("gnuplot topology_vne.dem", "w");		// process open, write over
  fclose(pipe);
  //sleep(1);
}


/*
  void MakeVNs(char order)

  Read VN[order].txt


*/
        
// Generate virtual network
void MakeVNs(char* order){ //
  int i,j,k,p,q;
  int cpu[MAXVNODENUM];
  int nodes;
  int links;
  int linkwt[MAXVNODENUM][MAXVNODENUM];
  int top, bot;
  int temp;
  int VNnum;
  int sT, rT; //
  char ch[2];
  int node1,node2,wt;
  stringstream st;
  string name;
  //    st<<"/home/js/INPUT/"<<"VN"<<order<<".txt";
  st<<"VN"<<order<<".txt";
  st>>name;
  ifstream vnf;
  vnf.open(name.c_str());

  VNs = new VNet*[VNNUM];

  for(VNnum=0;VNnum<VNNUM;VNnum++){	   
	vnf>>ch[0]>>ch[1];
	sT = 0;
	rT = 0;
	for(i=0;i<MAXVNODENUM;i++){
	  for(j=0;j<MAXVNODENUM;j++){
		linkwt[i][j] = 0;
	  }
	  cpu[i] = 0;
	}    
	vnf>>nodes>>links>>sT>>rT;				// Read # of V Nodes, # of V links, sT, rT
	VNs[VNnum] = new VNet(nodes, links, sT, rT);
	k=0;
	for(i=0;i<nodes;i++)	vnf>>cpu[i];			// Read CPU(node)
	for(i=0;i<links;i++){					// Read Capacity(link)
	  vnf>>node1>>node2>>wt;
	  linkwt[node1][node2]=wt;
	  linkwt[node2][node1]=wt;
	}
	// Plug Link data to VNs structure
	k=0;
	for(i=0;i<nodes;i++){     
	  for(j=i;j<nodes;j++){
		if(linkwt[i][j]>0){
		  VNs[VNnum]->Links[k] = new VLink();
		  VNs[VNnum]->Links[k]->node1 = i;
		  VNs[VNnum]->Links[k]->node2 = j;
		  for(p=0;p<MAXPATH;p++)	VNs[VNnum]->Links[k]->embeddedPath[p] = -1;
		  VNs[VNnum]->Links[k]->capacity = linkwt[i][j];
		  linkwt[i][j] = k+1;
		  linkwt[j][i] = k+1;
		  VNs[VNnum]->map[i][j]=1;
		  VNs[VNnum]->map[j][i]=1;
		  k++;
		}
	  }
	}
	// Plug Node data to VNs structure
	for(i=0;i<nodes;i++){
	  k=0;
	  for(j=0;j<nodes;j++){
		if(linkwt[i][j]>0)	k++;
	  }
	  VNs[VNnum]->Nodes[i] = new VNode(k);
	  VNs[VNnum]->Nodes[i]->cpu = cpu[i];
	  VNs[VNnum]->Nodes[i]->embeddedPNID = -1;
	  p=0;
	  for(j=0;j<nodes && p<k;j++){
		if(linkwt[i][j]>0){
		  VNs[VNnum]->Nodes[i]->Links[p] = linkwt[i][j]-1;
		  p++;
		}
	  }
	  VNs[VNnum]->Nodes[i]->NumLink = p;
	}
  }
  cout<<"# of VN Req :"<<VNnum<<endl;
  vnf.close();
  flog<<"MakeVNs() is DONE" << endl;
}

/*
  void MakeOriginCG()

  Generate Conflict Graph Depending on CONFMODE

*/

void MakeOriginCG(){
  //OriginCG
  int numlink = PN->NumLink;
  int i, j, k;
  int x, y;
  int ** onehop;
     
  Conflict = new int*[numlink];    
  for(i=0;i<numlink;i++){
	Conflict[i] = new int[numlink];
	for(j=0;j<numlink;j++)	Conflict[i][j]=0;
  }
  if(CONFMODE ==1){		// 1hop interference model - pair of links shares a node -> intefering pair
	for(i=0;i<numlink;i++){
	  for(j=i+1;j<numlink;j++){
		if((PN->Links[i]->node1==PN->Links[j]->node1)||(PN->Links[i]->node1==PN->Links[j]->node2)||(PN->Links[i]->node2==PN->Links[j]->node1)||(PN->Links[i]->node2==PN->Links[j]->node2)){
		  Conflict[i][j] = 1;
		  Conflict[j][i] = 1;
		}
	  }
	}
  }
  else if(CONFMODE == 2){ // 2hop interference model - 1hop + nodes of each link are connected with a link -> intefering pair
	onehop = new int*[numlink];
	for(i=0;i<numlink;i++){
	  onehop[i] = new int[numlink];
	  for(j=0;j<numlink;j++)	onehop[i][j]=0;
	}
	for(i=0;i<numlink;i++){
	  for(j=i+1;j<numlink;j++){
		if((PN->Links[i]->node1==PN->Links[j]->node1)||(PN->Links[i]->node1==PN->Links[j]->node2)||(PN->Links[i]->node2==PN->Links[j]->node1)||(PN->Links[i]->node2==PN->Links[j]->node2)){
		  onehop[i][j] = 1;
		  onehop[j][i] = 1;
		  Conflict[i][j] = 1;
		  Conflict[j][i] = 1;
		}
	  }
	} 
	for(i=0;i<numlink;i++){
	  for(j=0;j<numlink;j++){
		if(onehop[i][j]==1){
		  for(k=0;k<numlink;k++){
			if(onehop[j][k]==1)	Conflict[i][k] = 1;
		  }
		}
	  }
	  Conflict[i][i] = 0;
	}
    
	for( i=0;i<numlink;i++)	delete [] onehop[i];
	delete [] onehop;
  }
  //	else{//else. not implemented.
  //	}
  for(i=0;i<numlink;i++){
	for(j=0;j<numlink;j++){
	  if(Conflict[i][j]==1)
		PN->Links[i]->intfnum++;
	}
  }
  flog<<"MakeOriginCG() is DONE"<<endl;
}



// Find weighted shortest path between a pair of physical nodes in SN
void Calcspaths(){
  int i,j,k;
  // initialize
  for(i=0;i<SNODENUM;i++){
	for(j=0;j<SNODENUM;j++){
	  for(k=0;k<MAXPATH;k++){
		ShortestPath[i][j][k] = -1;
	  }
	}
  }
  flog<<"   Calcspaths() Initializing is DONE"<<endl;
  // calculate weighted distances
  for(i=0;i<SNODENUM;i++){
	for(j=0;j<SNODENUM;j++){
	  distances[i][j] = 0;
	}
  }

  flog<<"   Calcspaths() Updating distances is DONE"<<endl;
  for(i=0;i<SNODENUM;i++){
	SpathfromRoot(i);
  }
  flog<<"Calcspaths() is DONE"<<endl;
}


// Calcspaths() subroutine
void SpathfromRoot(int root){
  int i,j,k;
  int numnode = PN->NumNode;
  int numlink = PN->NumLink;
  int smallest; //dijkstra
  int temp; 
  bool findall = false;

  bool* found = new bool[numnode]; 			
  int* prev = new int[numnode]; 			
  double* distanceFromStart = new double[numnode]; 	
  double* weightedlength = new double[numlink];		
  int* Stack = new int[numnode]; 			

  for(i=0;i<numnode;i++){
	prev[i] = -1;
	distanceFromStart[i] = 999999;
	found[i] = false;
	Stack[i] = -1;
  }
  for(i=0;i<numlink;i++){
	switch (ALGTYPE_LENGTH){
	case 1:	// hop count
	  weightedlength[i] = 1;
	  break;
	case 2: // weighted lenght
	  weightedlength[i] = ((double)PN->Links[i]->intfnum)/PN->Links[i]->capacity;
	  break;
	}
  }  
  distanceFromStart[root] = 0;
  smallest = root;
  queue adjnodes = queue(numnode);
  double distance;
  while(!findall){
	findall = true;
	for(i=0;i<numnode;i++){
	  Stack[i] = -1;
	}
	while(!(adjnodes.isEmpty())){
	  adjnodes.dequeue();
	}

	for(i=0;i<numnode;i++){
	  if(found[i]==0){
		smallest = i;
		break;
	  }
	}
	for(i=0;i<numnode;i++){
	  if(found[i]==0 && distanceFromStart[i] < distanceFromStart[smallest])
		smallest = i;
	}
	found[smallest] = 1;

	for(i=0;i<numlink;i++){
	  int ad=-1;
	  if(PN->Links[i]->node1 == smallest)
		ad = PN->Links[i]->node2;
	  else if(PN->Links[i]->node2 == smallest)
		ad = PN->Links[i]->node1;
	  if((ad!=-1) && found[ad]==0)
		adjnodes.enqueue(ad);
	}
	while(!(adjnodes.isEmpty())){
	  temp = adjnodes.dequeue();
	  distance = Distance(smallest,temp,weightedlength) + distanceFromStart[smallest];
	  if(distance < distanceFromStart[temp]){
		distanceFromStart[temp] = distance;
		prev[temp] = smallest;
	  }
	}

	temp = smallest;
	k=0;
	while(temp!=-1){
	  Stack[k]=temp;
	  temp = prev[temp];
	  k++;
	}
	for(i=0;i<k-1;i++){
	  ShortestPath[root][smallest][i] = getSNlink(Stack[k-1-i],Stack[k-2-i]);
	}
	distances[root][smallest] = distanceFromStart[smallest];
	for(i=0; i<numnode; i++){
	  if(found[i]==0){
		findall = false;
		break;
	  }
	}
  }

  delete [] prev;
  delete [] found;
  delete [] weightedlength;
  delete [] Stack;
  delete [] distanceFromStart;


}


// Get distance between n1 & n2
double Distance(int n1, int n2, double* weightedlength){
  int i;
  for(i=0;i<(PN->NumLink);i++){
	if( ((PN->Links[i]->node1==n1) && (PN->Links[i]->node2==n2))||((PN->Links[i]->node1 == n2)&&(PN->Links[i]->node2==n1))){
	  return weightedlength[i];
	}
  }
  return -1;
}

// Get link number between node1 & node2
int getSNlink(int node1, int node2){
  int i;
  for( i=0;i<(PN->NumLink);i++){
	if( ((PN->Links[i]->node1==node1)&&(PN->Links[i]->node2==node2))||((PN->Links[i]->node1==node2)&&(PN->Links[i]->node2==node1))){
	  return i;
	}
  }
  return -1;
}

void ShortestTest(int source, int dest){
  int i;
  ofstream test;
  test.open("test_.dem");
  test<<"set noautoscale"<<endl;
  test<<"set style data points"<<endl;
  test<<"set xrange [0:"<<XRANGE<<"]"<<endl;
  test<<"set yrange [0:"<<(YRANGE+20)<<"]"<<endl;
  test<<"set term postscript eps color solid \"Time-Roman\" 30"<<endl;
  test<<"set output 'output_test_.eps'"<<endl;
  //	test<<"set output 'shortest_"<<source<<"to"<<dest<<".eps'"<<endl;
	
  for(i=0;i<MAXPATH && (ShortestPath[source][dest][i] !=-1);i++){
	test<<"set arrow from "<<(PN->Nodes[PN->Links[ShortestPath[source][dest][i]]->node1]->x)<<","<<(PN->Nodes[PN->Links[ShortestPath[source][dest][i]]->node1]->y);
	test<<" to "<<(PN->Nodes[PN->Links[ShortestPath[source][dest][i]]->node2]->x)<<","<<(PN->Nodes[PN->Links[ShortestPath[source][dest][i]]->node2]->y)<<" nohead lt -1 lw 5.0"<<endl;
  }
  for(i=0;i<(PN->NumLink);i++){
	test<<"set arrow from "<<(PN->Nodes[(PN->Links[i]->node1)]->x)<<","<<(PN->Nodes[(PN->Links[i]->node1)]->y)<<" to "<<(PN->Nodes[(PN->Links[i]->node2)]->x)<<","<<(PN->Nodes[(PN->Links[i]->node2)]->y)<<" nohead lt -1 lw 1.0"<<endl;	
  }
  test<<"plot 'node.txt' title \"Nodes\" pt 30 ps 1.5"<<endl;
  test.close();
  FILE* pipe1;
  pipe1 = popen("gnuplot test_.dem", "w");		// process open, write over
  fclose(pipe1);
}



void Unmap(int VID){
  int i,j,k;
  int numnode = VNs[VID]->NumNode;
  int numlink = VNs[VID]->NumLink;
  for(i=0;i<numnode;i++){
	PN->Nodes[VNs[VID]->Nodes[i]->embeddedPNID]->filledW -= VNs[VID]->Nodes[i]->cpu;
	for(j=0;j<MAXEMBEDNODE;j++){
	  if(PN->Nodes[VNs[VID]->Nodes[i]->embeddedPNID]->embedVNID[j] == VID){
		PN->Nodes[VNs[VID]->Nodes[i]->embeddedPNID]->embedVNID[j] = -1;
		PN->Nodes[VNs[VID]->Nodes[i]->embeddedPNID]->embedVNode[j] = -1;     
	  }
	}
	VNs[VID]->Nodes[i]->embeddedPNID=-1;
  }
  for(i=0;i<numlink;i++){
	for(j=0;j<MAXPATH;j++){
	  if(VNs[VID]->Links[i]->embeddedPath[j] >= 0){
		PN->Links[VNs[VID]->Links[i]->embeddedPath[j]]->filledW -= VNs[VID]->Links[i]->capacity;
		for(k=0;k<MAXEMBEDLINK; k++){
		  if(PN->Links[VNs[VID]->Links[i]->embeddedPath[j]]->embedVNID[k] == VID){
			PN->Links[VNs[VID]->Links[i]->embeddedPath[j]]->embedVNID[k] = -1;
			PN->Links[VNs[VID]->Links[i]->embeddedPath[j]]->embedVLink[k] = -1;
		  }
		}
	  }
	  VNs[VID]->Links[i]->embeddedPath[j]=-1;
	}
  }
  //	flog<<"Unmap() Umapping"<<VID<<" is DONE"<<endl;
  VNs[VID]->isEmbedded = false;
}


void NodeMapOrder(int VID, int* output){
  //////////////////////////////////////////////////////////////////////
  if(ALGTYPE_COUPLE == 3){			// FULL
	//////////////////////////////////////////////////////////////////////
	int BiggestNode;
	int Requiretemp;
	int BiggestValue;
	int hop;
	int len;
	int** BFSArr;
	int* Require;
	int i,j,k;
	int numnode = VNs[VID]->NumNode;
         
	BFSArr = new int*[numnode];
	Require = new int[numnode];

	for(i=0;i<numnode;i++){
	  BFSArr[i] = new int[numnode];
	  Require[i] = 0;
	}
	for(i=0;i<MAXVNODENUM;i++){
	  output[i]=-1;
	}

	BiggestNode=0;
	BiggestValue=0;
	k =0;
	for(i=0;i<numnode;i++){
	  k =0;
	  for(j=0;j<(VNs[VID]->Nodes[i]->NumLink);j++){
		k += VNs[VID]->Links[(VNs[VID]->Nodes[i]->Links[j])]->capacity;
	  }
	  k *= ALPHA;
	  k += VNs[VID]->Nodes[i]->cpu;
	  Require[i] = k;
	  if(k > BiggestValue){
		BiggestValue = k;
		BiggestNode = i;
	  }
	}
	BFS(VNs[VID]->map, VNs[VID]->NumNode, BiggestNode, BFSArr);
    hop=0;
    len=0;

    for(i=0;i<numnode;i++){
	  for(j=1;j<numnode;j++){
		if(BFSArr[i][j]==-1){
		  j=numnode;
		  continue;	// i.e., break;
		}
		else{
		  BiggestNode = BFSArr[i][j];
		  Requiretemp = Require[BFSArr[i][j]];
		  k = j;
		  while(k>0 && (Require[BFSArr[i][k-1]] < Require[BFSArr[i][k]]) ){
			BFSArr[i][k]=BFSArr[i][k-1];
			Require[BFSArr[i][k]] = Require[BFSArr[i][k-1]];
			k--;
		  }
		  BFSArr[i][k] = BiggestNode;
		  Require[BFSArr[i][k]] = Requiretemp;
		}
	  }
	}
	for(i=0;i<numnode;i++){
	  if(BFSArr[hop][len]==-1){
		hop++;
		len=0;
	  }
	  output[i] = BFSArr[hop][len];
	  len++;
	}
 
    delete [] Require;
	for(i=0;i<numnode;i++){
	  delete [] BFSArr[i];
	}
	delete [] BFSArr;
	//////////////////////////////////////////////////////////////////////
  }
  //////////////////////////////////////////////////////////////////////
  else if(ALGTYPE_COUPLE == 2){	// Certain/MID
	//////////////////////////////////////////////////////////////////////
	int* Require;
	int i,j,k;
	int numnode = VNs[VID]->NumNode;
	int temp;
	Require = new int[numnode];
    
	for(i=0;i<numnode;i++){
	  k=0;
	  for(j=0;j<VNs[VID]->Nodes[i]->NumLink;j++){
		k += VNs[VID]->Links[VNs[VID]->Nodes[i]->Links[j]]->capacity;
	  }
	  k *=ALPHA;
	  k += VNs[VID]->Nodes[i]->cpu;
	  Require[i]=k;
	  output[i]=i;
	}
	for(j=1;j<numnode;j++){
	  if(Require[output[j-1]]<Require[output[j]]){
		for(k=j;k>0 && Require[output[k-1]]<Require[output[k]];k--){
		  temp = output[k-1];
		  output[k-1]=output[k];
		  output[k]=temp;
		}
	  }
	}
	delete [] Require;
	//////////////////////////////////////////////////////////////////////
  }
  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////
  else if(ALGTYPE_COUPLE == 1){	// NON
	//////////////////////////////////////////////////////////////////////
	int* Require;
	int i,j,k;
	int numnode = VNs[VID]->NumNode;
	int temp;
	Require = new int[numnode];
    
	for(i=0;i<numnode;i++){
	  k=0;
	  for(j=0;j<VNs[VID]->Nodes[i]->NumLink;j++){
		k += VNs[VID]->Links[VNs[VID]->Nodes[i]->Links[j]]->capacity;
	  }
	  k *=ALPHA;
	  k += VNs[VID]->Nodes[i]->cpu;
	  Require[i]=k;
	  output[i]=i;
	}
	for(j=1;j<numnode;j++){
	  if(Require[output[j-1]]<Require[output[j]]){
		for(k=j;k>0 && Require[output[k-1]]<Require[output[k]];k--){
		  temp = output[k-1];
		  output[k-1]=output[k];
		  output[k]=temp;
		}
	  }
	}
	delete [] Require;
	//////////////////////////////////////////////////////////////////////
  }
  //////////////////////////////////////////////////////////////////////
  else{
	cout<<"There is no appropriate NodeMapOrder()"<<endl;
  }

}


void BFS(int** mappt, int numnode, int root, int** inout){ 
  int hop;
  int i,j,k;
  int temp;

  bool* checker;
  int endflag;
  queue que(numnode); 
  checker = new bool[numnode];

  for(i=0;i<numnode;i++)
	checker[i] = false;

  for(i=0;i<numnode;i++)
	for(j=0;j<numnode;j++)
	  inout[i][j] = -1;

  que.enqueue(root);
  checker[root]=true;
  if(root>=numnode)
	cout<<"error!"<<endl;

  inout[0][0]=root;
  hop=1;
  endflag = root;
  k=0;
     
  while(!que.isEmpty()){
	temp = que.dequeue();	
	if(temp == -1||que.bot<0||que.bot>=numnode){
	  cout<<"error!"<<endl;
	}
	j=0;
	if(temp==endflag){
	  j=1;
	}
	for(i=0;i<numnode;i++){
	  if(mappt[temp][i]==1 && (!checker[i])){
		if(hop>=numnode||k>=numnode||que.top<0||que.top>=numnode)
		  cout<<"error!"<<endl;

		inout[hop][k]=i;
		k++;
		que.enqueue(i);
		checker[i]=true;
		if(j==1)
		  endflag = i;
	  }
	}
	if(j==1){
	  hop++;
	  k=0;
	}
  }
  delete [] checker;
}

//SNodeOrder based on remaining capacity
void SNodeOrder(int* output){
  int i,j,k;
  int numnode = PN->NumNode;
  int temp1;
  int temp2;
  int* Residual;
  Residual = new int[numnode];

  for(i=0;i<numnode;i++){
	k = 0;
	for(j=0;j<(PN->Nodes[i]->NumLink);j++){
	  k += (PN->Links[(PN->Nodes[i]->Links[j])]->capacity - PN->Links[(PN->Nodes[i]->Links[j])]->filledW);
	}
	k *= ALPHA;
	k += (PN->Nodes[i]->cpu - PN->Nodes[i]->filledW);
	Residual[i] = k;
	output[i]=i;
  }
 
  for(i=1;i<numnode;i++){
	temp1 = output[i];
	temp2 = Residual[output[i]];
	j = i;
	while(j>0 && Residual[output[j-1]]<temp2){
	  output[j]=output[j-1];
	  Residual[output[j]]=Residual[output[j-1]];
	  j--;
	}
	output[j]=temp1;
	Residual[output[j]]=temp2;
  }
  delete [] Residual;
}



//NEmbedTo [Node Embed to] : embed node to bufSN
void NEmbedTo(int VID, int from, int to){
  //flog<<"Node Embed "<<from<<" -> "<<to<<endl;
  bufPN->Nodes[to]->filledW += VNs[VID]->Nodes[from]->cpu;
  for(int i = 0;i<MAXEMBEDNODE;i++){
	if(bufPN->Nodes[to]->embedVNID[i]==-1){
	  bufPN->Nodes[to]->embedVNID[i] = VID;
	  bufPN->Nodes[to]->embedVNode[i] = from;
	  break;
	}
  }
  VNs[VID]->Nodes[from]->embeddedPNID = to;
}

//EEmbedTo : : embed link to bufSN
void EEmbedTo(int VID, int from, int* to){
  // flog<<"Link Embed "<<from<<" -> "<<"path";
  for(int i=0;i<MAXPATH&&to[i]>=0;i++){
	//    	flog<<" "<<to[i];
	bufPN->Links[to[i]]->filledW += VNs[VID]->Links[from]->capacity;
	for(int j =0;j<MAXEMBEDLINK;j++){
	  if(bufPN->Links[to[i]]->embedVNID[j]==-1){
		bufPN->Links[to[i]]->embedVNID[j] = VID;
		bufPN->Links[to[i]]->embedVLink[j] = from;
		break;
	  }
	}
	VNs[VID]->Links[from]->embeddedPath[i] = to[i]; 
  }
  //flog<<endl;
}

// SUM norm
bool Embed_SUM_PBGON(int VID, int NID, int* embedArr){
  int i,j,k,p;
  int can[SNODENUM];		// candidate of phyical node
  int numnode = bufPN->NumNode;
  int temp;
  int* neighbor;
  int* tempnbr;
  int* cntvlinks;
  int neibnum = VNs[VID]->Nodes[NID]->NumLink;


  tempnbr = new int[neibnum];
  temp = 0;
  for(i=0;i<neibnum;i++){ // find tempnbr
	if(VNs[VID]->Links[VNs[VID]->Nodes[NID]->Links[i]]->node1 == NID)
	  tempnbr[i] = VNs[VID]->Links[VNs[VID]->Nodes[NID]->Links[i]]->node2;
	else
	  tempnbr[i] = VNs[VID]->Links[VNs[VID]->Nodes[NID]->Links[i]]->node1;
	k=0;
	temp++;
	for(j=0;j<numnode;j++){
	  if(tempnbr[i] == embedArr[j])
		k=1;
	}
	if(k==0){
	  tempnbr[i]=-1;
	  temp--;
	}
  }

  neighbor = new int[temp];
  j=0;
  for(i=0;i<neibnum;i++){
	if(tempnbr[i]!=-1){
	  for(k=0;k<numnode;k++){
		if(embedArr[k]==tempnbr[i]){
		  neighbor[j] = k;
		  j++;
		}
	  }
	}
  }
    
  cntvlinks = new int[temp];
  k=0;
  for(i=0;i<neibnum;i++){
	if(tempnbr[i]!=-1){
	  for(j=0;j<neibnum;j++){
		if(VNs[VID]->Links[VNs[VID]->Nodes[NID]->Links[j]]->node1 == tempnbr[i] || VNs[VID]->Links[VNs[VID]->Nodes[NID]->Links[j]]->node2 == tempnbr[i]){
		  cntvlinks[k]=VNs[VID]->Nodes[NID]->Links[j];
		  k++;
		}
	  }
	}
  }
    
  neibnum = temp;
    
  //find can
  j=0;
  k=1;
  for(i=0;i<numnode;i++){
	if((bufPN->Nodes[i]->cpu - bufPN->Nodes[i]->filledW >= VNs[VID]->Nodes[NID]->cpu) && embedArr[i] < 0){
	  can[j] = i;
	  j++;
	}
	else{
	  can[numnode-k]=-1;
	  k++;
	}

	if(k>numnode){
	  delete [] neighbor;
	  delete [] tempnbr;
	  delete [] cntvlinks;
	  return false;
	}
  }

  int cannum = j;
  double *cost = new double[cannum]; 
  int cantemp;
  double costtemp;
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  if(ALGTYPE_LENGTH == 1){		// hop distance only
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	int* Residual;
	int temp1;
	int temp2;
	Residual = new int[cannum];

	for(i=0;i<cannum;i++){
	  k=0;
	  for(j=0;j<PN->Nodes[can[i]]->NumLink;j++){
		k+=(PN->Links[(PN->Nodes[can[i]]->Links[j])]->capacity - PN->Links[(PN->Nodes[can[i]]->Links[j])]->filledW);
	  }
	  k *= ALPHA;
	  k += (PN->Nodes[can[i]]->cpu - PN->Nodes[can[i]]->filledW);
	  Residual[i]=k;
	}
	for(i=1;i<cannum;i++){
	  temp1 = can[i];
	  temp2 = Residual[i];
	  j=i;
	  while(j>0 && Residual[j-1]<temp2){
		can[j]=can[j-1];
		Residual[j]=Residual[j-1];
		j--;
	  }
	  can[j]=temp1;
	  Residual[j]=temp2;
	}
	delete [] Residual;
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  }
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  for(i=0;i<cannum;i++){
	cost[i] = 0.0;
	for(j=0;j<neibnum;j++){
	  if(NORMTYPE == 1){
		cost[i] += distances[can[i]][neighbor[j]] * VNs[VID]->Links[cntvlinks[j]]->capacity;
	  }
	  else if(NORMTYPE > 1){
		cost[i] += pow((double) (distances[can[i]][neighbor[j]]) * (VNs[VID]->Links[cntvlinks[j]]->capacity), (double)NORMTYPE);
	  }
	}
	if(NORMTYPE > 1){
	  cost[i] = pow(cost[i], 1.0/((double)NORMTYPE) );
	}
  }
  for(i=1;i<cannum;i++){
	costtemp = cost[i];
	cantemp = can[i];
	j=i;
	while(cost[j-1]>costtemp&&j>0){
	  cost[j] = cost[j-1];
	  can[j] = can[j-1];
	  j--;
	}
	cost[j]=costtemp;
	can[j]=cantemp;
  }
    
  delete [] cost;


  bool pbgok = false;
  delete tempPN;
  tempPN = new PNet(*bufPN);
  delete bufVN;
  bufVN = new VNet(*VNs[VID]);

  for(i=0;i<F;i++){
	delete bufPN;
	bufPN = new PNet(*tempPN);
	delete VNs[VID];
	VNs[VID] = new VNet(*bufVN);

	//can[i]에 임베딩 한다.

	NEmbedTo(VID,NID,can[i]);
	for(j=0;j<neibnum;j++){ 
	  k=0;
	  for(p=0;p<VNs[VID]->NumLink;p++){ 
		if((VNs[VID]->Links[p]->node1==NID && VNs[VID]->Links[p]->node2==embedArr[neighbor[j]])||(VNs[VID]->Links[p]->node2==NID && VNs[VID]->Links[p]->node1==embedArr[neighbor[j]])){
		  k = p;
		  break;	//	p = VNs[VID]->NumLink+10;
		}
	  }
	  EEmbedTo(VID, k, ShortestPath[can[i]][neighbor[j]]);
	}
	pbgok = PBGcheck(*bufPN);
	if(pbgok){
	  embedArr[can[i]]=NID;
	  break;	    
	}
  }
  if(!pbgok){
	delete bufPN;
	bufPN = new PNet(*tempPN);
	delete bufVN;
	bufVN = new VNet(*VNs[VID]);
	delete [] neighbor;
	delete [] tempnbr;
	delete [] cntvlinks;
	return false;
  }
  delete [] neighbor;
  delete [] tempnbr;
  delete [] cntvlinks;
  return true;
}

bool Embed_SUM_PBGOFF(int VID, int NID, int* embedArr){
  int i,j,k,p;
  int can[SNODENUM]; 
  int bestcan; 
  double bestcost; 
  double costtemp; 
  int numnode = bufPN->NumNode;
  int temp;
  int* neighbor;
  int* tempnbr; 
  int* cntvlinks;
  int neibnum = VNs[VID]->Nodes[NID]->NumLink; 

  tempnbr = new int[neibnum];
  temp = 0;
  for(i=0;i<neibnum;i++){
	if(VNs[VID]->Links[VNs[VID]->Nodes[NID]->Links[i]]->node1 == NID){
	  tempnbr[i] = VNs[VID]->Links[VNs[VID]->Nodes[NID]->Links[i]]->node2;
	}
	else{
	  tempnbr[i] = VNs[VID]->Links[VNs[VID]->Nodes[NID]->Links[i]]->node1;
	}
	k=0;
	temp++;
	for(j=0;j<numnode;j++){
	  if(tempnbr[i] == embedArr[j]){
		k=1;
	  }
	}
	if(k==0){
	  tempnbr[i]=-1;
	  temp--;
	}
  }
  neighbor = new int[temp];
  j=0;
  for(i=0;i<neibnum;i++){
	if(tempnbr[i]!=-1){
	  for(k=0;k<numnode;k++){
		if(embedArr[k]==tempnbr[i]){
		  neighbor[j] = k;
		  j++;
		}
	  }
	}
  }  
  cntvlinks = new int[temp];
  k=0;
  for(i=0;i<neibnum;i++){
	if(tempnbr[i]!=-1){
	  for(j=0;j<neibnum;j++){
		if(VNs[VID]->Links[VNs[VID]->Nodes[NID]->Links[j]]->node1 == tempnbr[i] || VNs[VID]->Links[VNs[VID]->Nodes[NID]->Links[j]]->node2 == tempnbr[i]){
		  cntvlinks[k]=VNs[VID]->Nodes[NID]->Links[j];
		  k++;
		}
	  }
	}
  }
  neibnum = temp;
  //find can
  j=0;
  k=1;
  for(i=0;i<numnode;i++){
	if((bufPN->Nodes[i]->cpu - bufPN->Nodes[i]->filledW >= VNs[VID]->Nodes[NID]->cpu) && embedArr[i]<0){
	  can[j] = i;
	  j++;
	}
	else{
	  can[numnode-k]=-1;
	  k++;
	}
	if(k>numnode){
	  delete [] neighbor;
	  delete [] tempnbr;
	  delete [] cntvlinks;			
	  return false;
	}
  }
  ////////////////////////////////////////////////////////////////////////////////////////////////
  if(ALGTYPE_LENGTH == 1){	// hop distance only
	////////////////////////////////////////////////////////////////////////////////////////////////
	int* Residual;
	int cannum=j;
	int temp1;
	int temp2;
	Residual = new int[cannum];
	for(i=0;i<cannum;i++){
	  k=0;
	  for(j=0;j<PN->Nodes[can[i]]->NumLink;j++){
		k+=(PN->Links[(PN->Nodes[can[i]]->Links[j])]->capacity - PN->Links[(PN->Nodes[can[i]]->Links[j])]->filledW);
	  }
	  k *=ALPHA;
	  k += (PN->Nodes[can[i]]->cpu - PN->Nodes[can[i]]->filledW);
	  Residual[i]=k;
	}
 
	for(i=1;i<cannum;i++){
	  temp1 = can[i];
	  temp2 = Residual[i];
	  j=i;
	  while(j>0 && Residual[j-1]<temp2){
		can[j]=can[j-1];
		Residual[j]=Residual[j-1];
		j--;
	  }
	  can[j]=temp1;
	  Residual[j]=temp2;
	}
	delete [] Residual;
  }
  for(i=0;can[i]>=0;i++){
	costtemp=0; 
	for(j=0;j<neibnum;j++){
	  if(NORMTYPE == 1){
		costtemp += distances[can[i]][neighbor[j]] * VNs[VID]->Links[cntvlinks[j]]->capacity;
	  }
	  else if(NORMTYPE > 1){
		costtemp += pow((double) (distances[can[i]][neighbor[j]]) * (VNs[VID]->Links[cntvlinks[j]]->capacity), (double)NORMTYPE);
	  }
	}
	if(NORMTYPE > 1){
	  costtemp = pow(costtemp, 1.0/((double)NORMTYPE) );
	}
	if(i==0){
	  bestcan = can[i];
	  bestcost = costtemp;
	}
	if(costtemp<bestcost){
	  bestcan = can[i];
	  bestcost = costtemp;
	}
  }

  NEmbedTo(VID,NID,bestcan);
  for(j=0;j<neibnum;j++){ 
	k=0;
	for(p=0;p<VNs[VID]->NumLink;p++){ 
	  if((VNs[VID]->Links[p]->node1==NID && VNs[VID]->Links[p]->node2 == embedArr[neighbor[j]])||(VNs[VID]->Links[p]->node2 == NID && VNs[VID]->Links[p]->node1 == embedArr[neighbor[j]])){
		k=p;
		break;	
	  }
	}
	EEmbedTo(VID, k ,ShortestPath[bestcan][neighbor[j]]);
  }
  embedArr[bestcan] = NID;
  delete [] neighbor;
  delete [] tempnbr;
  delete [] cntvlinks; 
  return true;
}


// MAX norm - Weight, PBGON
bool Embed_MAX_Weight_PBGON(int VID, int NID, int* embedArr){
  int i,j,k,p;
  int can[SNODENUM]; 
  int numnode = bufPN->NumNode;
  int temp;
  int* neighbor; 
  int* tempnbr;
  int neibnum = VNs[VID]->Nodes[NID]->NumLink; 
  int* cntvlinks; 

  tempnbr = new int[neibnum];
  temp = 0;
  for(i=0;i<neibnum;i++){ 
	if(VNs[VID]->Links[VNs[VID]->Nodes[NID]->Links[i]]->node1 == NID)
	  tempnbr[i] = VNs[VID]->Links[VNs[VID]->Nodes[NID]->Links[i]]->node2;
	else
	  tempnbr[i] = VNs[VID]->Links[VNs[VID]->Nodes[NID]->Links[i]]->node1;
	k = 0;
	temp++;
	for(j=0;j<numnode;j++){
	  if(tempnbr[i] == embedArr[j])
		k = 1;
	}
	if(k==0){
	  tempnbr[i]=-1;
	  temp--;
	}
  }

  neighbor = new int[temp];
  j = 0;
  for(i=0;i<neibnum;i++){ 
	if(tempnbr[i]!=-1){
	  for(k=0;k<numnode;k++){
		if(embedArr[k]==tempnbr[i]){
		  neighbor[j] = k;
		  j++;
		}
	  }
	}
  }
  cntvlinks = new int[temp];
  k = 0;
  for(i=0;i<neibnum;i++){
	if(tempnbr[i]!=-1){
	  for(j=0;j<neibnum;j++){
		if(VNs[VID]->Links[VNs[VID]->Nodes[NID]->Links[j]]->node1 == tempnbr[i] || VNs[VID]->Links[VNs[VID]->Nodes[NID]->Links[j]]->node2 == tempnbr[i]){
		  cntvlinks[k]=VNs[VID]->Nodes[NID]->Links[j];
		  k++;
		}
	  }
	}
  }
  neibnum = temp;
  //find can
  j=0;
  k=1;
  for(i=0;i<numnode;i++){
	if((bufPN->Nodes[i]->cpu - bufPN->Nodes[i]->filledW) >= VNs[VID]->Nodes[NID]->cpu && embedArr[i]<0 ){
	  can[j] = i;
	  j++;
	}
	else{
	  can[numnode-k]=-1;
	  k++;
	}	
	if(k>numnode){
	  delete [] neighbor;
	  delete [] tempnbr;
	  delete [] cntvlinks;
	  return false;
	}
  }
    

  int cannum = j;
  double *cost = new double[cannum];
  int cantemp;
  double costtemp;

  for(i=0;i<cannum;i++){
	cost[i] = 0.0;
	////////////////////////////////////////////////////////////////////////////////
	/*
	  for(j=0;j< PN->Nodes[can[i]]->NumLink;j++){
	  cost[i] += (PN->Links[(PN->Nodes[can[i]]->Links[j])]->capacity -PN->Links[(PN->Nodes[can[i]]->Links[j])]->filledW);
	  }
	*/
	for(j=0;j<neibnum;j++){
	  cost[i] += distances[can[i]][neighbor[j]]*VNs[VID]->Links[cntvlinks[j]]->capacity;
	}

	////////////////////////////////////////////////////////////////////////////////
  }
  for(i=1;i<cannum;i++){
	costtemp = cost[i];
	cantemp = can[i];
	j=i;
	while(cost[j-1]>costtemp&&j>0){
	  cost[j] = cost[j-1];
	  can[j] = can[j-1];
	  j--;
	}
	cost[j]=costtemp;
	can[j]=cantemp;
  }

  delete tempPN;
  tempPN = new PNet(*bufPN);
  delete bufVN;
  bufVN = new VNet(*VNs[VID]);

  for(i=0;i<cannum;i++){
	NEmbedTo(VID,NID,can[i]);
	for(j=0;j<neibnum;j++){ 
	  k=0;
	  for(p=0;p<VNs[VID]->NumLink;p++){ 
		if((VNs[VID]->Links[p]->node1==NID && VNs[VID]->Links[p]->node2==embedArr[neighbor[j]])||(VNs[VID]->Links[p]->node2==NID && VNs[VID]->Links[p]->node1==embedArr[neighbor[j]])){
		  k=p;
		  p = VNs[VID]->NumLink+10;
		}
	  }
	  EEmbedTo(VID, k, ShortestPath[can[i]][neighbor[j]]);
	}
	cost[i]=bufPN->embmetric();

	delete bufPN;
	bufPN = new PNet(*tempPN);
	delete VNs[VID];
	VNs[VID] = new VNet(*bufVN);

  }

  for(i=1;i<cannum;i++){
	costtemp = cost[i];
	cantemp = can[i];
	j=i;
	while(cost[j-1]>costtemp&&j>0){
	  cost[j] = cost[j-1];
	  can[j] = can[j-1];
	  j--;
	}
	cost[j]=costtemp;
	can[j]=cantemp;
  }
  delete [] cost;
  
  bool pbgok = false;

  delete tempPN;
  tempPN = new PNet(*bufPN);
  delete bufVN;
  bufVN = new VNet(*VNs[VID]);

  for(i=0;i<cannum;i++){
	delete bufPN;
	bufPN = new PNet(*tempPN);
	delete VNs[VID];
	VNs[VID] = new VNet(*bufVN);

	NEmbedTo(VID,NID,can[i]);
	for(j=0;j<neibnum;j++){ 
	  k=0;
	  for(p=0;p<VNs[VID]->NumLink;p++){
		if((VNs[VID]->Links[p]->node1==NID && VNs[VID]->Links[p]->node2 == embedArr[neighbor[j]])||(VNs[VID]->Links[p]->node2==NID && VNs[VID]->Links[p]->node1==embedArr[neighbor[j]])){
		  k = p;
		  break;		// p = VNs[VID]->NumLink+10;
		}
	  }
	  EEmbedTo(VID, k, ShortestPath[can[i]][neighbor[j]]);
	}
	pbgok = PBGcheck(*bufPN);
	if(pbgok){
	  embedArr[can[i]]=NID;
	  break;	    
	}
  }
  if(!pbgok){
	delete bufPN;
	bufPN = new PNet(*tempPN);
	delete VNs[VID];
	VNs[VID] = new VNet(*bufVN);
	delete [] neighbor;
	delete [] tempnbr;
	delete [] cntvlinks;
	return false;
  }
  delete [] neighbor;
  delete [] tempnbr;
  delete [] cntvlinks;
  return true;
}

bool PBGcheck(const PNet& PSN){

  int i,j,k;
  int** BFSArr;
  int numlink = PSN.NumLink;
  double* CGnodeW;
  int** CGtemp;
  int* haveW;
    
  int count = 0;
  int maxhop;

  BFSArr = new int*[numlink];
  CGnodeW = new double[numlink];
  CGtemp = new int*[numlink];
  haveW = new int[numlink];

  for(i=0;i<numlink;i++){
	CGtemp[i] = new int[numlink];
	BFSArr[i] = new int[numlink];
  }
    
  for(i=0;i<numlink;i++){
	CGnodeW[i]=((double)PSN.Links[i]->filledW/PSN.Links[i]->capacity);
	for(j=0;j<numlink;j++)
	  CGtemp[i][j]=Conflict[i][j];
  }

  for(i=0;i<numlink;i++){
	if(CGnodeW[i]<=0.0){
	  for(j=0;j<numlink;j++){
		CGtemp[i][j]=0;
		CGtemp[j][i]=0;
	  }
	  haveW[i]=0;
	}
	else{
	  haveW[i]=1;
	}
  }
  for(i=0;i<numlink;i++){
	if(haveW[i]==0){
	  continue;
	}
	BFS(CGtemp,numlink,i,BFSArr);
	for(j=0;j<numlink;j++){
	  if(BFSArr[j][0]==-1){
		maxhop=j-1;
		break;
	  }
	}

	count = 0;
	for(j=0;j<=maxhop;j++){
	  for(k=0;k<numlink;k++){
		if(BFSArr[j][k] != -1){
		  count++;
		}
		else{
		  break;	//k=numlink+1;
		}
	  }
	  if(j>=2){
		if(count>P*pow(j,Q)){
		  for(i=0;i<numlink;i++){
			delete [] CGtemp[i];
			delete [] BFSArr[i];
		  }
		  delete [] BFSArr;
		  delete [] CGnodeW;
		  delete [] CGtemp;
		  delete [] haveW;

		  return false;
		}
	  }    
	}
  }

  for(i=0;i<numlink;i++){
	delete [] CGtemp[i];
	delete [] BFSArr[i];
  }
  delete [] BFSArr;
  delete [] CGnodeW;
  delete [] CGtemp;
  delete [] haveW;
  
  return true;
}

// Feasibilty check
bool FeasCheck_SUF(const PNet& FSN){
  int i, j;
  int numnode = FSN.NumNode;
  int numlink = FSN.NumLink;
  double k;
  double temp;
  double largest; 
  double* costtemp;
  for(i=0;i<numnode;i++)
	if(FSN.Nodes[i]->filledW > FSN.Nodes[i]->cpu)
	  return false;
  for(i=0;i<numlink;i++)
	if(FSN.Links[i]->filledW > FSN.Links[i]->capacity)
	  return false;

  if(CONFMODE == 1){
	costtemp = new double[numnode];
	largest = 0;
	for(i=0;i<SNODENUM;i++){
	  k=0;
	  for(j=0;j<FSN.Nodes[i]->NumLink;j++){
		temp=(double)(FSN.Links[(FSN.Nodes[i]->Links[j])]->filledW)/(double)(FSN.Links[(FSN.Nodes[i]->Links[j])]->capacity);
		k += temp;
		if(largest<temp){
		  largest = temp;
		}
	  }
	  costtemp[i]=k;
	}

	j=0;
	for(i=1;i<SNODENUM;i++){
	  if(costtemp[i]>=costtemp[j])j=i;
	}
        
	if(costtemp[j]<=(2/3) || (costtemp[j]+largest)<=1){
	  delete [] costtemp;
	  return true;
	}
	else{
	  delete [] costtemp;
	}
  }
  costtemp = new double[numlink];
  for(i=0;i<numlink;i++){
	k = (double)(FSN.Links[i]->filledW)/(double)(FSN.Links[i]->capacity)/(1.0-2.0*EPS);
	for(j=0;j<numlink;j++){
	  if(Conflict[i][j]==1){
		k += (double)(FSN.Links[j]->filledW)/(double)(FSN.Links[j]->capacity)/(1.0-2.0*EPS);
	  }
	}
	costtemp[i] = k;
  }
  j=0;
  for(i=1;i<numlink;i++){
	if(costtemp[i]>costtemp[j])
	  j=i;
  }     
  if(costtemp[j]<=1){
	delete [] costtemp;
	return true;
  }
  else{
	delete [] costtemp;
	return false;
  }
}

bool Necessary_PBG(const PNet& FSN){
  int i,j,k,p,q,r,t;
  int numnode = FSN.NumNode;
  int numlink = FSN.NumLink;
  double* qlength;

  int mwisv;			
  double wmwis0, wmwis1;		

  int CGWnum=0;				
  int numvref;
  double* CGnodeW;			
    
  int **subneigh0, **subneigh1;	
  int *vsub0, *vsub1;		
  int *submwis0, *submwis1;	

  int** CGtemp;			
  int** CGtemp1;		
  int* haveW;			
  int* vref;			
    
  int* mwis; 

  for(i=0;i<numnode;i++)
	if(FSN.Nodes[i]->filledW > FSN.Nodes[i]->cpu)
	  return false;
  for(i=0;i<numlink;i++)
	if(FSN.Links[i]->filledW > FSN.Links[i]->capacity)
	  return false;

  qlength = new double[numlink];
  CGnodeW = new double[numlink];
  haveW = new int[numlink];
  vsub0 = new int[numlink];
  vsub1 = new int[numlink];
  submwis0 = new int[numlink];
  submwis1 = new int[numlink];
    
  vref = new int[numlink];
  mwis = new int[numlink];
    
  subneigh0 = new int*[numlink];
  subneigh1 = new int*[numlink];
  CGtemp = new int*[numlink];
  CGtemp1 = new int*[numlink];

  for(i=0;i<numlink;i++){
	subneigh0[i] = new int[numlink];
	subneigh1[i] = new int[numlink];
	CGtemp[i] = new int[numlink];
	CGtemp1[i] = new int[numlink];
  }

  
  for(i=0;i<numlink;i++){
	qlength[i] = 0;		// init qlength
	CGnodeW[i] = ((double)FSN.Links[i]->filledW/FSN.Links[i]->capacity);
	if(CGnodeW[i]>1){
	  delete [] qlength;
	  delete [] CGnodeW;
	  delete [] vsub0;
	  delete [] vsub1;
	  delete [] submwis0;
	  delete [] submwis1;
	  delete [] haveW;
	  delete [] vref;
	  delete [] mwis;
	  for(i=0;i<numlink;i++){
		delete [] subneigh0[i];
		delete [] subneigh1[i];
		delete [] CGtemp[i];
		delete [] CGtemp1[i];
	  }
	  delete [] subneigh0;
	  delete [] subneigh1;
	  delete [] CGtemp;
	  delete [] CGtemp1;
	  return false;
	}
	for(j=0;j<numlink;j++){
	  CGtemp[i][j]=Conflict[i][j];
	}
  }

  for(i=0;i<numlink;i++){
	if(CGnodeW[i]<=0.0){ 
	  for(j=0;j<numlink;j++){
		CGtemp[i][j] = 0;
		CGtemp[j][i] = 0;
	  }
	  haveW[i] = 0;
	}
	else{ 
	  haveW[i] = 1;
	  CGWnum++;
	}
  }

  time_t start, end; 
  time(&start);
	
  numvref = CGWnum;
  for(i=0;i<numlink;i++){
	vref[i] = haveW[i];
	mwis[i] = 0;
	for(j=0;j<numlink;j++){
	  CGtemp1[i][j]=CGtemp[i][j];
	}
  }
  q=0;
  int trycount = 0; 
  double MaxClique =0;
  while(q==0){  //MWIS finding loop 
	////////////////////////////////////////////////////////////////////////////// -RENEW
	mwisv = random(numlink);
	while(vref[mwisv]!=1){
	  mwisv = (mwisv+1)%numlink;
	}
	////////////////////////////////////////////////////////////////////////////// - RENEW
	int update=0; 
	int **BFSArr = new int*[numlink];
	for(r=0;r<numlink;r++){
	  BFSArr[r] = new int[numlink];
	}
	
	BFS(CGtemp1,numlink,mwisv,BFSArr);
	for(r=0;r<numvref;r++){
	  if(BFSArr[r][0]==-1)
		break;
	}
	////////////////////////////////////////////////////////////////////////////// - RENEW
	for(j=0;j<numlink;j++){
	  delete[] BFSArr[j];
	}
	/*
	  for(r=0;r<numlink;r++){
	  delete[] BFSArr[r];
	  }
	*/
	////////////////////////////////////////////////////////////////////////////// - RENEW
	delete [] BFSArr;

	//		int maxhop = r-1;
	int maxhop = numlink-1;

	    
	for(r=0;r<=maxhop;r++){
	  for(j=0;j<numlink;j++){ 
		if(r==0){
		  vsub0[j] = 0;
		  submwis0[j] = 0;
		}
		vsub1[j] = 0;
		submwis1[j] = 0;
		for(k=0;k<numlink;k++){
		  if(r==0){
			subneigh0[j][k]=0;
		  }
		  subneigh1[j][k]=0;
		}
	  }

	  if(r==0){
		BFSsub(CGtemp1,numlink,mwisv,r,subneigh0,vsub0);
		wmwis0 = AMWIS(subneigh0,vsub0,CGnodeW,numlink,submwis0);		////////////////////////////////////////////////////////////////////
		if(MaxClique<wmwis0){
		  MaxClique=wmwis0;
		  if(MaxClique>1){
			time(&end);
			time_t result = end -start; 

			delete [] qlength;
			delete [] CGnodeW;
			delete [] vsub0;
			delete [] vsub1;
			delete [] submwis0;
			delete [] submwis1;
			delete [] haveW;
			delete [] vref;
			delete [] mwis;
			for(i=0;i<numlink;i++){
			  delete [] subneigh0[i];
			  delete [] subneigh1[i];
			  delete [] CGtemp[i];
			  delete [] CGtemp1[i];
			}
			delete [] subneigh0;
			delete [] subneigh1;
			delete [] CGtemp;
			delete [] CGtemp1;
			return false;
		  }
		}


	  }
	  BFSsub(CGtemp1,numlink,mwisv,r+1,subneigh1,vsub1);
	  int num =0;
	  for(int z=0;z<numlink;z++){
		num+=vsub1[z];
	  }
	
	  if(FORCECUT1 == 1){
		if(num>M*log(numlink)/log(2)){ 
		  update = 1;
		  break;
		}
	  }
	
	  wmwis1 = AMWIS(subneigh1,vsub1,CGnodeW,numlink,submwis1);


	  if(MaxClique<wmwis1){
		MaxClique=wmwis1;
		if(MaxClique>1){
		  time(&end);
		  time_t result = end -start; 
		  delete [] qlength;
		  delete [] CGnodeW;
		  delete [] vsub0;
		  delete [] vsub1;
		  delete [] submwis0;
		  delete [] submwis1;
		  delete [] haveW;
		  delete [] vref;
		  delete [] mwis;
		  for(i=0;i<numlink;i++){
			delete [] subneigh0[i];
			delete [] subneigh1[i];
			delete [] CGtemp[i];
			delete [] CGtemp1[i];
		  }
		  delete [] subneigh0;
		  delete [] subneigh1;
		  delete [] CGtemp;
		  delete [] CGtemp1;
		  return false;
		}
	  }



	  if(wmwis1 <= (1+EPS)*wmwis0){
		update = 1;
		break;		        
	  }
	  else{
		for(j=0;j<numlink;j++){
		  vsub0[j]=vsub1[j];
		  submwis0[j]=submwis1[j];
		  for(k=0;k<numlink;k++){
			subneigh0[j][k]=subneigh1[j][k];
		  }
		}
		wmwis0 = wmwis1;
	  }
	}
	if(update==1){
	  for(k=0;k<numlink;k++){
		if(submwis0[k]==1)
		  mwis[k]=1;	
		if(vsub1[k]==1){
		  vref[k]=0;
		  numvref--;
		  for(j=0;j<numlink;j++){
			CGtemp1[k][j] = 0;
			CGtemp1[j][k] = 0;
		  }
		}
	  }
	}
	if(numvref==0)
	  q=1;
  }
  ///////////////////////////////////////////////////////////////////////	 
    
  delete [] qlength;
  delete [] CGnodeW;
  delete [] vsub0;
  delete [] vsub1;
  delete [] submwis0;
  delete [] submwis1;
  delete [] haveW;
  delete [] vref;
  delete [] mwis;

  for(i=0;i<numlink;i++){
	delete [] subneigh0[i];
	delete [] subneigh1[i];
	delete [] CGtemp[i];
	delete [] CGtemp1[i];
  }
    
  delete [] subneigh0;
  delete [] subneigh1;
  delete [] CGtemp;
  delete [] CGtemp1;

  return true;


}

bool FeasCheck_MWIS_NEC(const PNet& FSN){
  int i,j,k,p,q,r,t;
  int numnode = FSN.NumNode; 
  int numlink = FSN.NumLink;
  double* qlength;

  int mwisv;         
  double wmwis0, wmwis1; 

  int CGWnum=0; 
  int numvref;
  double* CGnodeW;
    
  int **subneigh0, **subneigh1;
  int *vsub0, *vsub1; 
  int *submwis0, *submwis1;

  int** CGtemp; 
  int** CGtemp1; 
  int* haveW; 
  int* vref; 
    
  int* mwis;
  bool feas;

  for(i=0;i<numnode;i++)
	if(FSN.Nodes[i]->filledW > FSN.Nodes[i]->cpu)
	  return false;
  for(i=0;i<numlink;i++)
	if(FSN.Links[i]->filledW > FSN.Links[i]->capacity)
	  return false;

  feas = Necessary_PBG(FSN);
  if(!feas)
	return false;

  qlength = new double[numlink];
  CGnodeW = new double[numlink];
  haveW = new int[numlink];
  vsub0 = new int[numlink];
  vsub1 = new int[numlink];
  submwis0 = new int[numlink];
  submwis1 = new int[numlink];
    
  vref = new int[numlink];
  mwis = new int[numlink];
    
  subneigh0 = new int*[numlink];
  subneigh1 = new int*[numlink];
  CGtemp = new int*[numlink];
  CGtemp1 = new int*[numlink];

  for(i=0;i<numlink;i++){
	subneigh0[i] = new int[numlink];
	subneigh1[i] = new int[numlink];
	CGtemp[i] = new int[numlink];
	CGtemp1[i] = new int[numlink];
  }

  

  for(i=0;i<numlink;i++){
	qlength[i] = 0;
	CGnodeW[i] = ((double)FSN.Links[i]->filledW/FSN.Links[i]->capacity)/(1.0-2.0*EPS);
	for(j=0;j<numlink;j++){
	  CGtemp[i][j]=Conflict[i][j];
	}
  }

  for(i=0;i<numlink;i++){
	if(CGnodeW[i]<=0.0){ 
	  for(j=0;j<numlink;j++){
		CGtemp[i][j] = 0;
		CGtemp[j][i] = 0;
	  }
	  haveW[i] = 0;
	}
	else{ 
	  haveW[i] = 1;
	  CGWnum++;
	}
  }

  time_t start, end;
  time(&start);
	
  //Over LONGSLOT, do e-MWIS and check feasibility
  for(t=0;t<LONGSLOT;t++){
	for(i=0;i<numlink;i++){
	  if(qlength[i]>QUPPER){
		time(&end);
		time_t result = end -start; //runtime
		delete [] qlength;
		delete [] CGnodeW;
		delete [] vsub0;
		delete [] vsub1;
		delete [] submwis0;
		delete [] submwis1;
		delete [] haveW;
		delete [] vref;
		delete [] mwis;

		for(i=0;i<numlink;i++){
		  delete [] subneigh0[i];
		  delete [] subneigh1[i];
		  delete [] CGtemp[i];
		  delete [] CGtemp1[i];
		}
		delete [] subneigh0;
		delete [] subneigh1;
		delete [] CGtemp;
		delete [] CGtemp1;
		return false;
	  }
	  qlength[i] += CGnodeW[i];
	} 
	numvref = CGWnum;

	for(i=0;i<numlink;i++){
	  vref[i] = haveW[i];
	  mwis[i] = 0;
	  for(j=0;j<numlink;j++){
		CGtemp1[i][j]=CGtemp[i][j];
	  }
	}

	q=0;
	int trycount = 0;
	
	while(q==0){  //MWIS finding loop 
	  ////////////////////////////////////////////////////////////////////////////// -RENEW
	  mwisv = random(numlink);
	  while(vref[mwisv]!=1){
		mwisv = (mwisv+1)%numlink;
	  }
	  /*
		do{
		mwisv = random(numlink);
		}while(vref[mwisv]!=1);
	  */
	  ////////////////////////////////////////////////////////////////////////////// -RENEW
	  int update=0; 

	  int **BFSArr = new int*[numlink];
	  for(r=0;r<numlink;r++){
		BFSArr[r] = new int[numlink];
	  }
		
	  BFS(CGtemp1,numlink,mwisv,BFSArr);
	  for(r=0;r<numvref;r++){
		if(BFSArr[r][0]==-1)
		  break;
	  }
	  ////////////////////////////////////////////////////////////////////////////// -RENEW		    
	  for(j=0;j<numlink;j++){
		delete[] BFSArr[j];
	  }

	  /*
		for(r=0;r<numlink;r++){
		delete[] BFSArr[r];
		}
	  */
	  ////////////////////////////////////////////////////////////////////////////// -RENEW
	  delete [] BFSArr;
	
	  int maxhop = r-1;
		    

	  for(r=0;r<=maxhop;r++){
		for(j=0;j<numlink;j++){
		  if(r==0){
			vsub0[j] = 0;
			submwis0[j] = 0;
		  }
		  vsub1[j] = 0;
		  submwis1[j] = 0;
		  for(k=0;k<numlink;k++){
			if(r==0){
			  subneigh0[j][k]=0;
			}
			subneigh1[j][k]=0;
		  }
		}
		if(r==0){
		  BFSsub(CGtemp1,numlink,mwisv,r,subneigh0,vsub0);
		  wmwis0 = AMWIS(subneigh0,vsub0,qlength,numlink,submwis0);
		}
		BFSsub(CGtemp1,numlink,mwisv,r+1,subneigh1,vsub1);
		int num =0;
		for(int z=0;z<numlink;z++){
		  num+=vsub1[z];
		}
		
		if(FORCECUT == 1){
		  if(num>M*log(numlink)/log(2)){
			update = 1;
			break;
		  }
		}
		
		wmwis1 = AMWIS(subneigh1,vsub1,qlength,numlink,submwis1);
	
		if(wmwis1 <= (1+EPS)*wmwis0){
		  update = 1;
		  break;		        
		}
		else{
		  for(j=0;j<numlink;j++){
			vsub0[j]=vsub1[j];
			submwis0[j]=submwis1[j];
			for(k=0;k<numlink;k++){
			  subneigh0[j][k]=subneigh1[j][k];
			}
		  }
		  wmwis0 = wmwis1;
		}
	  }
	  if(update==1){
		for(k=0;k<numlink;k++){
		  if(submwis0[k]==1)
			mwis[k]=1;	
		  if(vsub1[k]==1){
			vref[k]=0;
			numvref--;
			for(j=0;j<numlink;j++){
			  CGtemp1[k][j] = 0;
			  CGtemp1[j][k] = 0;
			}
		  }
		}
	  }
	  if(numvref==0)
		q=1;
	}
	
	for(i=0;i<numlink;i++){
	  if(mwis[i]==1){
		qlength[i] = (qlength[i]-1>=0) ? qlength[i]-1 : 0;
	  }
	}    
  }
	 
  time(&end);
  time_t result = end -start; 
    
  feas = true;
  delete [] qlength;
  delete [] CGnodeW;
  delete [] vsub0;
  delete [] vsub1;
  delete [] submwis0;
  delete [] submwis1;
  delete [] haveW;
  delete [] vref;
  delete [] mwis;

  for(i=0;i<numlink;i++){
	delete [] subneigh0[i];
	delete [] subneigh1[i];
	delete [] CGtemp[i];
	delete [] CGtemp1[i];
  }
    
  delete [] subneigh0;
  delete [] subneigh1;
  delete [] CGtemp;
  delete [] CGtemp1;

  return feas;
}



bool FeasCheck_MWIS(const PNet& FSN){ 
  bool flag;
  double Lower;
  int i,j,k,p,q,r,t;
  int numnode = FSN.NumNode; 
  int numlink = FSN.NumLink; 
  double* qlength;

  int mwisv;        
  double wmwis0, wmwis1; 

  int CGWnum=0; 
  int numvref;
  double* CGnodeW; 
    
  int **subneigh0, **subneigh1; 
  int *vsub0, *vsub1; 
  int *submwis0, *submwis1; 

  int** CGtemp; 
  int** CGtemp1; 
  int* haveW; 
  int* vref; 
    
  int* mwis;

  double MaxQ=0;

  for(i=0;i<numnode;i++)
	if(FSN.Nodes[i]->filledW > FSN.Nodes[i]->cpu)
	  return false;
  for(i=0;i<numlink;i++)
	if(FSN.Links[i]->filledW > FSN.Links[i]->capacity)
	  return false;
  if(!Necessary_PBG(FSN))
	return false;

  qlength = new double[numlink];
  CGnodeW = new double[numlink];
  haveW = new int[numlink];
  vsub0 = new int[numlink];
  vsub1 = new int[numlink];
  submwis0 = new int[numlink];
  submwis1 = new int[numlink];
    
  vref = new int[numlink];
  mwis = new int[numlink];
    
  subneigh0 = new int*[numlink];
  subneigh1 = new int*[numlink];
  CGtemp = new int*[numlink];
  CGtemp1 = new int*[numlink];

  for(i=0;i<numlink;i++){
	subneigh0[i] = new int[numlink];
	subneigh1[i] = new int[numlink];
	CGtemp[i] = new int[numlink];
	CGtemp1[i] = new int[numlink];
  }


  for(i=0;i<numlink;i++){
	qlength[i] = 0;	
	CGnodeW[i] = ((double)FSN.Links[i]->filledW/FSN.Links[i]->capacity)/(1.0-2.0*EPS);
	if(CGnodeW[i]>1){
	  delete [] qlength;
	  delete [] CGnodeW;
	  delete [] vsub0;
	  delete [] vsub1;
	  delete [] submwis0;
	  delete [] submwis1;
	  delete [] haveW;
	  delete [] vref;
	  delete [] mwis;
	  for(i=0;i<numlink;i++){
		delete [] subneigh0[i];
		delete [] subneigh1[i];
		delete [] CGtemp[i];
		delete [] CGtemp1[i];
	  }
	  delete [] subneigh0;
	  delete [] subneigh1;
	  delete [] CGtemp;
	  delete [] CGtemp1;
	  return false;
	}
	for(j=0;j<numlink;j++){
	  CGtemp[i][j]=Conflict[i][j];
	}
  }
  for(i=0;i<numlink;i++){
	if(CGnodeW[i]<=0.0){ 
	  for(j=0;j<numlink;j++){
		CGtemp[i][j] = 0;
		CGtemp[j][i] = 0;
	  }
	  haveW[i] = 0;
	}
	else{ 
	  haveW[i] = 1;
	  CGWnum++;
	}
  }

  time_t start, end; 
  time(&start);
  MaxQ =0;

  for(t=0;t<LONGSLOT;t++){
	MaxQ =0;
	for(i=0;i<numlink;i++){
	  if(qlength[i]>MaxQ){
		MaxQ = qlength[i];
	  }
	}
	//		if(t!=0 && MaxQ<(double)t*EPS*EPS/numlink/numlink){			// more Accurate
	Lower = 20.0;
	if((double)t/INVSLOPE < Lower)
	  Lower = (double)t/INVSLOPE;
	if(t!=0 && MaxQ< Lower){			
	  time(&end);
	  time_t result = end -start; 
	  delete [] qlength;
	  delete [] CGnodeW;
	  delete [] vsub0;
	  delete [] vsub1;
	  delete [] submwis0;
	  delete [] submwis1;
	  delete [] haveW;
	  delete [] vref;
	  delete [] mwis;

	  for(i=0;i<numlink;i++){
		delete [] subneigh0[i];
		delete [] subneigh1[i];
		delete [] CGtemp[i];
		delete [] CGtemp1[i];
	  }
	  delete [] subneigh0;
	  delete [] subneigh1;
	  delete [] CGtemp;
	  delete [] CGtemp1;
	  return true;
	}
	if(MaxQ > QUPPER || MaxQ> (LONGSLOT - t)){
	  time(&end);
	  time_t result = end -start; 
	  delete [] qlength;
	  delete [] CGnodeW;
	  delete [] vsub0;
	  delete [] vsub1;
	  delete [] submwis0;
	  delete [] submwis1;
	  delete [] haveW;
	  delete [] vref;
	  delete [] mwis;
	  for(i=0;i<numlink;i++){
		delete [] subneigh0[i];
		delete [] subneigh1[i];
		delete [] CGtemp[i];
		delete [] CGtemp1[i];
	  }
	  delete [] subneigh0;
	  delete [] subneigh1;
	  delete [] CGtemp;
	  delete [] CGtemp1;
	  return false;
	}


	for(i=0;i<numlink;i++)
	  qlength[i] += CGnodeW[i];

	numvref = CGWnum;

	for(i=0;i<numlink;i++){
	  vref[i] = haveW[i];
	  mwis[i] = 0;
	  for(j=0;j<numlink;j++){
		CGtemp1[i][j]=CGtemp[i][j];
	  }
	}

	q=0;
	int trycount = 0; 
	
	while(q==0){  //MWIS finding loop 
	  ////////////////////////////////////////////////////////////////////////////// -RENEW
	  mwisv = random(numlink);
	  while(vref[mwisv]!=1){
		mwisv = (mwisv+1)%numlink;
	  }
	  /*
		do{
		mwisv = random(numlink);
		}while(vref[mwisv]!=1);
	  */
	  ////////////////////////////////////////////////////////////////////////////// -RENEW
	  int update=0;

	  int **BFSArr = new int*[numlink];
	  for(r=0;r<numlink;r++){
		BFSArr[r] = new int[numlink];
	  }
		
	  BFS(CGtemp1,numlink,mwisv,BFSArr);
	  for(r=0;r<numvref;r++){
		if(BFSArr[r][0]==-1)
		  break;
	  }
	  ////////////////////////////////////////////////////////////////////////////// -RENEW		    
	  for(j=0;j<numlink;j++){
		delete[] BFSArr[j];
	  }

	  /*
		for(r=0;r<numlink;r++){
		delete[] BFSArr[r];
		}
	  */
	  ////////////////////////////////////////////////////////////////////////////// -RENEW
	  delete [] BFSArr;
	
	  int maxhop = r-1;
		    
	  for(r=0;r<=maxhop;r++){
		for(j=0;j<numlink;j++){
		  if(r==0){
			vsub0[j] = 0;
			submwis0[j] = 0;
		  }
		  vsub1[j] = 0;
		  submwis1[j] = 0;
		  for(k=0;k<numlink;k++){
			if(r==0){
			  subneigh0[j][k]=0;
			}
			subneigh1[j][k]=0;
		  }
		}
		if(r==0){
		  BFSsub(CGtemp1,numlink,mwisv,r,subneigh0,vsub0);
		  wmwis0 = AMWIS(subneigh0,vsub0,qlength,numlink,submwis0);
		}
		BFSsub(CGtemp1,numlink,mwisv,r+1,subneigh1,vsub1);
		int num =0;
		for(int z=0;z<numlink;z++){
		  num+=vsub1[z];
		}
		
		if(FORCECUT == 1){
		  if(num>M*log(numlink)/log(2)){
			update = 1;
			break;
		  }
		}
		
		wmwis1 = AMWIS(subneigh1,vsub1,qlength,numlink,submwis1);
	
		if(wmwis1 <= (1+EPS)*wmwis0){
		  update = 1;
		  break;		        
		}
		else{
		  for(j=0;j<numlink;j++){
			vsub0[j]=vsub1[j];
			submwis0[j]=submwis1[j];
			for(k=0;k<numlink;k++){
			  subneigh0[j][k]=subneigh1[j][k];
			}
		  }
		  wmwis0 = wmwis1;
		}
	  }
	  if(update==1){
		for(k=0;k<numlink;k++){
		  if(submwis0[k]==1)
			mwis[k]=1;	
		  if(vsub1[k]==1){
			vref[k]=0;
			numvref--;
			for(j=0;j<numlink;j++){
			  CGtemp1[k][j] = 0;
			  CGtemp1[j][k] = 0;
			}
		  }
		}
	  }
	  if(numvref==0)
		q=1;
	}
	for(i=0;i<numlink;i++){
	  if(mwis[i]==1){
		qlength[i] = (qlength[i]>1) ? qlength[i]-1: 0;
	  }
	}    
  }
	 
  time(&end);
  time_t result = end -start; 
    
  delete [] qlength;
  delete [] CGnodeW;
  delete [] vsub0;
  delete [] vsub1;
  delete [] submwis0;
  delete [] submwis1;
  delete [] haveW;
  delete [] vref;
  delete [] mwis;

  for(i=0;i<numlink;i++){
	delete [] subneigh0[i];
	delete [] subneigh1[i];
	delete [] CGtemp[i];
	delete [] CGtemp1[i];
  }
    
  delete [] subneigh0;
  delete [] subneigh1;
  delete [] CGtemp;
  delete [] CGtemp1;

  return false;
}

void BFSsub(int** wcg, int numnode, int root, int r, int** subneigh, int* vsub){

  int i,j;
  int **BFSArr;

  BFSArr = new int*[numnode];
  for(i=0;i<numnode;i++)
	BFSArr[i] = new int[numnode];

  BFS(wcg, numnode, root, BFSArr);
  
  for(i=0;i<=r;i++){
	for(j=0;j<numnode;j++){
	  if(BFSArr[i][j]!=-1)
		vsub[BFSArr[i][j]] = 1;
	}
  }

  for(i=0;i<numnode;i++){
	for(j=0;j<numnode;j++){
	  if(vsub[i]==1&&vsub[j]==1){
		subneigh[i][j] = wcg[i][j];
		subneigh[j][i] = wcg[j][i];
	  }
	}
  }
  for(i=0;i<numnode;i++)
	delete [] BFSArr[i];

  delete [] BFSArr;
}

double AMWIS(int** subneigh, int* vsub, double* qlength, int numlink, int* mwis){

  int i,j,k;
  double w;
  int vnum=0;
  int* alive;
  int* rmmwis;
  int* rmid;
  double *rmqlength;
  int** rmgraph;

  MWIS mw;  

  for(i=0;i<numlink;i++){
	if(vsub[i]==1)
	  vnum++;
  }
  alive = new int[vnum];
  rmmwis = new int[vnum];
  rmid = new int[vnum];
  rmqlength = new double[vnum];
  rmgraph = new int*[vnum];
  for(i=0;i<vnum;i++){
	rmgraph[i] = new int[vnum];
  }
    
  for(i=0;i<vnum;i++){
	alive[i]=1;
	rmmwis[i]=0;
	rmid[i]=-1;
	rmqlength[i]=0;
  }

  for(i=0;i<numlink;i++)
	mwis[i]=0;
    
  k=0;
  for(i=0;i<numlink;i++){
	if(vsub[i]==1){
	  rmid[k]=i;
	  k++;
	}
  }

  for(i=0;i<vnum;i++)
	rmqlength[i]=qlength[rmid[i]];

  for(i=0;i<vnum;i++){
	for(j=0;j<vnum;j++){
	  rmgraph[i][j]=subneigh[rmid[i]][rmid[j]];
	}
  }
    
  MWISFIND(rmgraph,rmmwis,alive,rmqlength,mw,vnum);
   
  w = mw.weight;

  for(i=0;i<vnum;i++){
	if(mw.IS[i]){
	  mwis[rmid[i]]=1;
	}
  }

  delete [] alive;
  delete [] rmmwis;
  delete [] rmid;
  delete [] rmqlength;
  for(i=0;i<vnum;i++)
	delete [] rmgraph[i];
  delete [] rmgraph;

  return w;
}

void MWISFIND(int** grph, int* mwis, int* alive, double* qlength, MWIS& mw1, int vnum){
    
  int i,j,k;
  int s,t,u;
  double maxw;
  int maxid;
  int flag;
    
  int* mwistmp = new int[vnum];
  int* alivetmp = new int[vnum];


  for(i=0;i<vnum;i++){   
	if(alive[i]==1){  
	  for(j=0;j<vnum;j++){
		mwistmp[j] = mwis[j];
		alivetmp[j] = alive[j];
	  }
	  mwistmp[i]=1; 
	  alivetmp[i]=0;
	  for(j=0;j<vnum;j++){
		if(grph[i][j]>0) alivetmp[j]=0;
	  }
	  k=0;
	  for(j=0;j<vnum;j++){
		if(alivetmp[j]==1)
		  k=1;
	  }

	  int flag = 1;
	  if(k==1){ 
		flag = 0;
		MWISFIND(grph, mwistmp, alivetmp, qlength, mw1, vnum);
	  }
	  if(flag){
		double neww =0;
		for(int s=0;s<vnum;s++){
		  neww += mwistmp[s]*qlength[s];
		}
		if(neww>mw1.weight){
		  mw1.vnum=vnum;
		  mw1.weight=neww;
		  delete[] mw1.IS; 
		  mw1.IS = new int[vnum];
		  for(int s=0;s<vnum;s++){
			mw1.IS[s]=mwistmp[s];
		  }
		}
	  }
	}
  }
  delete [] mwistmp;
  delete [] alivetmp;
}


// Calculate Revenue
int VNRevenue(VNet* VN){    
  int i;
  int revenue=0;
  for(i=0;i<VN->NumNode;i++)
	revenue += VN->Nodes[i]->cpu;
  for(i=0;i<VN->NumLink;i++)
	revenue += ALPHA*(VN->Links[i]->capacity);
  return revenue ;
}
int VNLinkAccept(VNet* VN){
  return (VN->NumLink);
}
int VNNodeAccept(VNet* VN){
  return (VN->NumNode);
}
int VNCapAccept(VNet* VN){
  int i;
  int cap=0;
  for(i=0;i<VN->NumLink;i++)
	cap += (VN->Links[i]->capacity);
  return cap;
}
int VNCpuAccept(VNet* VN){
  int i;
  int cpu=0;
  for(i=0;i<VN->NumNode;i++)
	cpu += VN->Nodes[i]->cpu;
  return cpu;
}




bool compembed(int VID){

  int i,j,k,r;
  int* vorder;
  int* vordertemp;
  int* embarr;
  int* sorder;
  int vnode1, vnode2, snode1, snode2;
  int pathtemp[MAXPATH];
    
  vorder = new int[VNs[VID]->NumNode];
  vordertemp = new int[MAXVNODENUM];
  embarr = new int[PN->NumNode];
  sorder = new int[PN->NumNode];
  for(i=0;i<PN->NumNode;i++){
	embarr[i]=-1;
  }
  NodeMapOrder(VID,vordertemp);
  for(i=0;i<VNs[VID]->NumNode;i++){
	vorder[i]=vordertemp[i];
  }
  SNodeOrder(sorder);
  for(i=0;i<VNs[VID]->NumNode;i++){
	for(j=0;j<PN->NumNode;j++){
		bufPN->Nodes[sorder[j]]->filledW += VNs[VID]->Nodes[i]->cpu;
		for(k=0;k<MAXEMBEDNODE; k++){
		  if(bufPN->Nodes[sorder[j]]->embedVNID[k] == -1){
			bufPN->Nodes[sorder[j]]->embedVNID[k] = VID;
			bufPN->Nodes[sorder[j]]->embedVNode[k] = i;
			break;
		  }
		}
		VNs[VID]->Nodes[i]->embeddedPNID = sorder[j];
		embarr[sorder[j]]=i;
		break;
	  }
	}
	if(j==PN->NumNode){
	  delete [] vorder;
	  delete [] embarr;
	  delete [] sorder;
	  return false;
	}

    
  for(i=0;i<VNs[VID]->NumLink;i++){
	vnode1 = VNs[VID]->Links[i]->node1;
	vnode2 = VNs[VID]->Links[i]->node2;
	snode1 = VNs[VID]->Nodes[vnode1]->embeddedPNID;
	snode2 = VNs[VID]->Nodes[vnode2]->embeddedPNID;
	for(k=0;k<MAXPATH;k++){
	  pathtemp[k]=ShortestPath[snode1][snode2][k];
	}
	for(j=0;j< MAXPATH && pathtemp[j]>=0; j++){
	  bufPN->Links[pathtemp[j]]->filledW += VNs[VID]->Links[i]->capacity;
	  for(k=0;k<MAXEMBEDLINK; k++){
		if(bufPN->Links[pathtemp[j]]->embedVNID[k]==-1){
		  bufPN->Links[pathtemp[j]]->embedVNID[k] = VID;
		  bufPN->Links[pathtemp[j]]->embedVLink[k] = i;
		  break;
		}
	  }
	  VNs[VID]->Links[i]->embeddedPath[j] = pathtemp[j];
	}
  }
  delete [] vorder;
  delete [] embarr;
  delete [] sorder;

  return true;
}


// certain/mid/well coupled
bool compembed(int VID, int rootnode){
  int i,j,k,r;
  int *vorder;
  int *embarr;
  int *sorder;
  int vnode1, vnode2, snode1, snode2;
  int pathtemp[MAXPATH];
    
  vorder = new int[VNs[VID]->NumNode];
  embarr = new int[PN->NumNode];
  sorder = new int[VNs[VID]->NumNode];

  for(i=0;i<PN->NumNode;i++){
	embarr[i]=-1;
  }
  NodeMapOrder(VID,vorder);
  Findsnode(sorder,rootnode,VNs[VID]->NumNode);

  for(i=0;i<VNs[VID]->NumNode;i++){
	bufPN->Nodes[sorder[i]]->filledW += VNs[VID]->Nodes[i]->cpu;
	for(k=0;k<MAXEMBEDNODE; k++){
	  if(bufPN->Nodes[sorder[i]]->embedVNID[k] == -1){
		bufPN->Nodes[sorder[i]]->embedVNID[k] = VID;
		bufPN->Nodes[sorder[i]]->embedVNode[k] = i;
		break;
	  }
	}
	if (bufPN->Nodes[sorder[i]]->filledW > bufPN->Nodes[sorder[i]]->cpu){
	  delete [] vorder;
	  delete [] embarr;
	  delete [] sorder;
	  return false;
	}

	VNs[VID]->Nodes[i]->embeddedPNID = sorder[i];
	embarr[sorder[i]]=i;
  }


  for(i=0;i<VNs[VID]->NumLink;i++){

	vnode1 = VNs[VID]->Links[i]->node1;
	vnode2 = VNs[VID]->Links[i]->node2;
	snode1 = VNs[VID]->Nodes[vnode1]->embeddedPNID;
	snode2 = VNs[VID]->Nodes[vnode2]->embeddedPNID;
	
	for(k=0;k<MAXPATH;k++){
	  pathtemp[k]=ShortestPath[snode1][snode2][k];
	}
	for(j=0;j< MAXPATH && pathtemp[j]>=0; j++){
	  bufPN->Links[pathtemp[j]]->filledW += VNs[VID]->Links[i]->capacity;
	  for(k=0;k<MAXEMBEDLINK; k++){
		if(bufPN->Links[pathtemp[j]]->embedVNID[k]==-1){
		  bufPN->Links[pathtemp[j]]->embedVNID[k] = VID;
		  bufPN->Links[pathtemp[j]]->embedVLink[k] = i;
		  break;
		}
	  }
	  VNs[VID]->Links[i]->embeddedPath[j] = pathtemp[j];
	}
  }

  delete [] vorder;
  delete [] embarr;
  delete [] sorder;
  return true;
}

void Findsnode(int* found, int rootnode, int num){
  int i,j,k;
  double* totald = new double[PN->NumNode];
  double minimumd;
  int best;

  found[0]=rootnode;
  for(i=1;i<num;i++){
	minimumd = 9999;
	for(j=0;j<PN->NumNode;j++){
	  totald[j]=0;
	  for(k=0;k<i;k++){
		totald[j]+=distances[found[k]][j];
	  }
	}
	for(j=0;j<i;j++){
	  totald[found[j]]=10000;
	}
	
	for(j=0;j<PN->NumNode;j++){
	  if(totald[j]<minimumd){
		best = j;
		minimumd = totald[j];
	  }
	}

	found[i]=best;
  }
  delete [] totald;
}
