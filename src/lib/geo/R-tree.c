#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "../tools/debug_alloc.h"
#include "R-tree.h"

//
// Useful defines
//

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

typedef double REALTYPE;
#define REALTYPE_MAX +1E+37
#define REALTYPE_MIN -1E+37

typedef long COORTYPE;
#define COORTYPE_MAX +2147483647L
#define COORTYPE_MIN -2147483648L

//
// RT_Rect definition
//

typedef struct RT_Rect
{
	COORTYPE x1, y1;
	COORTYPE x2, y2;
} RT_Rect;

RT_Rect rect_unite(RT_Rect *r1, RT_Rect *r2)
{
	RT_Rect ret;
	ret.x1 = MIN(r1->x1, r2->x1);
	ret.x2 = MAX(r1->x2, r2->x2);
	ret.y1 = MIN(r1->y1, r2->y1);
	ret.y2 = MAX(r1->y2, r2->y2);
	return ret;
}

#define rect_intersects(r1, r2) \
	((MAX((r1)->x1, (r2)->x1) <= MIN((r1)->x2, (r2)->x2)) \
	 && (MAX((r1)->y1, (r2)->y1) <= MIN((r1)->y2, (r2)->y2)))

#define rect_contains(r1, r2) \
	((r1)->x1 <= (r2)->x1 && (r1)->x2 >= (r2)->x2 \
	 && (r1)->y1 <= (r2)->y1 && (r1)->y2 >= (r2)->y2) 

#define rect_area(r) \
	((REALTYPE)((r)->x2 - (r)->x1 + 1) * (REALTYPE)((r)->y2 - (r)->y1 + 1))

#define rect_valid(r) \
	((r)->x1 <= (r)->x2 && (r)->y1 <= (r)->y2)

//
// R-tree implementation
//

#define RT_MAXNODES 25
#define RT_MINNODES 13

typedef struct RT_Branch
{
	RT_Rect mbr;
	struct RT_Node *child;
} RT_Branch;

typedef struct RT_Node
{
	int count;
	int level;
	RT_Branch branches[RT_MAXNODES];
} RT_Node;

typedef struct RT_Partition
{
	int which_group[RT_MAXNODES + 1];
	BOOL item_taken[RT_MAXNODES + 1];
	int item_count;
	int item_min;

	RT_Rect grp_mbr[2];
	REALTYPE grp_area[2];
	int grp_items[2];
} RT_Partition;

typedef struct RT_Split
{
	RT_Branch branch_buf[RT_MAXNODES + 1];
	int branch_count; // always be RT_MAXNODES+1
	RT_Rect tmp_mbr;
	REALTYPE tmp_area;
	RT_Partition partition;
} RT_Split;

// ~

static void __init_node(RT_Node *n)
{
	n->count = 0;
	n->level = -1;
	memset(n->branches, 0, sizeof(n->branches));
}

static RT_Node *__new_node()
{
	RT_Node *n = (RT_Node *)MALLOC(sizeof(RT_Node));
	if (n == NULL) {
		fprintf(stderr, "***[RTree] Lack of memory.\n");
		exit(1);
	}
	__init_node(n);
	return n;
}

static void __free_node(RT_Node *n)
{
	int i;

	if (n->level > 0) {
		for (i = 0; i < RT_MAXNODES; ++i)
			if (n->branches[i].child != NULL)
				__free_node(n->branches[i].child);
	}

	FREE(n);
}

static RT_Rect __node_mbr(RT_Node *n)
{
	assert(n->count > 0);

	RT_Rect ret = n->branches[0].mbr;
	int i = 1;

	while (i < n->count) {
		assert(n->branches[i].child != NULL);
		ret = rect_unite(&ret, &n->branches[i].mbr);
		++i;
	}

#ifndef NDEBUG
	for (; i < RT_MAXNODES; ++i)
		assert(n->branches[i].child == NULL);
#endif

	return ret;
}


