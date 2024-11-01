#ifndef S57_MODULE_H
#define S57_MODULE_H

#include <stdio.h>

#include <string>

#include "s57_utils.h"
#include "s57_record.h"
#include "iso8211_gloabal.h"

class ISO8211_EXPORT S57Module : public RefBase
{
private:
	FILE *_fp;
	std::string _fileName;

	Ref<S57DataDescripRecord> _ddr;
	Ref<S57DSInfoRecord> _dr_dsInfo;
	Ref<S57DSGeoRecord> _dr_dsGeo;
	Ref<S57DSAccuracyRecord> _dr_dsAcc;
	Ref<S57CatalogDirRecord> _dr_catDir;

private:
	void init();

public:
	S57Module();
	S57Module(std::string fileName);
	~S57Module();

	bool open(std::string fileName);
	void close();

	bool isOpen() const;
	bool atEnd() const;

	std::string fileName() const;

	Ref<S57DataDescripRecord> ddr() const;
	Ref<S57DSInfoRecord> generalInfoRecord() const;
	Ref<S57DSGeoRecord> geographicRecord() const;
	Ref<S57DSAccuracyRecord> accuracyRecord() const;
	Ref<S57CatalogDirRecord> catalogDirRecord() const;

	// Returns lexical level code
	int aall() const;
	int nall() const;

	S57RecordRef getNextRecord();
};

// Inline functions of S57Module

inline std::string S57Module::fileName() const
{ return _fileName; }

inline bool S57Module::isOpen() const
{ return _fp != NULL; }

inline Ref<S57DataDescripRecord> S57Module::ddr() const
{ return _ddr; }

inline Ref<S57DSInfoRecord> S57Module::generalInfoRecord() const
{ return _dr_dsInfo; }

inline Ref<S57DSGeoRecord> S57Module::geographicRecord() const
{ return _dr_dsGeo; }

inline Ref<S57DSAccuracyRecord> S57Module::accuracyRecord() const
{ return _dr_dsAcc; }

inline Ref<S57CatalogDirRecord> S57Module::catalogDirRecord() const
{ return _dr_catDir; }

#endif
