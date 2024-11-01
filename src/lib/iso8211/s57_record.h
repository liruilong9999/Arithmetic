#ifndef S57_RECORD_H
#define S57_RECORD_H

#include <string>
#include <vector>

#include "s57_utils.h"
#include "s57_field_codec.h"
#include "iso8211_gloabal.h"

const int FIELD_TAG_SIZE = 4;

class S57Module;

class ISO8211_EXPORT LRLeader
{
public:
    size_t _recordLength;
    char   _interchangeLevel;
    char   _leaderIdentifier;
    char   _extensionIndicator;
    char   _versionNumber;
    char   _applicationIndicator;
    size_t _fieldControlLength;
    size_t _fieldAreaOffset;
    char   _charSetIndicator[3];
    size_t _szFieldLen;
    size_t _szFieldPos;

public:
    LRLeader();
    LRLeader(std::string data);

    std::string toString() const;
};

class ISO8211_EXPORT LRDirEntry
{
public:
    char   _fieldTag[FIELD_TAG_SIZE + 1];
    size_t _fieldLen;
    size_t _fieldPos;

public:
    LRDirEntry();
    LRDirEntry(const char * tag, size_t len, size_t log);
};

class ISO8211_EXPORT LRHeader : public RefBase
{
public:
    LRLeader                _leader;
    std::vector<LRDirEntry> _dir;

public:
    LRHeader();
    LRHeader(std::string data);

    void encode(S57Encoder &);

    std::string toString() const;

    typedef std::vector<LRDirEntry>::const_iterator DirIterator;

    DirIterator begin();
    DirIterator end();
};

typedef Ref<LRHeader> LRHeaderRef;

class ISO8211_EXPORT S57Record : public RefBase
{
public:
    enum RecordType
    {
        DDR,
        DatasetInformation,
        DatasetGeographic,
        DatasetHistory,
        DatasetAccuracy,
        CatalogDirectory,
        CatalogCross,
        DataDictDefn,
        DataDictDomain,
        DataDictSchema,
        Feature,
        Vector
    };

private:
    std::string _data;

protected:
    RecordType  _recordType;
    LRHeaderRef _header;
    S57Module * _module;

    std::string _recordId; // raw data of field 0001

public:
    S57Record();
    S57Record(RecordType, S57Module * mod = NULL);
    // Constructs a default record with given record raw data
    S57Record(RecordType, std::string);
    virtual ~S57Record();

    static Ref<S57Record> decode(std::string, S57Module *);
    virtual void          encode(S57Encoder &);

    RecordType  recordType() const;
    LRHeaderRef header() const;

    void setRecordType(RecordType);
    void setHeader(LRHeaderRef);
    void setModule(S57Module *);
    void setRecordId(std::string);

    virtual std::string toString() const;
};

// S57Record inline functions

inline S57Record::RecordType S57Record::recordType() const
{
    return _recordType;
}

inline LRHeaderRef S57Record::header() const
{
    return _header;
}

inline void S57Record::setRecordType(S57Record::RecordType t)
{
    _recordType = t;
}

inline void S57Record::setHeader(LRHeaderRef hr)
{
    _header = hr;
}

inline void S57Record::setModule(S57Module * mod)
{
    _module = mod;
}

inline void S57Record::setRecordId(std::string s)
{
    _recordId = s;
}

typedef Ref<S57Record> S57RecordRef;

// ~

class ISO8211_EXPORT S57DataDescripRecord : public S57Record
{
private:
    std::string _fieldControl;
    StringList  _descripFields;

private:
    // Constructor with given header and field area
    S57DataDescripRecord(LRHeaderRef, std::string, S57Module *);

public:
    S57DataDescripRecord();
    ~S57DataDescripRecord();

    void encode(S57Encoder &);

    std::string toString() const;

    friend class S57Record;
};

// ~

class ISO8211_EXPORT S57DSInfoRecord : public S57Record
{
private:
    S57_DSID * _dsid;
    S57_DSSI * _dssi;

private:
    void init();

    // Constructor with given header and field area
    S57DSInfoRecord(LRHeaderRef, std::string, S57Module *);

public:
    S57DSInfoRecord();
    ~S57DSInfoRecord();

