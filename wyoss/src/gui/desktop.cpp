#include "common/types.h"
#include "gui/widget.h"
#include <gui/desktop.h>

namespace myos {
namespace gui {

Desktop::Desktop(common::uint32_t w, common::uint32_t h, common::uint8_t r,
                 common::uint8_t g, common::uint8_t b)
    : CompositeWidget(0, 0, 0, w, h, r, g, b), MouseX(w / 2), MouseY(h / 2) {}

Desktop::~Desktop() {}

void Desktop::Draw(common::GraphicsContext *gc) {
  CompositeWidget::Draw(gc);

  for (int i = 0; i < 4; i++) {
    gc->PutPixel(MouseX - i, MouseY, 0xFF, 0xFF, 0xFF);
    gc->PutPixel(MouseX + i, MouseY, 0xFF, 0xFF, 0xFF);
    gc->PutPixel(MouseX, MouseY - i, 0xFF, 0xFF, 0xFF);
    gc->PutPixel(MouseX, MouseY + i, 0xFF, 0xFF, 0xFF);
  }
}
void Desktop::OnMouseDown(common::uint8_t button) {
  CompositeWidget::OnMouseDown(MouseX, MouseY, button);
}
void Desktop::OnMouseUp(common::uint8_t button) {
  CompositeWidget::OnMouseUp(MouseX, MouseY, button);
}
void Desktop::OnMouseMove(int x, int y) {
  common::int32_t newMouseX = MouseX + x;
  common::int32_t newMouseY = MouseY + y;

  if (newMouseX < 0)
    newMouseX = 0;
  else if (newMouseX >= w)
    newMouseX = w - 1;

  if (newMouseY < 0)
    newMouseY = 0;
  if (newMouseY >= h)
    newMouseY = h - 1;

  CompositeWidget::OnMouseMove(MouseX, MouseY, newMouseX, newMouseY);

  MouseX = newMouseX;
  MouseY = newMouseY;
}
} // namespace gui
} // namespace myos
