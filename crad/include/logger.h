/** @file logger.h
 * @brief Logging macros
 * @details To reset the logging level in a file, (re)define the macro
 *   RAD_LOG_LEVEL
 * and include this file. */

#ifndef _LOGGER_MACROS_H_
#define _LOGGER_MACROS_H_

/* Environment dependent configurations */
#ifdef __unix__
  #define lock_output_buffer(buffer) flockfile(buffer)
  #define unlock_output_buffer(buffer) funlockfile(buffer)
  #define _LOG_THIS_THREAD_ID (unsigned long)pthread_self()
#else
  #define lock_output_buffer(buffer) _lock_file(buffer)
  #define unlock_output_buffer(buffer) _unlock_file(buffer)
  #define _LOG_THIS_THREAD_ID 
#endif /* __unix__ */

/* Create log mode strings */
#define _LOG_ERROR_TAG "Error         : "
#define _LOG_WARN_TAG "Warning       : "
#define _LOG_INFO_TAG "Info          : "
#define _LOG_VERB_TAG "Verbose       : "
#define _LOG_DEBUG_TAG "Debug         : "
#define _LOG_FUNCTION_TAG "Function      : "
#define _LOG_THREAD_TAG "Thread %ld : ", _LOG_THIS_THREAD_ID


/* Set log buffers */
#define _LOG_ERROR_BUFFER stderr
#define _LOG_ERROR_FILE NULL
#define _LOG_WARN_BUFFER stdout
#define _LOG_WARN_FILE NULL
#define _LOG_INFO_BUFFER stdout
#define _LOG_INFO_FILE NULL
#define _LOG_VERB_BUFFER stdout
#define _LOG_VERB_FILE NULL
#define _LOG_DEBUG_BUFFER stdout
#define _LOG_DEBUG_FILE NULL
#define _LOG_FUNCTION_BUFFER stdout
#define _LOG_FUNCTION_FILE NULL
#define _LOG_THREAD_BUFFER stdout
#define _LOG_THREAD_FILE NULL

/* Define logging macro with locking */
#define _LOG_OUT_AP(mode, ...) { \
		lock_output_buffer(_LOG##mode##BUFFER); \
		fprintf(_LOG##mode##BUFFER, _LOG##mode##TAG); \
		fprintf(_LOG##mode##BUFFER, __VA_ARGS__); \
		fprintf(_LOG##mode##BUFFER, "\n"); \
		unlock_output_buffer(_LOG##mode##BUFFER); \
	} 
#define _LOG_OUT(mode, ...) \
	_LOG_OUT_AP(mode,  __VA_ARGS__)

#endif /* _LOGGER_MACROS_H_ */

/* Define logging macro */
#ifdef LOGE
  #undef LOGE
#endif
#if (LM_LEVEL>0)
  #define LOGE(...) _LOG_OUT(_ERROR_, __VA_ARGS__)
#else
  #define LOGE(...)
#endif

#ifdef LOGW
  #undef LOGW
#endif
#if (LM_LEVEL>1)
  #define LOGW(...) _LOG_OUT(_WARN_, __VA_ARGS__)
#else
  #define LOGW(...)
#endif

#ifdef LOGI
  #undef LOGI
#endif
#if (LM_LEVEL>2)
  #define LOGI(...) _LOG_OUT(_INFO_, __VA_ARGS__)
#else
  #define LOGI(...)
#endif

#ifdef LOGV
  #undef LOGV
#endif
#if (LM_LEVEL>3)
  #define LOGV(...) _LOG_OUT(_VERB_, __VA_ARGS__)
#else
  #define LOGV(...)
#endif

#ifdef LOGD
  #undef LOGD
#endif
#if (LM_LEVEL>4)
  #define LOGD(...) _LOG_OUT(_DEBUG_, __VA_ARGS__)
#else
  #define LOGD(...)
#endif

#ifdef LOGF
  #undef LOGF
#endif
#if (LM_LEVEL>5)
  #define LOGF() _LOG_OUT(_FUNCTION_, "%s", __PRETTY_FUNCTION__)
#else
  #define LOGF(...)
#endif

#ifdef LOGT
  #undef LOGT
#endif
#if (LM_LEVEL>6)
  #define LOGT(...) _LOG_OUT_AP(_THREAD_, __VA_ARGS__)
#else
  #define LOGT(...)
#endif
