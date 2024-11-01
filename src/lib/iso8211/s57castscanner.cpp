#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <filesystem>
#include <math.h>
#include <sys/stat.h>
#include <assert.h>

#include "../tools/String.h"
#include "../geo/utmproject.h"
#include "../geo/grect.h"
#include "../geo/gcoord.h"
#include "../geo/R-tree.h"

#include "assure_fio.h"
#include "s57castscanner.h"

using namespace std;
using namespace Geo;
using namespace MyTools;

static bool onFatalError()
{
	int reply = 0;
	printf("Fatal error! Ignore it and continue? (y/n) ");
	do {
		reply = getchar();
	} while (reply == '\n');

	return (reply == 'y' || reply == 'Y');
}

//
// CastingSpatialItem used to save a casting S-57 vector recrod.
//
class CastingSpatialItem
{
private:
	static Mercator _mer;
	Int32 *_pcoord;

	void updateMBR(Int32 x, Int32 y);

public:
	IR_SpatialRec _r;
	Int32 *_coords;
	static UInt32 _curCoordPos;

	Int32 _y_max; 
	Int32 _x_max; 
	Int32 _y_min; 
	Int32 _x_min; 

public:
	CastingSpatialItem();
	~CastingSpatialItem();

	static void setProjDatumType(Mercator::DatumType);

	void allocBuffer();

	void setBeginNode(double ux, double uy);
	void setEndNode(double ux, double uy);
	void addCoords(double ux, double uy, int z);

	void checkIfClosed();

	GRect mbr() const;
};

Mercator CastingSpatialItem::_mer;
UInt32 CastingSpatialItem::_curCoordPos = 0;

void CastingSpatialItem::updateMBR(Int32 x, Int32 y)
{
	if (y > _y_max)
		_y_max = y;
	if (y < _y_min)
		_y_min = y;

	if (x > _x_max)
		_x_max = x;
	if (x < _x_min)
		_x_min = x;
}

CastingSpatialItem::CastingSpatialItem()
{
	memset(&_r, 0, sizeof(IR_SpatialRec));
	_coords = NULL;
	_pcoord = NULL;
	_y_max = LONG_MIN;
	_x_max = LONG_MIN;
	_y_min = LONG_MAX;
	_x_min = LONG_MAX;
}

CastingSpatialItem::~CastingSpatialItem()
{
	if (_coords != NULL)
		delete[] _coords;
}

void CastingSpatialItem::setProjDatumType(Mercator::DatumType dtype)
{
	_mer.setDatumType(dtype);
}

void CastingSpatialItem::allocBuffer()
{
	_coords = new Int32[_r.coordCount];
	if (_coords == NULL) {
		fprintf(stderr, "Lack of memory.\n");
		exit(1);
	}
	memset(_coords, 0, sizeof(Int32) * _r.coordCount);
	_pcoord = _coords;
}

void CastingSpatialItem::setBeginNode(double ux, double uy)
{
	double x, y;
	Int32 wx, wy;
	_mer.project(ux, uy, &x, &y);
	x += (signbit(x) ? -0.05 : 0.05);
	y += (signbit(y) ? -0.05 : 0.05);
	*_pcoord++ = wy = static_cast<Int32>(y * 10.0);
	*_pcoord++ = wx = static_cast<Int32>(x * 10.0);
#ifndef NDEBUG
	assert(_r.pairSize == 2);
#else
	if (_r.pairSize == 3)
		++_pcoord;
#endif

	updateMBR(wx, wy);
}

void CastingSpatialItem::setEndNode(double ux, double uy)
{
	double x, y;
	Int32 wx, wy;
	Int32 *ep = _coords + _r.coordCount - _r.pairSize;
	_mer.project(ux, uy, &x, &y);
	x += (signbit(x) ? -0.05 : 0.05);
	y += (signbit(y) ? -0.05 : 0.05);
	*ep++ = wy = static_cast<Int32>(y * 10.0);
	*ep++ = wx = static_cast<Int32>(x * 10.0);
	assert(_r.pairSize == 2);

	updateMBR(wx, wy);
}

void CastingSpatialItem::addCoords(double ux, double uy, int z)
{
	double x, y;
	Int32 wx, wy;
	_mer.project(ux, uy, &x, &y);
	x += (signbit(x) ? -0.05 : 0.05);
	y += (signbit(y) ? -0.05 : 0.05);
	*_pcoord++ = wy = static_cast<Int32>(y * 10.0);
	*_pcoord++ = wx = static_cast<Int32>(x * 10.0);
	if (_r.pairSize == 3)
		*_pcoord++ = z;

	updateMBR(wx, wy);
}

void CastingSpatialItem::checkIfClosed()
{
	Int32 *bp = _coords;
	Int32 *ep = _coords + _r.coordCount - _r.pairSize;
	_r.isClosed = (_r.pairSize == 2 && _r.coordCount > 2 
			&& bp[0] == ep[0] && bp[1] == ep[1]) ? 1 : 0;
}