    const S57_DSID * fieldDSID() const;
    const S57_DSSI * fieldDSSI() const;

    void encode(S57Encoder &);

    std::string toString() const;

    friend class S57Record;
};

inline const S57_DSID * S57DSInfoRecord::fieldDSID() const
{
    return _dsid;
}

inline const S57_DSSI * S57DSInfoRecord::fieldDSSI() const
{
    return _dssi;
}

// ~

class ISO8211_EXPORT S57DSGeoRecord : public S57Record
{
private:
    S57_DSPM * _dspm;
    S57_DSPR * _dspr;
    S57_DSRC * _dsrc;

private:
    void init();

    // Constructor with given header and field area
    S57DSGeoRecord(LRHeaderRef, std::string, S57Module *);

public:
    S57DSGeoRecord();
    ~S57DSGeoRecord();

    const S57_DSPM * fieldDSPM() const;
    const S57_DSPR * fieldDSPR() const;
    const S57_DSRC * fieldDSRC() const;

    void encode(S57Encoder &);

    std::string toString() const;

    friend class S57Record;
};

inline const S57_DSPM * S57DSGeoRecord::fieldDSPM() const
{
    return _dspm;
}

inline const S57_DSPR * S57DSGeoRecord::fieldDSPR() const
{
    return _dspr;
}

inline const S57_DSRC * S57DSGeoRecord::fieldDSRC() const
{
    return _dsrc;
}

// ~

class ISO8211_EXPORT S57DSHistoryRecord : public S57Record
{
private:
    S57_DSHT * _dsht;

private:
    void init();

    // Constructor with given header and field area
    S57DSHistoryRecord(LRHeaderRef, std::string, S57Module *);

public:
    S57DSHistoryRecord();
    ~S57DSHistoryRecord();

    const S57_DSHT * fieldDSHT() const;

    std::string toString() const;

    friend class S57Record;
};

inline const S57_DSHT * S57DSHistoryRecord::fieldDSHT() const
{
    return _dsht;
}

// ~

class ISO8211_EXPORT S57DSAccuracyRecord : public S57Record
{
private:
    S57_DSAC * _dsac;

private:
    void init();

    // Constructor with given header and field area
    S57DSAccuracyRecord(LRHeaderRef, std::string, S57Module *);

public:
    S57DSAccuracyRecord();
    ~S57DSAccuracyRecord();

    const S57_DSAC * fieldDSAC() const;

    std::string toString() const;

    friend class S57Record;
};

inline const S57_DSAC * S57DSAccuracyRecord::fieldDSAC() const
{
    return _dsac;
}

// ~

class ISO8211_EXPORT S57CatalogDirRecord : public S57Record
{
private:
    S57_CATD * _catd;

private:
    void init();

    // Constructor with given header and field area
    S57CatalogDirRecord(LRHeaderRef, std::string, S57Module *);

public:
    S57CatalogDirRecord();
    ~S57CatalogDirRecord();

    const S57_CATD * fieldCATD() const;

    std::string toString() const;

    friend class S57Record;
};

inline const S57_CATD * S57CatalogDirRecord::fieldCATD() const
{
    return _catd;
}

//~

class ISO8211_EXPORT S57FeatureRecord : public S57Record
{
private:
    S57_FRID *               _frid;
    S57_LNAM *               _foid;
    std::vector<S57_AttItem> _attfs;
    std::vector<S57_AttItem> _natfs;
    S57_UpdControl *         _ffpc;
    std::vector<S57_FFPT>    _ffpts;
    S57_UpdControl *         _fspc;
    std::vector<S57_FSPT>    _fspts;

private:
    void init();

    // Constructor with given header and field area
    S57FeatureRecord(LRHeaderRef, std::string, S57Module *);

public:
    S57FeatureRecord();
    ~S57FeatureRecord();

