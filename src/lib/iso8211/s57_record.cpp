#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <strstream>
#include <iomanip>

#include "assure_fio.h"
#include "s57_module.h"
#include "s57_record.h"

using namespace std;

// Update utils

void updateAttributes(std::vector<S57_AttItem> &dst, 
				const std::vector<S57_AttItem> &src);

template <class T>
void updatePointers(std::vector<T> &dst, 
				const std::vector<T> &src, 
				const S57_UpdControl *);

void updateCoords(vector<s57_b24> &dst, const vector<s57_b24> &src, 
				int pairSize, const S57_UpdControl *);

// LRLeader members

LRLeader::LRLeader()
{
	_recordLength = 0;
	_interchangeLevel = 0;
	_leaderIdentifier = 0;
	_extensionIndicator = 0;
	_versionNumber = 0;
	_applicationIndicator = 0;
	_fieldControlLength = 0;
	_fieldAreaOffset = 0;
	_charSetIndicator[0] = 0;
	_charSetIndicator[1] = 0;
	_charSetIndicator[2] = 0;
	_szFieldLen = 0;
	_szFieldPos = 0;
}

LRLeader::LRLeader(string data)
{
	_recordLength = atoi(data.substr(0, 5).c_str());
	_interchangeLevel = data[5];
	_leaderIdentifier = data[6];
	_extensionIndicator = data[7];
	_versionNumber = data[8];
	_applicationIndicator = data[9];
	_fieldControlLength = atoi(data.substr(10, 2).c_str());
	_fieldAreaOffset = atoi(data.substr(12, 5).c_str());
	_charSetIndicator[0] = data[17];
	_charSetIndicator[1] = data[18];
	_charSetIndicator[2] = data[19];
	_szFieldLen = data[20] - 0x30;
	_szFieldPos = data[21] - 0x30;
}

string LRLeader::toString() const
{
	ostrstream os;
	os << _recordLength << '/'
	   << _interchangeLevel << '/'
	   << _leaderIdentifier << '/'
	   << _extensionIndicator << '/'
	   << _versionNumber << '/'
	   << _applicationIndicator << '/'
	   << _fieldControlLength << '/'
	   << _fieldAreaOffset << '/'
	   << _charSetIndicator[0] << _charSetIndicator[1] << _charSetIndicator[2] << '/'
	   << _szFieldLen << _szFieldPos << "04" << ends;

	char *tmp = os.str();
	string result(tmp);
	delete[] tmp;
	return result;
}

// LRDirEntry members

LRDirEntry::LRDirEntry()
{
	memset(_fieldTag, 0, 5);
	_fieldLen = 0;
	_fieldPos = 0;
}

LRDirEntry::LRDirEntry(const char *tag, size_t len, size_t pos)
{
	strncpy(_fieldTag, tag, FIELD_TAG_SIZE);
	_fieldTag[FIELD_TAG_SIZE] = '\0';
	_fieldLen = len;
	_fieldPos = pos;
}

// LRHeader members

LRHeader::LRHeader()
	: RefBase()
{
}

LRHeader::LRHeader(string data)
	: RefBase(), _leader(data)
{
	// decode directory
	const int dirCount = (_leader._fieldAreaOffset - 24)
							/ (_leader._szFieldLen + _leader._szFieldPos + FIELD_TAG_SIZE);
	_dir.resize(dirCount);

	int pos = 24;
	for (int i = 0; i < dirCount; ++i) {
		strcpy(_dir[i]._fieldTag, data.substr(pos, FIELD_TAG_SIZE).c_str());
		pos += FIELD_TAG_SIZE;
		_dir[i]._fieldLen = atoi(data.substr(pos, _leader._szFieldLen).c_str());
		pos += _leader._szFieldLen;
		_dir[i]._fieldPos = atoi(data.substr(pos, _leader._szFieldPos).c_str());
		pos += _leader._szFieldPos;
	}

	assert(data[pos] = S57_FT);
}

LRHeader::DirIterator LRHeader::begin()
{
	LRHeader::DirIterator it = _dir.begin();
	return ++it; // skip the first field 0001(ISO/IEC 8211 Record Identifier)
}

LRHeader::DirIterator LRHeader::end()
{
	return _dir.end();
}

