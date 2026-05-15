#include "common/types.h"
#include <drivers/vga.h>

using namespace myos;
using namespace drivers;

namespace myos {
namespace drivers {

VideoGraphicsArray::VideoGraphicsArray()
    : miscPort(0x3c2), crtcIndexPort(0x3d4), crtcDataPort(0x3d5),
      sequencerIndexPort(0x3c4), sequencerDataPort(0x3c5),
      graphicsControllerIndexPort(0x3ce), graphicsControllerDataPort(0x3cf),
      attributeControllerPort(0x3c0), attributeControllerIndexPort(0x3c0),
      attributeControllerReadPort(0x3c1), attributeControllerWritePort(0x3c0),
      attributeControllerResetPort(0x3da) {}

VideoGraphicsArray::~VideoGraphicsArray() {}
void VideoGraphicsArray::WriteRegisters(common::uint8_t *registers) {
  miscPort.Write(*(registers++));

  for (common::uint8_t i = 0; i < 5; i++) {
    sequencerIndexPort.Write(i);
    sequencerDataPort.Write(*(registers++));
  }

  // cathorde ray tube controller

  // something about not having the device blow 🥲
  // and I don't understand this lol
  crtcIndexPort.Write(0x03);
  crtcDataPort.Write(crtcDataPort.Read() | 0x80);
  crtcIndexPort.Write(0x11);
  crtcDataPort.Write(crtcDataPort.Read() & ~0x80);

  // to avoid accidental wr
  registers[0x03] = registers[0x03] | 0x80;
  registers[0x11] = registers[0x11] & ~0x80;

  // proceed normally
  for (common::uint8_t i = 0; i < 25; i++) {
    crtcIndexPort.Write(i);
    crtcDataPort.Write(*(registers++));
  }

  for (common::uint8_t i = 0; i < 9; i++) {
    graphicsControllerIndexPort.Write(i);
    graphicsControllerDataPort.Write(*(registers++));
  }

  for (common::uint8_t i = 0; i < 21; i++) {
    attributeControllerResetPort.Read();
    attributeControllerIndexPort.Write(i);
    attributeControllerWritePort.Write(*(registers++));
  }

  attributeControllerResetPort.Read();
  attributeControllerIndexPort.Write(0x20);
}

bool VideoGraphicsArray::SupportsMode(common::uint32_t width,
                                      common::uint32_t height,
                                      common::uint32_t colordepth) {
  return width == 320 && height == 200 && colordepth == 8;
}

bool VideoGraphicsArray::SetMode(common::uint32_t width,
                                 common::uint32_t height,
                                 common::uint32_t colordepth) {
  if (!SupportsMode(width, height, colordepth))
    return false;

  unsigned char g_320x200x256[] = {
      /* MISC */
      0x63,
      /* SEQ */
      0x03, 0x01, 0x0F, 0x00, 0x0E,
      /* CRTC */
      0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F, 0x00, 0x41, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
      0xFF,
      /* GC */
      0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F, 0xFF,
      /* AC */
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
      0x0C, 0x0D, 0x0E, 0x0F, 0x41, 0x00, 0x0F, 0x00, 0x00};

  WriteRegisters(g_320x200x256);
  return true;
}
void VideoGraphicsArray::PutPixel(common::uint32_t x, common::int32_t y,
                                  common::uint8_t r, common::uint8_t g,
                                  common::uint8_t b) {
  PutPixel(x, y, GetColorIndex(r, g, b));
}

common::uint8_t *VideoGraphicsArray::GetGrameBufferSegment() {
  graphicsControllerIndexPort.Write(0x06);
  common::uint8_t segmentNumber =
      (graphicsControllerDataPort.Read() >> 2) & 0x03;

  switch (segmentNumber) {
  default:
  case 0:
    return (common::uint8_t *)0x00000;
  case 1:
    return (common::uint8_t *)0xA0000;
  case 2:
    return (common::uint8_t *)0xB0000;
  case 3:
    return (common::uint8_t *)0xB8000;
  }
}

void VideoGraphicsArray::PutPixel(common::int32_t x, common::int32_t y,
                                  common::uint8_t colorIndex) {
  if (x < 0 || 320 <= x || y < 0 || 200 <= y)
    return;
  common::uint8_t *pixelAddress = GetGrameBufferSegment() + 320 * y + x;
  *pixelAddress = colorIndex;
}

common::uint8_t VideoGraphicsArray::GetColorIndex(common::uint8_t r,
                                                  common::uint8_t g,
                                                  common::uint8_t b) {
  if (r == 0x00 && g == 0x00 && b == 0x00)
    return 0x00; // black
  if (r == 0x00 && g == 0x00 && b == 0xA8)
    return 0x01; // blue
  if (r == 0x00 && g == 0xA8 && b == 0x00)
    return 0x02; // green
  if (r == 0xA8 && g == 0x00 && b == 0x00)
    return 0x04; // red
  if (r == 0xFF && g == 0xFF && b == 0xFF)
    return 0x3F; // white
  return 0x00;
}

void VideoGraphicsArray::FillRectangle(common::uint32_t x, common::int32_t y,
                                       common::int32_t w, common::int32_t h,
                                       common::uint8_t r, common::uint8_t g,
                                       common::uint8_t b) {
  FillRectangle(x, y, w, h, GetColorIndex(r, g, b));
}
void VideoGraphicsArray::FillRectangle(common::uint32_t x, common::int32_t y,
                                       common::int32_t w, common::int32_t h,
                                       common::uint8_t colorIndex) {

  for (common::int32_t Y = y; Y < y + h; Y++)
    for (common::int32_t X = x; X < x + w; X++)
      PutPixel(X, Y, colorIndex);
}
} // namespace drivers
} // namespace myos
