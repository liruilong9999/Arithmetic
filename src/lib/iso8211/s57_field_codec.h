#ifndef S57_FIELD_CODEC_H
#define S57_FIELD_CODEC_H

#include <string>
#include "iso8211_gloabal.h"

class S57Decoder;
class S57Encoder;

/*
 * The concatenation of the RCNM and RCID subfields
 * is unique whithin the file in which the record is
 * contained.
 */
class ISO8211_EXPORT S57_NAME
{
public:
    s57_b11 _rcnm;
    s57_b14 _rcid;

public:
    S57_NAME();

    friend bool operator==(const S57_NAME &, const S57_NAME &);

    std::string toString() const;
};

inline S57_NAME::S57_NAME()
    : _rcnm(0)
    , _rcid(0)
{}

inline bool operator==(const S57_NAME & n1, const S57_NAME & n2)
{
    return n1._rcnm == n2._rcnm && n1._rcid == n2._rcid;
}

/*
 * The LNAM subfield is used as a foreign pointer in
 * the encoding of relations between feature records.
 */
class ISO8211_EXPORT S57_LNAM
{
public:
    s57_b12 _agen;
    s57_b14 _fidn;
    s57_b12 _fids;

public:
    S57_LNAM();
    S57_LNAM(S57Decoder &);

    friend bool operator==(const S57_LNAM &, const S57_LNAM &);

    std::string toString(bool repeat = false) const;
};

inline S57_LNAM::S57_LNAM()
    : _agen(0)
    , _fidn(0)
    , _fids(0)
{}

inline bool operator==(const S57_LNAM & ln1, const S57_LNAM & ln2)
{
    return (ln1._agen == ln2._agen) && (ln1._fidn == ln2._fidn) && (ln1._fids == ln2._fids);
}

/*
 * A date subfield int the fom: YYYYMMDD
 */
class ISO8211_EXPORT S57_Date
{
public:
    int _year;
    int _month;
    int _day;

public:
    S57_Date();

    std::string toString() const;
};

inline S57_Date::S57_Date()
    : _year(0)
    , _month(0)
    , _day(0)
{}

/*
 * Attribute or national attribute field structure
 */
class ISO8211_EXPORT S57_AttItem
{
public:
    s57_b12     _attl;
    std::string _atvl;

public:
    S57_AttItem();
    S57_AttItem(s57_b12 attl, std::string atvl);

    std::string toString(int llcode) const;
};

inline S57_AttItem::S57_AttItem()
    : _attl(0)
{}

inline S57_AttItem::S57_AttItem(s57_b12 attl, std::string atvl)
    : _attl(attl)
    , _atvl(atvl)
{}

/*
 * Control field structure
 */
class ISO8211_EXPORT S57_UpdControl
{
public:
    s57_b11 _instruction;
    s57_b12 _index;
    s57_b12 _count;

public:
    S57_UpdControl();
    S57_UpdControl(S57Decoder &);

    void encode(S57Encoder &);

    std::string toString() const;
};

/********************************************************/
/*                                                      */
/*        Data set general information record           */
/*                                                      */
/********************************************************/

/*
 * Data set identification field structure
 */
class ISO8211_EXPORT S57_DSID
{
public:
    S57_NAME    _name;
    s57_b11     _expp; // Exchange purpose
    s57_b11     _intu; // Intended usage
    std::string _dsnm; // Data set name
    s57_b12     _edtn; // Edition number
    s57_b12     _updn; // Update number
    S57_Date    _uadt; // Update application date
    S57_Date    _isdt; // Issue date
    std::string _sted; // Edition number of S-57
    s57_b11     _prsp; // Product specification
    std::string _psdn; // Product specification description
    std::string _pred; // Product specification edition number
    s57_b11     _prof; // Application profile identification
    s57_b12     _agen; // Producing agency
    std::string _comt; // Comment

public:
    S57_DSID();
    S57_DSID(S57Decoder &);

    void encode(S57Encoder &);

    std::string toString() const;
};

/*
 * Data set structure information field structure
 */
class ISO8211_EXPORT S57_DSSI
{
public:
    s57_b11 _dstr; // Data structure
    s57_b11 _aall; // ATTF lexical level
    s57_b11 _nall; // NATF lexical level
    s57_b14 _nomr; // Number of meta records
    s57_b14 _nocr; // Number of cartographic records
    s57_b14 _nogr; // Number of geo records
    s57_b14 _nolr; // Number of collection records
    s57_b14 _noin; // Number of isolated node records
    s57_b14 _nocn; // Number of connected node
    s57_b14 _noed; // Number of edge records
    s57_b14 _nofa; // Number of face records

public:
    S57_DSSI();
    S57_DSSI(S57Decoder &);

    void encode(S57Encoder &);

    std::string toString() const;
};

