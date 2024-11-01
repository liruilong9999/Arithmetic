#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <assert.h>

#include "assure_fio.h"
#include "s57_utils.h"
#include "s57extract.h"

using namespace std;

// S57Extract members

void S57Extract::onRecDsGeo(S57DSGeoRecord *r)
{
	const S57_DSPM *dspm = r->fieldDSPM();
	if (dspm != NULL)
		_comf = dspm->_comf;
}

void S57Extract::onPrepareParse(const DsItem &)
{
	_comf = 0.0;
}

void S57Extract::onParse(const DsItem &ds)
{
	char sbuf[8];
	sprintf_s(sbuf, "%d", _targetObjl);

	string fn = outputPath();
	fn.append(ds.family());
	fn.push_back('_');
	fn.append(sbuf);
	FILE *miffp = as_fopen((fn + ".MIF").c_str(), "wb");
	FILE *midfp = as_fopen((fn + ".MID").c_str(), "wb");

	writeMifHeader(miffp);

	const vector<S57FeatureRecordRef> *frList;
	if (_targetObjl < 300)
		frList = &grList();
	else if (_targetObjl < 400)
		frList = &mrList();
	else
		frList = &lrList();

	vector<S57FeatureRecordRef>::const_iterator fit = frList->begin();
	for (; fit != frList->end(); ++fit) {
		const S57FeatureRecord *theFr = fit->getPtr();
		if (theFr->isDeleted())
			continue;

		const S57_FRID *frid = theFr->fieldFRID();
		if (frid->_objl != _targetObjl)
			continue;

		if (frid->_objl == 129)
			writeSounding(theFr, miffp, midfp);
		else if (frid->_prim == PRIM_P)
			writePointObject(theFr, miffp, midfp);
		else if (frid->_prim == PRIM_L)
			writeLineObject(theFr, miffp, midfp);
		else if (frid->_prim == PRIM_A)
			writeAreaObject(theFr, miffp, midfp);
	}

	fclose(miffp);
	fclose(midfp);
}

void S57Extract::writeMifHeader(FILE *fp)
{
	fprintf(fp, "VERSION 300\r\n");
	fprintf(fp, "CHARSET \"WindowsLatin1\"\r\n");
	fprintf(fp, "COLUMNS 1\r\n");
	fprintf(fp, "\tFeatureID integer\r\n");
	fprintf(fp, "\r\n");
	fprintf(fp, "DATA\r\n");
}

void S57Extract::writeSounding(const S57FeatureRecord *fr, FILE *miffp, FILE *midfp)
{
}

void S57Extract::writePointObject(const S57FeatureRecord *fr, FILE *miffp, FILE *midfp)
{
	vector<S57_FSPT>::const_iterator fsit = fr->fieldsFSPT().begin();
	for (; fsit != fr->fieldsFSPT().end(); ++fsit) {
		S57VectorRecordRef toVr = findVectorTarget(fsit->_name);
		if (toVr.isNull() || toVr->isDeleted()) {
			fprintf(stderr, "Feature [%s]: invalid FSPT to %s\n", 
					fr->fieldFRID()->_name.toString().c_str(), 
					fsit->_name.toString().c_str());
			continue;
		}

		assert(toVr->coordType() == S57VectorRecord::SG2D);
		if (toVr->coords().size() != 2) {
			fprintf(stderr, "Vector [%s]: invalid SG2D field\n", toVr->fieldVRID()->_name.toString().c_str());
			continue;
		}

		fprintf(miffp, "POINT %.7lf %.7lf\r\n", toVr->coords()[1] / _comf, toVr->coords()[0] / _comf);
		fprintf(midfp, "%lu\r\n", fr->fieldFRID()->_name._rcid);
	}
}