static void __split_node(RTree *, RT_Node *, RT_Branch *, RT_Node **);

/**
 * Add a branch to a node.  Split the node if necessary.
 * Returns FALSE if node not split.  Old node updated.
 * Returns TRUE if node split, sets *new_node to address of new node.
 * Old node updated, becomes one of two.
 */
static BOOL __add_branch(RTree *tree, RT_Branch *b, RT_Node *n, RT_Node **new_node)
{
	if (n->count < RT_MAXNODES) {
		assert(n->branches[n->count].child == NULL);
		n->branches[n->count++] = *b;
		return FALSE;
	}
	else {
		__split_node(tree, n, b, new_node);
		return TRUE;
	}
}

static void __save_branches(RTree *tree, RT_Node *n, RT_Branch *b)
{
	int i;

#ifndef NDEBUG
	for (i = 0; i < RT_MAXNODES; ++i) 
		assert(n->branches[i].child != NULL); /* node should have every entry full */
#endif
	memcpy(tree->split->branch_buf, n->branches, sizeof(n->branches));
	tree->split->branch_buf[RT_MAXNODES] = *b;
	tree->split->branch_count = RT_MAXNODES + 1;

	RT_Rect tmpr = tree->split->branch_buf[0].mbr;
	for (i = 1; i < RT_MAXNODES + 1; ++i)
		tmpr = rect_unite(&tmpr, &(tree->split->branch_buf[i].mbr));

	tree->split->tmp_mbr = tmpr;
	tree->split->tmp_area = rect_area(&tmpr);
}

static void __load_branches(RTree *tree, RT_Node *n0, RT_Node *n1, RT_Partition *p)
{
	int i;

	for (i = 0; i < p->item_count; ++i) {
		assert(p->which_group[i] == 0 || p->which_group[i] == 1);
		if (p->which_group[i] == 0)
			__add_branch(tree, &(tree->split->branch_buf[i]), n0, NULL);
		else
			__add_branch(tree, &(tree->split->branch_buf[i]), n1, NULL);
	}
}

static void __group_node(RTree *tree, int i, int grp, RT_Partition *p)
{
	assert(!p->item_taken[i]);

	p->which_group[i] = grp;
	p->item_taken[i] = TRUE;

	if (p->grp_items[grp] == 0)
		p->grp_mbr[grp] = tree->split->branch_buf[i].mbr;
	else
		p->grp_mbr[grp] = rect_unite(p->grp_mbr + grp, &(tree->split->branch_buf[i].mbr));

	p->grp_area[grp] = rect_area(p->grp_mbr + grp);
	++p->grp_items[grp];
}

static void __select_seeds(RTree *tree, RT_Partition *p)
{
	int i, j;
	int seed0 = 0, seed1 = 0;
	REALTYPE worst, waste, area[RT_MAXNODES + 1];

	for (i = 0; i < p->item_count; ++i)
		area[i] = rect_area(&(tree->split->branch_buf[i].mbr));

	worst = -tree->split->tmp_area - 1;

	for (i = 0; i < p->item_count - 1; ++i)
		for (j = i + 1; j < p->item_count; ++j) {
			RT_Rect r = rect_unite(&(tree->split->branch_buf[i].mbr), &(tree->split->branch_buf[j].mbr));
			waste = rect_area(&r) - area[i] - area[j];
			if (waste > worst) {
				worst = waste;
				seed0 = i;
				seed1 = j;
			}
		}

	__group_node(tree, seed0, 0, p);
	__group_node(tree, seed1, 1, p);
}

