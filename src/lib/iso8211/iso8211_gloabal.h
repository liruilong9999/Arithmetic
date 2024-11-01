#ifndef ISO8211_GLOBAL_H__20241101
#define ISO8211_GLOBAL_H__20241101

#include <QtCore/qglobal.h>

#if defined(ISO8211_LIBRARY)
#define ISO8211_EXPORT Q_DECL_EXPORT
#else
#define ISO8211_EXPORT Q_DECL_IMPORT
#endif

#endif
