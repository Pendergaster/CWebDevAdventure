/************************************************************
 * Check license.txt in project root for license information *
 *********************************************************** */

#ifndef FILELOAD_H
#define FILELOAD_H

static void*
_load_file(char* const path, char* const filetype, size_t* fileSize) {
    *fileSize = 0;
    FILE* fp = fopen(path,filetype);
    if(fp == NULL ) return NULL;
    fseek(fp, 0, SEEK_END);
    size_t len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    void* ptrToMem = 0;
    ptrToMem = malloc(len);
    fread(ptrToMem,len,1,fp);
    if(fileSize) *fileSize = len;
    return ptrToMem;
}

static inline void*
load_binary_file(char* const path, size_t* fileSize) {
    return _load_file(path,"rb", fileSize);
}

static inline char*
load_file(char* const path, size_t* fileSize) {
    size_t size;
    char* data = _load_file(path,"r", &size);
    if(!data) return NULL;
    size -= 1;
    data[size] = '\0';
    if(fileSize) *fileSize = size;
    return data;
}

static char*
filename_get_ext(char *filename) {
    char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return NULL;
    return dot + 1;
}

#endif /* FILELOAD_H */