static void __grouping_method0(RTree *tree, RT_Partition *p, int minitems)
{
	int i;
	int group;

	/* initialize partition */
	memset(p, 0, sizeof(RT_Partition));
	p->item_count = tree->split->branch_count;
	p->item_min = minitems;
	for (i = 0; i < RT_MAXNODES + 1; ++i) {
		p->which_group[i] = -1;
		p->item_taken[i] = FALSE;
	}

	__select_seeds(tree, p);

	while (p->grp_items[0] + p->grp_items[1] < p->item_count 
			&& p->grp_items[0] < p->item_count - p->item_min 
			&& p->grp_items[1] < p->item_count - p->item_min) {
		int chosen = 0;
		int better_grp = 0;
		REALTYPE biggest_diff = (REALTYPE)-1;

		for (i = 0; i < p->item_count; ++i) {
			if (p->item_taken[i])
				continue;

			RT_Rect *pr, rect0, rect1;
			REALTYPE diff;

			pr = &(tree->split->branch_buf[i].mbr);
			rect0 = rect_unite(pr, &(p->grp_mbr[0]));
			rect1 = rect_unite(pr, &(p->grp_mbr[1]));
			diff = (rect_area(&rect0) - p->grp_area[0]) 
				- (rect_area(&rect1) - p->grp_area[1]);

			if (diff < 0) {
				group = 0;
				diff = -diff;
			}
			else
				group = 1;

			if (diff > biggest_diff) {
				biggest_diff = diff;
				chosen = i;
				better_grp = group;
			}
			else if (diff == biggest_diff 
					&& p->grp_items[group] < p->grp_items[better_grp]) {
				chosen = i;
				better_grp = group;
			}
		}
		__group_node(tree, chosen, better_grp, p);
	}

	/* if one group too full, put remaining rects in the other */
	if (p->grp_items[0] + p->grp_items[1] < p->item_count) {
		group = p->grp_items[0] >= p->item_count - p->item_min ? 1 : 0;
		for (i = 0; i < p->item_count; ++i)
			if (!p->item_taken[i])
				__group_node(tree, i, group, p);
	}

	assert(p->grp_items[0] + p->grp_items[1] == p->item_count);
	assert(p->grp_items[0] >= p->item_min && p->grp_items[1] >= p->item_min);
}

static void __split_node(RTree *tree, RT_Node *n, RT_Branch *b, RT_Node **new_node)
{
	RT_Partition *p;
	int level_save;

	assert(new_node != NULL);

	if (tree->split == NULL) {
		tree->split = (RT_Split *)MALLOC(sizeof(RT_Split));
		if (tree->split == NULL) {
			fprintf(stderr, "***[RTree] Lack of memory\n");
			exit(1);
		}
		memset(tree->split, 0, sizeof(RT_Split));
	}

	/* save all branches into a buffer, initialize old node */
	level_save = n->level;
	__save_branches(tree, n, b);
	__init_node(n);

	p = &(tree->split->partition);
	__grouping_method0(tree, p, RT_MINNODES);

	/* load branches from buffer into 2 nodes according to chosen partition */
	*new_node = __new_node();
	(*new_node)->level = n->level = level_save;
	__load_branches(tree, n, *new_node, p);

	assert(n->count + (*new_node)->count == p->item_count);
}

static int __pick_branch(RT_Rect *mbr, RT_Node *n)
{
	RT_Rect *r, tmpr;
	int i, best = 0;
	BOOL first_time = TRUE;
	REALTYPE incr, best_incr = (REALTYPE)-1;
	REALTYPE area, best_area = (REALTYPE)0;

	for (i = 0; i < n->count; ++i) {
		assert(n->branches[i].child != NULL);
		r = &(n->branches[i].mbr);
		area = rect_area(r);
		tmpr = rect_unite(mbr, r);
		incr = rect_area(&tmpr) - area;
		if (incr < best_incr || first_time) {
			best = i;
			best_area = area;
			best_incr = incr;
			first_time = FALSE;
		}
		else if (incr == best_incr && area < best_area) {
			best = i;
			best_area = area;
			best_incr = incr;
		}
	}

#ifndef NDEBUG
	for (; i < RT_MAXNODES; ++i)
		assert(n->branches[i].child == NULL);
#endif

	return best;
}

