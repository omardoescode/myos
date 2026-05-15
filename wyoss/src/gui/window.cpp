#include "gui/widget.h"
#include <gui/window.h>

namespace myos {
namespace gui {

Window::Window(Widget *parent, common::int32_t x, common::int32_t y,
               common::int32_t w, common::int32_t h, common::uint32_t r,
               common::uint32_t g, common::uint32_t b)
    : CompositeWidget(parent, x, y, w, h, r, g, b), Dragging(false) {}

void Window::OnMouseDown(common::int32_t x, common::int32_t y,
                         common::uint8_t button) {
  CompositeWidget::OnMouseDown(x, y, button);
  Dragging = button == 1;
}
void Window::OnMouseUp(common::int32_t x, common::int32_t y,
                       common::uint8_t button) {
  CompositeWidget::OnMouseUp(x, y, button);
  Dragging = false;
}
void Window::OnMouseMove(common::int32_t oldx, common::int32_t oldy,
                         common::int32_t newx, common::int32_t newy) {
  if (Dragging)
    x += newx - oldx, y += newy - oldy;
  CompositeWidget::OnMouseMove(oldx, oldy, newx, newy);
}
} // namespace gui

} // namespace myos