GRect CastingSpatialItem::mbr() const
{
	return GRect(GPoint(_x_min, _y_min), GPoint(_x_max, _y_max));
}

// ~

// S57CastScanner members

bool S57CastScanner::loadModuleList(string indexFile, 
						vector<IR_ModuleEntry *> &v, 
						int *major, int *minor)
{
	FILE *fp = as_fopen(indexFile.c_str(), "rb");

	char lh[2];
	as_fread(lh, 1, 2, fp);
	if (lh[0] != 'L' || lh[1] != 'H') {
		fprintf(stderr, "Seems not a IR module index file.\n");
		return false;
	}

	UInt16 ma, mi;
	as_fread(&ma, sizeof(UInt16), 1, fp);
	as_fread(&mi, sizeof(UInt16), 1, fp);

	if (major != NULL)
		*major = ma;
	if (minor != NULL)
		*minor = mi;

	IR_DataAreaLeader leader;
	as_fread(&leader, sizeof(IR_DataAreaLeader), 1, fp);
	if (leader.dataIdentifier != 'I') {
		fprintf(stderr, "Seems not a module index file.\n");
		return false;
	}

	IR_DirEntry dir;
	as_fread(&dir, sizeof(IR_DirEntry), 1, fp);

	assert(v.empty());
	v.clear();

	for (int i = 0; i < static_cast<int>(dir.size); ++i) {
		IR_ModuleEntry *entry = new IR_ModuleEntry;
		if (entry == NULL) {
			fprintf(stderr, "Lack of memory.\n");
			return false;
		}
		as_fread(entry, sizeof(IR_ModuleEntry), 1, fp);
		v.push_back(entry);
	}

	fclose(fp);

	return true;
}

void S57CastScanner::init()
{
	memset(&_irParam, 0, sizeof(_irParam));
	_comf = 0.0;
}

void S57CastScanner::registerCurModule()
{
	IR_ModuleEntry *ent = new IR_ModuleEntry;
	if (ent == NULL) {
		fprintf(stderr, "Lack of memory.\n");
		exit(1);
	}

	memset(ent, 0, sizeof(IR_ModuleEntry));

	ent->id = _irModuleDir.size() + 1;
	memcpy(ent->dsnm, _irParam.dsnm, 16);
	ent->mpdt = _irParam.mpdt;
	ent->intu = _irParam.intu;
	ent->edtn = _irParam.edtn;
	ent->updn = _irParam.updn;
	ent->scale = _irParam.cscl;
	ent->y_max = _irParam.y_max;
	ent->x_max = _irParam.x_max;
	ent->y_min = _irParam.y_min;
	ent->x_min = _irParam.x_min;

	// The dataset module must be added once.
	vector<IR_ModuleEntry *>::iterator it = _irModuleDir.begin();
	for (; it != _irModuleDir.end(); ++it) {
		if (memcmp(ent->dsnm, (*it)->dsnm, 16) == 0) {
			fprintf(stderr, "'%s\' already exists.\n", ent->dsnm);
			return;
		}
	}

	// Inserts it by order CSCL descending.
	it = _irModuleDir.begin();
	while (it != _irModuleDir.end() && (*it)->scale >= ent->scale)
		++it;
	_irModuleDir.insert(it, ent);
}