static BOOL __insert_rect(RTree *tree, RT_Rect *mbr, void *data, 
		RT_Node *n, RT_Node **new_node, int level)
{
	int i;
	RT_Branch b;
	RT_Node *new_node2;

	assert(level >= 0 && level <= n->level);

	/* still above level for insertion, go down tree recursively */
	if (n->level > level) {
		i = __pick_branch(mbr, n);

		if (!__insert_rect(tree, mbr, data, n->branches[i].child, &new_node2, level)) {
			n->branches[i].mbr = rect_unite(&(n->branches[i].mbr), mbr);
			return FALSE;
		}
		n->branches[i].mbr = __node_mbr(n->branches[i].child);
		b.child = new_node2;
		b.mbr = __node_mbr(new_node2);

		return __add_branch(tree, &b, n, new_node);
	}
	/* have reached level for insertion, add mbr, split if necessary */
	else if (n->level == level) {
		b.mbr = *mbr;
		b.child = (RT_Node *)data;

		return __add_branch(tree, &b, n, new_node);
	}

	/* cann't reach here */
	assert(0); 
	return FALSE;
}

static int __search_rect(RT_Node *n, RT_Rect *mbr, 
		RT_SearchFilter filter, void *arg)
{
	if (NULL == n)
	{
		return 0;
	}

	int nhits = 0;
	int i;

	/* this is an internal node in the tree */
	if (n->level > 0) {
		for (i = 0; i < n->count; ++i) {
			assert(n->branches[i].child != NULL);
			if (rect_intersects(mbr, &(n->branches[i].mbr)))
				nhits += __search_rect(n->branches[i].child, mbr, filter, arg);
		}
	}
	/* this is a leaf node */
	else {
		for (i = 0; i < n->count; ++i) {
			assert(n->branches[i].child != NULL);
			if (rect_intersects(mbr, &(n->branches[i].mbr))) {
				++nhits;
				if (filter((void *)(n->branches[i].child), arg) == 0)
					return nhits;
			}
		}
	}

	return nhits;
}

/*
 * ~
 */

RTree *rtreeCreate()
{
	RTree *tree = (RTree *)MALLOC(sizeof(RTree));
	if (tree == NULL) {
		fprintf(stderr, "***[RTree] Lack of memory\n");
		exit(1);
	}

	tree->root = __new_node();
	tree->root->level = 0; /* a leaf */
	tree->split = NULL;
	tree->isdummy = FALSE;
	tree->filter = NULL;
	tree->cb_arg = NULL;

	return tree;
}

void rtreeDestroy(RTree *tree)
{
	if (!tree->isdummy)
		__free_node(tree->root);

	if (tree->split != NULL)
		FREE(tree->split);

	FREE(tree);
}

void rtreeInsert(RTree *tree, int minX, int minY, int maxX, int maxY, void *data)
{
	RT_Node *new_node = NULL;

	RT_Rect mbr;
	mbr.x1 = minX;
	mbr.y1 = minY;
	mbr.x2 = maxX;
	mbr.y2 = maxY;

	assert(!tree->isdummy);

	if (__insert_rect(tree, &mbr, data, tree->root, &new_node, 0)) {
		RT_Node *new_root;
		RT_Branch b;

		new_root = __new_node();
		new_root->level = tree->root->level + 1;

		b.mbr = __node_mbr(tree->root);
		b.child = tree->root;
		__add_branch(tree, &b, new_root, NULL);

		b.mbr = __node_mbr(new_node);
		b.child = new_node;
		__add_branch(tree, &b, new_root, NULL);

		tree->root = new_root;
	}
}

void rtreeSetFilter(RTree *tree, RT_SearchFilter filter, void *arg)
{
	tree->filter = filter;
	tree->cb_arg = arg;
}

