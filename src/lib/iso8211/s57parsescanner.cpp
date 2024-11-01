#include <stdio.h>
#include <assert.h>

#include "assure_fio.h"
#include "s57_module.h"
#include "s57parsescanner.h"

using namespace std;
using namespace Geo;

// DsItem members

DsItem::DsItem(string pathName)
{
	_isAlive = true;

	string::size_type pos = pathName.rfind('/');
	if (pos != string::npos) {
		_path = pathName.substr(0, pos + 1);
		_family = pathName.substr(pos + 1);
	}
	else
		_family = pathName;
}

string DsItem::path() const
{ return _path; }

string DsItem::family() const
{ return _family; }

string DsItem::dsFile() const
{ return _path + _family + ".000"; }

bool DsItem::insertUpdateCell(string fileName)
{
	int num;

	// Converts the extension to update number
	string::size_type pos = fileName.rfind('.');
	assert(pos != string::npos);
	string s = fileName.substr(0, pos);
	if (s != (_path + _family))
		return false;
	num = strToInt(fileName.substr(pos + 1, 3));
	assert(num > 0 && num <= 999);

	// Inserts the number
	int i = 0;
	while (i < static_cast<int>(_upNums.size()) && num > _upNums[i])
		++i;
	assert(i == static_cast<int>(_upNums.size()) || num < _upNums[i]);
	_upNums.resize(_upNums.size() + 1);
	for (int j = _upNums.size() - 1; j > i; --j)
		_upNums[j] = _upNums[j - 1];
	_upNums[i] = num;

	return true;
}

int DsItem::updateCount() const
{ return _upNums.size(); }

DsItem::UpdateIte DsItem::updateBegin()
{ return DsItem::UpdateIte(this, _upNums.begin()); }

DsItem::UpdateIte DsItem::updateEnd()
{ return DsItem::UpdateIte(this, _upNums.end()); }

DsItem::UpdateIte::UpdateIte(DsItem *parent, vector<int>::iterator it)
	: _parent(parent), _it(it) {}

DsItem::UpdateIte &DsItem::UpdateIte::operator++()
{
	++_it;
	return *this;
}

string DsItem::UpdateIte::operator*() const
{
	char ext[8];
	sprintf_s(ext, ".%03d", *_it);
	return _parent->_path + _parent->_family + ext;
}

bool DsItem::UpdateIte::operator==(const UpdateIte &i) const
{ return (_parent == i._parent) && (_it == i._it); }

bool DsItem::UpdateIte::operator!=(const UpdateIte &i) const
{ return !operator==(i); }

int DsItem::UpdateIte::number() const
{ return *_it; }

//~

// S57ParseScanner members

void S57ParseScanner::init()
{
	_updatingEnabled = false;
	_precheckEnabled = false;
	_ignoreBaseCell = false;
	_projDatumType = Mercator::WGS84;
}

