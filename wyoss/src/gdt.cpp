#include <gdt.h>

using namespace myos;
using namespace myos::common;

GlobalDescriptorTable::GlobalDescriptorTable()
    : nullSegmentSelector(0, 0, 0), unusedSegmentSelector(0, 0, 0),
      codeSegmentSelector(0, 64 * 1024 * 1024, 0x9A),
      dataSegmentSelector(0, 64 * 1024 * 1024, 0x92) {

  uint32_t i[2];
  i[1] = (uint32_t)this;
  i[0] = sizeof(GlobalDescriptorTable) << 16;

  asm volatile("lgdt (%0)" : : "p"(((uint8_t *)i) + 2));
}

GlobalDescriptorTable::~GlobalDescriptorTable() {}

uint16_t GlobalDescriptorTable::DataSegmentSelector() {
  return (uint8_t *)&dataSegmentSelector - (uint8_t *)this;
}

uint16_t GlobalDescriptorTable::CodeSegmentSelector() {
  return (uint8_t *)&codeSegmentSelector - (uint8_t *)this;
}

GlobalDescriptorTable::SegmentSelector::SegmentSelector(uint32_t base,
                                                        uint32_t limit,
                                                        uint8_t flags) {
  uint8_t *target = (uint8_t *)this;

  if (limit <= 65536)
    /*
     0x40 = 0100 0000
     0 (G) = 0 (no granulaity)
     1 (D/B) = 1 (32-bit segments)
     0 -> not 64 bit long mode
     0 -> reserved/available

     0000 (since limit fits in low nibble, no need for hgh nibble)
     */
    target[6] = 0x40;
  else // Switch to 4 KiB-page units
  {
    if ((limit & 0xFFF) != 0xFFF)
      limit = (limit >> 12) - 1;
    else
      limit = (limit >> 12);

    /*
      0xC0 = 1100 0000
      1 (G) -> granuality
      1 -> 32-bit segment
      00

      0000 to be OR'ed later
     */
    target[6] = 0xC0;
  }

  target[0] = limit & 0xFF;
  target[1] = (limit >> 8) & 0xFF;
  target[6] |= (limit >> 16) & 0xF;

  // Set Base (Pointer)
  target[2] = (base >> 0) & 0xFF;
  target[3] = (base >> 8) & 0xFF;
  target[4] = (base >> 16) & 0xFF;
  target[7] = (base >> 24) & 0xFF;

  // Set Flags
  target[5] = flags;
}

uint32_t GlobalDescriptorTable::SegmentSelector::Base() {
  uint8_t *target = (uint8_t *)this;
  uint32_t result = target[7];
  result = (result << 8) + target[4];
  result = (result << 8) + target[3];
  result = (result << 8) + target[2];
  return result;
}

uint32_t GlobalDescriptorTable::SegmentSelector::Limit() {
  uint8_t *target = (uint8_t *)this;
  uint32_t result = target[6] & 0xF;
  result = (result << 8) + target[1];
  result = (result << 8) + target[0];

  if ((target[6] & 0xC0) == 0xC0)
    result = (result << 12) | 0xFFF;

  return result;
}