    const S57_FRID *                 fieldFRID() const;
    const S57_LNAM *                 fieldFOID() const;
    const std::vector<S57_AttItem> & fieldsATTF() const;
    const std::vector<S57_AttItem> & fieldsNATF() const;
    const S57_UpdControl *           fieldFFPC() const;
    const std::vector<S57_FFPT> &    fieldsFFPT() const;
    const S57_UpdControl *           fieldFSPC() const;
    const std::vector<S57_FSPT> &    fieldsFSPT() const;

    void encode(S57Encoder &);

    std::string toString() const;

    void markDeleted();
    bool isDeleted() const;

    void update(Ref<S57FeatureRecord> upr);

    friend class S57Record;
};

inline const S57_FRID * S57FeatureRecord::fieldFRID() const
{
    return _frid;
}

inline const S57_LNAM * S57FeatureRecord::fieldFOID() const
{
    return _foid;
}

inline const std::vector<S57_AttItem> & S57FeatureRecord::fieldsATTF() const
{
    return _attfs;
}

inline const std::vector<S57_AttItem> & S57FeatureRecord::fieldsNATF() const
{
    return _natfs;
}

inline const S57_UpdControl * S57FeatureRecord::fieldFFPC() const
{
    return _ffpc;
}

inline const std::vector<S57_FFPT> & S57FeatureRecord::fieldsFFPT() const
{
    return _ffpts;
}

inline const S57_UpdControl * S57FeatureRecord::fieldFSPC() const
{
    return _fspc;
}

inline const std::vector<S57_FSPT> & S57FeatureRecord::fieldsFSPT() const
{
    return _fspts;
}

// ~

class ISO8211_EXPORT S57VectorRecord : public S57Record
{
public:
    enum CoordType
    {
        SG2D,
        SG3D
    };

private:
    S57_VRID *               _vrid;
    std::vector<S57_AttItem> _attvs;
    S57_UpdControl *         _vrpc;
    std::vector<S57_VRPT>    _vrpts;
    S57_UpdControl *         _sgcc;
    CoordType                _coordType;
    std::vector<s57_b24>     _coords;

private:
    void init();

    // Constructor with given header and field area
    S57VectorRecord(LRHeaderRef, std::string, S57Module *);

public:
    S57VectorRecord();
    ~S57VectorRecord();

    const S57_VRID *                 fieldVRID() const;
    const std::vector<S57_AttItem> & fieldsATTV() const;
    const S57_UpdControl *           fieldVRPC() const;
    const std::vector<S57_VRPT> &    fieldsVRPT() const;
    const S57_UpdControl *           fieldSGCC() const;
    CoordType                        coordType() const;
    const std::vector<s57_b24> &     coords() const;

    void encode(S57Encoder &);

    std::string toString() const;

    void markDeleted();
    bool isDeleted() const;

    void update(Ref<S57VectorRecord> upr);

    friend class S57Record;
};

inline const S57_VRID * S57VectorRecord::fieldVRID() const
{
    return _vrid;
}

inline const std::vector<S57_AttItem> & S57VectorRecord::fieldsATTV() const
{
    return _attvs;
}

inline const S57_UpdControl * S57VectorRecord::fieldVRPC() const
{
    return _vrpc;
}

inline const std::vector<S57_VRPT> & S57VectorRecord::fieldsVRPT() const
{
    return _vrpts;
}

inline const S57_UpdControl * S57VectorRecord::fieldSGCC() const
{
    return _sgcc;
}

inline S57VectorRecord::CoordType S57VectorRecord::coordType() const
{
    return _coordType;
}

inline const std::vector<s57_b24> & S57VectorRecord::coords() const
{
    return _coords;
}

// ~

typedef Ref<S57DataDescripRecord> S57DataDescripRecordRef;
typedef Ref<S57DSInfoRecord>      S57DSInfoRecordRef;
typedef Ref<S57DSGeoRecord>       S57DSGeoRecordRef;
typedef Ref<S57DSHistoryRecord>   S57DSHistoryRecordRef;
typedef Ref<S57DSAccuracyRecord>  S57DSAccuracyRecordRef;
typedef Ref<S57CatalogDirRecord>  S57CatalogDirRecordRef;
typedef Ref<S57FeatureRecord>     S57FeatureRecordRef;
typedef Ref<S57VectorRecord>      S57VectorRecordRef;

#endif
