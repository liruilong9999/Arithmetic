#ifndef RTREE_H
#define RTREE_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int BOOL;
#ifdef TRUE
	#undef TRUE
	#undef FALSE
#endif
#define TRUE 1
#define FALSE 0

/*
 * If passed to a tree search, this callback function will be called
 * with the user data of each data mbr that overlaps the search mbr
 * plus whatever user specific pointer was passed to the search.
 * It can terminate the search early by returning 0 in which case
 * the search will return the number of hits found up to that point.
 */
typedef int (*RT_SearchFilter)(void *data, void *arg);

struct RT_Node;
struct RT_Split;

typedef struct RTree
{
	struct RT_Node *root;
	struct RT_Split *split;
	BOOL isdummy;

	RT_SearchFilter filter;
	void *cb_arg;
} RTree;

RTree *rtreeCreate();

void rtreeDestroy(RTree *);

// Inserts a rectangle to the tree. 
// NOTE: The data cann't be empty, i.e. cann't be NULL or 0.
void rtreeInsert(RTree *, int minX, int minY, int maxX, int maxY, void *data);

void rtreeSetFilter(RTree *, RT_SearchFilter filter, void *arg);

int rtreeSearch(RTree *, int minX, int minY, int maxX, int maxY);

void rtreeSave(RTree *, FILE *fp);
RTree *rtreeRejoint(void *treeImage);

#ifdef __cplusplus
}
#endif

#endif
