#include "pmap_t.h"

#include "cross_comp.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "ctype.h"
#include "errno.h"

#define LM_LEVEL 3
#include "logger.h"

#ifndef PMAP_T_PARAM_KEY_SZ
  #define PMAP_T_PARAM_KEY_SZ 32
#endif
#ifndef PMAP_T_PARAM_VALUE_SZ
  #define PMAP_T_PARAM_VALUE_SZ 128
#endif
#ifndef PMAP_T_STR_MASK
  #define PMAP_T_STR_MASK "%s = %s"
#endif
#ifndef PMAP_T_PARAM_BUFFER_SZ
  #define PMAP_T_PARAM_BUFFER_SZ 128
#endif

struct pmap_pair_st {
	char key[PMAP_T_PARAM_KEY_SZ];
	char value[PMAP_T_PARAM_VALUE_SZ];
};
typedef struct pmap_pair_st param_pair_t;


void alloc_pmap(pmap_t* pmap) {
	pmap->p = (param_pair_t*)realloc(pmap->p, (pmap->n+1)*sizeof(param_pair_t));
#ifdef PMAP_T_SAFE_MODE
	if(!pmap->p) {
		LOGE("Failed to allocate parameter pair.");
		pmap->n = 0;
		return;
	}
#endif /* PMAP_T_SAFE_MODE */
	++pmap->n;
}

/** @brief Initialize parameter map
 * @details Initializes a parameter map object using the given file. If the
 * function fails to open the parameter file, it returns a zero sized map.
 * The function parses key-value pairs using PMAP_T_PARAM_MASK. The key-value
 * array is dynamically initialized and should be deallocated by the user.
 * @param pmap Parameter map
 * @param pfilename Parameter filename
 * @return Zero on success, an error code otherwise. */
int pmap_init(pmap_t* pmap, const char* pfilename) {
	FILE* fh;
	char buf[PMAP_T_PARAM_BUFFER_SZ];
	char *key, *val;

	pmap->p = NULL;
	pmap->n = 0;

	fh = fopen(pfilename, "r");
#ifdef PMAP_T_SAFE_MODE
	if(!fh) {
		LOGE("Failed to open '%s' with error %d", pfilename, errno);
		return -1;
	}
#endif /* PMAP_T_SAFE_MODE */

	while(!feof(fh)) {
		if(fgets(buf, PMAP_T_PARAM_BUFFER_SZ, fh) != NULL) {
			if(strlen(buf)==PMAP_T_PARAM_BUFFER_SZ) {
				LOGE("Line too long in parameter file '%s'", pfilename);
				return -2;
			}
			key = buf;
			val = strchr(buf,'=');
			if(!val) { continue; }
			while(!isspace(*++key));
			*key = 0;
			key = buf;
			while(isspace(*key)) { ++key; };
			*val++ = 0;
			alloc_pmap(pmap);
			strncpy(pmap->p[pmap->n-1].key, key, PMAP_T_PARAM_KEY_SZ-1);
			strncpy(pmap->p[pmap->n-1].value, val, PMAP_T_PARAM_VALUE_SZ-1);
		}
	}

	fclose(fh);

	return 0;
}

/** @brief Add pair
 * @details Adds a key value pair into the parameter map.
 * @param pmap Parameter map 
 * @param key New key
 * @param val New value */
void pmap_add(pmap_t* pmap, const char* key, const char* val) {
	alloc_pmap(pmap);
	strncpy(pmap->p[pmap->n-1].key, key, PMAP_T_PARAM_KEY_SZ-1);
	snprintf(pmap->p[pmap->n-1].value, PMAP_T_PARAM_VALUE_SZ, "%s", val);
}

/** @brief Add pair from int value
 * @details Adds a key value pair into the parameter map. The passed value
 * is of integer data type. The function converts it to string and
 * calls pmap_add().
 * @param pmap Parameter map
 * @param key New key
 * @param val New value */
void pmap_add_int(pmap_t* pmap, const char* key, int val) {
	char sval[PMAP_T_PARAM_VALUE_SZ];
	snprintf(sval, PMAP_T_PARAM_VALUE_SZ, "%d", val);
	pmap_add(pmap, key, sval);
}


/** @brief Add pair from double value
 * @details Adds a key value pair into the parameter map. The passed value
 * is of double data type. The function converts it to string
 * and calls pmap_add(). The conversion  may result to data loss.
 * @param pmap Parameter map
 * @param key New key
 * @param val New value */
void pmap_add_double(pmap_t* pmap, const char* key, double val) {
	char sval[PMAP_T_PARAM_VALUE_SZ];
	snprintf(sval, PMAP_T_PARAM_VALUE_SZ, "%lf", val);
	pmap_add(pmap, key, sval);
}

/** @brief Save to file
 * @details Creates or rewrites a file from the passed filename. The function
 * saves the pairs of the parameter map line by line using the PMAP_T_STR_MASK.
 * @param pmap Parameter map
 * @param pfilename Filename */
void pmap_save(const pmap_t* pmap, const char* pfilename) {
	FILE* fh;
	fh = fopen(pfilename, "w");
#if PMAP_T_SAFE_MODE
	if(!fh) {
		LOGE("Failed to create file '%s' with error %d", pfilename, errno);
		return;
	}
#endif /* PMAP_T_SAFE_MODE */
	
	for(int i=0; i<pmap->n; ++i) {
		fprintf(fh, PMAP_T_STR_MASK "\n", pmap->p[i].key, pmap->p[i].value);
	}

	fclose(fh);
}

/** @brief Deallocate parameter map
 * @details Frees the key-value allocated memory, sets the pointer to NULL and
 * the number of pairs to zero.
 * @param pmap Parameter map */
void pmap_free(pmap_t* pmap) {
	free(pmap->p);
	pmap->p = NULL;
	pmap->n = 0;
}

/** @brief Find in parameter map
 * @details Searches for a pair with key value that matches the given key
 * and returns the value. If no match is found, the function returns NULL.
 * @param pmap Parameter map 
 * @param key Key
 * @return The value that corresponds to the passed key */
const char* pmap_find(const pmap_t* pmap, const char* key) {
	for(int i=0; i<pmap->n; ++i) {
		if(!strcmp(pmap->p[i].key,key)) { return pmap->p[i].value; }
	}
	return NULL;
}

/** @brief Get key
 * @details Returns the key saved at the passed index.
 * @param pmap Parameter map
 * @param i Index
 * @return The key string*/
const char* pmap_gkey(const pmap_t* pmap, int i) {
	return pmap->p[i].key;
}

/** @brief Get value
 * @details Returns the value saved at the passed index.
 * @param pmap Parameter map
 * @param i Index
 * @return The value string */
const char* pmap_gvalue(const pmap_t* pmap, int i) {
	return pmap->p[i].value;
}

/** @brief Copy value
 * @details Copies the value saved at the passed index to the given
 * output buffer.
 * @param valbuf Pointer to output buffer.
 * @param pmap Parameter map
 * @param i Index */
void pmap_cvalue(char**valbuf, const pmap_t* pmap, int i) {
	*valbuf = (char*)calloc(PMAP_T_PARAM_VALUE_SZ,sizeof(char));
	strncpy(*valbuf, pmap->p[i].value, PMAP_T_PARAM_VALUE_SZ);
}
