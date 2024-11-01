#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <strstream>
#include <iomanip>

#include "../tools/LString.h"

#include "s57_utils.h"
#include "assure_fio.h"
#include "s57_field_codec.h"

using namespace std;
using namespace MyTools;

// S57_NAME members

string S57_NAME::toString() const
{
	ostrstream os;
	os << (int)_rcnm << ':' << _rcid << ends;

	char *tmp = os.str();
	string result(tmp);
	delete[] tmp;
	return result;
}

// S57_LNAM members

S57_LNAM::S57_LNAM(S57Decoder &d)
{
	_agen = d.getUInt(2);
	_fidn = d.getUInt(4);
	_fids = d.getUInt(2);
}

string S57_LNAM::toString(bool repeat) const
{
	ostrstream os;
	os << INDENT << (repeat ? "*" : "") << "LNAM:" << (repeat ? " " : "  ")
		<< _agen << ':' << _fidn << ':' << _fids << endl << ends;

	char *tmp = os.str();
	string result(tmp);
	delete[] tmp;
	return result;
}

// S57_Date members

string S57_Date::toString() const
{
	ostrstream os;
	os << setfill('0') << setw(4) << _year << '/'
		<< setw(2) << _month << '/' << _day << ends;

	char *tmp = os.str();
	string result(tmp);
	delete[] tmp;
	return result;
}

// S57_AttItem members

string S57_AttItem::toString(int llcode) const
{
	ostrstream os;
	os << INDENT << "*ATTL: " << _attl << endl
		<< INDENT << "ATVL:  ";
	if (llcode == S57_LL2)
		os << LString::fromUcs2(_atvl.data(), _atvl.size(), false).toUtf8();
	else 
		os << _atvl;
	os << endl << ends;

	char *tmp = os.str();
	string result(tmp);
	delete[] tmp;
	return result;
}

// S57_UpdControl members

S57_UpdControl::S57_UpdControl()
{
	_instruction = 0;
	_index = 0;
	_count = 0;
}

S57_UpdControl::S57_UpdControl(S57Decoder &d)
{
	_instruction = d.getUInt(1);
	_index = d.getUInt(2);
	_count = d.getUInt(2);
}

void S57_UpdControl::encode(S57Encoder &e)
{
	e.setUInt(_instruction, 1);
	e.setUInt(_index, 2);
	e.setUInt(_count, 2);
}

string S57_UpdControl::toString() const
{
	ostrstream os;
	os << INDENT << "Instruction: " << (int)_instruction << endl
		<< INDENT << "Index: " << _index << endl
		<< INDENT << "Count: " << _count << endl << ends;

	char *tmp = os.str();
	string result(tmp);
	delete[] tmp;
	return result;
}

// S57_DSID members

S57_DSID::S57_DSID()
{
	_expp = 0;
	_intu = 0;
	_prsp = 0;
	_prof = 0;
	_agen = 0;
}

S57_DSID::S57_DSID(S57Decoder &d)
{
	_name._rcnm = d.getUInt(1);
	_name._rcid = d.getUInt(4);
	_expp = d.getUInt(1);
	_intu = d.getUInt(1);
	_dsnm = d.getString(S57_LL0);
	_edtn = strToInt(d.getString(S57_LL0));
	_updn = strToInt(d.getString(S57_LL0));
	_uadt = d.getDate();
	_isdt = d.getDate();
	_sted = d.getString(S57_LL0, 4);
	_prsp = d.getUInt(1);
	_psdn = d.getString(S57_LL0);
	_pred = d.getString(S57_LL0);
	_prof = d.getUInt(1);
	_agen = d.getUInt(2);
	_comt = d.getString(S57_LL0);
}

void S57_DSID::encode(S57Encoder &e)
{
	e.setUInt(_name._rcnm, 1);
	e.setUInt(_name._rcid, 4);
	e.setUInt(_expp, 1);
	e.setUInt(_intu, 1);
	e.setString(_dsnm, S57_LL0, true);
	e.setString(intToStr(_edtn), S57_LL0, true);
	e.setString(intToStr(_updn), S57_LL0, true);
	e.setDate(_uadt);
	e.setDate(_isdt);
	e.setString(_sted, S57_LL0, false);
	e.setUInt(_prsp, 1);
	e.setString(_psdn, S57_LL0, true);
	e.setString(_pred, S57_LL0, true);
	e.setUInt(_prof, 1);
	e.setUInt(_agen, 2);
	e.setString(_comt, S57_LL0, true);
}