void S57CastScanner::saveIrFile(FILE *fp)
{
	vector<IR_FFPtrRec *> irFfptList;
	vector<IR_FSPtrRec *> irFsptList;
	vector<IR_FeatureRec *> irFrList;
	vector<IR_DirEntry *> irAttrDir;
	string attrString;
	vector<CastingSpatialItem *> cspaList;
	RTree *tree;
	GRect dsMbr;

	vector<S57FeatureRecordRef> tmpfrs;
	tmpfrs.insert(tmpfrs.end(), grList().begin(), grList().end());
	tmpfrs.insert(tmpfrs.end(), mrList().begin(), mrList().end());
	tmpfrs.insert(tmpfrs.end(), lrList().begin(), lrList().end());

	// For each vector record extracts coordinates and gets the MBR.
	CastingSpatialItem::_curCoordPos = 0;
	vector<S57VectorRecordRef>::const_iterator vit = vrList().begin();
	for (; vit != vrList().end(); ++vit) {
		const S57VectorRecord *theVr = vit->getPtr(); // the vector record
		if (theVr->isDeleted())
			continue;

		CastingSpatialItem *cspa = new CastingSpatialItem;
		if (cspa == NULL) {
			fprintf(stderr, "Lack of memory.\n");
			exit(1);
		}
		cspaList.push_back(cspa);
		cspa->_r.rcnm = theVr->fieldVRID()->_name._rcnm;
		cspa->_r.rcid = theVr->fieldVRID()->_name._rcid;

		switch (cspa->_r.rcnm) {
			case RCNM_VI:
				++_irParam.noin;
				break;
			case RCNM_VC:
				++_irParam.nocn;
				break;
			case RCNM_VE:
				++_irParam.noed;
				break;
			default:
				break;
		}

		// Coordinates
		if (theVr->coordType() == S57VectorRecord::SG2D)
			cspa->_r.pairSize = 2;
		else if (theVr->coordType() == S57VectorRecord::SG3D)
			cspa->_r.pairSize = 3;
		else
			assert(0);
		cspa->_r.coordPos = CastingSpatialItem::_curCoordPos;
		cspa->_r.coordCount = theVr->coords().size() 
				+ theVr->fieldsVRPT().size() * cspa->_r.pairSize;
		cspa->allocBuffer();

		CastingSpatialItem::_curCoordPos += cspa->_r.coordCount;

		for (int i = 0; i < static_cast<int>(theVr->fieldsVRPT().size()); ++i) {
			const S57_VRPT &vrpt = theVr->fieldsVRPT()[i];
			S57VectorRecordRef toVr = findVectorTarget(vrpt._name);
			if (toVr.isNull() || toVr->isDeleted()) {
				// Fatal error!
				fprintf(stderr, "Vector [%s]: invalid VRPT to %s\n", 
						theVr->fieldVRID()->_name.toString().c_str(), vrpt._name.toString().c_str());
#ifdef NDEBUG
				if (onFatalError())
					return;
				else
#endif
				{
					printf("Exit not completed!\n");
					exit(1);
				}
			}

			if (toVr->fieldVRID()->_name._rcnm != RCNM_VC) {
				// Fatal error!
				fprintf(stderr, "Vector[%s]: invalid VRPT target, not a connected node\n", 
						toVr->fieldVRID()->_name.toString().c_str());
#ifdef NDEBUG
				if (onFatalError())
					return;
				else 
#endif
				{
					printf("Exit not completed!\n");
					exit(1);
				}
			}

			double uy = toVr->coords()[0] / _comf;
			double ux = toVr->coords()[1] / _comf;
			if (vrpt._topi == TOPI_B)
				cspa->setBeginNode(ux, uy);
			else if (vrpt._topi == TOPI_E)
				cspa->setEndNode(ux, uy);
		}

		const vector<s57_b24> &coords = theVr->coords();
		for (int i = 0; i < static_cast<int>(coords.size()); i += cspa->_r.pairSize) {
			double uy = coords[i] / _comf;
			double ux = coords[i + 1] / _comf;
			cspa->addCoords(ux, uy, coords[i + 2]);
		}

		cspa->checkIfClosed();

		// update the global MBR
		dsMbr |= cspa->mbr();
		assert(dsMbr.isValid());

		// Attributs
		vector<S57_AttItem>::const_iterator ait = theVr->fieldsATTV().begin();
		for (; ait != theVr->fieldsATTV().end(); ++ait) {
			switch (ait->_attl) {
			case 401: // POSACC
				cspa->_r.posacc = static_cast<UInt32>(strToFloat(ait->_atvl) * 10.0);
				break;
			case 402: // UQAPOS
				cspa->_r.quapos = strToInt(ait->_atvl);
				break;
			default:
				break;
			}
		}
	}

	// For each features, extracts its FFPT, FSPT and attributes.
	vector<S57FeatureRecordRef>::iterator fit = tmpfrs.begin();
	for (; fit != tmpfrs.end(); ++fit) {
		const S57FeatureRecord *theFr = fit->getPtr(); // the feature record
		if (theFr->isDeleted())
			continue;

		IR_FeatureRec *frbuf = new IR_FeatureRec;
		if (frbuf == NULL) {
			fprintf(stderr, "Lack of memory.\n");
			exit(1);
		}
		irFrList.push_back(frbuf);

		memset(frbuf, 0, sizeof(IR_FeatureRec));
		frbuf->rcnm = theFr->fieldFRID()->_name._rcnm;
		frbuf->rcid = theFr->fieldFRID()->_name._rcid;
		frbuf->prim = theFr->fieldFRID()->_prim;
		frbuf->grup = theFr->fieldFRID()->_grup;
		frbuf->objl = theFr->fieldFRID()->_objl;

		if (frbuf->objl < 300)
			++_irParam.nogr;
		else if (frbuf->objl < 400)
			++_irParam.nomr;
		else if (frbuf->objl < 500)
			++_irParam.nolr;

		if (theFr->fieldFOID() != NULL) {
			frbuf->agen = theFr->fieldFOID()->_agen;
			frbuf->fidn = theFr->fieldFOID()->_fidn;
			frbuf->fids = theFr->fieldFOID()->_fids;
		}

		// attributs
		frbuf->attrPos = irAttrDir.size();
		frbuf->attrCount = theFr->fieldsATTF().size() + theFr->fieldsNATF().size();

		const vector<S57_AttItem> *atts[2] = {
			&(theFr->fieldsATTF()), 
			&(theFr->fieldsNATF())
		};

		for (int i = 0; i < 2; ++i) {
			vector<S57_AttItem>::const_iterator ait = atts[i]->begin();
			for (; ait != atts[i]->end(); ++ait) {
				IR_DirEntry *ent = new IR_DirEntry;
				if (ent == NULL) {
					fprintf(stderr, "Lack of memory.\n");
					exit(1);
				}
				irAttrDir.push_back(ent);
				ent->label = ait->_attl;
				ent->pos = attrString.size();
				ent->size = ait->_atvl.size();

				attrString.append(ait->_atvl);
				attrString.push_back('\0');

				switch (ait->_attl) {
				case 132: // SCAMAX
					frbuf->scale_max = strToInt(ait->_atvl);
					break;
				case 133: // SCAMIN
					frbuf->scale_min = strToInt(ait->_atvl);
					break;
				default:
					break;
				}
			}
		}

		// FFPTs
		frbuf->ffptPos = irFfptList.size();
		frbuf->ffptCount = 0;

		vector<S57_FFPT>::const_iterator ffit = theFr->fieldsFFPT().begin();
		for (; ffit != theFr->fieldsFFPT().end(); ++ffit) {
			IR_FFPtrRec *ffrbuf = new IR_FFPtrRec;
			if (ffrbuf == NULL) {
				fprintf(stderr, "Lack of memory.\n");
				exit(1);
			}
			// Finds the target, get its index.
			unsigned int pos = 0;
			vector<S57FeatureRecordRef>::iterator toFr = tmpfrs.begin();
			for (; toFr != tmpfrs.end(); ++toFr) {
				if ((*toFr)->fieldFOID() != NULL && *((*toFr)->fieldFOID()) == ffit->_lnam)
					break;
				if (!(*toFr)->isDeleted())
					++pos;
			}

			if (toFr == tmpfrs.end()) 
				fprintf(stderr, "Feature [%s]: invalid FFPT to [%s]\n", 
						(*fit)->fieldFRID()->_name.toString().c_str(), ffit->_lnam.toString().c_str());

			if (toFr != tmpfrs.end() && !(*toFr)->isDeleted()) {
				ffrbuf->pos = pos;
				ffrbuf->rind = ffit->_rind;
				irFfptList.push_back(ffrbuf);
				++frbuf->ffptCount;
			}
			else
				delete ffrbuf;
		}

		// FSPTs
		frbuf->fsptPos = irFsptList.size();
		frbuf->fsptCount = 0;

		vector<S57_FSPT>::const_iterator fsit = theFr->fieldsFSPT().begin();
		for (; fsit != theFr->fieldsFSPT().end(); ++fsit) {
			IR_FSPtrRec *fsrbuf = new IR_FSPtrRec;
			if (fsrbuf == NULL) {
				fprintf(stderr, "Lack of memory.\n");
				exit(1);
			}
			// Finds the target, get its index.
			vector<CastingSpatialItem *>::iterator toSp = cspaList.begin();
			for (; toSp != cspaList.end(); ++toSp) {
				if ((*toSp)->_r.rcnm == fsit->_name._rcnm && (*toSp)->_r.rcid == fsit->_name._rcid)
					break;
			}

			if (toSp != cspaList.end()) {
				fsrbuf->pos = toSp - cspaList.begin();
				fsrbuf->ornt = fsit->_ornt;
				fsrbuf->usag = fsit->_usag;
				fsrbuf->mask = fsit->_mask;
				irFsptList.push_back(fsrbuf);
				++frbuf->fsptCount;
			}
			else
				delete fsrbuf;
		}
	}

	// Update the MBR of the features, and makes the quick
	// index of the geo features.
	tree = rtreeCreate();
	if (tree == NULL) {
		fprintf(stderr, "Fail to create R-tree.\n");
		exit(1);
	}

	vector<IR_FeatureRec *>::iterator ir_fit = irFrList.begin();
	for (; ir_fit != irFrList.end(); ++ir_fit) {
		GRect fmbr;
		for (int i = 0; i < (*ir_fit)->fsptCount; ++i) {
			IR_FSPtrRec *ir_fspt = irFsptList[(*ir_fit)->fsptPos + i];
			fmbr |= cspaList[ir_fspt->pos]->mbr();
		}
		(*ir_fit)->y_max = fmbr.top();
		(*ir_fit)->x_max = fmbr.right();
		(*ir_fit)->y_min = fmbr.bottom();
		(*ir_fit)->x_min = fmbr.left();

		if ((*ir_fit)->objl < 300) {
			/*assert(fmbr.isValid());*/
			// Here the index is global index of the feature records, 
			// but it is also the index of the geo features, because the geo features
			// in the front of the feature list.
			// I want to say is the value passed to the R-tree is it shoule to be.
			UInt32 index = ir_fit - irFrList.begin();
			assert(index < grList().size()); 
			rtreeInsert(tree, fmbr.left(), fmbr.bottom(), fmbr.right(), fmbr.top(), 
					reinterpret_cast<void *>(index + 1));
		}
	}

	assert(dsMbr.isValid());
	_irParam.y_max = dsMbr.top();
	_irParam.x_max = dsMbr.right();
	_irParam.y_min = dsMbr.bottom();
	_irParam.x_min = dsMbr.left();

	registerCurModule();

	////////////////////////////////////////////////
	//               Writes IR file               //
	////////////////////////////////////////////////

	IR_DataAreaLeader leader;
	IR_DirEntry *dir = NULL;
	long areaStart;
	long contentsStart;
	long posSave;
	int i;

	//
	// Writes the file header
	//
	char lh[2] = { 'L', 'H' };
	UInt16 major = IRV_MAJOR;
	UInt16 minor = IRV_MINOR;

	as_fwrite(lh, 1, 2, fp);
	as_fwrite(&major, sizeof(UInt16), 1, fp);
	as_fwrite(&minor, sizeof(UInt16), 1, fp);
	as_fwrite(&_irParam, sizeof(IR_DatasetParam), 1, fp);

	//
	// Writes record area 
	//
	dir = new IR_DirEntry[6];
	if (dir == NULL) {
		fprintf(stderr, "Lack of memory.\n");
		exit(1);
	}

	areaStart = as_ftell(fp);

	memset(&leader, 0, sizeof(IR_DataAreaLeader));
	memset(dir, 0, sizeof(IR_DirEntry) * 6);
	as_fwrite(&leader, sizeof(IR_DataAreaLeader), 1, fp);
	as_fwrite(dir, sizeof(IR_DirEntry), 6, fp);

	contentsStart = as_ftell(fp);

	dir[0].label = 1;
	dir[0].pos = 0;
	dir[0].size = irFfptList.size();
	for (i = 0; i < static_cast<int>(irFfptList.size()); ++i)
		as_fwrite(irFfptList[i], sizeof(IR_FFPtrRec), 1, fp);

	dir[1].label = 2;
	dir[1].pos = as_ftell(fp) - contentsStart;
	dir[1].size = irFsptList.size();
	for (i = 0; i < static_cast<int>(irFsptList.size()); ++i)
		as_fwrite(irFsptList[i], sizeof(IR_FSPtrRec), 1, fp);

	dir[2].label = 3;
	dir[2].pos = as_ftell(fp) - contentsStart;
	dir[2].size = 0;
	for (i = 0; i < static_cast<int>(grList().size()); ++i)
		if (!grList()[i]->isDeleted())
			++dir[2].size;

	dir[3].label = 4;
	dir[3].pos = as_ftell(fp) - contentsStart;
	dir[3].size = 0;
	for (i = 0; i < static_cast<int>(mrList().size()); ++i)
		if (!mrList()[i]->isDeleted())
			++dir[3].size;

	dir[4].label = 5;
	dir[4].pos = as_ftell(fp) - contentsStart;
	dir[4].size = 0;
	for (i = 0; i < static_cast<int>(lrList().size()); ++i)
		if (!lrList()[i]->isDeleted())
			++dir[4].size;

	for (i = 0; i < static_cast<int>(irFrList.size()); ++i)
		as_fwrite(irFrList[i], sizeof(IR_FeatureRec), 1, fp);

	dir[5].label = 6;
	dir[5].pos = as_ftell(fp) - contentsStart;
	dir[5].size = cspaList.size();
	for (i = 0; i < static_cast<int>(cspaList.size()); ++i)
		as_fwrite(&(cspaList[i]->_r), sizeof(IR_SpatialRec), 1, fp);

	posSave = as_ftell(fp);

	// writes back the data area header
	leader.dataIdentifier = 'R';
	leader.dataVersion = 1;
	leader.extension[0] = ' ';
	leader.extension[1] = ' ';
	leader.extension[2] = ' ';
	leader.extension[3] = ' ';
	leader.areaLength = posSave - areaStart;
	leader.dataOffset = sizeof(IR_DataAreaLeader) + sizeof(IR_DirEntry) * 6;
	leader.dirSize = 6;
	as_fseek(fp, areaStart, SEEK_SET);
	as_fwrite(&leader, sizeof(IR_DataAreaLeader), 1, fp);
	as_fwrite(dir, sizeof(IR_DirEntry), 6, fp);
	as_fseek(fp, posSave, SEEK_SET);

	delete[] dir;
	dir = NULL;

	//
	// Writes attribute area
	//
	areaStart = as_ftell(fp);

	size_t headerSize = sizeof(IR_DataAreaLeader) + sizeof(IR_DirEntry) * irAttrDir.size();
	leader.dataIdentifier = 'A';
	leader.dataVersion = 1;
	leader.extension[0] = ' ';
	leader.extension[1] = ' ';
	leader.extension[2] = ' ';
	leader.extension[3] = ' ';
	leader.areaLength = headerSize + attrString.size();
	leader.dataOffset = headerSize;
	leader.dirSize = irAttrDir.size();
	as_fwrite(&leader, sizeof(IR_DataAreaLeader), 1, fp);
	for (i = 0; i < static_cast<int>(irAttrDir.size()); ++i)
		as_fwrite(irAttrDir[i], sizeof(IR_DirEntry), 1, fp);
	as_fwrite(attrString.data(), 1, attrString.size(), fp);
	assert((as_ftell(fp) - areaStart) == leader.areaLength);

	//
	// Writes coordinate area
	//
	areaStart = as_ftell(fp);

	memset(&leader, 0, sizeof(IR_DataAreaLeader));
	as_fwrite(&leader, sizeof(IR_DataAreaLeader), 1, fp);
	for (i = 0; i < static_cast<int>(cspaList.size()); ++i)
		as_fwrite(cspaList[i]->_coords, sizeof(Int32), cspaList[i]->_r.coordCount, fp);

	posSave = as_ftell(fp);

	leader.dataIdentifier = 'C';
	leader.dataVersion = 1;
	leader.extension[0] = ' ';
	leader.extension[1] = ' ';
	leader.extension[2] = ' ';
	leader.extension[3] = ' ';
	leader.areaLength = posSave - areaStart;
	leader.dataOffset = sizeof(IR_DataAreaLeader);
	leader.dirSize = 0;
	as_fseek(fp, areaStart, SEEK_SET);
	as_fwrite(&leader, sizeof(IR_DataAreaLeader), 1, fp);
	as_fseek(fp, posSave, SEEK_SET);

	//
	// Writes R-tree area
	//
	writeRTreeArea(fp, tree);

	as_fflush(fp);

	// Release all allocated memory
	for (i = 0; i < static_cast<int>(irFfptList.size()); ++i)
		delete irFfptList[i];
	for (i = 0; i < static_cast<int>(irFsptList.size()); ++i)
		delete irFsptList[i];
	for (i = 0; i < static_cast<int>(irFrList.size()); ++i)
		delete irFrList[i];
	for (i = 0; i < static_cast<int>(irAttrDir.size()); ++i)
		delete irAttrDir[i];
	for (i = 0; i < static_cast<int>(cspaList.size()); ++i)
		delete cspaList[i];
	rtreeDestroy(tree);
}

