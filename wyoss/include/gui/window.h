#pragma once

#include "gui/widget.h"
namespace myos {
namespace gui {
class Window : public CompositeWidget {
protected:
  bool Dragging;

public:
  Window(Widget *parent, common::int32_t x, common::int32_t y,
         common::int32_t w, common::int32_t h, common::uint32_t r,
         common::uint32_t g, common::uint32_t b);
  ~Window();

  void OnMouseDown(common::int32_t x, common::int32_t y,
                   common::uint8_t button) override;
  void OnMouseUp(common::int32_t x, common::int32_t y,
                 common::uint8_t button) override;
  void OnMouseMove(common::int32_t oldx, common::int32_t oldy,
                   common::int32_t newx, common::int32_t newy) override;
};
} // namespace gui
} // namespace myos