string S57_DSID::toString() const
{
	ostrstream os;
	os << INDENT << "NAME:  " << _name.toString() << endl
		<< INDENT << "EXPP:  " << (int)_expp << endl
		<< INDENT << "INTU:  " << (int)_intu << endl
		<< INDENT << "DSNM:  " << _dsnm << endl
		<< INDENT << "EDTN:  " << _edtn << endl
		<< INDENT << "UPDN:  " << _updn << endl
		<< INDENT << "UADT:  " << _uadt.toString() << endl
		<< INDENT << "ISDT:  " << _isdt.toString() << endl
		<< INDENT << "STED:  " << _sted << endl
		<< INDENT << "PRSP:  " << (int)_prsp << endl
		<< INDENT << "PSDN:  " << _psdn << endl
		<< INDENT << "PRED:  " << _pred << endl
		<< INDENT << "PROF:  " << (int)_prof << endl
		<< INDENT << "AGEN:  " << _agen << endl
		<< INDENT << "COMT:  " << _comt << endl << ends;

	char *tmp = os.str();
	string result(tmp);
	delete[] tmp;
	return result;
}

// S57_DSSI members

S57_DSSI::S57_DSSI()
{
	_dstr = 0;
	_aall = 0;
	_nall = 0;
	_nomr = 0;
	_nocr = 0;
	_nogr = 0;
	_nolr = 0;
	_noin = 0;
	_nocn = 0;
	_noed = 0;
	_nofa = 0;
}

S57_DSSI::S57_DSSI(S57Decoder &d)
{
	_dstr = d.getUInt(1);
	_aall = d.getUInt(1);
	_nall = d.getUInt(1);
	_nomr = d.getUInt(4);
	_nocr = d.getUInt(4);
	_nogr = d.getUInt(4);
	_nolr = d.getUInt(4);
	_noin = d.getUInt(4);
	_nocn = d.getUInt(4);
	_noed = d.getUInt(4);
	_nofa = d.getUInt(4);
}

void S57_DSSI::encode(S57Encoder &e)
{
	e.setUInt(_dstr, 1);
	e.setUInt(_aall, 1);
	e.setUInt(_nall, 1);
	e.setUInt(_nomr, 4);
	e.setUInt(_nocr, 4);
	e.setUInt(_nogr, 4);
	e.setUInt(_nolr, 4);
	e.setUInt(_noin, 4);
	e.setUInt(_nocn, 4);
	e.setUInt(_noed, 4);
	e.setUInt(_nofa, 4);
}

string S57_DSSI::toString() const
{
	ostrstream os;
	os << INDENT << "DSTR:  " << (int)_dstr << endl
		<< INDENT << "AALL:  " << (int)_aall << endl
		<< INDENT << "NALL:  " << (int)_nall << endl
		<< INDENT << "NOMR:  " << _nomr << endl
		<< INDENT << "NOCR:  " << _nocr << endl
		<< INDENT << "NOGR:  " << _nogr << endl
		<< INDENT << "NOLR:  " << _nolr << endl
		<< INDENT << "NOIN:  " << _noin << endl
		<< INDENT << "NOCN:  " << _nocn << endl
		<< INDENT << "NOED:  " << _noed << endl
		<< INDENT << "NOFA:  " << _nofa << endl << ends;

	char *tmp = os.str();
	string result(tmp);
	delete[] tmp;
	return result;
}

// S57_DSPM members

S57_DSPM::S57_DSPM()
{
	_hdat = 0;
	_vdat = 0;
	_sdat = 0;
	_cscl = 0;
	_duni = 0;
	_huni = 0;
	_puni = 0;
	_coun = 0;
	_comf = 0;
	_somf = 0;
}

S57_DSPM::S57_DSPM(S57Decoder &d)
{
	_name._rcnm = d.getUInt(1);
	_name._rcid = d.getUInt(4);
	_hdat = d.getUInt(1);
	_vdat = d.getUInt(1);
	_sdat = d.getUInt(1);
	_cscl = d.getUInt(4);
	_duni = d.getUInt(1);
	_huni = d.getUInt(1);
	_puni = d.getUInt(1);
	_coun = d.getUInt(1);
	_comf = d.getUInt(4);
	_somf = d.getUInt(4);
	_comt = d.getString(S57_LL0);
}