void S57CastScanner::writeRTreeArea(FILE *fp, RTree *tree)
{
	IR_DataAreaLeader leader;
	long areaStart = as_ftell(fp);

	memset(&leader, 0, sizeof(IR_DataAreaLeader));
	as_fwrite(&leader, sizeof(IR_DataAreaLeader), 1, fp);
	rtreeSave(tree, fp);

	leader.dataIdentifier = 'Q';
	leader.dataVersion = 1;
	leader.extension[0] = ' ';
	leader.extension[1] = ' ';
	leader.extension[2] = ' ';
	leader.extension[3] = ' ';
	leader.areaLength = as_ftell(fp) - areaStart;
	leader.dataOffset = sizeof(IR_DataAreaLeader);
	leader.dirSize = 0;
	as_fseek(fp, areaStart, SEEK_SET);
	as_fwrite(&leader, sizeof(IR_DataAreaLeader), 1, fp);
}

void S57CastScanner::onRecDsInfo(S57DSInfoRecord *r)
{
	const S57_DSID *dsid = r->fieldDSID();
	if (dsid != NULL) {
		_irParam.intu = dsid->_intu;
		_irParam.edtn = dsid->_edtn;
		_irParam.updn = dsid->_updn;
		_irParam.prsp = dsid->_prsp;
		_irParam.agen = dsid->_agen;
	}

	/*
	const S57_DSSI *dssi = r->fieldDSSI();
	if (dssi != NULL) {
		_irParam.nomr = dssi->_nomr;
		_irParam.nogr = dssi->_nogr;
		_irParam.nolr = dssi->_nolr;
		_irParam.noin = dssi->_noin;
		_irParam.nocn = dssi->_nocn;
		_irParam.noed = dssi->_noed;
	}
	*/
}