#define MAX_CHECK_STEPS 3
bool S57ParseScanner::checkDsValid(string fileName)
{
	bool ok = true;
	int nsteps = 0;
	S57Module mod;

	printf("%s\t", fileName.c_str());
	fflush(stdout);

	if (!mod.open(fileName))
		return false;

	while (!mod.atEnd() && nsteps < MAX_CHECK_STEPS) {
		Ref<S57Record> r = mod.getNextRecord();
		assert(!r.isNull());
		if (r->recordType() == S57Record::DatasetInformation) {
			++nsteps;
			const S57DSInfoRecord *inf = reinterpret_cast<const S57DSInfoRecord *>(r.getPtr());
			const S57_DSID *dsid = inf->fieldDSID();
			if (dsid == NULL) {
				printf("\n  No field DSID");
				ok = false;
				continue; // go next record
			}
			/*
			if (dsid->_sted != "03.1") { // some dataset yes
				printf("\n  DSID::STED = %s", dsid->_sted.c_str());
				ok = false;
			}
			*/
			if (dsid->_prsp != 1) {
				printf("\n  DSID::PRSP = %d", dsid->_prsp);
				ok = false;
			}

			const S57_DSSI *dssi = inf->fieldDSSI();
			if (dssi == NULL) {
				printf("\n  No field DSSI");
				ok = false;
				continue; // go next record
			}
			if (dssi->_dstr != 2) {
				printf("\n  DSSI::DSTR = %d", dssi->_dstr);
				ok = false;
			}
		}
		else if (r->recordType() == S57Record::DatasetGeographic) {
			++nsteps;
			const S57DSGeoRecord *geo = reinterpret_cast<const S57DSGeoRecord *>(r.getPtr());
			const S57_DSPM *dspm = geo->fieldDSPM();
			if (dspm == NULL) {
				printf("\n  No field DSPM");
				ok = false;
				continue; // go next record
			}
			if (dspm->_hdat != 2) {
				printf("\n  DSPM::HDAT = %d", dspm->_hdat);
				ok = false;
			}
			if (dspm->_coun != 1) {
				printf("\n  DSPM::COUN = %d", dspm->_coun);
				ok = false;
			}
		}
		else if (r->recordType() == S57Record::CatalogDirectory) {
			++nsteps;
			const S57CatalogDirRecord *cat = reinterpret_cast<const S57CatalogDirRecord *>(r.getPtr());
			const S57_CATD *catd = cat->fieldCATD();
			if (catd == NULL) {
				printf("\n  No field CATD");
				ok = false;
				continue; // go next record
			}
			if (catd->_impl != "BIN") {
				printf("\n  CATD::IMPL = \"%s\"", catd->_impl.c_str());
				ok = false;
			}
		}
	}

	if (ok)
		printf("[\033[32mOK\033\\[0m] ");

	putchar('\n');

	return ok;
}

void S57ParseScanner::upcellDispatch()
{
	list<string>::iterator it = _upCells.begin();
	while (it != _upCells.end()) {
		vector<DsItem>::iterator jt = _dsList.begin();
		bool found = false;
		for (; jt != _dsList.end() && !found; ++jt)
			found = jt->insertUpdateCell(*it);

		if (found)
			it = _upCells.erase(it);
		else
			++it;
	}

	if (!_upCells.empty()) {
		fprintf(stderr, "Isolated update cells:\n");
		it = _upCells.begin();
		for (; it != _upCells.end(); ++it)
			fprintf(stderr, "  %s\n", it->c_str());
	}
}

void S57ParseScanner::updateDataset(DsItem &ds)
{
	printf("Merging update:");

	// for each update cells
	S57Module upCell;
	DsItem::UpdateIte it = ds.updateBegin();
	for (; it != ds.updateEnd(); ++it) {
		printf(" %d", it.number());
		if (!upCell.open(*it))
			continue;
		// for each update records
		while (!upCell.atEnd()) {
			S57RecordRef r = upCell.getNextRecord();
			assert(!r.isNull());
			if (r->recordType() == S57Record::DatasetInformation) {
				S57DSInfoRecord *inf = reinterpret_cast<S57DSInfoRecord *>(r.getPtr());
				const S57_DSID *up_dsid = inf->fieldDSID();
				if (up_dsid == NULL) {
					putchar('?');
					break; // go next update cell
				}
				if (up_dsid->_edtn == 0) {
					putchar('X');
					ds._isAlive = false;
					return;
				}
				if (up_dsid->_edtn != _dsinfRec->fieldDSID()->_edtn) {
					putchar('!');
					break; // go next update cell
				}
			}
			else if (r->recordType() == S57Record::Feature) {
				S57FeatureRecord *fr = reinterpret_cast<S57FeatureRecord *>(r.getPtr());
				const S57_FRID *up_frid = fr->fieldFRID();
				if (up_frid == NULL)
					continue; // go next update record
				// Insert
				if (up_frid->_ruin == S57_UI_I) {
					if (findFeatureTarget(up_frid->_name, up_frid->_objl).isNull())
						onRecFeature(fr);
					else
						fprintf(stderr, "target [%s] already exist\n", up_frid->_name.toString().c_str());
				}
				// Delete
				else if (up_frid->_ruin == S57_UI_D) {
					S57FeatureRecordRef target = findFeatureTarget(up_frid->_name, up_frid->_objl);
					if (!target.isNull())
						target->markDeleted();
					else
						fprintf(stderr, "target [%s] not found\n", up_frid->_name.toString().c_str());
				}
				// Modify
				else if (up_frid->_ruin == S57_UI_M) {
					S57FeatureRecordRef target = findFeatureTarget(up_frid->_name, up_frid->_objl);
					if (!target.isNull())
						target->update(fr);
					else
						fprintf(stderr, "target [%s] not found\n", up_frid->_name.toString().c_str());
				}
			}
			else if (r->recordType() == S57Record::Vector) {
				S57VectorRecord *vr = reinterpret_cast<S57VectorRecord *>(r.getPtr());
				const S57_VRID *up_vrid = vr->fieldVRID();
				if (up_vrid == NULL)
					continue; // go next update record
				// Insert
				if (up_vrid->_ruin == S57_UI_I) {
					if (findVectorTarget(up_vrid->_name).isNull())
						onRecSpatial(vr);
					else
						fprintf(stderr, "target [%s] already exist\n", up_vrid->_name.toString().c_str());
				}
				// Delete
				else if (up_vrid->_ruin == S57_UI_D) {
					S57VectorRecordRef target = findVectorTarget(up_vrid->_name);
					if (!target.isNull())
						target->markDeleted();
					else
						fprintf(stderr, "target [%s] not found\n", up_vrid->_name.toString().c_str());
				}
				// Modify
				else if (up_vrid->_ruin == S57_UI_M) {
					S57VectorRecordRef target = findVectorTarget(up_vrid->_name);
					if (!target.isNull())
						target->update(vr);
					else
						fprintf(stderr, "target [%s] not found\n", up_vrid->_name.toString().c_str());
				}
			}
		}
		fflush(stdout);
	}

	if (ds.updateCount() > 0)
		putchar('\n');
}

