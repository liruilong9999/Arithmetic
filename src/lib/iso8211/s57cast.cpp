#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#include "s57castscanner.h"
#include "../tools/debug_alloc.h"

using namespace std;

class DebugAllocPrinter
{
public:
	DebugAllocPrinter() {}
	~DebugAllocPrinter() { PRINT_ALLOC_STATS(); }
};
static DebugAllocPrinter g_debugAllocPrinter;

static void usage()
{
	printf("Convert the S57 dataset to related-image file.\n"
			"usage: s57cast [-hcRBal] [-projdatum] [SOURCE] [-P] [DEST]\n"
			"options:\n"
			"  -h\t Show this usage help.\n"
			"  -c\t Check data set before casting.\n"
			"  -R\t Convert all files under each directory, recursively.\n"
			"  -B\t Handle base cells only.\n"
			"  -a\t Append new datasets to DEST lib.\n"
			"  -l\t List module entries.\n"
			"  -projdatum\t Set projection datum for Mercator.\n"
			"      1: Krassovsky (BeiJing 54)\n"
			"      2: IAG75 (XiAn 80)\n"
			"      3: WGS84, by default\n"
			"      4: Web Mercator\n"
			"  -P\t Create destination dir if it not exist.\n");
}

#if 0 //TODO


int main(int argc, char *argv[])
{
	struct stat stbuf;
	bool appendMode = false;
	bool createDest = false;

	S57CastScanner scanner;
	scanner.setRecursive(false);
	scanner.setPrecheck(false);
	scanner.setUpdating(true);

	for (;;) {
		int c = getopt(argc, argv, "hcRBalP1234");
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			usage();
			return 0;
		case 'R':
			scanner.setRecursive(true);
			break;
		case 'B':
			scanner.setUpdating(false);
			break;
		case 'a':
			appendMode = true;
			break;
		case 'c':
			scanner.setPrecheck(true);
			break;
		case 'l':
			if (optind >= argc)
				usage();
			else
				S57CastScanner::listModuleEntries(argv[optind]);
			return 0;
		case '1':
			scanner.setProjDatumType(Geo::Mercator::Krassovsky);
			break;
		case '2':
			scanner.setProjDatumType(Geo::Mercator::IAG75);
			break;
		case '3':
			scanner.setProjDatumType(Geo::Mercator::WGS84);
			break;
		case '4':
			scanner.setProjDatumType(Geo::Mercator::WebMercator);
			break;
		case 'P':
			createDest = true;
			break;
		default:
			usage();
			return -1;
		}
	}

	string source, dest;
	if (optind < argc)
		source = argv[optind++];
	if (optind < argc)
		dest = argv[optind++];

	if (optind < argc || source.empty()) {
		usage();
		return -1;
	}

	if (dest.empty())
		dest = source;

	// Checks the dest
	if (access(dest.c_str(), F_OK) != 0) {
		if (createDest) {
			if (mkdir(dest.c_str(), 0777) != 0) {
				printf("%s: %s\n", dest.c_str(), strerror(errno));
				return -1;
			}
		}
		else {
			printf("%s: directory not exist\n", dest.c_str());
			return -1;
		}
	}
	else {
		int ret = lstat(dest.c_str(), &stbuf);
		if (ret != 0 || !S_ISDIR(stbuf.st_mode)) {
			printf("%s: directory not exist or is a file\n", dest.c_str());
			return -1;
		}
	}

	// Checks if the source is dir or file
	int ret = lstat(source.c_str(), &stbuf);
	if (ret != 0) {
		printf("%s: file or directory not exist\n", source.c_str());
		return -1;
	}

	scanner.setOutputPath(dest);

	// Checks the output dir
	string s = dest;
	if (!s.empty() && s[s.length() - 1] != '/')
		s.push_back('/');
	s.append("index");
	if (access(s.c_str(), F_OK) == 0) {
		if (appendMode)
			scanner.setIndexFile(dest);
		else {
			int reply = 0;
			printf("index file is already exist, ignore it? (Y/n) ");
			do {
				reply = getchar();
			} while (reply == '\n');
			if (reply != 'Y')
				return 0;
		}
	}


	if (S_ISDIR(stbuf.st_mode))
		scanner.scan(source);
	else if (S_ISREG(stbuf.st_mode))
		scanner.castDataset(source);

	scanner.saveIndexFile();
	scanner.clear();
	return 0;
}
#endif // 0
