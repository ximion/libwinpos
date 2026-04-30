#pragma once
#include <QtCore/QtGlobal>

#ifdef WINPOSQT_BUILD_LIBRARY
#define WINPOSQT_EXPORT Q_DECL_EXPORT
#else
#define WINPOSQT_EXPORT Q_DECL_IMPORT
#endif
