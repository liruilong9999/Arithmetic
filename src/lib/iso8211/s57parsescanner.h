#ifndef S57PARSESCANNER_H
#define S57PARSESCANNER_H

#include <vector>
#include <list>
#include <string>

#include "../geo/utmproject.h"

#include "s57_utils.h"
#include "s57_record.h"
#include "iso8211_gloabal.h"

class ISO8211_EXPORT DsItem
{
private:
    std::string      _path;
    std::string      _family; // File name of the data set, without extension.
    std::vector<int> _upNums; // Update numbers, in ASC order.

public:
    bool _isAlive;

public:
    // Constructs a DsItem using the file path and name,
    // but without externsion.
    DsItem(std::string pathName);

    // Returns the member _path
    std::string path() const;
    // Returns the member _family
    std::string family() const;
    // Returns the filename of the data set. (_path/_family.000)
    std::string dsFile() const;

    bool insertUpdateCell(std::string fileName);

    int updateCount() const;

    class UpdateIte;
    friend class UpdateIte;
    class UpdateIte
    {
    private:
        DsItem *                   _parent;
        std::vector<int>::iterator _it;

        // Constructor for DsItem
        UpdateIte(DsItem *, std::vector<int>::iterator);

    public:
        UpdateIte & operator++();
        std::string operator*() const;
        bool        operator==(const UpdateIte &) const;
        bool        operator!=(const UpdateIte &) const;

        int number() const;

        friend class DsItem;
    };

    UpdateIte updateBegin();
    UpdateIte updateEnd();
};

class ISO8211_EXPORT S57ParseScanner : public S57DatasetScanner
{
private:
    bool                     _updatingEnabled;
    bool                     _precheckEnabled;
    bool                     _ignoreBaseCell;
    std::vector<DsItem>      _dsList;
    Geo::Mercator::DatumType _projDatumType;

    // Temp update cell list
    std::list<std::string> _upCells;

    // S-57 records
    S57DSInfoRecordRef               _dsinfRec;
    S57DSGeoRecordRef                _dsgeoRec;
    S57DSAccuracyRecordRef           _dsaccRec;
    std::vector<S57FeatureRecordRef> _grList; // Geo features
    std::vector<S57FeatureRecordRef> _mrList; // Meta features
    std::vector<S57FeatureRecordRef> _lrList; // Collection features
    std::vector<S57VectorRecordRef>  _vrList; // Vector records

private:
    void init();
    bool checkDsValid(std::string fileName);
    void upcellDispatch();
    void updateDataset(DsItem &);
    void doParse(DsItem &);

    // inherits from S57DatasetScanner
    void onDataset(std::string);

protected:
    void setIgnoreBaseCell(bool);
    bool ignoreBaseCell() const;

    void parseOneDataset(std::string fileName);

    S57FeatureRecordRef findFeatureTarget(const S57_NAME &, s57_b12 objl);
    S57VectorRecordRef  findVectorTarget(const S57_NAME &);

    S57DSInfoRecordRef                       s57DsInfoRecord() const;
    S57DSGeoRecordRef                        s57DsGeoRecord() const;
    S57DSAccuracyRecordRef                   s57DsAccuracyRecord() const;
    const std::vector<S57FeatureRecordRef> & grList() const;
    const std::vector<S57FeatureRecordRef> & mrList() const;
    const std::vector<S57FeatureRecordRef> & lrList() const;
    const std::vector<S57VectorRecordRef> &  vrList() const;

protected:
    virtual void onRecDsInfo(S57DSInfoRecord *);
    virtual void onRecDsGeo(S57DSGeoRecord *);
    virtual void onRecDsAccuracy(S57DSAccuracyRecord *);
    virtual void onRecFeature(S57FeatureRecord *);
    virtual void onRecSpatial(S57VectorRecord *);

    virtual void onPrepareParse(const DsItem &);
    virtual void onParse(const DsItem &);

public:
    S57ParseScanner();
    virtual ~S57ParseScanner() = 0;

    void clear();

    Geo::Mercator::DatumType projDatumType() const;
    void                     setProjDatumType(Geo::Mercator::DatumType);

    bool updatingEnabled() const;
    void setUpdating(bool);

    bool precheckEnabled() const;
    void setPrecheck(bool);

    void scan(std::string path);
};

// S57ParseScanner inline functions

inline void S57ParseScanner::setIgnoreBaseCell(bool on)
{
    _ignoreBaseCell = on;
}

inline bool S57ParseScanner::ignoreBaseCell() const
{
    return _ignoreBaseCell;
}

inline Geo::Mercator::DatumType S57ParseScanner::projDatumType() const
{
    return _projDatumType;
}

inline void S57ParseScanner::setProjDatumType(Geo::Mercator::DatumType dtype)
{
    _projDatumType = dtype;
}

inline bool S57ParseScanner::updatingEnabled() const
{
    return _updatingEnabled;
}

inline void S57ParseScanner::setUpdating(bool on)
{
    _updatingEnabled = on;
}

inline bool S57ParseScanner::precheckEnabled() const
{
    return _precheckEnabled;
}

inline void S57ParseScanner::setPrecheck(bool on)
{
    _precheckEnabled = on;
}

inline S57DSInfoRecordRef S57ParseScanner::s57DsInfoRecord() const
{
    return _dsinfRec;
}

inline S57DSGeoRecordRef S57ParseScanner::s57DsGeoRecord() const
{
    return _dsgeoRec;
}

inline S57DSAccuracyRecordRef S57ParseScanner::s57DsAccuracyRecord() const
{
    return _dsaccRec;
}

inline const std::vector<S57FeatureRecordRef> & S57ParseScanner::grList() const
{
    return _grList;
}

inline const std::vector<S57FeatureRecordRef> & S57ParseScanner::mrList() const
{
    return _mrList;
}

inline const std::vector<S57FeatureRecordRef> & S57ParseScanner::lrList() const
{
    return _lrList;
}

inline const std::vector<S57VectorRecordRef> & S57ParseScanner::vrList() const
{
    return _vrList;
}

// ~

#endif
