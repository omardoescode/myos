#pragma once

#include "common/types.h"

namespace myos {
namespace filesystem {

struct DirEntry {
  char name[13];
  bool isDirectory;
  common::uint32_t size;
  common::uint32_t firstCluster;
};

class FileSystem {
public:
  virtual ~FileSystem() {}
  virtual common::int32_t ReadFile(const char *name, common::uint8_t *buffer,
                                   common::uint32_t maxSize) = 0;
  virtual common::int32_t WriteFile(const char *name,
                                    const common::uint8_t *buffer,
                                    common::uint32_t size) = 0;
  virtual common::int32_t ReadDirectory(DirEntry *entries,
                                        common::uint32_t maxEntries) = 0;
};

} // namespace filesystem
} // namespace myos
