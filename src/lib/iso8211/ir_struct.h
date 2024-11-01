// Defination of S-57 related-image structures.

#ifndef IR_STRUCT_H
#define IR_STRUCT_H

#ifdef __cplusplus
extern "C"
{
#endif

typedef unsigned char  UInt8;
typedef unsigned short UInt16;
typedef unsigned long  UInt32;
typedef long           Int32;

#define IRV_MAJOR 1
#define IRV_MINOR 1

#pragma pack(2)
typedef struct IR_DataAreaLeader
{
    char   dataIdentifier;
    UInt8  dataVersion; // Data version
    char   extension[4];
    UInt32 areaLength; // Number of bytes in data area
    UInt32 dataOffset; // Start address of contents
    UInt32 dirSize;    // Number of directory entries
} IR_DataAreaLeader;

typedef struct IR_DirEntry
{
    UInt16 label;
    UInt32 pos;  // Offset or index
    UInt32 size; // Bytes or entries
} IR_DirEntry;

typedef struct IR_DatasetParam
{
    char   dsnm[16]; // Data set name, terminated by '\0'
    UInt8  intu;     // Intended usage
    UInt16 edtn;     // Edition number
    UInt16 updn;     // Update number
    UInt8  prsp;     // Product specification
    UInt16 agen;     // Producing agency

    UInt32 nomr; // Number of meta records
    UInt32 nogr; // Number of geo records
    UInt32 nolr; // Number of collection records
    UInt32 noin; // Number of isolated node records
    UInt32 nocn; // Number of connected node records
    UInt32 noed; // Number of edge records

    UInt8  mpdt; // Mercator projection datum
    UInt8  hdat; // Horizontal geodetic datum
    UInt8  vdat; // Vertical datum
    UInt8  sdat; // Sounding datum
    UInt32 cscl; // Compilation scale of data

    UInt32 pacc; // *Absolute positional accuracy in decimeters
    UInt32 hacc; // *Absolute horizontal/vertical accuracy in decimeters
    UInt32 sacc; // *Absolute sounding accuracy in decimeters

    Int32 y_max; // Northernmost Y of data coverage in decimeters
    Int32 x_max; // Easternmost X of data coverage in decimeters
    Int32 y_min; // Southernmost Y of data coverage in decimeters
    Int32 x_min; // Westernmost X of data coverage in decimeters
} IR_DatasetParam;

typedef struct IR_FeatureRec
{
    UInt8  rcnm; // 100
    UInt32 rcid;
    UInt8  prim; // Object geometric primitive
    UInt8  grup; // Group
    UInt16 objl; // Object label/code

    UInt16 agen; // Producing agency
    UInt32 fidn; // Feature identification number
    UInt16 fids; // Feature identification subdivision

    UInt32 attrPos;   // Start index of attribute entry
    UInt8  attrCount; // Number of attribute entries

    UInt32 ffptPos;   // Start index of FFPT entry
    UInt16 ffptCount; // Number of FFPT entries

    UInt32 fsptPos;   // Start index of FSPT entry
    UInt16 fsptCount; // Number of FSPT entries

    Int32  y_max;     // Northernmost Y of the feature in decimeters
    Int32  x_max;     // Easternmost X of the feature in decimeters
    Int32  y_min;     // Southernmost Y of the feature in decimeters
    Int32  x_min;     // Westernmost X of the feature in decimeters
    UInt32 scale_max; // Value of attribute SCAMAX
    UInt32 scale_min; // Value of attribute SCAMIN
} IR_FeatureRec;

typedef struct IR_SpatialRec
{
    UInt8  rcnm;
    UInt32 rcid;

    UInt8 isClosed; // If is a closed edge.

    UInt8  pairSize;   // Coordinate pair size, 2(SG2D) or 3(SG3D)
    UInt32 coordPos;   // Start index of coordinate
    UInt32 coordCount; // Number of coordinate

    UInt32 posacc; // Value of attribute POSACC in decimeters
    UInt8  quapos; // Value of attribute QUAPOS in decimeters
} IR_SpatialRec;

typedef struct IR_FFPtrRec
{
    UInt32 pos;  // The feature record references to
    UInt8  rind; // Relationship indicator
} IR_FFPtrRec;

typedef struct IR_FSPtrRec
{
    UInt32 pos;  // The spatial record references to
    UInt8  ornt; // Orentation
    UInt8  usag; // Usage indicator
    UInt8  mask; // Masking indicator
} IR_FSPtrRec;

typedef struct IR_ModuleEntry
{
    UInt16 id;
    char   dsnm[16];
    UInt8  mpdt;
    UInt8  intu;
    UInt16 edtn;
    UInt16 updn;
    UInt32 scale;
    Int32  y_max; // Northernmost Y of data coverage in decimeters
    Int32  x_max; // Easternmost X of data coverage in decimeters
    Int32  y_min; // Southernmost Y of data coverage in decimeters
    Int32  x_min; // Westernmost X of data coverage in decimeters
} IR_ModuleEntry;
#pragma pack()

#ifdef __cplusplus
}
#endif

#endif