void LRHeader::encode(S57Encoder &e)
{
	char sbuf[24];
	sprintf(sbuf, "%05u", _leader._recordLength);
	sbuf[5] = _leader._interchangeLevel;
	sbuf[6] = _leader._leaderIdentifier;
	sbuf[7] = _leader._extensionIndicator;
	sbuf[8] = _leader._versionNumber;
	sbuf[9] = _leader._applicationIndicator;
	strcpy(sbuf + 10, _leader._leaderIdentifier == 'L' ? "09" : "  ");
	sprintf(sbuf + 12, "%05u", _leader._fieldAreaOffset);
	memcpy(sbuf + 17, _leader._charSetIndicator, 3);
	sbuf[20] = _leader._szFieldLen + 0x30;
	sbuf[21] = _leader._szFieldPos + 0x30;
	sbuf[22] = '0';
	sbuf[23] = '4';
	e.writeBlock(sbuf, 24);

	vector<LRDirEntry>::const_iterator it = _dir.begin();
	for (; it != _dir.end(); ++it) {
		sprintf(sbuf, "%s%0*u%0*u", it->_fieldTag, 
				_leader._szFieldLen, it->_fieldLen, 
				_leader._szFieldPos, it->_fieldPos);
		e.writeBlock(sbuf, strlen(sbuf));
	}
	e.endField();
}

string LRHeader::toString() const
{
	ostrstream os;
	os << "Leader" << endl
		<< INDENT << _leader.toString() << endl
		<< "Directory" << endl << INDENT;

	vector<LRDirEntry>::const_iterator it = _dir.begin();
	for (; it != _dir.end(); ++it)
		os << it->_fieldTag 
			<< setfill('0') << setw(_leader._szFieldLen) << it->_fieldLen
			<< setfill('0') << setw(_leader._szFieldPos) << it->_fieldPos;
	os << endl << ends;

	char *tmp = os.str();
	string result(tmp);
	delete[] tmp;
	return result;
}

// S57Record members

S57Record::S57Record()
	: RefBase()
{
	_module = NULL;
}

S57Record::S57Record(S57Record::RecordType type, S57Module *mod)
	: RefBase()
{
	_recordType = type;
	_module = mod;
}

S57Record::S57Record(S57Record::RecordType type, string data)
	: RefBase()
{ 
	_data = data;
	_recordType = type;
	_header = new LRHeader(data);
	_module = NULL;
}

S57Record::~S57Record()
{
}

Ref<S57Record> S57Record::decode(string data, S57Module *mod)
{
	Ref<S57Record> res;
	LRHeaderRef hr = new LRHeader(data);
	const string tag1(hr->_dir[1]._fieldTag);
	const string fieldArea(data.substr(hr->_leader._fieldAreaOffset));

	if (hr->_dir.size() < 2) {
		fprintf(stderr, "Seems not a S57 record, directory too small.\n");
		return res;
	}

	if (tag1 == "0001")
		res = new S57DataDescripRecord(hr, fieldArea, mod);
	else if (tag1 == "DSID")
		res = new S57DSInfoRecord(hr, fieldArea, mod);
	else if (tag1 == "DSPM")
		res = new S57DSGeoRecord(hr, fieldArea, mod);
	else if (tag1 == "DSHT")
		res = new S57DSHistoryRecord(hr, fieldArea, mod);
	else if (tag1 == "DSAC")
		res = new S57DSAccuracyRecord(hr, fieldArea, mod);
	else if (tag1 == "CATD")
		res = new S57CatalogDirRecord(hr, fieldArea, mod);
	else if (tag1 == "CATX")
		res = new S57Record(S57Record::CatalogCross, data);
	else if (tag1 == "DDDF")
		res = new S57Record(S57Record::DataDictDefn, data);
	else if (tag1 == "DDDI")
		res = new S57Record(S57Record::DataDictDomain, data);
	else if (tag1 == "DDSI")
		res = new S57Record(S57Record::DataDictSchema, data);
	else if (tag1 == "FRID")
		res = new S57FeatureRecord(hr, fieldArea, mod);
	else if (tag1 == "VRID")
		res = new S57VectorRecord(hr, fieldArea, mod);
	else
		fprintf(stderr, "Unknown S57 record with the first field: %s\n", tag1.c_str());

	if (strncmp(hr->_dir[0]._fieldTag, "0001", 4) == 0)
		res->setRecordId(fieldArea.substr(hr->_dir[0]._fieldPos, 
					hr->_dir[0]._fieldLen));

	return res;
}

