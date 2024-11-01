#ifndef S57_UTIL_H
#define S57_UTIL_H

#include <string>
#include <vector>

#include "iso8211_gloabal.h"

/*
 * S57 integer type
 */
typedef unsigned char  s57_b11;
typedef unsigned short s57_b12;
typedef unsigned long  s57_b14;

typedef signed char  s57_b21;
typedef signed short s57_b22;
typedef signed long  s57_b24;

/*
 * Field and Subfield termination
 */
const int S57_UT = 0x1f; // Unit Terminator
const int S57_FT = 0x1e; // Field Terminator

/*
 * Lexical level code
 */
const int S57_LL0 = 0; // ASCII text
const int S57_LL1 = 1; // Latin alphabet 1
const int S57_LL2 = 2; // UCS-2 implementation levle 1

/*
 * value that not relevant
 */
const int S57_NR = 255;

/*
 * Update instruction
 */
const s57_b11 S57_UI_I = 1; // Insert
const s57_b11 S57_UI_D = 2; // Delete
const s57_b11 S57_UI_M = 3; // Modify

/*
 * Record name [RCNM]
 */
const s57_b11 RCNM_DS = 10;  // Data Set General Information
const s57_b11 RCNM_DP = 20;  // Data Set Geographic Reference
const s57_b11 RCNM_DH = 30;  // Data Set History
const s57_b11 RCNM_DA = 40;  // Data Set Accruacy
const s57_b11 RCNM_CR = 60;  // Catalogue Cross Reference
const s57_b11 RCNM_ID = 70;  // Data Dictionary Definition
const s57_b11 RCNM_OD = 80;  // Data Dictionary Domain
const s57_b11 RCNM_IS = 90;  // Data Dictionary Schema
const s57_b11 RCNM_FE = 100; // Feature
const s57_b11 RCNM_VI = 110; // Vector - Isolated node
const s57_b11 RCNM_VC = 120; // Vector - Connected node
const s57_b11 RCNM_VE = 130; // Vector - Edge
const s57_b11 RCNM_VF = 140; // Vector - Face

/*
 * Data structure [DSTR]
 */
const s57_b11 DSTR_CS = 1;      // Catrographic spaghetti
const s57_b11 DSTR_CN = 2;      // Chain-node
const s57_b11 DSTR_PG = 3;      // Planar graph
const s57_b11 DSTR_FT = 4;      // Full Topology
const s57_b11 DSTR_NO = S57_NR; // Topology is not relevant

/*
 * Coordinate units [COUN]
 */
const s57_b11 COUN_LL = 1; // Latitude and logitude in degress of arc
const s57_b11 COUN_EN = 2; // Easting/Northing in meters
const s57_b11 COUN_UC = 3; // Units on chart/map in milimeters

/*
 * Data set projection [PROJ]
 */
const s57_b11 PROJ_ALA = 1;  // Albert equal area
const s57_b11 PROJ_AZA = 2;  // Azimuth equal area
const s57_b11 PROJ_AZD = 3;  // Azimuth equal distance
const s57_b11 PROJ_GNO = 4;  // Gnonomic
const s57_b11 PROJ_HOM = 5;  // Hotine oblique Mercator
const s57_b11 PROJ_LCC = 6;  // Lambert conformal conic
const s57_b11 PROJ_LEA = 7;  // Lambert equal area
const s57_b11 PROJ_MER = 8;  // Mercator
const s57_b11 PROJ_OME = 9;  // Oblique Mercator
const s57_b11 PROJ_ORT = 10; // Orthographic
const s57_b11 PROJ_PST = 11; // Polar stereo graphic
const s57_b11 PROJ_POL = 12; // Polyconic
const s57_b11 PROJ_TME = 13; // Transverse Mercator
const s57_b11 PROJ_OST = 14; // Oblique stereographic

/*
 * Object geometric primitive [PRIM]
 * XXX The geometric primitive for all soundings must be point.
 */
const s57_b11 PRIM_P = 1;      // Point
const s57_b11 PRIM_L = 2;      // Line
const s57_b11 PRIM_A = 3;      // Area
const s57_b11 PRIM_N = S57_NR; // Object does not directly reference and geometry
                               // This value is used for feature records which do
                               // not reference and spatial records (e.g. collection
                               // feature records).

/*
 * Orientation [ORNT]
 */
const s57_b11 ORNT_F = 1;      // Forward
const s57_b11 ORNT_R = 2;      // Reverse
const s57_b11 ORNT_N = S57_NR; // Direction is not relevant

/*
 * Usage indicator [USAG]
 */
const s57_b11 USAG_E = 1;      // Exterior
const s57_b11 USAG_I = 2;      // Interior
const s57_b11 USAG_C = 3;      // Exterior boundary truncated by the data limit
const s57_b11 USAG_N = S57_NR; // Usage is not relevant

/*
 * Topology indicator [TOPI]
 */
