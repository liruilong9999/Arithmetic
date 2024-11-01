#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <assert.h>

#include <vector>
#include <list>
#include <string>

#include "ir_struct.h"
#include "assure_fio.h"
#include "s57_utils.h"
#include "s57_module.h"

using namespace std;

class DsItem
{
private:
  string _path;
  string _family; // File name of the data set, without extension.
  vector<int> _upNum; // Update numbers, in ASC order.

public:
  IR_ModuleEntry _irEntry;
  bool _isAlive;

public:
  // Constructs a DsItem using the file path and name,
  // but without externsion.
  DsItem(string pathName);

  // Returns the member _path
  string path() const;
  // Returns the member _family
  string family() const;
  // Returns the filename of the data set. (_path/_family.000)
  string dsFile() const;

  bool insertUpdateCell(string fileName);

  int updateCount() const;

  class UpdateIte;
  friend class UpdateIte;
  class UpdateIte
  {
  private:
    DsItem *_parent;
    vector<int>::iterator _it;

    // Constructor for DsItem
    UpdateIte(DsItem *, vector<int>::iterator);

  public:
    UpdateIte &operator++();
    string operator*() const;
    bool operator==(const UpdateIte &) const;
    bool operator!=(const UpdateIte &) const;

    int number() const;

    friend class DsItem;
  };

  UpdateIte updateBegin();
  UpdateIte updateEnd();
};

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
  sprintf(ext, ".%03d", *_it);
  return _parent->_path + _parent->_family + ext;
}

inline bool DsItem::UpdateIte::operator==(const UpdateIte &i) const
  { return (_parent == i._parent) && (_it == i._it); }

inline bool DsItem::UpdateIte::operator!=(const UpdateIte &i) const
  { return !operator==(i); }

inline int DsItem::UpdateIte::number() const
  { return *_it; }

DsItem::DsItem(string pathName)
{
  memset(&_irEntry, 0, sizeof(_irEntry));
  _isAlive = true;

  string::size_type pos = pathName.rfind('/');
  if (pos != string::npos) {
    _path = pathName.substr(0, pos + 1);
    _family = pathName.substr(pos + 1);
  }
  else
    _family = pathName;
}

inline string DsItem::path() const
  { return _path; }

inline string DsItem::family() const
  { return _family; }

inline string DsItem::dsFile() const
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
  while (i < static_cast<int>(_upNum.size()) && num > _upNum[i])
    ++i;
  assert(i == static_cast<int>(_upNum.size()) || num < _upNum[i]);
  _upNum.resize(_upNum.size() + 1);
  for (int j = _upNum.size() - 1; j > i; --j)
    _upNum[j] = _upNum[j - 1];
  _upNum[i] = num;

  return true;
}

inline int DsItem::updateCount() const
  { return _upNum.size(); }

inline DsItem::UpdateIte DsItem::updateBegin()
  { return DsItem::UpdateIte(this, _upNum.begin()); }

inline DsItem::UpdateIte DsItem::updateEnd()
  { return DsItem::UpdateIte(this, _upNum.end()); }

//~

class DsReidScanner : public S57DatasetScanner
{
private:
  bool _allowUpate;
  bool _checkDsValid;
  string _output;
  vector<DsItem> _dsList;

  // Temp update cell list
  list<string> _upCells;

  // Statistics
  int _baseCellCount;
  vector<IR_ModuleEntry *> _irModuleDir;

  // Variables for casting
  IR_DatasetParam _irParam;
  double _comf;

  // S-57 feature and vector records
  vector<S57FeatureRecordRef> _grList; // Geo features
  vector<S57FeatureRecordRef> _mrList; // Meta features
  vector<S57FeatureRecordRef> _lrList; // Collection features
  vector<S57VectorRecordRef> _vrList; // Vector records

private:
  bool checkDsValid(string fileName);
  void reDispatch();
  void registerCurModule();

  void updateDataset(DsItem &);
  S57FeatureRecordRef findFeatureTarget(const S57_NAME &, s57_b12, bool remove);
  S57VectorRecordRef findVectorTarget(const S57_NAME &, bool remove);

