#ifndef S57EXTRACT_H
#define S57EXTRACT_H

#include <string>

#include "s57parsescanner.h"

#include "iso8211_gloabal.h"

class ISO8211_EXPORT S57Extract : public S57ParseScanner
{
private:
    std::string _targetDs;
    int         _targetObjl;
    std::string _outputPath;
    double      _comf;

private:
    // inherits form S57ParseScanner
    void onRecDsGeo(S57DSGeoRecord *);
    void onPrepareParse(const DsItem &);
    void onParse(const DsItem &);

    // interface to be hidden
    void setProjDatumType(Geo::Mercator::DatumType);
    void clear();
    void scan(std::string);

    void writeMifHeader(FILE *);
    void writeSounding(const S57FeatureRecord *, FILE * mif, FILE * mid);
    void writePointObject(const S57FeatureRecord *, FILE * mif, FILE * mid);
    void writeLineObject(const S57FeatureRecord *, FILE * mif, FILE * mid);
    void writeAreaObject(const S57FeatureRecord *, FILE * mif, FILE * mid);

    void init();

public:
    S57Extract();
    S57Extract(std::string targetDs, std::string outputPath);
    ~S57Extract();

    void        setTargetDataset(std::string fileName);
    std::string targetDataset() const;

    void        setOutputPath(std::string path);
    std::string outputPath() const;

    void extract(int objl);
};

// S57Extract inline functions

inline void S57Extract::setTargetDataset(std::string fileName)
{
    _targetDs = fileName;
}

inline std::string S57Extract::targetDataset() const
{
    return _targetDs;
}

inline std::string S57Extract::outputPath() const
{
    return _outputPath;
}

// ~

#endif
