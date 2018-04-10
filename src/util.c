#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#ifndef __WIN32__
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#else
#include <windows.h>
#endif

#include "defs.h"
#include "util.h"

void *map_file(char *name, int shared, long64 *size)
{
#ifndef __WIN32__
  struct stat statbuf;
  int fd = open(name, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "Could not open %s for reading.\n", name);
    exit(1);
  }
  fstat(fd, &statbuf);
  *size = statbuf.st_size;
#ifdef __linux__
  void *data = mmap(NULL, statbuf.st_size, PROT_READ,
		    shared ? MAP_SHARED : MAP_PRIVATE | MAP_POPULATE, fd, 0);
#else
  void *data = mmap(NULL, statbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
#endif
  if (data == MAP_FAILED) {
    printf("Could not mmap() %s.\n", name);
    exit(1);
  }
  close(fd);
  return data;
#else
  HANDLE h = CreateFile(name, GENERIC_READ, FILE_SHARE_READ, NULL,
			  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (h == INVALID_HANDLE_VALUE) {
    fprintf(stderr, "Could not open %s for reading.\n", name);
    exit(1);
  }
  DWORD size_low, size_high;
  size_low = GetFileSize(h, &size_high);
  *size = ((long64)size_high << 32) | (long64)size_low;
  HANDLE map = CreateFileMapping(h, NULL, PAGE_READONLY, size_high, size_low,
				  NULL);
  if (map == NULL) {
    fprintf(stderr, "CreateFileMapping() failed.\n");
    exit(1);
  }
  void *data = MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
  if (data == NULL) {
    fprintf(stderr, "MapViewOfFile() failed.\n");
    exit(1);
  }
  CloseHandle(h);
  return data;
#endif
}

void unmap_file(void *data, long64 size)
{
#ifndef __WIN32__
  munmap(data, size);
#else
  UnmapViewOfFile(data);
#endif
}

void *alloc_aligned(long64 size, uintptr_t alignment)
{
#ifndef __WIN32__
  void *ptr;

  posix_memalign(&ptr, alignment, size);
  if (ptr == NULL) {
    fprintf(stderr, "Could not allocate sufficient memory.\n");
    exit(1);
  }

  return ptr;
#else
  void *ptr;

  ptr = malloc(size + alignment - 1);
  if (ptr == NULL) {
    fprintf(stderr, "Could not allocate sufficient memory.\n");
    exit(1);
  }
  ptr = (void *)((uintptr_t)(ptr + alignment - 1) & ~(alignment - 1));

  return ptr;
#endif
}

void *alloc_huge(long64 size)
{
#ifndef __WIN32__
  void *ptr;

  posix_memalign(&ptr, 2 * 1024 * 1024, size);
  if (ptr == NULL) {
    fprintf(stderr, "Could not allocate sufficient memory.\n");
    exit(1);
  }
#ifdef MADV_HUGEPAGE
  madvise(ptr, size, MADV_HUGEPAGE);
#endif

  return ptr;
#else
  void *ptr;

  ptr = malloc(size);
  if (ptr == NULL) {
    fprintf(stderr, "Could not allocate sufficient memory.\n");
    exit(1);
  }

  return ptr;
#endif
}

