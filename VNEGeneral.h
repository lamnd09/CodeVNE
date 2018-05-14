#define MAXPATH 30		// Maximum (hop) length of a path
#define MAXWINQ 50		// Initial Window queue size
#define EXPANDWINQ 0	// Noe expand window queue (the oldest input will be deleted when queue is full and there is arrival)
//#define EXPANDWINQ 1	// Expand window queue

#define MAXEBEDVN 2000	// Maximum number of virtual network requests which can be accepted and embedded
#define MAXEMBEDNODE 50	// Maximum number of virtual nodes which can be embedded on a substrate node.
#define MAXEMBEDLINK 50	// Maximum number of virtual links which can be embedded on a substrate link.

//// Interference
#define CONFMODE 2		// 1 : 1hop, 2 : 2hop, 3 : distance based conflict
//#define CONFLINKD 35 // conflict distance when CONFMODE = 3

//// Norm (SUM vs. MAX)
#define NORMTYPE 1			// 1: 1-norm -SUM
//#define NORMTYPE 2		// 2: 2-norm
//#define NORMTYPE -1		// -1: infinity norm -MAX

//// Resource Weight Ratio
#define ALPHA 10				// parameter in revenue

////////////////////////////////
// Run Time Parameter (highly related with feasibility check)
////////////////////////////////
#define EPS 0.15		// epsilon
#define LONGSLOT 2000	// # of (1-e)-MWIS Test slots
//#define FORCECUT 0	// Not cut by force
#define FORCECUT 1		// Cut by force
//#define FORCECUT1 0	// Not cut by force - in Necessary
#define FORCECUT1 1		// Cut by force - in Necessary

// Polynomial growth bound function [P*log(# nodes in CG*r^Q)]
#define P 3
#define Q 3
#define F 7				// Try F times for checking PBG condition
#define M 3				// When find MWIS, if # of subneighborhood nodes exeeds M1*log(# nodes) then find  another mwisv

#define INVSLOPE 50
#define QUPPER 40		// Necessary

#define WINNUM 500		// # of time window
#define MAXDELAY 0		// How much embedding failure can be accepted? (upper bound of retry)

