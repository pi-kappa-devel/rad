/** @file rad_conf.h
 * @brief Configuration file 
 * @details The file is auto-generated by cmake. It contains 
 *  - versioning information,
 *  - file path information for data and temporary files and 
 *  - thread configuration. */

#ifndef _RAD_CONF_H_
#define _RAD_CONF_H_

/** rad major version */
#define RAD_VERSION_MAJOR 1
/** rad minor version */
#define RAD_VERSION_MINOR 4
/** rad patch version */
#define RAD_VERSION_PATCH 0
/** rad tweak version */
#define RAD_VERSION_TWEAK 

/** Data's path */
#define RAD_DATA_DIR "/home/ntelispak/workspace/rad/data"
/** Temporary data's path */
#define RAD_TEMP_DIR "/home/ntelispak/workspace/rad/tmp"
/** Path buffer size */
#define RAD_PATH_BUFFER_SZ 512

/** Log frequency in iterations */
#define RAD_LOG_CYCLE 20
/** Save frequency in iterations */
#define RAD_SAVE_CYCLE 100

/** Number of threads */
#define RAD_NUM_THREADS 3

#endif /* _RAD_CONF_H_ */