void S57Record::encode(S57Encoder &)
{
	fprintf(stderr, "(%d) Nothing to be encoded.\n", _recordType);
}

string S57Record::toString() const
{
	string s("=- S57 record -=\n");
	s += _header->toString();

	if (!_data.empty()) {
		s += "Field area\n";
		s += INDENT;
		s += strToPrintable(_data.substr(_header->_leader._fieldAreaOffset));
	}

	return s + "\n\n";
}

// S57DataDescripRecord members

S57DataDescripRecord::S57DataDescripRecord()
	: S57Record(DDR)
{
}

S57DataDescripRecord::S57DataDescripRecord(LRHeaderRef hr, string s, S57Module *mod)
	: S57Record(DDR, mod)
{
	setHeader(hr);

	_fieldControl = s.substr(_header->_dir[0]._fieldPos, 
							_header->_dir[0]._fieldLen);

	LRHeader::DirIterator it = _header->begin();
	for (; it != _header->end(); ++it)
		_descripFields.push_back(s.substr(it->_fieldPos, it->_fieldLen));
}

S57DataDescripRecord::~S57DataDescripRecord()
{
}

void S57DataDescripRecord::encode(S57Encoder &e)
{
	_header->encode(e);
	e.writeBlock(_fieldControl.data(), _fieldControl.size());
	StringList::const_iterator it = _descripFields.begin();
	for (; it != _descripFields.end(); ++it)
		e.writeBlock(it->data(), it->size());
}

string S57DataDescripRecord::toString() const
{
	string s("=- DDR -=\n");
	s += _header->toString();

	s += "Field control field\n";
	s += INDENT;
	s += _fieldControl;
	s += "\n";

	s += "Data descriptive fields\n";
	StringList::const_iterator it = _descripFields.begin();
	for (; it != _descripFields.end(); ++it) {
		s += INDENT;
		s += *it;
		s += "\n";
	}

	return s;
}

// S57DSInfoRecord members

void S57DSInfoRecord::init()
{
	_dsid = NULL;
	_dssi = NULL;
}

S57DSInfoRecord::S57DSInfoRecord()
	: S57Record(DatasetInformation)
{
	init();
}

S57DSInfoRecord::S57DSInfoRecord(LRHeaderRef hr, string s, S57Module *mod)
	: S57Record(DatasetInformation, mod)
{
	init();
	setHeader(hr);

	S57Decoder d;
	LRHeader::DirIterator it = _header->begin();
	for (; it != _header->end(); ++it) {
		const string tag(it->_fieldTag);
		d.setFieldData(s.substr(it->_fieldPos, it->_fieldLen));
		if (tag == "DSID")
			_dsid = new S57_DSID(d);
		else if (tag == "DSSI")
			_dssi = new S57_DSSI(d);
	}
}

S57DSInfoRecord::~S57DSInfoRecord()
{
	if (_dsid != NULL)
		delete _dsid;
	if (_dssi != NULL)
		delete _dssi;
}

void S57DSInfoRecord::encode(S57Encoder &e)
{
	_header->encode(e);

	if (!_recordId.empty())
		e.writeBlock(_recordId.data(), _recordId.size());
	if (_dsid != NULL) {
		_dsid->encode(e);
		e.endField();
	}
	if (_dssi != NULL) {
		_dssi->encode(e);
		e.endField();
	}
}

string S57DSInfoRecord::toString() const
{
	string s("=- Data set general information record -=\n");
	s += _header->toString();

	if (_dsid != NULL) {
		s += "| DSID | Data Set Identification |\n";
		s += _dsid->toString();
	}
	if (_dssi != NULL) {
		s += "| DSSI | Data Set Structure Information |\n";
		s += _dssi->toString();
	}

	return s;
}

// S57DSGeoRecord members

void S57DSGeoRecord::init()
{
	_dspm = NULL;
	_dspr = NULL;
	_dsrc = NULL;
}

S57DSGeoRecord::S57DSGeoRecord()
	: S57Record(DatasetGeographic)
{
	init();
}