  void doCast(DsItem &);
  void onRecDsInfo(S57DSInfoRecord *);
  void onRecDsGeo(S57DSGeoRecord *);
  void onRecDsAccuracy(S57DSAccuracyRecord *);
  void onRecFeature(S57FeatureRecord *);
  void onRecSpatial(S57VectorRecord *);

  void reid();
  void resetVRPtr(S57_NAME nm, s57_b14 newId);
  void resetFSPtr(S57_NAME nm, s57_b14 newId);

  void saveReidFile(S57Module &m, S57Encoder &e);

protected:
  void onDataset(string);

public:
  DsReidScanner();
  virtual ~DsReidScanner();

  bool updateEnabled() const;
  void setUpdateEnabled(bool);

  bool checkEnabled() const;
  void setCheckEnabled(bool);

  string outputPath() const;
  void setOutputPath(string);

  void castDataset(string fileName);

  void scan(string path);
};

#define CHECK_RECORDS 3
bool DsReidScanner::checkDsValid(string fileName)
{
  bool ok = true;
  int nsteps = 0;

  printf("%s\t", fileName.c_str());
  fflush(stdout);

  S57Module mod(fileName);
  if (!mod.isOpen())
    return false;

  while (!mod.atEnd() && nsteps < CHECK_RECORDS) {
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
      if (dsid->_sted != "03.1") {
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
    printf("[ \033[32mOK\033\[0m ]");
  putchar('\n');

  return ok;
}

void DsReidScanner::reDispatch()
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

void DsReidScanner::registerCurModule()
{
  IR_ModuleEntry *ent = new IR_ModuleEntry;
  if (ent == NULL) {
    fprintf(stderr, "Lack of memory.\n");
    exit(1);
  }

  memset(ent, 0, sizeof(IR_ModuleEntry));
  ent->id = _irModuleDir.size() + 1;
  memcpy(ent->dsnm, _irParam.dsnm, 16);
  ent->scale = _irParam.cscl;
  ent->y_max = _irParam.y_max;
  ent->x_max = _irParam.x_max;
  ent->y_min = _irParam.y_min;
  ent->x_min = _irParam.x_min;

  vector<IR_ModuleEntry *>::iterator it = _irModuleDir.begin();
  for (; it != _irModuleDir.end(); ++it)
    if (memcmp(ent->dsnm, (*it)->dsnm, 16) == 0) {
      fprintf(stderr, "\'%s\' already exists.\n", ent->dsnm);
      exit(1);
    }

  _irModuleDir.push_back(ent);
}

void DsReidScanner::updateDataset(DsItem &ds)
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
	if (up_dsid->_edtn != _irParam.edtn || up_dsid->_updn != _irParam.updn + 1) {
	  putchar('!');
	  break; // go next update cell
	}
	_irParam.updn = up_dsid->_updn;
      }
      else if (r->recordType() == S57Record::Feature) {
        S57FeatureRecord *fr = reinterpret_cast<S57FeatureRecord *>(r.getPtr());
	const S57_FRID *up_frid = fr->fieldFRID();
	if (up_frid == NULL)
	  continue; // go next update record
	// insert
	if (up_frid->_ruin == S57_UI_I) {
	  if (up_frid->_objl < 300)
	    _grList.push_back(fr);
	  else if (up_frid->_objl < 400)
	    _mrList.push_back(fr);
	  else if (up_frid->_objl < 500)
	    _lrList.push_back(fr);
	}
	// delete
	else if (up_frid->_ruin == S57_UI_D) {
	  S57FeatureRecordRef target = findFeatureTarget(up_frid->_name, up_frid->_objl, true);
	  if (target.isNull())
	    fprintf(stderr, "Target not found: %s\n", up_frid->_name.toString().c_str());
	}
	// modify
	else if (up_frid->_ruin == S57_UI_M) {
	  S57FeatureRecordRef target = findFeatureTarget(up_frid->_name, up_frid->_objl, false);
	  if (target.isNull())
	    fprintf(stderr, "Target not found: %s\n", up_frid->_name.toString().c_str());
	  else
	    target->update(fr);
	}
      }
      else if (r->recordType() == S57Record::Vector) {
        S57VectorRecord *vr = reinterpret_cast<S57VectorRecord *>(r.getPtr());
	const S57_VRID *up_vrid = vr->fieldVRID();
	if (up_vrid == NULL)
	  continue; // go next update record
	// insert
	if (up_vrid->_ruin == S57_UI_I)
	  _vrList.push_back(vr);
	// delete
	else if (up_vrid->_ruin == S57_UI_D) {
	  S57VectorRecordRef target = findVectorTarget(up_vrid->_name, true);
	  if (target.isNull())
	    fprintf(stderr, "Target not found: %s\n", up_vrid->_name.toString().c_str());
	}
	// modify
	else if (up_vrid->_ruin == S57_UI_M) {
	  S57VectorRecordRef target = findVectorTarget(up_vrid->_name, false);
	  if (target.isNull())
	    fprintf(stderr, "Target not found: %s\n", up_vrid->_name.toString().c_str());
	  else
	    target->update(vr);
        }
      }
    }
    fflush(stdout);
  }

  if (ds.updateCount() > 0)
    putchar('\n');
}

