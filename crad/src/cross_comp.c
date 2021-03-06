#include "cross_comp.h"

#if defined(__unix__) || defined(__APPLE__)

#include "sys/stat.h"
#include "sys/types.h"
#include "unistd.h"

#else

#include "Shlobj.h"
#include "Shlobj_core.h"

#endif /* __unix__ || __APPLE__ */

#include "limits.h"
#include "stdio.h"
#include "string.h"

#if defined(__unix__) || defined(__APPLE__)

/** @brief Make multiple directories
 * @details Creates the directories of the passed path that do not
 * already exist in the file system.
 * @param path Path
 * @param mode Access mode mask
 * @return Zero if success, an error status code otherwise. */
int mkdirp(const char *path, int mode) {
  char buffer[PATH_MAX];
  strncpy(buffer, path, PATH_MAX - 1);
  char *prev = NULL, *cur = buffer;

  while ((cur = strstr(cur, CCM_FILE_SYSTEM_SEP)) != NULL) {
    prev = cur++;
  }
  *prev = 0;
  struct stat sb;
  if (stat(buffer, &sb) != 0 || !S_ISDIR(sb.st_mode)) {
    mkdirp(buffer, mode);
  }
  *prev = '/';
  return mkdir(buffer, mode);
}

#else

/** @brief Make multiple directories
 * @details Creates the directories of the passed path that do not
 * already exist in the file system.
 * @param path Path
 * @return Zero if success, an error status code otherwise. */
int mkdirp_w(const char *path) { return SHCreateDirectoryEx(NULL, path, NULL); }

#endif /* __unix__ || __APPLE__ */