void S57ParseScanner::doParse(DsItem &ds)
{
	onPrepareParse(ds);

	_dsinfRec.release();
	_dsgeoRec.release();
	_dsaccRec.release();
	_grList.clear();
	_mrList.clear();
	_lrList.clear();
	_vrList.clear();

	printf("Parsing %s\n", ds.dsFile().c_str());

	S57Module mod(ds.dsFile());
	if (!mod.isOpen())
		return;

	while (!mod.atEnd()) {
		Ref<S57Record> r = mod.getNextRecord();
		assert(!r.isNull());
		switch (r->recordType()) {
		case S57Record::DatasetInformation:
			_dsinfRec = reinterpret_cast<S57DSInfoRecord *>(r.getPtr());
			onRecDsInfo(reinterpret_cast<S57DSInfoRecord *>(r.getPtr()));
			break;
		case S57Record::DatasetGeographic:
			_dsgeoRec = reinterpret_cast<S57DSGeoRecord *>(r.getPtr());
			onRecDsGeo(reinterpret_cast<S57DSGeoRecord *>(r.getPtr()));
			break;
		case S57Record::DatasetAccuracy:
			_dsaccRec = reinterpret_cast<S57DSAccuracyRecord *>(r.getPtr());
			onRecDsAccuracy(reinterpret_cast<S57DSAccuracyRecord *>(r.getPtr()));
			break;
		case S57Record::Feature:
			onRecFeature(reinterpret_cast<S57FeatureRecord *>(r.getPtr()));
			break;
		case S57Record::Vector:
			onRecSpatial(reinterpret_cast<S57VectorRecord *>(r.getPtr()));
			break;
		default:
			break;
		}
	}

	if (_dsinfRec.isNull() || _dsgeoRec.isNull()) {
		printf("invalid dataset\n");
		return;
	}

	if (ds.updateCount() > 0)
		updateDataset(ds);

	onParse(ds);
}

void S57ParseScanner::parseOneDataset(string filepath)
{
	clear();

	string path, basename;

	string::size_type pos1 = filepath.rfind('/');
	if (pos1 != string::npos) {
		++pos1;
		path = filepath.substr(0, pos1);
	}
	else {
		pos1 = 0;
		path = ".";
	}
	path = getAbsolutePath(path);
	if (path.empty()) {
		fprintf(stderr, "%s: file or path not found\n", path.c_str());
		return;
	}

	string::size_type pos2 = filepath.find('.', pos1);
	if (pos2 != string::npos)
		basename = filepath.substr(pos1, pos2 - pos1);
	else
		basename = filepath.substr(pos1);

	_dsList.push_back(DsItem(path + basename));

	bool recursionSaved = isRecursive();
	setRecursive(false);
	setIgnoreBaseCell(true);
	this->scan(path);
	setIgnoreBaseCell(false);
	setRecursive(recursionSaved);
}

