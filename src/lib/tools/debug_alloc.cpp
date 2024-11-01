#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
#include <cstring>
#include <assert.h>

#include "debug_alloc.h"

#define FROM_MALLOC	1
#define FROM_STRDUP	2
#define FROM_NEW	3
#define FROM_NEW_ARR	4
#define FROM_NEW_OBJARR	5

#define BE_REALLOCATED	0x10

struct SrcModule;

struct MemEntry
{
	int _flags;
	void *_addr;
	size_t _size;
	int _lno;
	MemEntry *_next;
	SrcModule *_parent;

	int from() const;
	bool isReallocated() const;
	const char *toString(bool format) const;
};

struct SrcModule
{
	char *_srcName;
	MemEntry *_child;
	int _childCount;
	SrcModule *_sibling;

	void addEntry(int f, void *p, size_t sz, int lno);
	MemEntry *findEntry(void *p);
	MemEntry *popEntry(void *p);

	const char *srcName() const;
	int allocCount() const;
};

inline int MemEntry::from() const
{
	return _flags & 0xf;
}

inline bool MemEntry::isReallocated() const
{
	return _flags & 0xf0;
}

const char *MemEntry::toString(bool alignment) const
{
	static char buf[256];
	static const char *fromstr[] = { "MALLOC", "STRDUP", "NEW", "NEW_ARR", "NEW_OBJARR" };
	const char *fmt = alignment ? "%-20s%6d %-10p%8u %s%s" : "%s@%d %p %u %s%s";
	sprintf_s(buf, fmt, 
			_parent->srcName(), _lno, _addr, _size, 
			isReallocated() ? "*" : "", 
			fromstr[from() - 1]);
	return buf;
}

void SrcModule::addEntry(int from, void *p, size_t sz, int lno)
{
	MemEntry *ent = reinterpret_cast<MemEntry *>(malloc(sizeof(MemEntry)));
	if (ent == NULL) {
		perror("Lack of memory");
		exit(1);
	}
	ent->_flags = from;
	ent->_addr = p;
	ent->_size = sz;
	ent->_lno = lno;
	ent->_next = _child;
	ent->_parent = this;
	_child = ent;
	++_childCount;
}

MemEntry *SrcModule::findEntry(void *p)
{
	MemEntry *pent = _child;
	while (pent != NULL && pent->_addr != p)
		pent = pent->_next;
	return pent;
}

MemEntry *SrcModule::popEntry(void *p)
{
	MemEntry *pent = _child;
	MemEntry **pprev = &_child;
	while (pent != NULL && pent->_addr != p) {
		pprev = &(pent->_next);
		pent = pent->_next;
	}

	if (pent != NULL) {
		*pprev = pent->_next;
		--_childCount;
	}

	return pent;
}

inline const char *SrcModule::srcName() const
{
	static const char *EmptyName = "";
	return _srcName != NULL ? _srcName : EmptyName;
}

inline int SrcModule::allocCount() const
{
	return _childCount;
}

// ~

static SrcModule *modq = NULL;

SrcModule *setModule(const char *src)
{
	SrcModule *pmod = modq;
	while (pmod != NULL && strcmp(pmod->srcName(), src) != 0)
		pmod = pmod->_sibling;

	if (pmod == NULL) {
		pmod = static_cast<SrcModule *>(malloc(sizeof(SrcModule)));
		if (pmod == NULL) {
			perror("Lack of memory");
			exit(1);
		}
		pmod->_srcName = strdup(src);
		pmod->_child = NULL;
		pmod->_childCount = 0;
		pmod->_sibling = modq;
		modq = pmod;
	}

	return pmod;
}

MemEntry *findPtr(void *ptr)
{
	MemEntry *res = NULL;
	SrcModule *pmod = modq;
	while (pmod != NULL && (res = pmod->findEntry(ptr)) == NULL)
		pmod = pmod->_sibling;
	return res;
}

// ~

void *dbg_calloc(size_t nmemb, size_t size, const char *src, int lno)
{
	void *ptr = calloc(nmemb, size);
	SrcModule *m = setModule(src);
	m->addEntry(FROM_MALLOC, ptr, size, lno);
	return ptr;
}

void *dbg_malloc(size_t size, const char *src, int lno)
{
	void *ptr = malloc(size);
	SrcModule *m = setModule(src);
	m->addEntry(FROM_MALLOC, ptr, size, lno);
	return ptr;
}