void S57_DSPM::encode(S57Encoder &e)
{
	e.setUInt(_name._rcnm, 1);
	e.setUInt(_name._rcid, 4);
	e.setUInt(_hdat, 1);
	e.setUInt(_vdat, 1);
	e.setUInt(_sdat, 1);
	e.setUInt(_cscl, 4);
	e.setUInt(_duni, 1);
	e.setUInt(_huni, 1);
	e.setUInt(_puni, 1);
	e.setUInt(_coun, 1);
	e.setUInt(_comf, 4);
	e.setUInt(_somf, 4);
	e.setString(_comt, S57_LL0, true);
}

string S57_DSPM::toString() const
{
	ostrstream os;
	os << INDENT << "NAME:  " << _name.toString() << endl
		<< INDENT << "HDAT:  " << (int)_hdat << endl
		<< INDENT << "VDAT:  " << (int)_vdat << endl
		<< INDENT << "SDAT:  " << (int)_sdat << endl
		<< INDENT << "CSCL:  " << _cscl << endl
		<< INDENT << "DUNI:  " << (int)_duni << endl
		<< INDENT << "HUNI:  " << (int)_huni << endl
		<< INDENT << "PUNI:  " << (int)_puni << endl
		<< INDENT << "COUN:  " << (int)_coun << endl
		<< INDENT << "COMF:  " << _comf << endl
		<< INDENT << "SOMF:  " << _somf << endl
		<< INDENT << "COMT:  " << _comt << endl << ends;

	char *tmp = os.str();
	string result(tmp);
	delete[] tmp;
	return result;
}

// S57_DSPR members

S57_DSPR::S57_DSPR()
{
	_proj = 0;
	_prp1 = 0;
	_prp2 = 0;
	_prp3 = 0;
	_prp4 = 0;
	_feas = 0;
	_fnor = 0;
	_fpmf = 0;
}

S57_DSPR::S57_DSPR(S57Decoder &d)
{
	_proj = d.getUInt(1);
	_prp1 = d.getSInt(4);
	_prp2 = d.getSInt(4);
	_prp3 = d.getSInt(4);
	_prp4 = d.getSInt(4);
	_feas = d.getSInt(4);
	_fnor = d.getSInt(4);
	_fpmf = d.getUInt(4);
	_comt = d.getString(S57_LL0);
}

string S57_DSPR::toString() const
{
	ostrstream os;
	os << INDENT << "PROJ:  " << (int)_proj << endl
		<< INDENT << "PRP1:  " << _prp1 << endl
		<< INDENT << "PRP2:  " << _prp2 << endl
		<< INDENT << "PRP3:  " << _prp3 << endl
		<< INDENT << "PRP4:  " << _prp4 << endl
		<< INDENT << "FEAS:  " << _feas << endl
		<< INDENT << "FNOR:  " << _fnor << endl
		<< INDENT << "FPMF:  " << _fpmf << endl
		<< INDENT << "COMT:  " << _comt << endl << ends;

	char *tmp = os.str();
	string result(tmp);
	delete[] tmp;
	return result;
}

// S57_DSRC members

S57_DSRC::S57_DSRC()
{
	_rpid = 0;
	_ryco = 0;
	_rxco = 0;
	_curp = 0;
	_fpmf = 0;
	_rxvl = 0;
	_ryvl = 0;
}

S57_DSRC::S57_DSRC(S57Decoder &d)
{
	_rpid = d.getUInt(1);
	_ryco = d.getSInt(4);
	_rxco = d.getSInt(4);
	_curp = d.getUInt(1);
	_fpmf = d.getUInt(4);
	_rxvl = d.getSInt(4);
	_ryvl = d.getSInt(4);
	_comt = d.getString(S57_LL0);
}

string S57_DSRC::toString() const
{
	ostrstream os;
	os << INDENT << "RPID:  " << (int)_rpid << endl
		<< INDENT << "RYCO:  " << _ryco << endl
		<< INDENT << "RXCO:  " << _rxco << endl
		<< INDENT << "CURP:  " << (int)_curp << endl
		<< INDENT << "FPMF:  " << _fpmf << endl
		<< INDENT << "RXVL:  " << _rxvl << endl
		<< INDENT << "RYVL:  " << _ryvl << endl
		<< INDENT << "COMT:  " << _comt << endl << ends;

	char *tmp = os.str();
	string result(tmp);
	delete[] tmp;
	return result;
}

