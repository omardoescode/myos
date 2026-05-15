// Refactored by Claude Code: extracted FAT32 logic into a Fat32 class
// implementing the FileSystem interface (read/write/list).
#include "filesystem/fat.h"
#include "common/types.h"

void printf(const char *);
void printHex(myos::common::uint8_t);

namespace myos {
namespace filesystem {

static void printHex16(common::uint16_t v) {
  printHex((v >> 8) & 0xFF);
  printHex(v & 0xFF);
}
static void printHex32(common::uint32_t v) {
  printHex((v >> 24) & 0xFF);
  printHex((v >> 16) & 0xFF);
  printHex((v >> 8) & 0xFF);
  printHex(v & 0xFF);
}
static void printAscii(const common::uint8_t *s, int n) {
  char foo[2] = {0, 0};
  for (int i = 0; i < n; i++) {
    foo[0] = s[i] ? (char)s[i] : ' ';
    printf(foo);
  }
}

static char upper(char c) {
  return (c >= 'a' && c <= 'z') ? (char)(c - 32) : c;
}

static bool NameMatches(const DirectoryEntryFat32 &e, const char *name) {
  int j = 0;
  for (int i = 0; i < 8; i++) {
    if (e.name[i] == ' ')
      break;
    if (upper(name[j++]) != (char)e.name[i])
      return false;
  }
  if (e.ext[0] != ' ') {
    if (name[j++] != '.')
      return false;
    for (int i = 0; i < 3; i++) {
      if (e.ext[i] == ' ')
        break;
      if (upper(name[j++]) != (char)e.ext[i])
        return false;
    }
  }
  return name[j] == 0;
}

static void FormatName(const DirectoryEntryFat32 &e, char out[13]) {
  int n = 0;
  for (int j = 0; j < 8 && e.name[j] != ' '; j++)
    out[n++] = (char)e.name[j];
  if (e.ext[0] != ' ') {
    out[n++] = '.';
    for (int j = 0; j < 3 && e.ext[j] != ' '; j++)
      out[n++] = (char)e.ext[j];
  }
  out[n] = 0;
}

Fat32::Fat32(drivers::AdvancedTechnologyAttachment *hd_,
             common::uint32_t partitionOffset_)
    : hd(hd_), partitionOffset(partitionOffset_) {
  hd->Read28(partitionOffset, (common::uint8_t *)&bpb,
             sizeof(BiosParameterBlock32));
  fatStart = partitionOffset + bpb.reservedSectors;
  dataStart = fatStart + bpb.fatCopies * bpb.tableSize;
  rootLba = dataStart + (bpb.rootCluster - 2) * bpb.sectorsPerCluster;
}

common::uint32_t Fat32::NextCluster(common::uint32_t cluster) {
  common::uint8_t fatbuffer[512];
  common::uint32_t sector = cluster / (512 / sizeof(common::uint32_t));
  hd->Read28(fatStart + sector, fatbuffer, 512);
  common::uint32_t offset = cluster % (512 / sizeof(common::uint32_t));
  return ((common::uint32_t *)fatbuffer)[offset] & 0x0FFFFFFF;
}

bool Fat32::FindEntry(const char *name, DirectoryEntryFat32 *out) {
  DirectoryEntryFat32 dirent[16];
  for (common::uint32_t s = 0; s < bpb.sectorsPerCluster; s++) {
    hd->Read28(rootLba + s, (common::uint8_t *)&dirent[0], 512);
    for (int i = 0; i < 16; i++) {
      if (dirent[i].name[0] == 0x00)
        return false;
      if (dirent[i].name[0] == (common::uint8_t)0xE5)
        continue;
      if ((dirent[i].attributes & 0x0F) == 0x0F)
        continue;
      if (dirent[i].attributes & 0x08)
        continue;
      if (NameMatches(dirent[i], name)) {
        *out = dirent[i];
        return true;
      }
    }
  }
  return false;
}

void Fat32::PrintInfo() {
  printf("\nFAT32 BPB @ lba=0x");
  printHex32(partitionOffset);
  printf("\n  oem='");
  printAscii(bpb.softName, 8);
  printf("'\n  bytes/sector=0x");
  printHex16(bpb.bytesPerSector);
  printf(" sec/cluster=0x");
  printHex(bpb.sectorsPerCluster);
  printf(" reserved=0x");
  printHex16(bpb.reservedSectors);
  printf(" fats=0x");
  printHex(bpb.fatCopies);
  printf("\n  fatStart=0x");
  printHex32(fatStart);
  printf(" dataStart=0x");
  printHex32(dataStart);
  printf(" rootLba=0x");
  printHex32(rootLba);
  printf("\n");
}

common::int32_t Fat32::ReadDirectory(DirEntry *entries,
                                     common::uint32_t maxEntries) {
  DirectoryEntryFat32 dirent[16];
  common::uint32_t found = 0;
  for (common::uint32_t s = 0; s < bpb.sectorsPerCluster; s++) {
    hd->Read28(rootLba + s, (common::uint8_t *)&dirent[0], 512);
    for (int i = 0; i < 16; i++) {
      if (dirent[i].name[0] == 0x00)
        return (common::int32_t)found;
      if (dirent[i].name[0] == (common::uint8_t)0xE5)
        continue;
      if ((dirent[i].attributes & 0x0F) == 0x0F)
        continue;
      if (dirent[i].attributes & 0x08)
        continue;
      if (found >= maxEntries)
        return (common::int32_t)found;
      FormatName(dirent[i], entries[found].name);
      entries[found].isDirectory = (dirent[i].attributes & 0x10) != 0;
      entries[found].size = dirent[i].size;
      entries[found].firstCluster =
          ((common::uint32_t)dirent[i].firstClusterHi << 16) |
          dirent[i].firstClusterLow;
      found++;
    }
  }
  return (common::int32_t)found;
}

common::int32_t Fat32::ReadFile(const char *name, common::uint8_t *buffer,
                                common::uint32_t maxSize) {
  DirectoryEntryFat32 e;
  if (!FindEntry(name, &e))
    return -1;
  common::uint32_t cluster =
      ((common::uint32_t)e.firstClusterHi << 16) | e.firstClusterLow;
  common::uint32_t remaining = e.size < maxSize ? e.size : maxSize;
  common::uint32_t written = 0;
  common::uint8_t sectorbuf[512];
  while (remaining > 0 && cluster < 0x0FFFFFF8) {
    common::uint32_t firstSector =
        dataStart + bpb.sectorsPerCluster * (cluster - 2);
    for (common::uint32_t so = 0;
         remaining > 0 && so < bpb.sectorsPerCluster; so++) {
      hd->Read28(firstSector + so, sectorbuf, 512);
      common::uint32_t chunk = remaining > 512 ? 512 : remaining;
      for (common::uint32_t k = 0; k < chunk; k++)
        buffer[written + k] = sectorbuf[k];
      written += chunk;
      remaining -= chunk;
    }
    cluster = NextCluster(cluster);
  }
  return (common::int32_t)written;
}

void Fat32::WriteFatSector(common::uint32_t sec, common::uint8_t *buf) {
  for (common::uint32_t f = 0; f < bpb.fatCopies; f++)
    hd->Write28(fatStart + f * bpb.tableSize + sec, buf, 512);
}

void Fat32::FreeChain(common::uint32_t c) {
  common::uint8_t fatbuf[512];
  common::uint32_t curSec = 0xFFFFFFFF;
  while (c >= 2 && c < 0x0FFFFFF8) {
    common::uint32_t sec = c / 128;
    common::uint32_t off = c % 128;
    if (sec != curSec) {
      if (curSec != 0xFFFFFFFF)
        WriteFatSector(curSec, fatbuf);
      hd->Read28(fatStart + sec, fatbuf, 512);
      curSec = sec;
    }
    common::uint32_t next = ((common::uint32_t *)fatbuf)[off] & 0x0FFFFFFF;
    ((common::uint32_t *)fatbuf)[off] = 0;
    c = next;
  }
  if (curSec != 0xFFFFFFFF)
    WriteFatSector(curSec, fatbuf);
}

common::uint32_t Fat32::AllocateChain(common::uint32_t count,
                                      common::uint32_t *out) {
  common::uint8_t fatbuf[512];
  common::uint32_t found = 0;
  for (common::uint32_t s = 0; s < bpb.tableSize && found < count; s++) {
    hd->Read28(fatStart + s, fatbuf, 512);
    for (int o = 0; o < 128 && found < count; o++) {
      common::uint32_t c = s * 128 + o;
      if (c < 2)
        continue;
      if ((((common::uint32_t *)fatbuf)[o] & 0x0FFFFFFF) == 0)
        out[found++] = c;
    }
  }
  if (found < count)
    return 0;
  common::uint32_t curSec = 0xFFFFFFFF;
  for (common::uint32_t i = 0; i < count; i++) {
    common::uint32_t c = out[i];
    common::uint32_t sec = c / 128;
    common::uint32_t off = c % 128;
    if (sec != curSec) {
      if (curSec != 0xFFFFFFFF)
        WriteFatSector(curSec, fatbuf);
      hd->Read28(fatStart + sec, fatbuf, 512);
      curSec = sec;
    }
    common::uint32_t val = (i + 1 < count) ? out[i + 1] : 0x0FFFFFFF;
    ((common::uint32_t *)fatbuf)[off] = val;
  }
  if (curSec != 0xFFFFFFFF)
    WriteFatSector(curSec, fatbuf);
  return out[0];
}

static void MakeShortName(const char *name, common::uint8_t out8[8],
                          common::uint8_t out3[3]) {
  for (int i = 0; i < 8; i++)
    out8[i] = ' ';
  for (int i = 0; i < 3; i++)
    out3[i] = ' ';
  int j = 0;
  for (int i = 0; i < 8 && name[j] && name[j] != '.'; i++, j++)
    out8[i] = (common::uint8_t)upper(name[j]);
  while (name[j] && name[j] != '.')
    j++;
  if (name[j] == '.') {
    j++;
    for (int i = 0; i < 3 && name[j]; i++, j++)
      out3[i] = (common::uint8_t)upper(name[j]);
  }
}

common::int32_t Fat32::WriteFile(const char *name,
                                 const common::uint8_t *buffer,
                                 common::uint32_t size) {
  common::uint8_t shortName[8], shortExt[3];
  MakeShortName(name, shortName, shortExt);

  DirectoryEntryFat32 dirent[16];
  common::uint32_t targetSector = 0;
  int targetIdx = -1;
  common::uint32_t freeSector = 0;
  int freeIdx = -1;
  bool stop = false;

  for (common::uint32_t s = 0; s < bpb.sectorsPerCluster && !stop; s++) {
    hd->Read28(rootLba + s, (common::uint8_t *)&dirent[0], 512);
    for (int i = 0; i < 16; i++) {
      common::uint8_t n0 = dirent[i].name[0];
      if (n0 == 0x00) {
        if (freeIdx < 0) {
          freeSector = s;
          freeIdx = i;
        }
        stop = true;
        break;
      }
      if (n0 == 0xE5) {
        if (freeIdx < 0) {
          freeSector = s;
          freeIdx = i;
        }
        continue;
      }
      if ((dirent[i].attributes & 0x0F) == 0x0F)
        continue;
      bool match = true;
      for (int j = 0; j < 8 && match; j++)
        if (dirent[i].name[j] != shortName[j])
          match = false;
      for (int j = 0; j < 3 && match; j++)
        if (dirent[i].ext[j] != shortExt[j])
          match = false;
      if (match) {
        targetSector = s;
        targetIdx = i;
        stop = true;
        break;
      }
    }
  }

  if (targetIdx >= 0) {
    hd->Read28(rootLba + targetSector, (common::uint8_t *)&dirent[0], 512);
    common::uint32_t oldC =
        ((common::uint32_t)dirent[targetIdx].firstClusterHi << 16) |
        dirent[targetIdx].firstClusterLow;
    if (oldC >= 2 && oldC < 0x0FFFFFF8)
      FreeChain(oldC);
  } else {
    if (freeIdx < 0)
      return -1;
    targetSector = freeSector;
    targetIdx = freeIdx;
  }

  common::uint32_t clusterBytes =
      (common::uint32_t)bpb.sectorsPerCluster * 512;
  common::uint32_t clusterCount = (size + clusterBytes - 1) / clusterBytes;
  common::uint32_t chainBuf[64];
  if (clusterCount > 64)
    return -1;
  common::uint32_t firstCluster = 0;
  if (clusterCount > 0) {
    firstCluster = AllocateChain(clusterCount, chainBuf);
    if (firstCluster == 0)
      return -1;
  }

  common::uint8_t secbuf[512];
  common::uint32_t pos = 0;
  for (common::uint32_t ci = 0; ci < clusterCount; ci++) {
    common::uint32_t firstSec =
        dataStart + bpb.sectorsPerCluster * (chainBuf[ci] - 2);
    for (common::uint32_t so = 0; so < bpb.sectorsPerCluster && pos < size;
         so++) {
      common::uint32_t chunk = size - pos > 512 ? 512 : size - pos;
      for (common::uint32_t k = 0; k < chunk; k++)
        secbuf[k] = buffer[pos + k];
      for (common::uint32_t k = chunk; k < 512; k++)
        secbuf[k] = 0;
      hd->Write28(firstSec + so, secbuf, 512);
      pos += chunk;
    }
  }

  hd->Read28(rootLba + targetSector, (common::uint8_t *)&dirent[0], 512);
  DirectoryEntryFat32 &e = dirent[targetIdx];
  for (int j = 0; j < 8; j++)
    e.name[j] = shortName[j];
  for (int j = 0; j < 3; j++)
    e.ext[j] = shortExt[j];
  e.attributes = 0x20;
  e.reserved = 0;
  e.cTimeTenth = 0;
  e.cTime = 0;
  e.cDate = 0;
  e.aTime = 0;
  e.wTime = 0;
  e.wDate = 0;
  e.firstClusterHi = (firstCluster >> 16) & 0xFFFF;
  e.firstClusterLow = firstCluster & 0xFFFF;
  e.size = size;
  hd->Write28(rootLba + targetSector, (common::uint8_t *)&dirent[0], 512);
  hd->Flush();

  return (common::int32_t)size;
}

void Fat32::DumpRoot() {
  DirEntry entries[16];
  common::int32_t n = ReadDirectory(entries, 16);
  for (common::int32_t k = 0; k < n; k++) {
    printf(entries[k].name);
    printf("\n");
    if (entries[k].isDirectory)
      continue;

    common::uint32_t cluster = entries[k].firstCluster;
    common::uint32_t remaining = entries[k].size;
    common::uint8_t buf[513];
    while (remaining > 0 && cluster < 0x0FFFFFF8) {
      common::uint32_t firstSector =
          dataStart + bpb.sectorsPerCluster * (cluster - 2);
      for (common::uint32_t so = 0;
           remaining > 0 && so < bpb.sectorsPerCluster; so++) {
        hd->Read28(firstSector + so, buf, 512);
        common::uint32_t chunk = remaining > 512 ? 512 : remaining;
        buf[chunk] = '\0';
        printf((const char *)buf);
        remaining -= chunk;
      }
      cluster = NextCluster(cluster);
    }
    printf("\n---\n");
  }
}

void ReadBiosBlock(drivers::AdvancedTechnologyAttachment *hd,
                   common::uint32_t partitionOffset) {
  Fat32 fs(hd, partitionOffset);
  fs.PrintInfo();
  fs.DumpRoot();
}

} // namespace filesystem
} // namespace myos