/********************************************************/
/*                                                      */
/*        Data set geographic reference record          */
/*                                                      */
/********************************************************/

/*
 * Data set geographic reference record structure
 */
class ISO8211_EXPORT S57_DSPM
{
public:
    S57_NAME    _name;
    s57_b11     _hdat; // Horizontal geodetic datum
    s57_b11     _vdat; // Vertical datum
    s57_b11     _sdat; // Sounding datum
    s57_b14     _cscl; // Compilation scale of data
    s57_b11     _duni; // Units of depth measurement
    s57_b11     _huni; // Units of height measurement
    s57_b11     _puni; // Units of positional accuracy
    s57_b11     _coun; // Coordinate units
    s57_b14     _comf; // Coordinate multiplication factor
    s57_b14     _somf; // Sounding multiplication factor
    std::string _comt; // Comment

public:
    S57_DSPM();
    S57_DSPM(S57Decoder &);

    void encode(S57Encoder &);

    std::string toString() const;
};

/*
 * Data set projection field structure
 */
class ISO8211_EXPORT S57_DSPR
{
public:
    s57_b11     _proj; // Projection
    s57_b24     _prp1; // Projection parameter 1~4
    s57_b24     _prp2;
    s57_b24     _prp3;
    s57_b24     _prp4;
    s57_b24     _feas; // False Easting
    s57_b24     _fnor; // False Northing
    s57_b14     _fpmf; // Floating point multiplication factor
    std::string _comt; // Comment

public:
    S57_DSPR();
    S57_DSPR(S57Decoder &);

    std::string toString() const;
};

/*
 * Data set registration control field structure
 */
class ISO8211_EXPORT S57_DSRC
{
public:
    s57_b11     _rpid; // Registration point ID
    s57_b24     _ryco; // Registration point Latitude or Northing
    s57_b24     _rxco; // Registration point Longitude of Easting
    s57_b11     _curp; // Coordinate units for registration point
    s57_b14     _fpmf; // Floating point multiplication factor
    s57_b24     _rxvl; // REgistration point X-value
    s57_b24     _ryvl; // REgistration point Y-value
    std::string _comt; // Comment

public:
    S57_DSRC();
    S57_DSRC(S57Decoder &);

    std::string toString() const;
};

/********************************************************/
/*                                                      */
/*        Data set history record                       */
/*                                                      */
/********************************************************/

/*
 * Data set history field structure
 */
class ISO8211_EXPORT S57_DSHT
{
public:
    S57_NAME    _name;
    s57_b12     _prco; // Producing agency code
    S57_Date    _esdt; // Earliest source date
    S57_Date    _lsdt; // Latest source date
    std::string _dcrt; // Data collection criteria
    S57_Date    _codt; // Compilation date;
    std::string _comt; // Comment

public:
    S57_DSHT();
    S57_DSHT(S57Decoder &);

    std::string toString() const;
};

/********************************************************/
/*                                                      */
/*        Data set accuracy record                      */
/*                                                      */
/********************************************************/

/*
 * Data set accuracy field structure
 */
class ISO8211_EXPORT S57_DSAC
{
public:
    S57_NAME    _name;
    s57_b14     _pacc; // Absolute positional accuracy
    s57_b14     _hacc; // Absolute horizontal/vertical measurement accuracy
    s57_b14     _sacc; // Absolute souding accuracy;
    s57_b14     _fpmf; // Floating point multiplication factor
    std::string _comt; // Comment

public:
    S57_DSAC();
    S57_DSAC(S57Decoder &);

    std::string toString() const;
};

/********************************************************/
/*                                                      */
/*        Catalogue directory record                    */
/*                                                      */
/********************************************************/

/*
 * Catalogue directory field structure
 * XXX always encoded using the ASCII implementation
 */
class ISO8211_EXPORT S57_CATD
{
public:
    S57_NAME    _name;
    std::string _file; // File name
    std::string _lfil; // File long name
    std::string _volm; // Volume
    std::string _impl; // Implementation
    double      _slat; // Southernmost latitude
    double      _wlon; // Westernmost longitude
    double      _nlat; // Northernmost latitude
    double      _elon; // Easternmost longitude
    std::string _crcs; // CRC
    std::string _comt; // Comment

public:
    S57_CATD();
    S57_CATD(S57Decoder &);

    std::string toString() const;
};

/********************************************************/
/*                                                      */
/*        Feature record                                */
/*                                                      */
/********************************************************/

/*
 * Feature record identifier field structure
 */
class ISO8211_EXPORT S57_FRID
{
public:
    S57_NAME _name;
    s57_b11  _prim; // Object geometric primitive
    s57_b11  _grup; // Group
    s57_b12  _objl; // Object label/code
    s57_b12  _rver; // Record version
    s57_b11  _ruin; // Record update instruction

public:
    S57_FRID();
    S57_FRID(S57Decoder &);

