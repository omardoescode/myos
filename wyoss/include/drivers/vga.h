#pragma once

#include <common/types.h>
#include <drivers/driver.h>
#include <hardwarecommunication/port.h>

namespace myos {
namespace drivers {

class VideoGraphicsArray {
protected:
  hardwarecommunication::Port8Bit miscPort;
  hardwarecommunication::Port8Bit crtcIndexPort;
  hardwarecommunication::Port8Bit crtcDataPort;
  hardwarecommunication::Port8Bit sequencerIndexPort;
  hardwarecommunication::Port8Bit sequencerDataPort;
  hardwarecommunication::Port8Bit graphicsControllerIndexPort;
  hardwarecommunication::Port8Bit graphicsControllerDataPort;
  hardwarecommunication::Port8Bit attributeControllerPort;
  hardwarecommunication::Port8Bit attributeControllerIndexPort;
  hardwarecommunication::Port8Bit attributeControllerReadPort;
  hardwarecommunication::Port8Bit attributeControllerWritePort;
  hardwarecommunication::Port8Bit attributeControllerResetPort;

  void WriteRegisters(common::uint8_t *registers);
  common::uint8_t *GetGrameBufferSegment();

  virtual common::uint8_t GetColorIndex(common::uint8_t r, common::uint8_t g,
                                        common::uint8_t b);

public:
  VideoGraphicsArray();
  ~VideoGraphicsArray();

  virtual bool SupportsMode(common::uint32_t width, common::uint32_t height,
                            common::uint32_t colordepth);
  virtual bool SetMode(common::uint32_t width, common::uint32_t height,
                       common::uint32_t colordepth);
  // NOTE: We support 8-color-depth only, but we hide this fact from the users.
  virtual void PutPixel(common::uint32_t x, common::int32_t y,
                        common::uint8_t r, common::uint8_t g,
                        common::uint8_t b);
  virtual void PutPixel(common::int32_t x, common::int32_t y,
                        common::uint8_t colorIndex);

  virtual void FillRectangle(common::uint32_t x, common::int32_t y,
                             common::int32_t w, common::int32_t h,
                             common::uint8_t r, common::uint8_t g,
                             common::uint8_t b);
  virtual void FillRectangle(common::uint32_t x, common::int32_t y,
                             common::int32_t w, common::int32_t h,
                             common::uint8_t colorIndex);
};
} // namespace drivers
} // namespace myos