void S57CastScanner::onRecDsGeo(S57DSGeoRecord *r)
{
	const S57_DSPM *dspm = r->fieldDSPM();
	if (dspm != NULL) {
		_irParam.hdat = dspm->_hdat;
		_irParam.vdat = dspm->_vdat;
		_irParam.sdat = dspm->_sdat;
		_irParam.cscl = dspm->_cscl;
		_comf = dspm->_comf;
	}
}

void S57CastScanner::onRecDsAccuracy(S57DSAccuracyRecord *r)
{
	const S57_DSAC *dsac = r->fieldDSAC();
	if (dsac != NULL) {
		double fpmf = static_cast<double>(dsac->_fpmf);
		_irParam.pacc = static_cast<UInt32>((dsac->_pacc / fpmf) * 10);
		_irParam.hacc = static_cast<UInt32>((dsac->_hacc / fpmf) * 10);
		_irParam.sacc = static_cast<UInt32>((dsac->_sacc / fpmf) * 10);
	}
}

void S57CastScanner::onPrepareParse(const DsItem &ds)
{
	memset(&_irParam, 0, sizeof(_irParam));
	_comf = 0.0;

	strncpy(_irParam.dsnm, ds.family().c_str(), 15);
	_irParam.mpdt = static_cast<UInt8>(projDatumType());
}