void S57Extract::writeLineObject(const S57FeatureRecord *fr, FILE *miffp, FILE *midfp)
{
	if (fr->fieldsFSPT().size() == 0)
		return;

	fprintf(miffp, "PLINE");
	if (fr->fieldsFSPT().size() > 1)
		fprintf(miffp, " MULTIPLE %zd", fr->fieldsFSPT().size());
	fprintf(miffp, "\r\n");

	vector<S57_FSPT>::const_iterator fsit = fr->fieldsFSPT().begin();
	for (; fsit != fr->fieldsFSPT().end(); ++fsit) {
		S57VectorRecordRef toVr = findVectorTarget(fsit->_name);
		if (toVr.isNull() || toVr->isDeleted()) {
			fprintf(stderr, "Feature [%s]: invalid FSPT to %s\n", 
					fr->fieldFRID()->_name.toString().c_str(), 
					fsit->_name.toString().c_str());
			continue;
		}

		const vector<S57_VRPT> &vrpts = toVr->fieldsVRPT();
		if (vrpts.size() != 2) {
			fprintf(stderr, "Vector [%s]: invalid VRPT field\n", toVr->fieldVRID()->_name.toString().c_str());
			continue;
		}
		assert(vrpts[0]._topi == TOPI_B);
		assert(vrpts[1]._topi == TOPI_E);
		S57VectorRecordRef beginVr = findVectorTarget(vrpts[0]._name);
		S57VectorRecordRef endVr = findVectorTarget(vrpts[1]._name);
		if (beginVr.isNull() || endVr.isNull()) {
			fprintf(stderr, "Vector [%s]: invalid VRPT field\n", toVr->fieldVRID()->_name.toString().c_str());
			continue;
		}

		assert(beginVr->coordType() == S57VectorRecord::SG2D);
		if (beginVr->coords().empty() || beginVr->coords().size() % 2 != 0) {
			fprintf(stderr, "Vector [%s]: invalid SG2D field\n", beginVr->fieldVRID()->_name.toString().c_str());
			continue;
		}

		assert(endVr->coordType() == S57VectorRecord::SG2D);
		if (endVr->coords().empty() || endVr->coords().size() % 2 != 0) {
			fprintf(stderr, "Vector [%s]: invalid SG2D field\n", endVr->fieldVRID()->_name.toString().c_str());
			continue;
		}

		assert(toVr->coordType() == S57VectorRecord::SG2D);

		fprintf(miffp, "%zd\r\n", toVr->coords().size() / 2 + 2);
		fprintf(miffp, "%.7lf %.7lf\r\n", beginVr->coords()[1] / _comf, beginVr->coords()[0] / _comf);
		vector<s57_b24>::const_iterator it = toVr->coords().begin();
		for (; it != toVr->coords().end(); it += 2)
			fprintf(miffp, "%.7lf %.7lf\r\n", *(it + 1) / _comf, *it / _comf);
		fprintf(miffp, "%.7lf %.7lf\r\n", endVr->coords()[1] / _comf, endVr->coords()[0] / _comf);
	}

	fprintf(midfp, "%lu\r\n", fr->fieldFRID()->_name._rcid);
}

