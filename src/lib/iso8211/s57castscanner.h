#ifndef S57CASTSCANNER_H
#define S57CASTSCANNER_H

#include <vector>
#include <list>
#include <string>

#include "ir_struct.h"
#include "s57_module.h"
#include "s57parsescanner.h"

struct RTree;

class S57CastScanner : public S57ParseScanner
{
private:
	std::string _outputPath;

	// Statistics
	std::vector<IR_ModuleEntry *> _irModuleDir;

	// Variables for casting a dataset
	IR_DatasetParam _irParam;
	double _comf;

private:
	static bool loadModuleList(std::string indexFile, 
					std::vector<IR_ModuleEntry *> &v, 
					int *major = NULL, int *minor = NULL);

private:
	void init();

	void registerCurModule();

	void saveIrFile(FILE *fp);
	void writeRTreeArea(FILE *fp, struct RTree *tree);

protected:
	// inherits from S57ParseScanner
	virtual void onRecDsInfo(S57DSInfoRecord *);
	virtual void onRecDsGeo(S57DSGeoRecord *);
	virtual void onRecDsAccuracy(S57DSAccuracyRecord *);
	virtual void onPrepareParse(const DsItem &);
	virtual void onParse(const DsItem &);

public:
	// Constructs a empty object.
	S57CastScanner();
	// Constructs a object with an exist module directory.
	// The object will load the file "index"
	S57CastScanner(std::string moduleDir);
	virtual ~S57CastScanner();

	// override S57ParseScanner
	void setProjDatumType(Geo::Mercator::DatumType);

	void setIndexFile(std::string moduleDir);

	std::string outputPath() const;
	void setOutputPath(std::string);

	const IR_ModuleEntry *castDataset(std::string filepath);

	std::vector<IR_ModuleEntry> getModuleList() const;
	bool removeModuleEntry(std::string name);

	void saveIndexFile();

	static void listModuleEntries(std::string indexFile);
};

// S57CastScanner inline functions

inline std::string S57CastScanner::outputPath() const
{ return _outputPath; }

// ~

#endif