S57DSGeoRecord::S57DSGeoRecord(LRHeaderRef hr, string s, S57Module *mod)
	: S57Record(DatasetGeographic, mod)
{
	init();
	setHeader(hr);

	S57Decoder d;
	LRHeader::DirIterator it = _header->begin();
	for (; it != _header->end(); ++it) {
		const string tag(it->_fieldTag);
		d.setFieldData(s.substr(it->_fieldPos, it->_fieldLen));
		if (tag == "DSPM")
			_dspm = new S57_DSPM(d);
		else if (tag == "DSPR")
			_dspr = new S57_DSPR(d);
		else if (tag == "DSRC")
			_dsrc = new S57_DSRC(d);
	}
}

S57DSGeoRecord::~S57DSGeoRecord()
{
	if (_dspm != NULL)
		delete _dspm;
	if (_dspr != NULL)
		delete _dspr;
	if (_dsrc != NULL)
		delete _dsrc;
}

void S57DSGeoRecord::encode(S57Encoder &e)
{
	_header->encode(e);

	if (!_recordId.empty())
		e.writeBlock(_recordId.data(), _recordId.size());
	if (_dspm != NULL) {
		_dspm->encode(e);
		e.endField();
	}
}

string S57DSGeoRecord::toString() const
{
	string s("=- Data set geographic refrence record -=\n");
	s += _header->toString();

	if (_dspm != NULL) {
		s += "| DSPM | Data Set Parameter |\n";
		s += _dspm->toString();
	}
	if (_dspr != NULL) {
		s += "| DSPR | Data Set Projection |\n";
		s += _dspr->toString();
	}
	if (_dsrc != NULL) {
		s += "| DSRC | Data Set Registration Control |\n";
		s += _dsrc->toString();
	}

	return s;
}

// S57DSHistoryRecord members

void S57DSHistoryRecord::init()
{
	_dsht = NULL;
}

S57DSHistoryRecord::S57DSHistoryRecord()
	: S57Record(DatasetHistory)
{
	init();
}

S57DSHistoryRecord::S57DSHistoryRecord(LRHeaderRef hr, string s, S57Module *mod)
	: S57Record(DatasetHistory, mod)
{
	init();
	setHeader(hr);

	LRHeader::DirIterator it = _header->begin();
	if (it != _header->end()) {
		S57Decoder d(s.substr(it->_fieldPos, it->_fieldLen));
		_dsht = new S57_DSHT(d);
	}
}

S57DSHistoryRecord::~S57DSHistoryRecord()
{
	if (_dsht != NULL)
		delete _dsht;
}

string S57DSHistoryRecord::toString() const
{
	string s("=- Data set history record -=\n");
	s += _header->toString();

	if (_dsht != NULL) {
		s += "| DSHT | Data Set History |\n";
		s += _dsht->toString();
	}

	return s;
}

// S57DSAccuracyRecord members

void S57DSAccuracyRecord::init()
{
	_dsac = NULL;
}

S57DSAccuracyRecord::S57DSAccuracyRecord()
	: S57Record(DatasetAccuracy)
{
	init();
}

S57DSAccuracyRecord::S57DSAccuracyRecord(LRHeaderRef hr, string s, S57Module *mod)
	: S57Record(DatasetAccuracy, mod)
{
	init();
	setHeader(hr);

	LRHeader::DirIterator it = _header->begin();
	if (it != _header->end()) {
		S57Decoder d(s.substr(it->_fieldPos, it->_fieldLen));
		_dsac = new S57_DSAC(d);
	}
}

S57DSAccuracyRecord::~S57DSAccuracyRecord()
{
	if (_dsac != NULL)
		delete _dsac;
}

string S57DSAccuracyRecord::toString() const
{
	string s("=- Data set accuracy record -=\n");
	s += _header->toString();

	if (_dsac != NULL) {
		s += "| DSAC | Data Set Accruacy |\n";
		s += _dsac->toString();
	}

	return s + "\n";
}

// S57CatalogDirRecord members

void S57CatalogDirRecord::init()
{
	_catd = NULL;
}

S57CatalogDirRecord::S57CatalogDirRecord()
	: S57Record(CatalogDirectory)
{
	init();
}

S57CatalogDirRecord::S57CatalogDirRecord(LRHeaderRef hr, string s, S57Module *mod)
	: S57Record(CatalogDirectory, mod)
{
	init();
	setHeader(hr);

	LRHeader::DirIterator it = _header->begin();
	if (it != _header->end()) {
		S57Decoder d(s.substr(it->_fieldPos, it->_fieldLen));
		_catd = new S57_CATD(d);
	}
}

