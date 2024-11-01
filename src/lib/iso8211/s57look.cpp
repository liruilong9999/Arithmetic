#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "s57_utils.h"
#include "s57_module.h"

using namespace std;

#define LOOK_FIELD		1
#define LOOK_SUBFIELD	2
#define LOOK_ALL		255

class DsLookScanner : public S57DatasetScanner
{
private:
	int _look;

	string _field;
	string _subField;

protected:
	void onDataset(string);

public:
	DsLookScanner();

	int lookWhat() const;
	void setLook(int);

	void setLookField(string f, string sub);

	void dumpContents(string fileName);
};

void DsLookScanner::onDataset(string fileName)
{
	dumpContents(fileName);
}

DsLookScanner::DsLookScanner()
	: _look(LOOK_ALL)
{
}

int DsLookScanner::lookWhat() const
{
	return _look;
}

void DsLookScanner::setLook(int v)
{
	_look = v;
}

void DsLookScanner::setLookField(string f, string sub)
{
	_field = f;
	_subField = sub;
}

void DsLookScanner::dumpContents(string fileName)
{
	printf("%s\n", fileName.c_str());

	S57Module mod(fileName);
	if (!mod.isOpen())
		return;

	while (!mod.atEnd()) {
		Ref<S57Record> r = mod.getNextRecord();
		if (r.isNull())
			break;
		if (_look == LOOK_ALL)
			printf("%s\n", r->toString().c_str());
	}
}

// ~

static void usage()
{
	printf("Dump the S57 contents.\n"
			"usage: s57look [-hR] [-s FIELD::SUBFIELD] FILE\n"
			"options:\n"
			"  -h\t Show this usage help.\n"
			"  -R\t Read all files under each directory, recursively.\n");
	/*"  -s\t Print the sub-field value.\n");*/
}

#if 0 //TODO

int main(int argc, char *argv[])
{
	DsLookScanner dslook;
	dslook.setRecursive(false);

	for (;;) {
		int c = getopt(argc, argv, "hR");
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			usage();
			return 0;
		case 'R':
			dslook.setRecursive(true);
			break;
		/*
		case 'f':
		lookwhat = LOOK_FIELD;
		f_arg.assign(optarg);
		break;
		case 's':
		lookwhat = LOOK_SUBFIELD;
		dslook.setLook(LOOK_SUBFIELD);
		s_arg.assign(optarg);
		break;
		*/
		default:
			usage();
			return -1;
		}
	}

	if (optind >= argc) {
		usage();
		return -1;
	}

	const char *path = argv[optind];
	struct stat stbuf;
	int ret = lstat(path, &stbuf);
	if (ret != 0) {
		fprintf(stderr, "%s: file or directory not exist\n", path);
		return -1;
	}

	if (S_ISDIR(stbuf.st_mode))
		dslook.scan(path);
	else
		dslook.dumpContents(path);

	return 0;
}
#endif // 0