const s57_b11 TOPI_B = 1; // Beginning node
const s57_b11 TOPI_E = 2; // End node

/*
 * Masking indicator [MASK]
 */
const s57_b11 MASK_M = 1;      // Mask
const s57_b11 MASK_S = 2;      // Show
const s57_b11 MASK_N = S57_NR; // Masking is not relevant

/*
 * Exchange purpose [EXPP]
 */
const s57_b11 EXPP_N = 1; // Data set is new
const s57_b11 EXPP_R = 2; // Data set is a revision to an existing one

extern const char * INDENT;

/*
 * general-purpose pointer wrapper with reference counting
 */
class ISO8211_EXPORT RefBase
{
private:
    int _refCount;

public:
    RefBase();

    int ref();
    int unref();
    int refCount() const;
};

inline RefBase::RefBase()
    : _refCount(0)
{}

inline int RefBase::ref()
{
    return _refCount++;
}

inline int RefBase::unref()
{
    return --_refCount;
}

inline int RefBase::refCount() const
{
    return _refCount;
}

template <class T>
class ISO8211_EXPORT Ref
{
private:
    T * _rep;

public:
    Ref();
    Ref(T * p);
    Ref(const Ref & r);
    Ref & operator=(const Ref & r);
    ~Ref();

    T *  operator->() const;
    T &  operator()();
    bool operator==(const Ref & r) const;

    T *       getPtr();
    const T * getPtr() const;
    bool      isNull() const;

    void release();
};

template <class T>
inline Ref<T>::Ref()
    : _rep((T *)NULL)
{}

template <class T>
inline Ref<T>::Ref(T * p)
    : _rep(p)
{
    if (p != (T *)NULL)
        p->ref();
}

template <class T>
inline Ref<T>::Ref(const Ref<T> & r)
    : _rep(r._rep)
{
    if (_rep != (T *)NULL)
        _rep->ref();
}

template <class T>
inline Ref<T> & Ref<T>::operator=(const Ref<T> & r)
{
    if (r._rep != (T *)NULL)
        r._rep->ref();
    if (_rep != (T *)NULL && _rep->unref() == 0)
        delete _rep;
    _rep = r._rep;
    return *this;
}

template <class T>
inline Ref<T>::~Ref()
{
    release();
}

template <class T>
inline T * Ref<T>::operator->() const
{
    return _rep;
}

template <class T>
inline T & Ref<T>::operator()()
{
    return *_rep;
}

template <class T>
inline bool Ref<T>::operator==(const Ref<T> & r) const
{
    return _rep == r._rep;
}

template <class T>
inline T * Ref<T>::getPtr()
{
    return _rep;
}

template <class T>
inline const T * Ref<T>::getPtr() const
{
    return _rep;
}

template <class T>
inline bool Ref<T>::isNull() const
{
    return _rep == (T *)NULL;
}

template <class T>
inline void Ref<T>::release()
{
    if (_rep != (T *)NULL && _rep->unref() == 0)
        delete _rep;
    _rep = NULL;
}

// ~

ISO8211_EXPORT std::string strToPrintable(std::string);
ISO8211_EXPORT std::string intToStr(long i);

ISO8211_EXPORT long   strToInt(std::string);
ISO8211_EXPORT double strToFloat(std::string s);

typedef std::vector<std::string> StringList;

ISO8211_EXPORT void setDataDir(const std::string & dir);
ISO8211_EXPORT std::string getAbsolutePath(const std::string & path);

/*
// Splits the string str using sep as separator.
// Returns the list of strings.
// If allowEmptyEntry is TRUE, also empty entries are inserted into the list, else not.
StringList splitString(char sep, std::string str, bool allowEmptyEntry = false);
StringList splitString(std::string sep, std::string str, bool allowEmptyEntry = false);

// Get all S57 data set files within the given dirName
void scanS57DataSetFiles(StringList &fl, const char *prefix, const char *dirName, bool recursive);
*/

class ISO8211_EXPORT S57DatasetScanner
{
private:
    bool _recursive;

private:
    void doScan(const std::string & prefix, const std::string & dirName);

protected:
    // This function will be called while a S-57 dataset found.
    // the fileName is absolute filename of the dataset.
    virtual void onDataset(std::string fileName) = 0;

public:
    // Constructs a scanner with recursive = false
    S57DatasetScanner();
    virtual ~S57DatasetScanner();

    bool isRecursive() const;
    void setRecursive(bool);

    void scan(std::string path);
};

// S57DatasetScanner inline functions

inline S57DatasetScanner::S57DatasetScanner()
    : _recursive(false)
{}

inline S57DatasetScanner::~S57DatasetScanner()
{}

inline bool S57DatasetScanner::isRecursive() const
{
    return _recursive;
}

inline void S57DatasetScanner::setRecursive(bool enabled)
{
    _recursive = enabled;
}

// ~

#endif