int rtreeSearch(RTree *tree, int minX, int minY, int maxX, int maxY)
{
	assert(tree->filter != NULL);

	RT_Rect mbr;
	mbr.x1 = minX;
	mbr.y1 = minY;
	mbr.x2 = maxX;
	mbr.y2 = maxY;
	return __search_rect(tree->root, &mbr, tree->filter, tree->cb_arg);
}

/*
 * R-tree serialization
 */

void __write_node(RT_Node *n, FILE *fp)
{
	int i;

	assert(n != NULL && fp != NULL);

	if (fwrite(n, sizeof(RT_Node), 1, fp) != 1) {
		fprintf(stderr, "***[RTree] I/O error\n");
		exit(1);
	}

	if (n->level != 0)
		for (i = 0; i < n->count; ++i)
			__write_node(n->branches[i].child, fp);
}

void __link_node(RT_Node **np, RT_Node **next_node)
{
	int i;

	assert(np != NULL && next_node != NULL);

	*np = *next_node;
	++*next_node;

	if ((*np)->level != 0)
		for (i = 0; i < (*np)->count; ++i)
			__link_node(&((*np)->branches[i].child), next_node);
}

void rtreeSave(RTree *tree, FILE *fp)
{
	__write_node(tree->root, fp);
}

RTree *rtreeRejoint(void *treeImage)
{
	RT_Node *n_it = (RT_Node *)treeImage;

	RTree *tree = (RTree *)MALLOC(sizeof(RTree));
	if (tree == NULL)
		return NULL;

	tree->root = NULL;
	tree->split = NULL;
	tree->isdummy = TRUE;
	tree->filter = NULL;
	tree->cb_arg = NULL;

	__link_node(&(tree->root), &n_it);

	//printf("R-tree rejoint: linked %d nodes.\n", (n_it - (RT_Node *)treeImage));

	return tree;
}

/*
 * Testing
 */

#include <sys/stat.h>

RT_Rect rects[] = {
	{ 31,   8,  45,  30 },
	{ 22,  23,  55,  35 },
	{ 80,   8,  97,  36 },
	{ 89,  13, 116,  23 },
	{  8,  59,  25, 100 },
	{  8, 111,  24, 132 },
	{ 39,  77,  56,  87 },
	{ 80,  78,  91, 105 },
	{ 80, 114,  99, 128 },
	{ 98,  82, 115, 110 },
	{ 30, 115, 136, 134 }
};

int nrects = sizeof(rects) / sizeof(RT_Rect);

BOOL myFilter(void *data, void *arg)
{
	arg = arg;
	printf("%d\n", (int)data);
	return TRUE;
}

#if 0
int main()
{
	int i, nhits;

	/* Creates a R-tree */
	RTree *tree = rtreeCreate();

	/* Insertion */
	for (i = 0; i < nrects; ++i) {
		RT_Rect *r = rects + i;
		rtreeInsert(tree, r->x1, r->y1, r->x2, r->y2, (void *)(i + 1));
	}

	/* Search */
	rtreeSetFilter(tree, myFilter, NULL);
	nhits = rtreeSearch(tree, 2, 57, 60, 133);
	printf("Searching done. %d hits\n", nhits);

	/* Saving */
	FILE *fp = fopen("R-tree_image", "wb");
	rtreeSave(tree, fp);
	fclose(fp);

	rtreeDestroy(tree);
	tree = NULL;

	PRINT_ALLOC_STATS();

	/* Loading */
	struct stat st;
	char *buf;
	fp = fopen("R-tree_image", "rb");
	fstat(fileno(fp), &st);
	buf = (char *)malloc(st.st_size);
	if (fread(buf, st.st_size, 1, fp) != 1)
		return 1;
	tree = rtreeRejoint(buf);

	/* Re-search */
	rtreeSetFilter(tree, myFilter, NULL);
	nhits = rtreeSearch(tree, 2, 57, 60, 133);
	printf("Searching done. %d hits\n", nhits);

	rtreeDestroy(tree);

	free(buf);

	PRINT_ALLOC_STATS();

	return 0;
}
#endif