// S57_DSHT members

S57_DSHT::S57_DSHT()
{ 
	_prco = 0; 
}

S57_DSHT::S57_DSHT(S57Decoder &d)
{
	_name._rcnm = d.getUInt(1);
	_name._rcid = d.getUInt(4);
	_prco = d.getUInt(2);
	_esdt = d.getDate();
	_lsdt = d.getDate();
	_dcrt = d.getString(S57_LL0);
	_codt = d.getDate();
	_comt = d.getString(S57_LL0);
}

string S57_DSHT::toString() const
{
	ostrstream os;
	os << INDENT << "NAME:  " << _name.toString() << endl
		<< INDENT << "PRCO:  " << _prco << endl
		<< INDENT << "ESDT:  " << _esdt.toString() << endl
		<< INDENT << "LSDT:  " << _lsdt.toString() << endl
		<< INDENT << "DCRT:  " << _dcrt << endl
		<< INDENT << "CODT:  " << _codt.toString() << endl
		<< INDENT << "COMT:  " << _comt << endl << ends;

	char *tmp = os.str();
	string result(tmp);
	delete[] tmp;
	return result;
}

// S57_DSAC members

S57_DSAC::S57_DSAC()
{
	_pacc = 0;
	_hacc = 0;
	_sacc = 0;
	_fpmf = 0;
}

S57_DSAC::S57_DSAC(S57Decoder &d)
{
	_name._rcnm = d.getUInt(1);
	_name._rcid = d.getUInt(4);
	_pacc = d.getUInt(4);
	_hacc = d.getUInt(4);
	_sacc = d.getUInt(4);
	_fpmf = d.getUInt(4);
	_comt = d.getString(S57_LL0);
}

string S57_DSAC::toString() const
{
	ostrstream os;
	os << INDENT << "NAME:  " << _name.toString() << endl
		<< INDENT << "PACC:  " << _pacc << endl
		<< INDENT << "HACC:  " << _hacc << endl
		<< INDENT << "SACC:  " << _sacc << endl
		<< INDENT << "FPMF:  " << _fpmf << endl
		<< INDENT << "COMT:  " << _comt << endl << ends;

	char *tmp = os.str();
	string result(tmp);
	delete[] tmp;
	return result;
}

// S57_CATD members

S57_CATD::S57_CATD()
{
	_slat = 0.0;
	_wlon = 0.0;
	_nlat = 0.0;
	_elon = 0.0;
}

S57_CATD::S57_CATD(S57Decoder &d)
{
	_name._rcnm = strToInt(d.getString(S57_LL0, 2));
	_name._rcid = strToInt(d.getString(S57_LL0, 10));
	_file = d.getString(S57_LL0);
	_lfil = d.getString(S57_LL0);
	_volm = d.getString(S57_LL0);
	_impl = d.getString(S57_LL0, 3);
	_slat = strToFloat(d.getString(S57_LL0));
	_wlon = strToFloat(d.getString(S57_LL0));
	_nlat = strToFloat(d.getString(S57_LL0));
	_elon = strToFloat(d.getString(S57_LL0));
	_crcs = d.getString(S57_LL0);
	_comt = d.getString(S57_LL0);
}

string S57_CATD::toString() const
{
	ostrstream os;
	os << INDENT << "NAME:  " << _name.toString() << endl
		<< INDENT << "FILE:  " << _file << endl
		<< INDENT << "LFIL:  " << _lfil << endl
		<< INDENT << "VOLM:  " << _volm << endl
		<< INDENT << "IMPL:  " << _impl << endl
		<< INDENT << "SLAT:  " << _slat << endl
		<< INDENT << "WLON:  " << _wlon << endl
		<< INDENT << "NLAT:  " << _nlat << endl
		<< INDENT << "ELON:  " << _elon << endl
		<< INDENT << "CRCS:  " << _crcs << endl
		<< INDENT << "COMT:  " << _comt << endl << ends;

	char *tmp = os.str();
	string result(tmp);
	delete[] tmp;
	return result;
}

// S57_FRID members

S57_FRID::S57_FRID()
{
	_prim = 0;
	_grup = 0;
	_objl = 0;
	_rver = 0;
	_ruin = 0;
}