S57FeatureRecordRef DsReidScanner::findFeatureTarget(const S57_NAME &nm, s57_b12 objl, bool remove)
{
  vector<S57FeatureRecordRef> *l = NULL;
  S57FeatureRecordRef res;

  if (objl < 300)
    l = &_grList;
  else if (objl < 400)
    l = &_mrList;
  else if (objl < 500)
    l = &_lrList;
  else
    return res;

  vector<S57FeatureRecordRef>::iterator it = l->begin();
  for (; it != l->end(); ++it) {
    const S57_FRID *frid = (*it)->fieldFRID();
    if (frid != NULL && frid->_name == nm) {
      res = *it;
      if (remove)
        l->erase(it);
      break;
    }
  }

  return res;
}

S57VectorRecordRef DsReidScanner::findVectorTarget(const S57_NAME &nm, bool remove)
{
  S57VectorRecordRef res;
  vector<S57VectorRecordRef>::iterator it = _vrList.begin();
  for (; it != _vrList.end(); ++it) {
    const S57_VRID *vrid = (*it)->fieldVRID();
    if (vrid != NULL && vrid->_name == nm) {
      res = *it;
      if (remove)
        _vrList.erase(it);
      break;
    }
  }

  return res;
}

void DsReidScanner::doCast(DsItem &ds)
{
  memset(&_irParam, 0, sizeof(_irParam));
  _comf = 0.0;

  _grList.clear();
  _mrList.clear();
  _lrList.clear();
  _vrList.clear();

  printf("Casting %s\n", ds.dsFile().c_str());

  // Sets data set name
  strncpy(_irParam.dsnm, ds.family().c_str(), 15);

  // Reads all records.
  // Data set information record, Data set Geographic record, 
  // Data set accruacy record to form IR_DatasetParam structure.
  S57Module mod(ds.dsFile());
  if (!mod.isOpen())
    return;
  while (!mod.atEnd()) {
    Ref<S57Record> r = mod.getNextRecord();
    assert(!r.isNull());
    switch (r->recordType()) {
    case S57Record::DatasetInformation:
      onRecDsInfo(reinterpret_cast<S57DSInfoRecord *>(r.getPtr()));
      break;
    case S57Record::DatasetGeographic:
      onRecDsGeo(reinterpret_cast<S57DSGeoRecord *>(r.getPtr()));
      break;
    case S57Record::DatasetAccuracy:
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

  if (ds.updateCount() > 0)
    updateDataset(ds);

  reid();

  string ofile = outputPath();
  if (ofile[ofile.length() - 1] != '/')
    ofile.push_back('/');
  ofile.append(ds.family());
  ofile.append(".000");
  FILE *fp = as_fopen(ofile.c_str(), "wb");
  S57Encoder e(fp);
  saveReidFile(mod, e);
  fclose(fp);
}

void DsReidScanner::onRecDsInfo(S57DSInfoRecord *r)
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

void DsReidScanner::onRecDsGeo(S57DSGeoRecord *r)
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

void DsReidScanner::onRecDsAccuracy(S57DSAccuracyRecord *r)
{
  const S57_DSAC *dsac = r->fieldDSAC();
  if (dsac != NULL) {
    double fpmf = static_cast<double>(dsac->_fpmf);
    _irParam.pacc = static_cast<UInt32>((dsac->_pacc / fpmf) * 10);
    _irParam.hacc = static_cast<UInt32>((dsac->_hacc / fpmf) * 10);
    _irParam.sacc = static_cast<UInt32>((dsac->_sacc / fpmf) * 10);
  }
}

void DsReidScanner::onRecFeature(S57FeatureRecord *r)
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

void DsReidScanner::onRecSpatial(S57VectorRecord *r)
{
  _vrList.push_back(r);
}

void DsReidScanner::reid()
{
  s57_b14 oid;
  s57_b14 nid = 1;

  vector<S57VectorRecordRef>::iterator vit = _vrList.begin();
  for (; vit != _vrList.end(); ++vit) {
    S57_VRID *vrid = const_cast<S57_VRID *>((*vit)->fieldVRID());
    if (vrid != NULL && vrid->_name._rcnm == RCNM_VI) {
      oid = vrid->_name._rcid;
      resetFSPtr(vrid->_name, nid);
      vrid->_name._rcid = nid++;
    }
  }

  nid = 1;
  vit = _vrList.begin();
  for (; vit != _vrList.end(); ++vit) {
    S57_VRID *vrid = const_cast<S57_VRID *>((*vit)->fieldVRID());
    if (vrid != NULL && vrid->_name._rcnm == RCNM_VC) {
      oid = vrid->_name._rcid;
      resetVRPtr(vrid->_name, nid);
      vrid->_name._rcid = nid++;
    }
  }

  nid = 1;
  vit = _vrList.begin();
  for (; vit != _vrList.end(); ++vit) {
    S57_VRID *vrid = const_cast<S57_VRID *>((*vit)->fieldVRID());
    if (vrid != NULL && vrid->_name._rcnm == RCNM_VE) {
      oid = vrid->_name._rcid;
      resetFSPtr(vrid->_name, nid);
      vrid->_name._rcid = nid++;
    }
  }
}

void DsReidScanner::resetVRPtr(S57_NAME nm, s57_b14 newId)
{
  vector<S57VectorRecordRef>::iterator it = _vrList.begin();
  for (; it != _vrList.end(); ++it) {
    vector<S57_VRPT>::const_iterator jt = (*it)->fieldsVRPT().begin();
    for (; jt != (*it)->fieldsVRPT().end(); ++jt) {
      S57_VRPT *vrpt = const_cast<S57_VRPT *>(&*jt);
      if (vrpt->_name == nm)
        vrpt->_name._rcid = newId;
    }
  }
}

void DsReidScanner::resetFSPtr(S57_NAME nm, s57_b14 newId)
{
  vector<S57FeatureRecordRef> *a[3] = { &_grList, &_mrList, &_lrList };
  
  for (int i = 0; i < 3; ++i) {
    vector<S57FeatureRecordRef>::iterator it = a[i]->begin();
    for (; it != a[i]->end(); ++it) {
      vector<S57_FSPT>::const_iterator jt = (*it)->fieldsFSPT().begin();
      for (; jt != (*it)->fieldsFSPT().end(); ++jt) {
        S57_FSPT *fspt = const_cast<S57_FSPT *>(&*jt);
        if (fspt->_name == nm)
          fspt->_name._rcid = newId;
      }
    }
  }
}

void DsReidScanner::saveReidFile(S57Module &m, S57Encoder &e)
{
  S57Record *r = NULL;
  
  r = m.ddr().getPtr();
  if (r != NULL)
    r->encode(e);

  r = m.generalInfoRecord().getPtr();
  if (r != NULL)
    r->encode(e);

  r = m.geographicRecord().getPtr();
  if (r != NULL)
    r->encode(e);

  vector<S57VectorRecordRef>::iterator vit = _vrList.begin();
  for (; vit != _vrList.end(); ++vit)
    (*vit)->encode(e);

  vector<S57FeatureRecordRef>::iterator fit = _mrList.begin();
  for (; fit != _mrList.end(); ++fit)
    (*fit)->encode(e);

  fit = _grList.begin();
  for (; fit != _grList.end(); ++fit)
    (*fit)->encode(e);

  fit = _lrList.begin();
  for (; fit != _lrList.end(); ++fit)
    (*fit)->encode(e);
}

void DsReidScanner::onDataset(string fileName)
{
  string::size_type pos = fileName.rfind('.');
  assert(pos != string::npos);
  if (fileName.substr(pos + 1, 3) == "000") { // is base cell
    ++_baseCellCount;
    if (!_checkDsValid || checkDsValid(fileName))
      _dsList.push_back(DsItem(fileName.substr(0, pos)));
  }
  else if (_allowUpate) { // is update cell
    if (_dsList.empty() || !_dsList.back().insertUpdateCell(fileName))
      _upCells.push_back(fileName);
  }
}

DsReidScanner::DsReidScanner()
{
  _allowUpate = true;
  _checkDsValid = false;
  _baseCellCount = 0;
}

DsReidScanner::~DsReidScanner()
{
  for (int i = 0; i < static_cast<int>(_irModuleDir.size()); ++i)
    delete _irModuleDir[i];
}

inline bool DsReidScanner::updateEnabled() const
  { return _allowUpate; }

inline void DsReidScanner::setUpdateEnabled(bool v)
  { _allowUpate = v; }

inline bool DsReidScanner::checkEnabled() const
  { return _checkDsValid; }

inline void DsReidScanner::setCheckEnabled(bool v)
  { _checkDsValid = v; }

inline string DsReidScanner::outputPath() const
  { return _output; }

inline void DsReidScanner::setOutputPath(string path)
  { _output = path; }

void DsReidScanner::castDataset(string fileName)
{
  string::size_type pos = fileName.rfind('.');
  DsItem ds(fileName.substr(0, pos));
  doCast(ds);
}

void DsReidScanner::scan(string path)
{
  _dsList.clear();
  _upCells.clear();
  _baseCellCount = 0;

  S57DatasetScanner::scan(path);
  reDispatch();

  if (_checkDsValid) {
    fprintf(stderr, "Total %d dataset found, %d processable.\n", 
            _baseCellCount, _dsList.size());
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
    doCast(*it);
}

// ~

static void usage()
{
  printf("Re-sequence object id.\n"
         "usage: s57reid [-hcR] SOURCE DEST\n"
         "options:\n"
	 "  -h\t Show this usage help.\n"
	 "  -c\t Check data set before casting.\n"
	 "  -R\t Convert all files under each directory, recursively.\n");
}

#if 0 //TODO

int main(int argc, char *argv[])
{
  struct stat stbuf;

  DsReidScanner scanner;
  scanner.setRecursive(false);
  scanner.setCheckEnabled(false);
  scanner.setUpdateEnabled(false);

  for (;;) {
    int c = getopt(argc, argv, "hcR");
    if (c == -1)
      break;

    switch (c) {
    case 'h':
      usage();
      return 0;
    case 'R':
      scanner.setRecursive(true);
      break;
    case 'c':
      scanner.setCheckEnabled(true);
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

  if (optind < argc || source.empty() || dest.empty()) {
    usage();
    return -1;
  }

  // Checks the dest
  int ret = lstat(dest.c_str(), &stbuf);
  if (ret != 0 || !S_ISDIR(stbuf.st_mode)) {
    fprintf(stderr, "%s: directory not exist or is a file\n", dest.c_str());
    return -1;
  }

  // Checks if the source is dir or file
  ret = lstat(source.c_str(), &stbuf);
  if (ret != 0) {
    fprintf(stderr, "%s: file or directory not exist\n", source.c_str());
    return -1;
  }

  scanner.setOutputPath(dest);

  if (S_ISDIR(stbuf.st_mode))
    scanner.scan(source);
  else
    scanner.castDataset(source);

  return 0;
}
#endif // 0