void *dbg_realloc(void *ptr, size_t size, const char *src, int lno)
{
	if (ptr == NULL)
		return dbg_malloc(size, src, lno);

	MemEntry *ent = findPtr(ptr);
	if (ent == NULL) {
		fprintf(stderr, "Unmatched realloc: %s@%d %p\n", src, lno, ptr);
		exit(1);
	}
	if (ent->from() != FROM_MALLOC) {
		fprintf(stderr, "Misused realloc: %s@%d [%s]\n", src, lno, ent->toString(false));
		exit(1);
	}

	if (size == 0) {
		ent = ent->_parent->popEntry(ptr);
		assert(ent != NULL);
		free(ent);
		free(ptr);
		return NULL;
	}
	else {
		ent->_addr = realloc(ptr, size);
		ent->_size = size;
		ent->_flags |= BE_REALLOCATED;
		return ent->_addr;
	}
}

char *dbg_strdup(const char *s, const char *src, int lno)
{
	char *ptr = strdup(s);
	SrcModule *m = setModule(src);
	m->addEntry(FROM_STRDUP, ptr, strlen(s) + 1, lno);
	return ptr;
}

void dbg_free(void *ptr, const char *src, int lno)
{
	MemEntry *ent = findPtr(ptr);
	if (ent == NULL) {
		fprintf(stderr, "Unmatched free: %s@%d %p\n", src, lno, ptr);
		exit(1);
	}
	if (ent->from() != FROM_MALLOC && ent->from() != FROM_STRDUP) {
		fprintf(stderr, "Misused free: %s@%d [%s]\n", src, lno, ent->toString(false));
		exit(1);
	}
	ent = ent->_parent->popEntry(ptr);
	assert(ent != NULL);
	free(ent);

	// do the real free
	free(ptr);
}

#ifdef __cplusplus
void *operator new(size_t size, const char *src, int lno, bool)
{
	void *ptr = malloc(size);
	SrcModule *m = setModule(src);
	m->addEntry(FROM_NEW, ptr, size, lno);
	return ptr;
}

void *operator new[](size_t size, const char *src, int lno, bool oarr)
{
	void *res = malloc(size);
	void *ptr = oarr ? reinterpret_cast<void *>(reinterpret_cast<long long>(res) + 4) : res;
	SrcModule *mod = setModule(src);
	mod->addEntry(oarr ? FROM_NEW_OBJARR : FROM_NEW_ARR, ptr, size, lno);
	return res;
}

void operator delete(void *ptr)
{
	// do the real delete
	free(ptr);
}

void operator delete[](void *ptr)
{
	// do the real delete
	free(ptr);
}

void removeEntry(void *ptr, const char *src, int lno, int flag)
{
	MemEntry *ent = findPtr(ptr);
	if (ent == NULL) {
		fprintf(stderr, "Unmatched delete: %s@%d %p\n", src, lno, ptr);
		exit(1);
	}

	bool misused = false;
	switch (flag) {
		case 0:
			misused = ent->from() != FROM_NEW;
			break;
		case 1:
			misused = ent->from() != FROM_NEW_ARR;
			break;
		case 2:
			misused = ent->from() != FROM_NEW_OBJARR;
			break;
		default:
			break;
	}
	if (misused) {
		fprintf(stderr, "Misused delete: %s@%d [%s]\n", src, lno, ent->toString(false));
		exit(1);
	}

	ent = ent->_parent->popEntry(ptr);
	assert(ent != NULL);
	free(ent);
}

#endif /* __cplusplus */

void assureNoLeaks(const char *src)
{
	SrcModule *mod = setModule(src);
	if (mod->allocCount() > 0) {
		fprintf(stderr, "Memory leak: %d alloc(s) unrelease.\n", mod->allocCount());
		exit(1);
	}
}

void printAllocStats()
{
	printf("-----------------------------------------------------\n");
	printf("%-20s%6s %-10s%8s %s\n", "Source", "Line", "Ptr", "Size", "From");
	SrcModule *pmod = modq;
	for (; pmod != NULL; pmod = pmod->_sibling) {
		MemEntry *pent = pmod->_child;
		for (; pent != NULL; pent = pent->_next)
			printf("%s\n", pent->toString(true));
		if (pmod->allocCount() > 0)
			printf("  Total: %d\n", pmod->allocCount());
	}
	printf("-----------------------------------------------------\n");
}