S57CatalogDirRecord::~S57CatalogDirRecord()
{
	if (_catd != NULL)
		delete _catd;
}

string S57CatalogDirRecord::toString() const
{
	string s("=- Catalogue directory record -=\n");
	s += _header->toString();

	if (_catd != NULL) {
		s += "| CATD | Catalogue Directory |\n";
		s += _catd->toString();
	}

	return s;
}

// S57FeatureRecord members

void S57FeatureRecord::init()
{
	_frid = NULL;
	_foid = NULL;
	_ffpc = NULL;
	_fspc = NULL;
}

S57FeatureRecord::S57FeatureRecord()
	: S57Record(Feature)
{
	init();
}

S57FeatureRecord::S57FeatureRecord(LRHeaderRef hr, string s, S57Module *mod)
	: S57Record(Feature, mod)
{
	init();
	setHeader(hr);

	S57Decoder d;
	LRHeader::DirIterator it = _header->begin();
	for (; it != _header->end(); ++it) {
		const string tag(it->_fieldTag);
		d.setFieldData(s.substr(it->_fieldPos, it->_fieldLen));
		if (tag == "FRID")
			_frid = new S57_FRID(d);
		else if (tag == "FOID")
			_foid = new S57_LNAM(d);
		else if (tag == "ATTF") {
			/* XXX Wrong code!
			   while (!d.isEnd())
			   _attfs.push_back(S57_AttItem(d.getUInt(2), d.getString(_module->aall())));
			 */
			while (!d.isEnd()) {
				int attl = d.getUInt(2);
				string atvl = d.getString(_module->aall());
				_attfs.push_back(S57_AttItem(attl, atvl));
			}
		}
		else if (tag == "NATF") {
			/* XXX Wrong code!
			   while (!d.isEnd())
			   _natfs.push_back(S57_AttItem(d.getUInt(2), d.getString(_module->nall())));
			 */
			while (!d.isEnd()) {
				int attl = d.getUInt(2);
				string atvl = d.getString(_module->nall());
				_natfs.push_back(S57_AttItem(attl, atvl));
			}
		}
		else if (tag == "FFPC")
			_ffpc = new S57_UpdControl(d);
		else if (tag == "FFPT") {
			while (!d.isEnd())
				_ffpts.push_back(S57_FFPT(d));
		}
		else if (tag == "FSPC")
			_fspc = new S57_UpdControl(d);
		else if (tag == "FSPT") {
			while (!d.isEnd())
				_fspts.push_back(S57_FSPT(d));
		}
	}
}

S57FeatureRecord::~S57FeatureRecord()
{
	if (_frid != NULL)
		delete _frid;
	if (_foid != NULL)
		delete _foid;
	if (_ffpc != NULL)
		delete _ffpc;
	if (_fspc != NULL)
		delete _fspc;
}

void S57FeatureRecord::encode(S57Encoder &e)
{
	_header->encode(e);

	if (!_recordId.empty())
		e.writeBlock(_recordId.data(), _recordId.size());
	if (_frid != NULL) {
		_frid->encode(e);
		e.endField();
	}
	if (_foid != NULL) {
		e.setUInt(_foid->_agen, 2);
		e.setUInt(_foid->_fidn, 4);
		e.setUInt(_foid->_fids, 2);
		e.endField();
	}

	vector<S57_AttItem>::const_iterator it = _attfs.begin();
	for (; it != _attfs.end(); ++it) {
		e.setUInt(it->_attl, 2);
		e.setString(it->_atvl, _module->aall(), true);
	}
	if (!_attfs.empty())
		e.endField(_module->aall());

	it = _natfs.begin();
	for (; it != _natfs.end(); ++it) {
		e.setUInt(it->_attl, 2);
		e.setString(it->_atvl, _module->nall(), true);
	}
	if (!_natfs.empty())
		e.endField(_module->nall());

	if (_ffpc != NULL) { 
		_ffpc->encode(e);
		e.endField();
	}

	vector<S57_FFPT>::iterator jt = _ffpts.begin();
	for (; jt != _ffpts.end(); ++jt)
		jt->encode(e);
	if (!_ffpts.empty())
		e.endField();

	if (_fspc != NULL) {
		_fspc->encode(e);
		e.endField();
	}

	vector<S57_FSPT>::iterator kt = _fspts.begin();
	for (; kt != _fspts.end(); ++kt)
		kt->encode(e);
	if (!_fspts.empty())
		e.endField();
}

