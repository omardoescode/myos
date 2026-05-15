#include "filesystem/msdospart.h"
#include "filesystem/fat.h"

void printf(const char *);
void printHex(myos::common::uint8_t);

namespace myos {
namespace filesystem {

void MSDOSPartitionTable::ReadPartition(
    drivers::AdvancedTechnologyAttachment *hd) {

  MasterBootRecord mbr;
  hd->Read28(0, (common::uint8_t *)&mbr, sizeof(MasterBootRecord));

  for (int p = 0; p < 4; p++) {
    printf("\nP");
    printHex((common::uint8_t)(p + 1));
    printf(": id=");
    printHex(mbr.primaryPartition[p].partition_id);
    printf(" lba=");
    common::uint32_t lba = mbr.primaryPartition[p].start_lba;
    printHex((lba >> 24) & 0xFF);
    printHex((lba >> 16) & 0xFF);
    printHex((lba >> 8) & 0xFF);
    printHex(lba & 0xFF);
    printf(" len=");
    common::uint32_t len = mbr.primaryPartition[p].length;
    printHex((len >> 24) & 0xFF);
    printHex((len >> 16) & 0xFF);
    printHex((len >> 8) & 0xFF);
    printHex(len & 0xFF);
  }
  printf("\n");

  printf("\nMBR magic: ");
  printHex((mbr.magicnumber >> 8) & 0xFF);
  printHex(mbr.magicnumber & 0xFF);
  if (mbr.magicnumber != 0xAA55) {
    printf(" BAD");
    return;
  } else
    printf(" OK");

  for (int i = 0; i < 4; i++) {
    if (mbr.primaryPartition[i].partition_id == 0x00)
      continue;
    printf("\nPartition ");
    printHex(i & 0xFF);

    if (mbr.primaryPartition[i].bootable == 0x80)
      printf(" bootable. Type ");
    else
      printf(" not bootable. Type ");

    printHex(mbr.primaryPartition[i].partition_id & 0xFF);

    ReadBiosBlock(hd, mbr.primaryPartition[i].start_lba);
  }
}
} // namespace filesystem
} // namespace myos