S57_FRID::S57_FRID(S57Decoder &d)
{
	_name._rcnm = d.getUInt(1);
	_name._rcid = d.getUInt(4);
	_prim = d.getUInt(1);
	_grup = d.getUInt(1);
	_objl = d.getUInt(2);
	_rver = d.getUInt(2);
	_ruin = d.getUInt(1);
}

void S57_FRID::encode(S57Encoder &e)
{
	e.setUInt(_name._rcnm, 1);
	e.setUInt(_name._rcid, 4);
	e.setUInt(_prim, 1);
	e.setUInt(_grup, 1);
	e.setUInt(_objl, 2);
	e.setUInt(_rver, 2);
	e.setUInt(_ruin, 1);
}

string S57_FRID::toString() const
{
	ostrstream os;
	os << INDENT << "NAME:  " << _name.toString() << endl
		<< INDENT << "PRIM:  " << (int)_prim << endl
		<< INDENT << "GRUP:  " << (int)_grup << endl
		<< INDENT << "OBJL:  " << _objl << endl
		<< INDENT << "RVER:  " << _rver << endl
		<< INDENT << "RUIN:  " << (int)_ruin << endl << ends;

	char *tmp = os.str();
	string result(tmp);
	delete[] tmp;
	return result;
}

// S57_FFPT members

S57_FFPT::S57_FFPT()
{
	_rind = 0;
}

S57_FFPT::S57_FFPT(S57Decoder &d)
{
	_lnam = d.unpackLongName();
	_rind = d.getUInt(1); 
	_comt = d.getString(S57_LL0);
}

void S57_FFPT::encode(S57Encoder &e)
{
	e.packLongName(_lnam);
	e.setUInt(_rind, 1);
	e.setString(_comt, S57_LL0, true);
}

string S57_FFPT::toString() const
{
	ostrstream os;
	os << _lnam.toString(true) 
		<< INDENT << "RIND:  " << (int)_rind << endl
		<< INDENT << "COMT:  " << _comt << endl << ends;

	char *tmp = os.str();
	string result(tmp);
	delete[] tmp;
	return result;
}

// S57_FSPT members

S57_FSPT::S57_FSPT()
{
	_ornt = 0;
	_usag = 0;
	_mask = 0;
}

S57_FSPT::S57_FSPT(S57Decoder &d)
{
	_name = d.unpackName();
	_ornt = d.getUInt(1);
	_usag = d.getUInt(1);
	_mask = d.getUInt(1);
}

void S57_FSPT::encode(S57Encoder &e)
{
	e.packName(_name);
	e.setUInt(_ornt, 1);
	e.setUInt(_usag, 1);
	e.setUInt(_mask, 1);
}

string S57_FSPT::toString() const
{
	ostrstream os;
	os << INDENT << "*NAME: " << _name.toString() << endl
		<< INDENT << "ORNT:  " << (int)_ornt << endl
		<< INDENT << "USAG:  " << (int)_usag << endl
		<< INDENT << "MASK:  " << (int)_mask << endl << ends;

	char *tmp = os.str();
	string result(tmp);
	delete[] tmp;
	return result;
}

// S57_VRID members

S57_VRID::S57_VRID()
{
	_rver = 0;
	_ruin = 0;
}

S57_VRID::S57_VRID(S57Decoder &d)
{
	_name._rcnm = d.getUInt(1);
	_name._rcid = d.getUInt(4);
	_rver = d.getUInt(2);
	_ruin = d.getUInt(1);
}

void S57_VRID::encode(S57Encoder &e)
{
	e.setUInt(_name._rcnm, 1);
	e.setUInt(_name._rcid, 4);
	e.setUInt(_rver, 2);
	e.setUInt(_ruin, 1);
}

string S57_VRID::toString() const
{
	ostrstream os;
	os << INDENT << "NAME:  " << _name.toString() << endl
		<< INDENT << "RVER:  " << _rver << endl
		<< INDENT << "RUIN:  " << (int)_ruin << endl << ends;

	char *tmp = os.str();
	string result(tmp);
	delete[] tmp;
	return result;
}

// S57_VRPT members

S57_VRPT::S57_VRPT()
{
	_ornt = 0;
	_usag = 0;
	_topi = 0;
	_mask = 0;
}

S57_VRPT::S57_VRPT(S57Decoder &d)
{
	_name = d.unpackName();
	_ornt = d.getUInt(1);
	_usag = d.getUInt(1);
	_topi = d.getUInt(1);
	_mask = d.getUInt(1);
}