string S57FeatureRecord::toString() const
{
	string s("=- Feature record -=\n");
	s += _header->toString();

	if (_frid != NULL) {
		s += "| FRID | Feature Record Identifier |\n";
		s += _frid->toString();
	}
	if (_foid != NULL) {
		s += "| FOID | Feature Object Identifier |\n";
		s += _foid->toString();
	}
	if (!_attfs.empty()) {
		s += "| ATTF | Feature record attribute |\n";
		vector<S57_AttItem>::const_iterator it = _attfs.begin();
		for (; it != _attfs.end(); ++it)
			s += it->toString(_module->aall());
	}
	if (!_natfs.empty()) {
		s += "| NATF | Feature record national attribute |\n";
		vector<S57_AttItem>::const_iterator it = _natfs.begin();
		for (; it != _natfs.end(); ++it)
			s += it->toString(_module->nall());
	}
	if (_ffpc != NULL) {
		s += " | FFPC [Upd] | Feature Record to Feature Object Pointer Control |\n";
		s += _ffpc->toString();
	}
	if (!_ffpts.empty()) {
		s += "| FFPT | Feature Record to Feature Object Pointer |\n";
		vector<S57_FFPT>::const_iterator it = _ffpts.begin();
		for (; it != _ffpts.end(); ++it)
			s += it->toString();
	}
	if (_fspc != NULL) {
		s += "| FSPC [Upd] | Feature Record to Spatial Record Pointer Control |\n";
		s += _fspc->toString();
	}
	if (!_fspts.empty()) {
		s += "| FSPT | Feature Record to Spatial Record Pointer |\n";
		vector<S57_FSPT>::const_iterator it = _fspts.begin();
		for (; it != _fspts.end(); ++it)
			s += it->toString();
	}

	return s;
}

void S57FeatureRecord::markDeleted()
{
	if (_frid != NULL)
		_frid->_ruin = S57_UI_D;
}

bool S57FeatureRecord::isDeleted() const
{
	return _frid == NULL || _frid->_ruin == S57_UI_D;
}

void S57FeatureRecord::update(Ref<S57FeatureRecord> upr)
{
	assert(!upr.isNull());
	assert(upr->fieldFRID() != NULL);
	assert(_frid != NULL);

	const S57_FRID *up_frid = upr->fieldFRID();

	if (_frid->_rver != up_frid->_rver - 1)
		fprintf(stderr, "Unordered update record: %s\n", up_frid->_name.toString().c_str());

	updateAttributes(_attfs, upr->fieldsATTF());
	updateAttributes(_natfs, upr->fieldsNATF());

	if (upr->fieldFFPC() != NULL)
		updatePointers(_ffpts, upr->fieldsFFPT(), upr->fieldFFPC());
	if (upr->fieldFSPC() != NULL)
		updatePointers(_fspts, upr->fieldsFSPT(), upr->fieldFSPC());

	// Update the record version
	_frid->_rver = up_frid->_rver;
}

// S57VectorRecord members

void S57VectorRecord::init()
{
	_vrid = NULL;
	_vrpc = NULL;
	_sgcc = NULL;
	_coordType = S57VectorRecord::SG2D;
}

S57VectorRecord::S57VectorRecord()
	: S57Record(Vector)
{
	init();
}

