
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <xgboost/c_api.h>
#include <vector>
#include <dmlc/io.h>
#include <cstring>

#include "xgboost_u.h"

extern "C" {
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
}

void host_helloworld() {
  LOG(INFO) << "Ocall: host_helloworld";
}

DIR* host_opendir(char* path) {
  LOG(INFO) << "Ocall: opendir";
  return opendir(path);
}

struct stat host_stat(char* path) {
  LOG(INFO) << "Ocall: stat";
  struct stat sb;
  if (stat(path, &sb) == -1) {
    int errsv = errno;
    LOG(FATAL) << "LocalFileSystem.GetPathInfo: "
      << path << " error: " << strerror(errsv);
  }
  return sb;
}

FILE* host_fopen(char* fname, char* flag) {
  LOG(INFO) << "Ocall: fopen";
  FILE* fp;
#if DMLC_USE_FOPEN64
  fp = fopen64(fname, flag);
#else  // DMLC_USE_FOPEN64
  fp = fopen(fname, flag);
#endif  // DMLC_USE_FOPEN64
  return fp;
}

void host_fclose(FILE* fp) {
  LOG(INFO) << "Ocall: fclose";
  std::fclose(fp);
}

FILE* host_fseek(FILE* fp, long pos) {
  LOG(INFO) << "Ocall: fseek";
#ifndef _MSC_VER
  CHECK(!std::fseek(fp, pos, SEEK_SET));  // NOLINT(*)
#else  // _MSC_VER
  CHECK(!_fseeki64(fp, pos, SEEK_SET));
#endif  // _MSC_VER
  return fp;
}

// Buffer should be freed by caller
void* host_fread_one(FILE* fp, size_t size) {
  LOG(INFO) << "Ocall: fread";
  char* buffer = (char*) malloc (sizeof(char)*size);
  int n = std::fread(buffer, 1, size, fp);
  return buffer;
}

void host_fwrite_one(void* buffer, size_t count, FILE* fp) {
  LOG(INFO) << "Ocall: fwrite";
  CHECK(std::fwrite(const_cast<const void*> (buffer), 1, count, fp) == count) 
    << "FileStream.Write incomplete";
}

//std::vector<dmlc::io::FileInfo>* host_opendir_and_readdir(char* path) {
void* host_opendir_and_readdir(char* path) {
  LOG(INFO) << "Ocall: opendir_and_readdir";
  DIR *dir = opendir(path);
  if (dir == NULL) {
    int errsv = errno;
    LOG(FATAL) << "LocalFileSystem.ListDirectory " << path
      <<" error: " << strerror(errsv);
  }
  struct dirent *ent;
  std::vector<char*>* out_list = new std::vector<char*>;
  while ((ent = readdir(dir)) != NULL) {
    if (!strcmp(ent->d_name, ".")) continue;
    if (!strcmp(ent->d_name, "..")) continue;
    out_list->push_back(ent->d_name);
  }
  closedir(dir);
  return (void*)out_list;
}