void S57_VRPT::encode(S57Encoder &e)
{
	e.packName(_name);
	e.setUInt(_ornt, 1);
	e.setUInt(_usag, 1);
	e.setUInt(_topi, 1);
	e.setUInt(_mask, 1);
}

string S57_VRPT::toString() const
{
	ostrstream os;
	os << INDENT << "*NAME: " << _name.toString() << endl
		<< INDENT << "ORNT:  " << (int)_ornt << endl
		<< INDENT << "USAG:  " << (int)_usag << endl
		<< INDENT << "TOPI:  " << (int)_topi << endl
		<< INDENT << "MASK:  " << (int)_mask << endl << ends;

	char *tmp = os.str();
	string result(tmp);
	delete[] tmp;
	return result;
}

// S57_SG2D members

// S57_SG3D members

// S57Decoder members

void S57Decoder::init()
{
	_buf = NULL;
	_curptr = NULL;
	_endptr = NULL;
}

#ifndef NDEBUG
void S57Decoder::checkCurPtr(const char *func)
{
	if (_curptr >= _endptr) {
		fprintf(stderr, "*** %s: check current ptr fault (overflow)\n", func);
		assert(0);
	}

	if ((_curptr == _endptr - 1)
		&& (*_curptr != S57_FT && *_curptr != S57_UT)) {
		fprintf(stderr, "*** %s: check current ptr fault (no terminator)\n", func);
		assert(0);
	}
}
#endif

S57Decoder::S57Decoder()
{
	init();
}

S57Decoder::S57Decoder(string fieldData)
{
	init();
	setFieldData(fieldData);
}

S57Decoder::~S57Decoder()
{
	if (_buf != NULL)
		delete[] _buf;
}

void S57Decoder::setFieldData(string s)
{
	if (_buf != NULL)
		delete[] _buf;

	size_t bufsz = s.size();
	_buf = new char [bufsz];
	if (_buf == NULL)
		bufsz = 0;

	memcpy(_buf, s.data(), bufsz);
	_curptr = _buf;
	_endptr = _curptr + bufsz;
}

s57_b24 S57Decoder::getSInt(int width)
{
	s57_b24 n = 0;

	if (isEnd()) {
		fprintf(stderr, "*** Unexpected end of field\n");
		return 0;
	}

	switch (width) {
	case 1:
		n = *reinterpret_cast<s57_b21 *>(_curptr);
		_curptr += 1;
		break;
	case 2:
		n = *reinterpret_cast<s57_b22 *>(_curptr);
		_curptr += 2;
		break;
	case 4:
		n = *reinterpret_cast<s57_b24 *>(_curptr);
		_curptr += 4;
		break;
	default:
		assert(0);
		break;
	}

#ifndef NDEBUG
	checkCurPtr(__FUNCTION__);
#endif
	return n;
}

s57_b14 S57Decoder::getUInt(int width)
{
	s57_b14 n = 0;

	if (isEnd()) {
		fprintf(stderr, "*** Unexpected end of field\n");
		return 0;
	}

	switch (width) {
	case 1:
		n = *reinterpret_cast<s57_b11 *>(_curptr);
		_curptr += 1;
		break;
	case 2:
		n = *reinterpret_cast<s57_b12 *>(_curptr);
		_curptr += 2;
		break;
	case 4:
		n = *reinterpret_cast<s57_b14 *>(_curptr);
		_curptr += 4;
		break;
	default:
		assert(0);
		break;
	}

#ifndef NDEBUG
	checkCurPtr(__FUNCTION__);
#endif
	return n;
}

string S57Decoder::getString(int ll, int len)
{
	string s;

	if (isEnd()) {
		fprintf(stderr, "*** Unexpected end of field\n");
		return s;
	}

	if (len != -1) {
		s.assign(_curptr, len);
		_curptr += len;
	}
	else {
		char *p = _curptr;
		if (ll == S57_LL2) { 
			while (p < _endptr 
					&& *reinterpret_cast<s57_b12 *>(p) != S57_UT 
					&& *reinterpret_cast<s57_b12 *>(p) != S57_FT) {
				s.push_back(*p++);
				s.push_back(*p++);
			}
			_curptr = p + 2;
		}
		else {
			while (p < _endptr && *p != S57_UT && *p != S57_FT)
				s.push_back(*p++);
			_curptr = p + 1;
		}
	}

#ifndef NDEBUG
	checkCurPtr(__FUNCTION__);
#endif
	return s;
}

