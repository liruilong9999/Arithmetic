#ifndef TOOLS_GLOBAL_H__20241101
#define TOOLS_GLOBAL_H__20241101

#include <QtCore/qglobal.h>

#if defined(TOOLS_LIBRARY)
#define TOOLS_EXPORT Q_DECL_EXPORT
#else
#define TOOLS_EXPORT Q_DECL_IMPORT
#endif

#endif