void S57CastScanner::onParse(const DsItem &ds)
{
	string ofile = outputPath();
	ofile.append(ds.family());
	ofile.append(".cds");
	FILE *fp = as_fopen(ofile.c_str(), "wb");
	saveIrFile(fp);
	fclose(fp);
}

S57CastScanner::S57CastScanner()
	: S57ParseScanner()
{
	init();
}

S57CastScanner::S57CastScanner(string moduleDir)
	: S57ParseScanner()
{
	init();
	setIndexFile(moduleDir);
}

S57CastScanner::~S57CastScanner()
{
	vector<IR_ModuleEntry *>::iterator it = _irModuleDir.begin();
	for (; it != _irModuleDir.end(); ++it)
		delete *it;
	_irModuleDir.clear();
}

void S57CastScanner::setProjDatumType(Mercator::DatumType dtype)
{
	S57ParseScanner::setProjDatumType(dtype);
	CastingSpatialItem::setProjDatumType(dtype);
}

void S57CastScanner::setIndexFile(string moduleDir)
{
	moduleDir = getAbsolutePath(moduleDir);

	string s = moduleDir;
	if (!s.empty() && s[s.length() - 1] != '/')
		s.push_back('/');
	s.append("index");

	FILE *fp = fopen(s.c_str(), "rb");
	if (fp != NULL) {
		fclose(fp);
		loadModuleList(s, _irModuleDir, NULL, NULL);
	}
	else
		fprintf(stderr, "Cann't open file \"%s\"\n", s.c_str());
}