S57_NAME S57Decoder::unpackName()
{
	S57_NAME name;
	name._rcnm = getUInt(1);
	name._rcid = getUInt(4);
#ifndef NDEBUG
	checkCurPtr(__FUNCTION__);
#endif
	return name;
}

S57_LNAM S57Decoder::unpackLongName()
{
	S57_LNAM lname;
	lname._agen = getUInt(2);
	lname._fidn = getUInt(4);
	lname._fids = getUInt(2);
#ifndef NDEBUG
	checkCurPtr(__FUNCTION__);
#endif
	return lname;
}

S57_Date S57Decoder::getDate()
{
	S57_Date dt;
	char buf[5];

	if (isEnd()) {
		fprintf(stderr, "*** Unexpected end of field\n");
		return dt;
	}

	memcpy(buf, _curptr, 4); // get year block
	buf[4] = 0;
	dt._year = atoi(buf);

	memcpy(buf, _curptr + 4, 2); // get month block
	buf[2] = 0;
	dt._month = atoi(buf);

	memcpy(buf, _curptr + 6, 2); // get day block
	buf[2] = 0;
	dt._day = atoi(buf);

	_curptr += 8;

#ifndef NDEBUG
	checkCurPtr(__FUNCTION__);
#endif
	return dt;
}

bool S57Decoder::isEnd() const
{
	if (_curptr >= _endptr)
		return true;
	else if (_curptr == _endptr - 1 && *_curptr == S57_FT)
		return true;
	else if (_curptr == _endptr - 2 && *(s57_b12 *)_curptr == S57_FT)
		return true;
	else
		return false;
}

// S57Encoder members

S57Encoder::S57Encoder(FILE *fp)
	: _fp(fp)
{
}

S57Encoder::~S57Encoder()
{
}

void S57Encoder::setUInt(s57_b14 n, int width)
{
	if (width == 1) {
		s57_b11 n1 = n;
		as_fwrite(&n1, 1, 1, _fp);
	}
	else if (width == 2) {
		s57_b12 n2 = n;
		as_fwrite(&n2, 2, 1, _fp);
	}
	else if (width == 4)
		as_fwrite(&n, 4, 1, _fp);
	else
		assert(0);
}

void S57Encoder::setSInt(s57_b24 n, int width)
{
	if (width == 1) {
		s57_b21 n1 = n;
		as_fwrite(&n1, 1, 1, _fp);
	}
	else if (width == 2) {
		s57_b22 n2 = n;
		as_fwrite(&n2, 2, 1, _fp);
	}
	else if (width == 4)
		as_fwrite(&n, 4, 1, _fp);
	else
		assert(0);
}

void S57Encoder::setString(std::string s, int ll, bool setUT)
{
	as_fwrite(s.data(), 1, s.size(), _fp);

	if (setUT) {
		if (ll == S57_LL2) {
			s57_b12 ut2 = S57_UT;
			as_fwrite(&ut2, 2, 1, _fp);
		}
		else {
			s57_b11 ut1 = S57_UT;
			as_fwrite(&ut1, 1, 1, _fp);
		}
	}
}

void S57Encoder::setDate(const S57_Date &date)
{
	char sbuf[9];
	snprintf(sbuf, 9, "%4d%02d%02d", date._year, date._month, date._day);
	as_fwrite(sbuf, 1, 8, _fp);
}

void S57Encoder::packName(const S57_NAME &name)
{
	setUInt(name._rcnm, 1);
	setUInt(name._rcid, 4);
}

void S57Encoder::packLongName(const S57_LNAM &lname)
{
	setUInt(lname._agen, 2);
	setUInt(lname._fidn, 4);
	setUInt(lname._fids, 2);
}

void S57Encoder::writeBlock(const char *p, size_t sz)
{
	as_fwrite(p, 1, sz, _fp);
}

void S57Encoder::endField(int ll)
{
	if (ll == S57_LL2) {
		s57_b12 ft2 = S57_FT;
		as_fwrite(&ft2, 2, 1, _fp);
	}
	else {
		s57_b11 ft1 = S57_FT;
		as_fwrite(&ft1, 1, 1, _fp);
	}
}
