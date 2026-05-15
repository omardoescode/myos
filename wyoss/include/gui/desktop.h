#pragma once

#include "common/graphicscontext.h"
#include "common/types.h"
#include "drivers/mouse.h"
#include "gui/widget.h"

namespace myos {
namespace gui {
class Desktop : public CompositeWidget, public drivers::MouseEventHandler {
protected:
  common::uint32_t MouseX;
  common::uint32_t MouseY;

public:
  Desktop(common::uint32_t w, common::uint32_t h, common::uint8_t r,
          common::uint8_t g, common::uint8_t b);

  ~Desktop();
  void Draw(common::GraphicsContext *);
  void OnMouseDown(common::uint8_t button);
  void OnMouseUp(common::uint8_t button);
  void OnMouseMove(int x, int y);
};
} // namespace gui

} // namespace myos
