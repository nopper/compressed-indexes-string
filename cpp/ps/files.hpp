#pragma once

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <memory>

#ifndef __linux__
#define POSIX_FADV_NORMAL 0 /* [MC1] no further special treatment */
#define POSIX_FADV_RANDOM 1 /* [MC1] expect random page refs */
#define POSIX_FADV_SEQUENTIAL 2 /* [MC1] expect sequential page refs */
#define POSIX_FADV_WILLNEED 3 /* [MC1] will need these pages */
#define POSIX_FADV_DONTNEED 4 /* [MC1] dont need these pages */
#endif

namespace ps {
namespace files {

class writable_file : boost::noncopyable {
 public:
  writable_file()
  {}

  virtual ~writable_file()
  {}

  virtual bool append(const char * src, size_t length) = 0;
  virtual bool append(const std::string& data) {
    return append(data.data(), data.size());
  }
  virtual bool close() = 0;
  virtual bool flush() = 0;
  virtual bool sync() = 0;
  virtual uint64_t get_file_size() {
    return 0;
  }
};

class random_access_file {
 public:
  random_access_file() { }
  virtual ~random_access_file() {};

  virtual bool read(uint64_t offset, size_t n, std::string* result,
                    char* scratch) const = 0;


  enum AccessPattern { NORMAL, RANDOM, SEQUENTIAL, WILLNEED, DONTNEED };

  virtual void hint(AccessPattern pattern) {}

  // Remove any kind of caching of data from the offset to offset+length
  // of this file. If the length is 0, then it refers to the end of file.
  // If the system is not caching the file contents, then this is a noop.
  virtual bool invalidate_cache(size_t offset, size_t length)
  {
    return false;
  }
};

// Use posix write to write data to a file.
class posix_writable_file : public writable_file {
 private:
  const std::string filename_;
  int fd_;
  size_t cursize_;      // current size of cached data in buf_
  size_t capacity_;     // max size of buf_
  std::unique_ptr<char[]> buf_;           // a buffer to cache writes
  uint64_t filesize_;
  bool pending_sync_;

 public:
  posix_writable_file(const std::string& fname, int fd, size_t capacity)
      : filename_(fname),
        fd_(fd),
        cursize_(0),
        capacity_(capacity),
        buf_(new char[capacity]),
        filesize_(0),
        pending_sync_(false)
  {}

  ~posix_writable_file() {
    if (fd_ >= 0) {
      posix_writable_file::close();
    }
  }

  virtual bool append(const char * src, size_t length)
  {
    size_t left = length;
    bool s;
    pending_sync_ = true;

    // if there is no space in the cache, then flush
    if (cursize_ + left > capacity_)
    {
      s = flush();
      if (!s)
        return false;
      // Increase the buffer size, but capped at 1MB
      if (capacity_ < (1<<20))
      {
        capacity_ *= 2;
        buf_.reset(new char[capacity_]);
      }
      assert(cursize_ == 0);
    }

    // if the write fits into the cache, then write to cache
    // otherwise do a write() syscall to write to OS buffers.
    if (cursize_ + left <= capacity_)
    {
      memcpy(buf_.get()+cursize_, src, left);
      cursize_ += left;
    }
    else
    {
      while (left != 0)
      {
        ssize_t done = write(fd_, src, left);
        if (done < 0)
        {
          if (errno == EINTR)
            continue;
          return false;
        }

        left -= done;
        src += done;
      }
    }
    filesize_ += length;
    return true;
  }

  virtual bool close()
  {
    bool s = flush(); // flush cache to OS

    if (!s)
      return s;

    if (::close(fd_) < 0)
      s = false;

    fd_ = -1;
    return s;
  }

  virtual bool flush()
  {
    size_t left = cursize_;
    char* src = buf_.get();
    while (left != 0)
    {
      ssize_t done = write(fd_, src, left);
      if (done < 0)
      {
        if (errno == EINTR)
          continue;
        return false;
      }
      left -= done;
      src += done;
    }
    cursize_ = 0;

    return true;
  }

