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
    char* data = _load_file(path,"r", fileSize);
    *fileSize -= 1;
    data[*fileSize] = '\0';
    return data;
}

static char*
filename_get_ext(char *filename) {
    char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

#endif /* FILELOAD_H */