void S57ParseScanner::onDataset(string filepath)
{
	string::size_type pos = filepath.rfind('.');
	assert(pos != string::npos);

	if (filepath.substr(pos + 1, 3) == "000") { // is base cell
		if (!_ignoreBaseCell) {
			if (!_precheckEnabled || checkDsValid(filepath))
				_dsList.push_back(DsItem(filepath.substr(0, pos)));
		}
	}
	else if (_updatingEnabled) { // is update cell
		if (_dsList.empty() || !_dsList.back().insertUpdateCell(filepath))
			if (!_ignoreBaseCell)
				_upCells.push_back(filepath);
	}
}

S57FeatureRecordRef S57ParseScanner::findFeatureTarget(const S57_NAME &nm, s57_b12 objl)
{
	vector<S57FeatureRecordRef> *l = NULL;
	if (objl < 300)
		l = &_grList;
	else if (objl < 400)
		l = &_mrList;
	else if (objl < 500)
		l = &_lrList;
	else
		return NULL;

	S57FeatureRecordRef res;
	vector<S57FeatureRecordRef>::iterator it = l->begin();
	for (; it != l->end() && res.isNull(); ++it)
		if ((*it)->fieldFRID()->_name == nm)
			res = *it;
	return res;
}

S57VectorRecordRef S57ParseScanner::findVectorTarget(const S57_NAME &nm)
{
	S57VectorRecordRef res;
	vector<S57VectorRecordRef>::iterator it = _vrList.begin();
	for (; it != _vrList.end() && res.isNull(); ++it)
		if ((*it)->fieldVRID()->_name == nm)
			res = *it;
	return res;
}

void S57ParseScanner::onRecDsInfo(S57DSInfoRecord *)
{
	// do nothing
}

void S57ParseScanner::onRecDsGeo(S57DSGeoRecord *)
{
	// do nothing
}

void S57ParseScanner::onRecDsAccuracy(S57DSAccuracyRecord *)
{
	// do nothing
}

void S57ParseScanner::onRecFeature(S57FeatureRecord *r)
{
	if (r == NULL || r->fieldFRID() == NULL) {
		fprintf(stderr, "Bad feature record.\n");
		return;
	}

	int objl = r->fieldFRID()->_objl;
	if (objl < 300)
		_grList.push_back(r);
	else if (objl < 400)
		_mrList.push_back(r);
	else if (objl < 500)
		_lrList.push_back(r);
	else
		fprintf(stderr, "Unhandled feature record with OBJL=%d\n", objl);
}

void S57ParseScanner::onRecSpatial(S57VectorRecord *r)
{
	if (r == NULL || r->fieldVRID() == NULL) {
		fprintf(stderr, "Bad vector record.\n");
		return;
	}

	_vrList.push_back(r);
}

void S57ParseScanner::onPrepareParse(const DsItem &)
{
	// do nothing
}

void S57ParseScanner::onParse(const DsItem &)
{
	// do nothing
}

S57ParseScanner::S57ParseScanner()
{
	init();
}

S57ParseScanner::~S57ParseScanner()
{
	clear();
}

void S57ParseScanner::clear()
{
	_dsList.clear();
	_upCells.clear();

	_dsinfRec.release();
	_dsgeoRec.release();
	_dsaccRec.release();
	_grList.clear();
	_mrList.clear();
	_lrList.clear();
	_vrList.clear();
}

void S57ParseScanner::scan(string path)
{
	S57DatasetScanner::scan(path);
	upcellDispatch();

	if (_precheckEnabled) {
		printf("%zd datasets processed.\n", _dsList.size());
		if (_dsList.size() == 0)
			return;

		int reply = 0;
		fprintf(stderr, "Continue? (y/n) ");
		do {
			reply = getchar();
		} while (reply == '\n');
		if (reply != 'y' && reply != 'Y')
			return;
	}

	vector<DsItem>::iterator it = _dsList.begin();
	for (; it != _dsList.end(); ++it)
		doParse(*it);
}

// ~
