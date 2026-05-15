#pragma once
#include <common/types.h>

namespace myos {

struct MemoryChunk {
  MemoryChunk *prev;
  MemoryChunk *next;
  bool allocated;

  common::size_t size;
};

class MemoryManager {
protected:
  MemoryChunk *head;

public:
  static MemoryManager *activeMemoryManager;

  MemoryManager(common::size_t first, common::size_t size);
  ~MemoryManager();

  void *malloc(common::size_t size);
  void free(void *ptr);
};

} // namespace myos

void *operator new(unsigned size);
void *operator new[](unsigned size);

void *operator new(unsigned size, void *ptr);
void *operator new[](unsigned size, void *ptr);

void operator delete(void *ptr);
void operator delete[](void *ptr);