S57VectorRecord::S57VectorRecord(LRHeaderRef hr, string s, S57Module *mod)
	: S57Record(Vector, mod)
{
	init();
	setHeader(hr);

	S57Decoder d;
	LRHeader::DirIterator it = _header->begin();
	for (; it != _header->end(); ++it) {
		const string tag(it->_fieldTag);
		d.setFieldData(s.substr(it->_fieldPos, it->_fieldLen));
		if (tag == "VRID")
			_vrid = new S57_VRID(d);
		else if (tag == "ATTV") {
			/* XXX Wrong code!
			   while (!d.isEnd())
			   _attvs.push_back(S57_AttItem(d.getUInt(2), d.getString(S57_LL0)));
			 */
			while (!d.isEnd()) {
				int attl = d.getUInt(2);
				string atvl = d.getString(S57_LL0); // the string domain is always "Basic Text"
				_attvs.push_back(S57_AttItem(attl, atvl));
			}
		}
		else if (tag == "VRPC")
			_vrpc = new S57_UpdControl(d);
		else if (tag == "VRPT") {
			while (!d.isEnd())
				_vrpts.push_back(S57_VRPT(d));
		}
		else if (tag == "SGCC")
			_sgcc = new S57_UpdControl(d);
		else if (tag == "SG2D") {
			_coordType = S57VectorRecord::SG2D;
			while (!d.isEnd())
				_coords.push_back(d.getSInt(4));
			assert(_coords.size() % 2 == 0);
		}
		else if (tag == "SG3D") {
			_coordType = S57VectorRecord::SG3D;
			while (!d.isEnd())
				_coords.push_back(d.getSInt(4));
			assert(_coords.size() % 3 == 0);
		}
	}
}

S57VectorRecord::~S57VectorRecord()
{
	if (_vrid != NULL)
		delete _vrid;
	if (_vrpc != NULL)
		delete _vrpc;
	if (_sgcc != NULL)
		delete _sgcc;
}

void S57VectorRecord::encode(S57Encoder &e)
{
	_header->encode(e);

	if (!_recordId.empty())
		e.writeBlock(_recordId.data(), _recordId.size());

	if (_vrid != NULL) {
		_vrid->encode(e);
		e.endField();
	}

	vector<S57_AttItem>::const_iterator it = _attvs.begin();
	for (; it != _attvs.end(); ++it) {
		e.setUInt(it->_attl, 2);
		e.setString(it->_atvl, _module->aall(), true);
	}
	if (!_attvs.empty())
		e.endField(_module->aall());

	vector<S57_VRPT>::iterator jt = _vrpts.begin();
	for (; jt != _vrpts.end(); ++jt)
		jt->encode(e);
	if (!_vrpts.empty())
		e.endField();

	if (_sgcc != NULL) {
		_sgcc->encode(e);
		e.endField();
	}

	vector<s57_b24>::const_iterator kt = _coords.begin();
	for (; kt != _coords.end(); ++kt)
		e.setSInt(*kt, 4);
	if (!_coords.empty())
		e.endField();
}

string S57VectorRecord::toString() const
{
	string s("=- Vector record -=\n");
	s += _header->toString();

	if (_vrid != NULL) {
		s += "| VRID | Vector Record Identifier |\n";
		s += _vrid->toString();
	}
	if (!_attvs.empty()) {
		s += "| ATTV | Vector record attribute |\n";
		vector<S57_AttItem>::const_iterator it = _attvs.begin();
		for (; it != _attvs.end(); ++it)
			s += it->toString(_module->aall());
	}
	if (_vrpc != NULL) {
		s += "| VRPC [Upd] | Vector Record Pointer Control |\n";
		s += _vrpc->toString();
	}
	if (!_vrpts.empty()) {
		s += "| VRPT | Vector Record Pointer |\n";
		vector<S57_VRPT>::const_iterator it = _vrpts.begin();
		for (; it != _vrpts.end(); ++it)
			s += it->toString();
	}
	if (_sgcc != NULL) {
		s += "| SGCC [Upd] | Coordinate control |\n";
		s += _sgcc->toString();
	}
	if (!_coords.empty()) {
		if (_coordType == SG2D)
			s += "| SG2D | 2-D Coordinate |\n";
		else if (_coordType == SG3D)
			s += "| SG3D | 3-D Coordinate |\n";
		else
			assert(0);
		int i = 0;
		ostrstream os;
		while (i < static_cast<int>(_coords.size())) {
			os << INDENT << "*YCOO: " << _coords[i++] << endl;
			os << INDENT << "XCOO:  " << _coords[i++] << endl;
			if (_coordType == SG3D)
				os << INDENT << "VE3D:  " << _coords[i++] << endl;
		}
		os << ends;
		char *tmp = os.str();
		s += tmp;
		delete[] tmp;
	}

	return s;
}

void S57VectorRecord::markDeleted()
{
	if (_vrid != NULL)
		_vrid->_ruin = S57_UI_D;
}

bool S57VectorRecord::isDeleted() const
{
	return _vrid == NULL || _vrid->_ruin == S57_UI_D;
}

