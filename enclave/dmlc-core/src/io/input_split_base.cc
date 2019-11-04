// Copyright by Contributors
#include <dmlc/logging.h>
#include <dmlc/common.h>
#include <algorithm>
#include "./line_split.h"

#if DMLC_USE_REGEX
#include <regex>
#endif

namespace dmlc {
namespace io {
void InputSplitBase::Init(FileSystem *filesys,
                          const char *uri,
                          size_t align_bytes,
                          const bool recurse_directories) {
  fprintf(stdout, "InputSplitBase::Init 0\n");
  this->filesys_ = filesys;
  // initialize the path
  this->InitInputFileInfo(uri, recurse_directories);
  fprintf(stdout, "InputSplitBase::Init 1\n");
  file_offset_.resize(files_.size() + 1);
  file_offset_[0] = 0;
  for (size_t i = 0; i < files_.size(); ++i) {
    file_offset_[i + 1] = file_offset_[i] + files_[i].size;
    CHECK(files_[i].size % align_bytes == 0)
        << "file do not align by " << align_bytes << " bytes";
  }
  this->align_bytes_ = align_bytes;
  fprintf(stdout, "InputSplitBase::Init 2\n");
}

void InputSplitBase::ResetPartition(unsigned rank,
                                    unsigned nsplit) {
  fprintf(stdout, "InputSplitBase::ResetPartition 0\n");
  size_t ntotal = file_offset_.back();
  size_t nstep = (ntotal + nsplit - 1) / nsplit;
  // align the nstep to 4 bytes
  nstep = ((nstep + align_bytes_ - 1) / align_bytes_) * align_bytes_;
  offset_begin_ = std::min(nstep * rank, ntotal);
  offset_end_ = std::min(nstep * (rank + 1), ntotal);
  offset_curr_ = offset_begin_;
  fprintf(stdout, "InputSplitBase::ResetPartition 1\n");
  if (offset_begin_ == offset_end_) return;
  file_ptr_ = std::upper_bound(file_offset_.begin(),
                               file_offset_.end(),
                               offset_begin_) - file_offset_.begin() - 1;
  file_ptr_end_ = std::upper_bound(file_offset_.begin(),
                                   file_offset_.end(),
                                   offset_end_) - file_offset_.begin() - 1;
  fprintf(stdout, "InputSplitBase::ResetPartition 2\n");
  if (fs_ != NULL) {
    delete fs_; fs_ = NULL;
  }
  fprintf(stdout, "InputSplitBase::ResetPartition 3\n");
  // find the exact ending position
  if (offset_end_ != file_offset_[file_ptr_end_]) {
    CHECK(offset_end_ >file_offset_[file_ptr_end_]);
    CHECK(file_ptr_end_ < files_.size());
    fs_ = filesys_->OpenForRead(files_[file_ptr_end_].path);
    fs_->Seek(offset_end_ - file_offset_[file_ptr_end_]);
    offset_end_ += SeekRecordBegin(fs_);
    delete fs_;
  }
  fprintf(stdout, "InputSplitBase::ResetPartition 4\n");
  fs_ = filesys_->OpenForRead(files_[file_ptr_].path);
  fprintf(stdout, "InputSplitBase::ResetPartition 5\n");
  if (offset_begin_ != file_offset_[file_ptr_]) {
    fs_->Seek(offset_begin_ - file_offset_[file_ptr_]);
    offset_begin_ += SeekRecordBegin(fs_);
  }
  fprintf(stdout, "InputSplitBase::ResetPartition 6\n");
  this->BeforeFirst();
  fprintf(stdout, "InputSplitBase::ResetPartition 7\n");
}

void InputSplitBase::BeforeFirst(void) {
  fprintf(stdout, "InputSplitBase::BeforeFirst 0\n");
  if (offset_begin_ >= offset_end_) return;
  size_t fp = std::upper_bound(file_offset_.begin(),
                               file_offset_.end(),
                               offset_begin_) - file_offset_.begin() - 1;
  fprintf(stdout, "InputSplitBase::BeforeFirst 1\n");
  if (file_ptr_ != fp) {
    delete fs_;
    file_ptr_ = fp;
    fs_ = filesys_->OpenForRead(files_[file_ptr_].path);
  }
  fprintf(stdout, "InputSplitBase::BeforeFirst 2\n");
  // seek to beginning of stream
  fs_->Seek(offset_begin_ - file_offset_[file_ptr_]);
  fprintf(stdout, "InputSplitBase::BeforeFirst 3\n");
  offset_curr_ = offset_begin_;
  tmp_chunk_.begin = tmp_chunk_.end = NULL;
  // clear overflow buffer
  overflow_.clear();
  fprintf(stdout, "InputSplitBase::BeforeFirst 4\n");
}

InputSplitBase::~InputSplitBase(void) {
  delete fs_;
  // no need to delete filesystem, it was singleton
}

std::string InputSplitBase::StripEnd(std::string str, char ch) {
  while (str.length() != 0 && str[str.length() - 1] == ch) {
    str.resize(str.length() - 1);
  }
  return str;
}

std::vector<URI> InputSplitBase::ConvertToURIs(const std::string& uri) {
  fprintf(stdout, "InputSplitBase::ConvertToURIs 0\n");
  // split by :
  const char dlm = ';';
  std::vector<std::string> file_list = Split(uri, dlm);
  std::vector<URI> expanded_list;

  // expand by match regex pattern.
  for (size_t i = 0; i < file_list.size(); ++i) {
    fprintf(stdout, "InputSplitBase::ConvertToURIs 1\n");
    URI path(file_list[i].c_str());
    fprintf(stdout, "InputSplitBase::ConvertToURIs 2\n");
    size_t pos = path.name.rfind('/');
    fprintf(stdout, "InputSplitBase::ConvertToURIs 3\n");
    if (pos == std::string::npos || pos + 1 == path.name.length()) {
      expanded_list.push_back(path);
    } else {
      fprintf(stdout, "InputSplitBase::ConvertToURIs 4\n");
      URI dir = path;
      dir.name = path.name.substr(0, pos);
      std::vector<FileInfo> dfiles;
      fprintf(stdout, "InputSplitBase::ConvertToURIs 4-1\n");
      filesys_->ListDirectory(dir, &dfiles);
      fprintf(stdout, "InputSplitBase::ConvertToURIs 4-2\n");
      bool exact_match = false;
      for (size_t i = 0; i < dfiles.size(); ++i) {
        if (StripEnd(dfiles[i].path.name, '/') == StripEnd(path.name, '/')) {
          expanded_list.push_back(dfiles[i].path);
          exact_match = true;
          break;
        }
      }
      fprintf(stdout, "InputSplitBase::ConvertToURIs 5\n");
#if DMLC_USE_REGEX
      fprintf(stdout, "InputSplitBase::ConvertToURIs 6\n");
      if (!exact_match) {
        std::string spattern = path.name;
        try {
          std::regex pattern(spattern);
          for (size_t i = 0; i < dfiles.size(); ++i) {
            if (dfiles[i].type != kFile || dfiles[i].size == 0) continue;
            std::string stripped = StripEnd(dfiles[i].path.name, '/');
            std::smatch base_match;
            if (std::regex_match(stripped, base_match, pattern)) {
              for (size_t j = 0; j < base_match.size(); ++j) {
                if (base_match[j].str() == stripped) {
                  expanded_list.push_back(dfiles[i].path); break;
                }
              }
            }
          }
        } catch (std::regex_error& e) {
          //LOG(FATAL) << e.what() << " bad regex " << spattern
          //           << "This could due to compiler version, g++-4.9 is needed";
          fprintf(stdout, "ConvertToURIs FAILED\n");
        }
        fprintf(stdout, "InputSplitBase::ConvertToURIs 7\n");
      }
#endif  // DMLC_USE_REGEX
    }
  }
  return expanded_list;
}

void InputSplitBase::InitInputFileInfo(const std::string& uri,
                                       const bool recurse_directories) {
  fprintf(stdout, "InputSplitBase::InitInputFileInfo 0\n");
  std::vector<URI> expanded_list = this->ConvertToURIs(uri);
  fprintf(stdout, "InputSplitBase::InitInputFileInfo 1\n");
  for (size_t i = 0; i < expanded_list.size(); ++i) {
    const URI& path = expanded_list[i];
    fprintf(stdout, "InputSplitBase::InitInputFileInfo 1-1\n");
    FileInfo info = filesys_->GetPathInfo(path);
    fprintf(stdout, "InputSplitBase::InitInputFileInfo 1-2\n");
    if (info.type == kDirectory) {
      std::vector<FileInfo> dfiles;
      if (!recurse_directories) {
        fprintf(stdout, "InputSplitBase::InitInputFileInfo 1-3\n");
        filesys_->ListDirectory(info.path, &dfiles);
        fprintf(stdout, "InputSplitBase::InitInputFileInfo 1-4\n");
      } else {
        fprintf(stdout, "InputSplitBase::InitInputFileInfo 1-5\n");
        filesys_->ListDirectoryRecursive(info.path, &dfiles);
        fprintf(stdout, "InputSplitBase::InitInputFileInfo 1-6\n");
      }
      for (size_t i = 0; i < dfiles.size(); ++i) {
        if (dfiles[i].size != 0 && dfiles[i].type == kFile) {
          files_.push_back(dfiles[i]);
        }
      }
    } else {
      if (info.size != 0) {
        files_.push_back(info);
      }
    }
  }
  fprintf(stdout, "InputSplitBase::InitInputFileInfo 2\n");
  CHECK_NE(files_.size(), 0U)
      << "Cannot find any files that matches the URI pattern " << uri;
}

size_t InputSplitBase::Read(void *ptr, size_t size) {
  fprintf(stdout, "InputSplitBase::Read 0\n");
  const bool is_text_parser = this->IsTextParser();

  if (fs_ == NULL) {
    return 0;
  }
  if (offset_begin_ >= offset_end_) return 0;
  if (offset_curr_ +  size > offset_end_) {
    size = offset_end_ - offset_curr_;
  }
  if (size == 0) return 0;
  size_t nleft = size;
  char *buf = reinterpret_cast<char*>(ptr);
  while (true) {
    size_t n = fs_->Read(buf, nleft);
    nleft -= n; buf += n;
    offset_curr_ += n;
    if (nleft == 0) break;
    if (n == 0) {
      if (is_text_parser) {
        // Insert a newline between files to handle files with NOEOL.
        // See https://github.com/dmlc/dmlc-core/pull/385 for explanation.
        buf[0] = '\n'; ++buf; --nleft;
      }
      if (offset_curr_ != file_offset_[file_ptr_ + 1]) {
        //LOG(ERROR) << "curr=" << offset_curr_
        //           << ",begin=" << offset_begin_
        //           << ",end=" << offset_end_
        //           << ",fileptr=" << file_ptr_
        //           << ",fileoffset=" << file_offset_[file_ptr_ + 1];
        //for (size_t i = 0; i < file_ptr_; ++i) {
        //  LOG(ERROR) << "offset[" << i << "]=" << file_offset_[i];
        //}
        //LOG(FATAL) << "file offset not calculated correctly";
        fprintf(stdout, "Read FAILED\n");
      }
      if (file_ptr_ + 1 >= files_.size()) break;
      file_ptr_ += 1;
      delete fs_;
      fs_ = filesys_->OpenForRead(files_[file_ptr_].path);
    }
  }
  fprintf(stdout, "InputSplitBase::Read 1\n");
  return size - nleft;
}

bool InputSplitBase::ReadChunk(void *buf, size_t *size) {
  fprintf(stdout, "InputSplitBase::ReadChunk 0\n");
  size_t max_size = *size;
  if (max_size <= overflow_.length()) {
    *size = 0; return true;
  }
  if (overflow_.length() != 0) {
    std::memcpy(buf, BeginPtr(overflow_), overflow_.length());
  }
  size_t olen = overflow_.length();
  overflow_.resize(0);
  size_t nread = this->Read(reinterpret_cast<char*>(buf) + olen,
                            max_size - olen);
  nread += olen;
  if (nread == 0) return false;
  if (this->IsTextParser()) {
    if (nread == olen) {
      // Insert a newline between files to handle files with NOEOL.
      // See https://github.com/dmlc/dmlc-core/pull/452 for explanation.
      char *bufptr = reinterpret_cast<char*>(buf);
      bufptr[nread] = '\n';
      nread++;
    }
  } else {
    if (nread != max_size) {
      *size = nread;
      return true;
    }
  }
  const char *bptr = reinterpret_cast<const char*>(buf);
  // return the last position where a record starts
  const char *bend = this->FindLastRecordBegin(bptr, bptr + nread);
  *size = bend - bptr;
  overflow_.resize(nread - *size);
  if (overflow_.length() != 0) {
    std::memcpy(BeginPtr(overflow_), bend, overflow_.length());
  }
  fprintf(stdout, "InputSplitBase::ReadChunk 1\n");
  return true;
}

bool InputSplitBase::Chunk::Load(InputSplitBase *split, size_t buffer_size) {
  fprintf(stdout, "InputSplitBase::Chunk::Load 0\n");
  data.resize(buffer_size + 1);
  while (true) {
    // leave one tail chunk
    size_t size = (data.size() - 1) * sizeof(uint32_t);
    // set back to 0 for string safety
    data.back() = 0;
    if (!split->ReadChunk(BeginPtr(data), &size)) return false;
    if (size == 0) {
      data.resize(data.size() * 2);
    } else {
      begin = reinterpret_cast<char *>(BeginPtr(data));
      end = begin + size;
      break;
    }
  }
  fprintf(stdout, "InputSplitBase::Chunk::Load 1\n");
  return true;
}

bool InputSplitBase::Chunk::Append(InputSplitBase *split, size_t buffer_size) {
  fprintf(stdout, "InputSplitBase::Chunk::Append 0\n");
  size_t previous_size = end - begin;
  data.resize(data.size() + buffer_size);
  while (true) {
    // leave one tail chunk
    size_t size = buffer_size * sizeof(uint32_t);
    // set back to 0 for string safety
    data.back() = 0;
    if (!split->ReadChunk(reinterpret_cast<char *>(BeginPtr(data)) + previous_size, &size))
      return false;
    if (size == 0) {
      data.resize(data.size() * 2);
    } else {
      begin = reinterpret_cast<char *>(BeginPtr(data));
      end = begin + previous_size + size;
      break;
    }
  }
  fprintf(stdout, "InputSplitBase::Chunk::Append 1\n");
  return true;
}

bool InputSplitBase::ExtractNextChunk(Blob *out_chunk, Chunk *chunk) {
  fprintf(stdout, "InputSplitBase::ExtractNextChunk 0\n");
  if (chunk->begin == chunk->end) return false;
  out_chunk->dptr = chunk->begin;
  out_chunk->size = chunk->end - chunk->begin;
  chunk->begin = chunk->end;
  fprintf(stdout, "InputSplitBase::ExtractNextChunk 1\n");
  return true;
}
}  // namespace io
}  // namespace dmlc