void S57CastScanner::setOutputPath(string path)
{
	_outputPath = getAbsolutePath(path);
}

const IR_ModuleEntry *S57CastScanner::castDataset(string filepath)
{
	int nModuleEntriesSave = _irModuleDir.size();

	parseOneDataset(filepath);

	// Something wrong happend
	if (nModuleEntriesSave == static_cast<int>(_irModuleDir.size()))
		return NULL;

	// Returns the added new module entry,
	// the entry has the maximum ID.
	vector<IR_ModuleEntry *>::iterator it = _irModuleDir.begin();
	for (; it != _irModuleDir.end(); ++it)
		if ((*it)->id == _irModuleDir.size())
			break;
	assert(it != _irModuleDir.end());
	return *it;
}

vector<IR_ModuleEntry> S57CastScanner::getModuleList() const
{
	vector<IR_ModuleEntry> res;
	res.reserve(_irModuleDir.size());

	vector<IR_ModuleEntry *>::const_iterator it = _irModuleDir.begin();
	for (; it != _irModuleDir.end(); ++it)
		res.push_back(**it);

	return res;
}

bool S57CastScanner::removeModuleEntry(string name)
{
	bool found = false;

	vector<IR_ModuleEntry *>::iterator it = _irModuleDir.begin();
	for (; it != _irModuleDir.end(); ++it) {
		if (name == (*it)->dsnm) {
			delete *it;
			_irModuleDir.erase(it);
			found = true;
			break;
		}
	}

	if (!found)
		fprintf(stderr, "Module entry \"%s\" not found.\n", name.c_str());

	return found;
}