  virtual bool sync() {
    bool s = flush();
    if (!s)
      return false;
    if (pending_sync_ && fsync(fd_) < 0) {
      return false;
    }
    pending_sync_ = false;
    return true;
  }

  virtual uint64_t get_file_size() {
    return filesize_;
  }
};

int Fadvise(int fd, off_t offset, size_t len, int advice)
{
#ifdef OS_LINUX
  return posix_fadvise(fd, offset, len, advice);
#else
  return 0;  // simply do nothing.
#endif
}

class posix_random_access_file: public random_access_file {
 private:
  std::string filename_;
  int fd_;
  bool use_os_buffer_;

 public:
  posix_random_access_file(const std::string& fname, int fd, bool use_os_buffer)
      : filename_(fname), fd_(fd), use_os_buffer_(use_os_buffer)
  {}

  virtual ~posix_random_access_file() { close(fd_); }

  virtual bool read(uint64_t offset, size_t n, std::string* result,
                    char* scratch) const
  {
    ssize_t r = -1;
    size_t left = n;
    char* ptr = scratch;
    while (left > 0)
    {
      r = pread(fd_, ptr, left, static_cast<off_t>(offset));
      if (r <= 0)
      {
        if (errno == EINTR)
          continue;
        break;
      }
      ptr += r;
      offset += r;
      left -= r;
    }

    *result = std::string(scratch, (r < 0) ? 0 : n - left);
    if (r < 0)
      return false;
    if (!use_os_buffer_) {
      // we need to fadvise away the entire range of pages because
      // we do not want readahead pages to be cached.
      Fadvise(fd_, 0, 0, POSIX_FADV_DONTNEED); // free OS pages
    }
    return true;
  }

  virtual void hint(AccessPattern pattern) {
    switch(pattern) {
      case NORMAL:
        Fadvise(fd_, 0, 0, POSIX_FADV_NORMAL);
        break;
      case RANDOM:
        Fadvise(fd_, 0, 0, POSIX_FADV_RANDOM);
        break;
      case SEQUENTIAL:
        Fadvise(fd_, 0, 0, POSIX_FADV_SEQUENTIAL);
        break;
      case WILLNEED:
        Fadvise(fd_, 0, 0, POSIX_FADV_WILLNEED);
        break;
      case DONTNEED:
        Fadvise(fd_, 0, 0, POSIX_FADV_DONTNEED);
        break;
      default:
        assert(false);
        break;
    }
  }

  virtual bool invalidate_cache(size_t offset, size_t length) {
#ifndef OS_LINUX
    return true;
#else
    // free OS pages
    int ret = Fadvise(fd_, offset, length, POSIX_FADV_DONTNEED);
    if (ret == 0)
      return true;
    return false;
#endif
  }
};

bool get_file_size(const std::string& fname, uint64_t* size)
{
  struct stat sbuf;
  if (stat(fname.c_str(), &sbuf) != 0) {
    *size = 0;
    return false;
  } else {
    *size = sbuf.st_size;
  }
  return true;
}

bool create_dir(const std::string& name)
{
    if (mkdir(name.c_str(), 0755) != 0)
      return false;
    return true;
}

bool new_writable_file(const std::string& fname,
                       std::unique_ptr<writable_file>* result)
{
  result->reset();
  int fd = -1;
  do {
    fd = open(fname.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0644);
  } while (fd < 0 && errno == EINTR);
  if (fd < 0)
    return false;
  else
    result->reset(
        new posix_writable_file(fname, fd, 65536)
    );
  return true;
}

bool new_random_access_file(const std::string& fname,
                            std::unique_ptr<random_access_file>* result)
{
    result->reset();
    bool s = true;
    int fd = open(fname.c_str(), O_RDONLY);
    if (fd < 0)
      return false;
    else
      result->reset(new posix_random_access_file(fname, fd, false));

    return s;
}

}
}
