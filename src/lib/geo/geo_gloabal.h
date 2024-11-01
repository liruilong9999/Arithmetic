#ifndef GEO_GLOBAL_H__20241101
#define GEO_GLOBAL_H__20241101

#include <QtCore/qglobal.h>

#if defined(GEO_LIBRARY)
#define GEO_EXPORT Q_DECL_EXPORT
#else
#define GEO_EXPORT Q_DECL_IMPORT
#endif

#endif