void S57Extract::writeAreaObject(const S57FeatureRecord *fr, FILE *miffp, FILE *midfp)
{
	string sbuf;
	vector<s57_b24> v;
	int nPolygons = 0;

	vector<S57_FSPT>::const_iterator fsit = fr->fieldsFSPT().begin();
	for (; fsit != fr->fieldsFSPT().end(); ++fsit) {
		S57VectorRecordRef toVr = findVectorTarget(fsit->_name);
		if (toVr.isNull() || toVr->isDeleted()) {
			fprintf(stderr, "Feature [%s]: invalid FSPT to %s\n", 
					fr->fieldFRID()->_name.toString().c_str(), 
					fsit->_name.toString().c_str());
			continue;
		}

		const vector<S57_VRPT> &vrpts = toVr->fieldsVRPT();
		if (vrpts.size() != 2) {
			fprintf(stderr, "Vector [%s]: invalid VRPT field\n", toVr->fieldVRID()->_name.toString().c_str());
			continue;
		}
		assert(vrpts[0]._topi == TOPI_B);
		assert(vrpts[1]._topi == TOPI_E);
		S57VectorRecordRef beginVr = findVectorTarget(vrpts[0]._name);
		S57VectorRecordRef endVr = findVectorTarget(vrpts[1]._name);
		if (beginVr.isNull() || endVr.isNull()) {
			fprintf(stderr, "Vector [%s]: invalid VRPT field\n", toVr->fieldVRID()->_name.toString().c_str());
			continue;
		}

		assert(beginVr->coordType() == S57VectorRecord::SG2D);
		if (beginVr->coords().empty() || beginVr->coords().size() % 2 != 0) {
			fprintf(stderr, "Vector [%s]: invalid SG2D field\n", beginVr->fieldVRID()->_name.toString().c_str());
			continue;
		}

		assert(endVr->coordType() == S57VectorRecord::SG2D);
		if (endVr->coords().empty() || endVr->coords().size() % 2 != 0) {
			fprintf(stderr, "Vector [%s]: invalid SG2D field\n", endVr->fieldVRID()->_name.toString().c_str());
			continue;
		}

		assert(toVr->coordType() == S57VectorRecord::SG2D);

		if (fsit->_ornt == ORNT_R) {
			v.push_back(endVr->coords()[1]);
			v.push_back(endVr->coords()[0]);
			vector<s57_b24>::const_reverse_iterator rit = toVr->coords().rbegin();
			while (rit != toVr->coords().rend()) {
				v.push_back(*rit++);
				v.push_back(*rit++);
			}
			v.push_back(beginVr->coords()[1]);
			v.push_back(beginVr->coords()[0]);
		}
		else {
			v.push_back(beginVr->coords()[1]);
			v.push_back(beginVr->coords()[0]);
			vector<s57_b24>::const_iterator it = toVr->coords().begin();
			for (; it != toVr->coords().end(); it += 2) {
				v.push_back(*(it + 1));
				v.push_back(*it);
			}
			v.push_back(endVr->coords()[1]);
			v.push_back(endVr->coords()[0]);
		}

		if (v[0] == v[v.size() - 2] && v[1] == v[v.size() - 1]) {
			char tmp[64];
			sprintf_s(tmp, "%zd\r\n", v.size() / 2);
			sbuf.append(tmp);
			for (int i = 0; i < static_cast<int>(v.size()); i += 2) {
				sprintf_s(tmp, "%.7lf %.7lf\r\n", v[i] / _comf, v[i + 1] / _comf);
				sbuf.append(tmp);
			}

			v.clear();
			++nPolygons;
		}
	}

	if (nPolygons != 0) {
		fprintf(miffp, "REGION %d\r\n", nPolygons);
		fprintf(miffp, "%s", sbuf.c_str());
		fprintf(midfp, "%lu\r\n", fr->fieldFRID()->_name._rcid);
	}
}

void S57Extract::init()
{
	_targetObjl = 0;
	_comf = 0.0;
}

S57Extract::S57Extract()
	: S57ParseScanner()
{
	init();
}

S57Extract::S57Extract(string targetDs, string outputPath)
	: S57ParseScanner()
{
	init();
	setTargetDataset(targetDs);
	setOutputPath(outputPath);
}

S57Extract::~S57Extract()
{
}

void S57Extract::setOutputPath(string path)
{ 
	_outputPath = getAbsolutePath(path);
}

void S57Extract::extract(int objl)
{
	_targetObjl = objl;

	parseOneDataset(_targetDs);
}

// ~

static void usage()
{
	printf("Extract coordinates from S57 dataset.\n"
			"usage: s57extr [-hB] SOURCE OBJL [-P] DEST\n"
			"options:\n"
			"  -h\t Show this usage help.\n"
			"  -B\t Handle base cells only.\n"
			"  -P\t Create destination dir if it not exist.\n");
}

#if 0 //TODO



int main(int argc, char *argv[])
{
	struct stat stbuf;
	bool createDest = false;
	int objl = 0;

	S57Extract extr;
	extr.setPrecheck(false);
	extr.setUpdating(true);

	for (;;) {
		int c = getopt(argc, argv, "hBP");
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			usage();
			return 0;
		case 'B':
			extr.setUpdating(false);
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
		objl = atoi(argv[optind++]);
	if (optind < argc)
		dest = argv[optind++];

	if (optind < argc || source.empty() || objl == 0) {
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

	extr.setTargetDataset(source);
	extr.setOutputPath(dest);
	extr.extract(objl);
	return 0;
}
#endif //
