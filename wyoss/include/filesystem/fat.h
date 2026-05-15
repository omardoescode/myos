// Refactored by Claude Code: extracted FAT32 logic into a Fat32 class
// implementing the FileSystem interface (read/write/list).
#pragma once

#include "common/types.h"
#include "drivers/ata.h"
#include "filesystem/fs_interface.h"

namespace myos {
namespace filesystem {

struct BiosParameterBlock32 {
  common::uint8_t jump[3];
  common::uint8_t softName[8];
  common::uint16_t bytesPerSector;
  common::uint8_t sectorsPerCluster;
  common::uint16_t reservedSectors;
  common::uint8_t fatCopies;
  common::uint16_t rootDirEntries;
  common::uint16_t totalSectors;
  common::uint8_t mediaType;
  common::uint16_t fatSectorCount;
  common::uint16_t sectorsPerTrack;
  common::uint16_t headCount;
  common::uint32_t hiddenSectors;
  common::uint32_t totalSectorCount;

  // extended
  common::uint32_t tableSize;
  common::uint16_t extFlags;
  common::uint16_t fatVersion;
  common::uint32_t rootCluster;
  common::uint16_t fatInfoSector;
  common::uint16_t backupSector;
  common::uint8_t reserved0[12];
  common::uint8_t driveNumber;
  common::uint8_t reserved1;
  common::uint8_t bootSignature;
  common::uint32_t volumeId;
  common::uint8_t volumeLabel[11];
  common::uint8_t fatTypeLabel[8];
} __attribute__((packed));

struct DirectoryEntryFat32 {
  common::uint8_t name[8];
  common::uint8_t ext[3];
  common::uint8_t attributes;
  common::uint8_t reserved;
  common::uint8_t cTimeTenth;
  common::uint16_t cTime;
  common::uint16_t cDate;
  common::uint16_t aTime;
  common::uint16_t firstClusterHi;
  common::uint16_t wTime;
  common::uint16_t wDate;
  common::uint16_t firstClusterLow;
  common::uint32_t size;
} __attribute__((packed));

class Fat32 : public FileSystem {
  drivers::AdvancedTechnologyAttachment *hd;
  common::uint32_t partitionOffset;
  BiosParameterBlock32 bpb;
  common::uint32_t fatStart;
  common::uint32_t dataStart;
  common::uint32_t rootLba;

  common::uint32_t NextCluster(common::uint32_t cluster);
  bool FindEntry(const char *name, DirectoryEntryFat32 *out);
  void WriteFatSector(common::uint32_t sec, common::uint8_t *buf);
  void FreeChain(common::uint32_t cluster);
  common::uint32_t AllocateChain(common::uint32_t count, common::uint32_t *out);

public:
  Fat32(drivers::AdvancedTechnologyAttachment *hd,
        common::uint32_t partitionOffset);

  void PrintInfo();
  void DumpRoot();

  common::int32_t ReadFile(const char *name, common::uint8_t *buffer,
                           common::uint32_t maxSize) override;
  common::int32_t WriteFile(const char *name, const common::uint8_t *buffer,
                            common::uint32_t size) override;
  common::int32_t ReadDirectory(DirEntry *entries,
                                common::uint32_t maxEntries) override;
};

void ReadBiosBlock(drivers::AdvancedTechnologyAttachment *hd,
                   common::uint32_t partitionOffset);

} // namespace filesystem
} // namespace myos