    void encode(S57Encoder &);

    std::string toString() const;
};

/*
 * Feature record to feature object pointer field structure
 */
class ISO8211_EXPORT S57_FFPT
{
public:
    S57_LNAM    _lnam;
    s57_b11     _rind; // Relationship indicator
    std::string _comt; // Comment

public:
    S57_FFPT();
    S57_FFPT(S57Decoder &);

    void encode(S57Encoder &);

    std::string toString() const;
};

/*
 * Feature record to spatial record pointer field structure
 */
class ISO8211_EXPORT S57_FSPT
{
public:
    S57_NAME _name;
    s57_b11  _ornt;
    s57_b11  _usag;
    s57_b11  _mask;

public:
    S57_FSPT();
    S57_FSPT(S57Decoder &);

    void encode(S57Encoder &);

    std::string toString() const;
};

/********************************************************/
/*                                                      */
/*        Spatial record                                */
/*                                                      */
/********************************************************/

/*
 * Vector record identifier field structure
 */
class ISO8211_EXPORT S57_VRID
{
public:
    S57_NAME _name;
    s57_b12  _rver; // Record version
    s57_b11  _ruin; // Record update instruction

public:
    S57_VRID();
    S57_VRID(S57Decoder &);

    void encode(S57Encoder &);

    std::string toString() const;
};

/*
 * Vector record pointer field structure
 */
class ISO8211_EXPORT S57_VRPT
{
public:
    S57_NAME _name;
    s57_b11  _ornt; // Orientation
    s57_b11  _usag; // Usage indicator
    s57_b11  _topi; // Topology indicator
    s57_b11  _mask; // Masking indicator

public:
    S57_VRPT();
    S57_VRPT(S57Decoder &);

    void encode(S57Encoder &);

    std::string toString() const;
};

/*
 * 2-D Coordinate field structure
 */
class ISO8211_EXPORT S57_SG2D
{
public:
    s57_b24 _ycoo;
    s57_b24 _xcoo;

public:
    S57_SG2D();
    S57_SG2D(s57_b24 y, s57_b24 x);
};

inline S57_SG2D::S57_SG2D()
    : _ycoo(0)
    , _xcoo(0)
{}

inline S57_SG2D::S57_SG2D(s57_b24 y, s57_b24 x)
    : _ycoo(y)
    , _xcoo(x)
{}

/*
 * 3-D Coordinate field structure
 */
class ISO8211_EXPORT S57_SG3D
{
public:
    s57_b24 _ycoo;
    s57_b24 _xcoo;
    s57_b24 _ve3d;

public:
    S57_SG3D();
    S57_SG3D(s57_b24 y, s57_b24 x, s57_b24 v);
};

inline S57_SG3D::S57_SG3D()
    : _ycoo(0)
    , _xcoo(0)
    , _ve3d(0)
{}

inline S57_SG3D::S57_SG3D(s57_b24 y, s57_b24 x, s57_b24 v)
    : _ycoo(y)
    , _xcoo(x)
    , _ve3d(v)
{}

/*
 * S57 field decoder
 */
class ISO8211_EXPORT S57Decoder
{
private:
    char * _buf;
    char * _curptr;
    char * _endptr;

private:
    void init();
#ifndef NDEBUG
    void checkCurPtr(const char * func);
#endif

public:
    S57Decoder();
    S57Decoder(std::string);
    ~S57Decoder();

    void setFieldData(std::string);

    // Get signed integer with width number of bytes
    s57_b24 getSInt(int width);

    // Get unsigned integer with width number of bytes
    s57_b14 getUInt(int width);

    // Get string with length chars
    // If len is -1, the result string is stopping at unit/field terminator.
    // The lexical level is indicated by ll.
    std::string getString(int ll, int len = -1);

    // Unpack S57_NAME structure from a 40-bit-string
    // The values must be stored in the "little-endian"
    S57_NAME unpackName();

    // Unpack S57_LNAM structure from a 64-bit-string
    // The values must be stored in the "little-endian"
    S57_LNAM unpackLongName();

    S57_Date getDate();

    bool isEnd() const;
};

/*
 * S57 field encoder
 */
class ISO8211_EXPORT S57Encoder
{
private:
    FILE * _fp;

public:
    S57Encoder(FILE * fp);
    ~S57Encoder();

    void setUInt(s57_b14 n, int width);
    void setSInt(s57_b24 n, int width);
    void setString(std::string s, int ll, bool setUT);
    void setDate(const S57_Date & date);
    void packName(const S57_NAME & name);
    void packLongName(const S57_LNAM & lname);

    void writeBlock(const char * p, size_t sz);

    void endField(int ll = S57_LL0);
};

#endif