void S57VectorRecord::update(Ref<S57VectorRecord> upr)
{
	assert(!upr.isNull());
	assert(upr->fieldVRID() != NULL);
	assert(_vrid != NULL);

	const S57_VRID *up_vrid = upr->fieldVRID();

	if (_vrid->_rver != up_vrid->_rver - 1)
		fprintf(stderr, "Unordered update record: %s\n", up_vrid->_name.toString().c_str());

	updateAttributes(_attvs, upr->fieldsATTV());

	if (upr->fieldVRPC() != NULL)
		updatePointers(_vrpts, upr->fieldsVRPT(), upr->fieldVRPC());

	if (upr->fieldSGCC() != NULL)
		updateCoords(_coords, upr->coords(), 
				_coordType == S57VectorRecord::SG2D ? 2 : 3, 
				upr->fieldSGCC());

	// Update the record version
	_vrid->_rver = up_vrid->_rver;
}

// ~

void updateAttributes(vector<S57_AttItem> &dst, const vector<S57_AttItem> &src)
{
	vector<S57_AttItem>::const_iterator s_it = src.begin();
	for (; s_it != src.end(); ++s_it) {
		vector<S57_AttItem>::iterator d_it = dst.begin();
		for (; d_it != dst.end(); ++d_it)
			if (s_it->_attl == d_it->_attl)
				break;

		if (d_it == dst.end())
			dst.push_back(*s_it);
		else {
			string s_atvl = s_it->_atvl;
			if ((s_atvl.size() == 1 && s_atvl[0] == 0x7f)
				|| (s_atvl.size() == 2 && *reinterpret_cast<const s57_b12 *>(s_atvl.data()) == 0x007f))
				dst.erase(d_it);
			else
				d_it->_atvl = s_atvl;
		}
	}
}

template <class T>
void updatePointers(vector<T> &dst, const vector<T> &src, const S57_UpdControl *ctrl)
{
	assert(ctrl != NULL && ctrl->_index > 0);

	// NOTE: the index starts from 1
	class vector<T>::iterator pos;
	if ((ctrl->_index - 1) <= static_cast<int>(dst.size()))
		pos = dst.begin() + (ctrl->_index - 1);
	else
		return;

	if (ctrl->_instruction == S57_UI_I) {
		class vector<T>::const_iterator src_endpos = src.begin() + ctrl->_count;
		if (src_endpos > src.end())
			src_endpos = src.end();
		dst.insert(pos, src.begin(), src_endpos);
	}
	else if (ctrl->_instruction == S57_UI_D) {
		class vector<T>::iterator endpos = pos + ctrl->_count;
		if (endpos > dst.end())
			endpos = dst.end();
		dst.erase(pos, endpos);
	}
	else if (ctrl->_instruction == S57_UI_M) {
		int i = 0;
		int j = ctrl->_index - 1;
		while (i < static_cast<int>(ctrl->_count) 
				&& i < static_cast<int>(src.size()) 
				&& j < static_cast<int>(dst.size()))
			dst[j++] = src[i++];
	}
}

void updateCoords(vector<s57_b24> &dst, const vector<s57_b24> &src, 
				int pairSize, const S57_UpdControl *ctrl)
{
	assert(ctrl != NULL && ctrl->_index > 0);

	vector<s57_b24>::iterator pos;
	if ((ctrl->_index - 1) <= static_cast<int>(dst.size()) / pairSize)
		pos = dst.begin() + (ctrl->_index - 1) * pairSize;
	else
		return;

	if (ctrl->_instruction == S57_UI_I) {
		vector<s57_b24>::const_iterator src_endpos = src.begin() + ctrl->_count * pairSize;
		if (src_endpos > src.end())
			src_endpos = src.end();
		dst.insert(pos, src.begin(), src_endpos);
	}
	else if (ctrl->_instruction == S57_UI_D) {
		vector<s57_b24>::iterator endpos = pos + ctrl->_count * pairSize;
		if (endpos > dst.end())
			endpos = dst.end();
		dst.erase(pos, endpos);
	}
	else if (ctrl->_instruction == S57_UI_M) {
		int i = 0;
		int j = (ctrl->_index - 1) * pairSize;
		while (i < static_cast<int>(ctrl->_count * pairSize) 
				&& i < static_cast<int>(src.size()) 
				&& j < static_cast<int>(dst.size()))
			dst[j++] = src[i++];
	}
}
