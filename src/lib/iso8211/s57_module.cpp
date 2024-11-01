#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <string>

#include "s57_module.h"

using namespace std;

// S57Module members

void S57Module::init()
{
	_fp = NULL;
}

S57Module::S57Module()
	: RefBase()
{
	init();
}

S57Module::S57Module(string fileName)
	: RefBase()
{
	init();
	open(fileName);
}

S57Module::~S57Module()
{
	close();
}

bool S57Module::open(string fileName)
{
	if (isOpen())
		close();

	_fileName = fileName;

	_fp = fopen(fileName.c_str(), "rb");
	if (_fp == NULL) {
		fprintf(stderr, "%s: %s\n", fileName.c_str(), strerror(errno));
		return false;
	}

	int flags = 0xf; // CATD|DSAC|DSPM|DSID
	S57RecordRef tmpr = getNextRecord();
	if (tmpr.isNull() || tmpr->recordType() != S57Record::DDR) {
		fprintf(stderr, "Seems not a S-57 file.\n");
		close();
		return false;
	}
	_ddr = reinterpret_cast<S57DataDescripRecord *>(tmpr.getPtr());

	LRHeader::DirIterator it = tmpr->header()->begin();
	for (; flags != 0 && it != tmpr->header()->end(); ++it) {
		if (strcmp(it->_fieldTag, "DSID") == 0)
			flags &= 0xe;
		else if (strcmp(it->_fieldTag, "DSPM") == 0)
			flags &= 0xd;
		else if (strcmp(it->_fieldTag, "DSAC") == 0)
			flags &= 0xb;
		else if (strcmp(it->_fieldTag, "CATD") == 0)
			flags &= 0x7;
	}

	while (flags != 0xf) {
		tmpr = getNextRecord();
		if (tmpr.isNull())
			break;
		if (tmpr->recordType() == S57Record::DatasetInformation) {
			_dr_dsInfo = reinterpret_cast<S57DSInfoRecord *>(tmpr.getPtr());
			flags |= 1;
		}
		else if (tmpr->recordType() == S57Record::DatasetGeographic) {
			_dr_dsGeo = reinterpret_cast<S57DSGeoRecord *>(tmpr.getPtr());
			flags |= 2;
		}
		else if (tmpr->recordType() == S57Record::DatasetAccuracy) {
			_dr_dsAcc = reinterpret_cast<S57DSAccuracyRecord *>(tmpr.getPtr());
			flags |= 4;
		}
		else if (tmpr->recordType() == S57Record::CatalogDirectory) {
			_dr_catDir = reinterpret_cast<S57CatalogDirRecord *>(tmpr.getPtr());
			flags |= 8;
		}
	}

	rewind(_fp);
	return true;
}

void S57Module::close()
{
	if (_fp != NULL)
		fclose(_fp);

	_fp = NULL;
	_fileName.clear();
}

bool S57Module::atEnd() const
{
	int c = fgetc(_fp);
	bool res = (c == EOF);
	ungetc(c, _fp);
	return res;
}

int S57Module::aall() const
{
	if (!_dr_dsInfo.isNull() && _dr_dsInfo->fieldDSSI() != NULL)
		return _dr_dsInfo->fieldDSSI()->_aall;
	else
		return S57_LL1;
}

int S57Module::nall() const
{
	if (!_dr_dsInfo.isNull() && _dr_dsInfo->fieldDSSI() != NULL)
		return _dr_dsInfo->fieldDSSI()->_nall;
	else
		return S57_LL1;
}

S57RecordRef S57Module::getNextRecord()
{
	S57RecordRef r;
	int ret;

	// read record length
	int posSave = ftell(_fp);
	char numbuf[8];
	if ((ret = fread(numbuf, 1, 5, _fp)) != 5) {
		if (ferror(_fp))
			fprintf(stderr, "File I/O error: %s\n", strerror(errno));
		return NULL;
	}
	numbuf[5] = 0;

	// read the whole record
	const int reclen = atoi(numbuf);
	char *recbuf = new char [reclen];
	if (recbuf == NULL)
		return NULL;

	fseek(_fp, posSave, SEEK_SET);
	if ((ret = fread(recbuf, 1, reclen, _fp)) != reclen) {
		if (ferror(_fp))
			fprintf(stderr, "File I/O error: %s\n", strerror(errno));
	}
	else
		r = S57Record::decode(string(recbuf, reclen), this);

	delete[] recbuf;
	return r;
}

