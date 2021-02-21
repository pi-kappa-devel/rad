/** @file cross_comp.h
 * @brief Cross compilation functionality.
 * @details Contains compiler and OS specific functionality. The main
 * implementation targets are unix based systems. This file contains the
 * adjustments needed to support also windows targets. */

#ifndef _CC_MACROS_H_
#define _CC_MACROS_H_

#if defined(__unix__) || defined(__APPLE__)
#define CCM_FILE_SYSTEM_SEP "/"

int mkdirp(const char *path, int mode);

#elif defined(_WIN32) || defined(_WIN64)
#define CCM_FILE_SYSTEM_SEP "\\"

#define _CRT_SECURE_NO_WARNINGS

#define __PRETTY_FUNCTION__ __FUNCSIG__

#define mkdirp(path, mode) mkdirp_w(path)

int mkdirp_w(const char *path);

#define localtime_r(prawtime, ptm) localtime_s(ptm, prawtime);
#define asctime_r(ptm, timestamp) asctime_s(timestamp, 26, ptm);

#else
#error Operating system is not supported.

#endif /* __unix__ || __APPLE__ */

/* Stringification macros */

#define _CCM_STRINGIFY_EVAL_(x) #x
#define CCM_STRINGIFY(x) _CCM_STRINGIFY_EVAL_(x)

#endif /* _CC_MACROS_H_ */