void S57CastScanner::saveIndexFile()
{
	IR_DataAreaLeader leader;
	IR_DirEntry dir;
	long areaStart, posSave;
	const char lh[2] = { 'L', 'H' };
	UInt16 major, minor;

	/* Save Module List */

	RTree *tree = rtreeCreate();
	if (tree == NULL) {
		fprintf(stderr, "Fail to create R-tree.\n");
		exit(1);
	}

	string ofile = outputPath();
	ofile.append("index");
	FILE *fp = as_fopen(ofile.c_str(), "wb");

	//
	// Writes the file header
	//
	major = 1;
	minor = 1;
	as_fwrite(lh, 1, 2, fp);
	as_fwrite(&major, sizeof(UInt16), 1, fp);
	as_fwrite(&minor, sizeof(UInt16), 1, fp);

	//
	// Writes the module entries
	//
	areaStart = as_ftell(fp);

	memset(&leader, 0, sizeof(IR_DataAreaLeader));
	as_fwrite(&leader, sizeof(IR_DataAreaLeader), 1, fp);

	dir.label = 0;
	dir.pos = 0;
	dir.size = _irModuleDir.size();
	as_fwrite(&dir, sizeof(IR_DirEntry), 1, fp);

	// resets ids
	vector<IR_ModuleEntry *>::iterator it = _irModuleDir.begin();
	int id = 1;
	for (; it != _irModuleDir.end(); ++it)
		(*it)->id = id++;

	it = _irModuleDir.begin();
	for (; it != _irModuleDir.end(); ++it) {
		const IR_ModuleEntry *e = *it;
		as_fwrite(e, sizeof(IR_ModuleEntry), 1, fp);
		rtreeInsert(tree, e->x_min, e->y_min, e->x_max, e->y_max, 
				reinterpret_cast<void *>(e->id));
	}

	posSave = as_ftell(fp);

	leader.dataIdentifier = 'I';
	leader.dataVersion = 1;
	leader.extension[0] = ' ';
	leader.extension[1] = ' ';
	leader.extension[2] = ' ';
	leader.extension[3] = ' ';
	leader.areaLength = posSave - areaStart;
	leader.dataOffset = sizeof(IR_DataAreaLeader) + sizeof(IR_DirEntry);
	leader.dirSize = 1;
	as_fseek(fp, areaStart, SEEK_SET);
	as_fwrite(&leader, sizeof(IR_DataAreaLeader), 1, fp);
	as_fseek(fp, posSave, SEEK_SET);

	//
	// Writes R-tree area
	//
	writeRTreeArea(fp, tree);

	rtreeDestroy(tree);
	fclose(fp);
}

void S57CastScanner::listModuleEntries(string indexFile)
{
	vector<IR_ModuleEntry *> v;
	int major, minor;

#if 0

	struct stat stbuf;
	int ret = lstat(indexFile.c_str(), &stbuf);
	if (ret != 0) {
		perror("lstat()");
		return;
	}

	if (S_ISDIR(stbuf.st_mode))
		indexFile.append("/index");
#else

	if (!std::filesystem::exists(indexFile))
	{
		perror("lstat()");
		return;
	}

	if (std::filesystem::is_directory(indexFile))
	{
		indexFile.append("/index");
	}
#endif // 0

	if (!loadModuleList(indexFile, v, &major, &minor))
		return;

	printf("Index file version: %d.%d\n", major, minor);

	Mercator m;
	vector<IR_ModuleEntry *>::iterator it = v.begin();
	for (; it != v.end(); ++it) {
		IR_ModuleEntry *entry = *it;
		printf("%-5u", entry->id);
		printf("%-11s", entry->dsnm);

		m.setDatumType(static_cast<Mercator::DatumType>(entry->mpdt));
		switch (m.datumType()) {
		case Mercator::Krassovsky:
			printf("Kra   ");
			break;
		case Mercator::IAG75:
			printf("IAG75 ");
			break;
		case Mercator::WGS84:
			printf("WGS84 ");
			break;
		case Mercator::WebMercator:
			printf("Web   ");
			break;
		default:
			assert(0);
			break;
		}

		double ux, uy;
		char sbuf[32];
		m.aproject(entry->x_min / 10.0, entry->y_min / 10.0, &ux, &uy);
		string s = "[";
		GCoord(ux).toString(sbuf, GCoord::X, GCoord::DMm | GCoord::PrefixInd, false, '^');
		s.append(sbuf);
		s.push_back(' ');
		GCoord(uy).toString(sbuf, GCoord::Y, GCoord::DMm | GCoord::PrefixInd, false, '^');
		s.append(sbuf);
		s.append("]");
		printf("%-27s", s.c_str());

		m.aproject(entry->x_max / 10.0, entry->y_max / 10.0, &ux, &uy);
		s = "[";
		GCoord(ux).toString(sbuf, GCoord::X, GCoord::DMm | GCoord::PrefixInd, false, '^');
		s.append(sbuf);
		s.push_back(' ');
		GCoord(uy).toString(sbuf, GCoord::Y, GCoord::DMm | GCoord::PrefixInd, false, '^');
		s.append(sbuf);
		s.append("]");
		printf("%-27s", s.c_str());

		printf(" %d", entry->intu);

		printf(" %d-%-5d", entry->edtn, entry->updn);

		printf("%lu", entry->scale);

		printf("\n");
	}

	printf("  Total module(s): %zu\n", v.size());

	for (it = v.begin(); it != v.end(); ++it)
		delete *it;
}

// ~
