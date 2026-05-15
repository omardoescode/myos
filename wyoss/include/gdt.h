#pragma once
#include <common/types.h>

namespace myos {

class GlobalDescriptorTable {
public:
  class SegmentSelector {
  private:
    common::uint16_t limit_lo;
    common::uint16_t base_lo;
    common::uint8_t base_hi;
    common::uint8_t type;
    common::uint8_t flags_limit_hi;
    common::uint8_t base_vhi;

  public:
    SegmentSelector(common::uint32_t base, common::uint32_t limit, common::uint8_t type);
    common::uint32_t Base();
    common::uint32_t Limit();
  } __attribute__((packed));

  SegmentSelector nullSegmentSelector;
  SegmentSelector unusedSegmentSelector;
  SegmentSelector codeSegmentSelector;
  SegmentSelector dataSegmentSelector;

public:
  GlobalDescriptorTable();
  ~GlobalDescriptorTable();

  common::uint16_t CodeSegmentSelector();
  common::uint16_t DataSegmentSelector();
};

}
