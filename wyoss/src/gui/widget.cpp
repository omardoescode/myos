#include "common/types.h"
#include "drivers/keyboard.h"
#include <gui/widget.h>

namespace myos {
namespace gui {
Widget::Widget(Widget *parent, common::int32_t x, common::int32_t y,
               common::int32_t w, common::int32_t h, common::uint32_t r,
               common::uint32_t g, common::uint32_t b)
    : parent(parent), x(x), y(y), w(w), h(h), r(r), g(g), b(b),
      focussable(true), myos::drivers::KeyboardEventHandler() {}

Widget::~Widget() {}

void Widget::GetFocus(Widget *widget) {
  if (parent != 0)
    parent->GetFocus(widget);
}
void Widget::ModelToScreen(common::int32_t &x, common::int32_t &y) {
  if (parent != 0)
    parent->ModelToScreen(x, y);
  x += this->x;
  y += this->y;
}
void Widget::Draw(common::GraphicsContext *gc) {
  int X = 0;
  int Y = 0;
  ModelToScreen(X, Y);
  gc->FillRectangle(X, Y, w, h, r, g, b);
}
void Widget::OnMouseDown(common::int32_t x, common::int32_t y,
                         common::uint8_t button) {
  if (!focussable)
    return;
  GetFocus(this);
}

bool Widget::ContainsCoordinates(common::uint32_t tx, common::uint32_t ty) {
  return x <= tx && tx < x + w && y <= ty && ty < y + h;
}
void Widget::OnMouseUp(common::int32_t x, common::int32_t y,
                       common::uint8_t button) {}
void Widget::OnMouseMove(common::int32_t oldx, common::int32_t oldy,
                         common::int32_t newx, common::int32_t newy) {}

CompositeWidget::CompositeWidget(Widget *parent, common::int32_t x,
                                 common::int32_t y, common::int32_t w,
                                 common::int32_t h, common::uint32_t r,
                                 common::uint32_t g, common::uint32_t b)
    : Widget(parent, x, y, w, h, r, g, b), focussedChild(0), numChildren(0) {}
CompositeWidget::~CompositeWidget() {}
void CompositeWidget::GetFocus(Widget *widget) {
  focussedChild = widget;
  if (parent != 0)
    parent->GetFocus(this);
}
void CompositeWidget::Draw(common::GraphicsContext *gc) {
  Widget::Draw(gc);
  for (int i = numChildren - 1; i >= 0; --i)
    children[i]->Draw(gc);
}
void CompositeWidget::OnMouseDown(common::int32_t x, common::int32_t y,
                                  common::uint8_t button) {
  for (int i = 0; i < numChildren; i++)
    if (children[i]->ContainsCoordinates(x - this->x, y - this->y))
      children[i]->OnMouseDown(x - this->x, y - this->y, button);
}
void CompositeWidget::OnMouseUp(common::int32_t x, common::int32_t y,
                                common::uint8_t button) {
  for (int i = 0; i < numChildren; i++)
    if (children[i]->ContainsCoordinates(x - this->x, y - this->y))
      children[i]->OnMouseUp(x - this->x, y - this->y, button);
}

void CompositeWidget::OnMouseMove(common::int32_t oldx, common::int32_t oldy,
                                  common::int32_t newx, common::int32_t newy) {
  int firstchild = -1;
  for (int i = 0; i < numChildren; i++)
    if (children[i]->ContainsCoordinates(oldx - this->x, oldy - this->y)) {
      children[i]->OnMouseMove(oldx - this->x, oldy - this->y, newx - this->x,
                               newy - this->y);
      firstchild = i;
      break;
    }

  for (int i = 0; i < numChildren; i++)
    if (children[i]->ContainsCoordinates(newx - this->x, newy - this->y)) {
      // To avoid getting the same event twice if it moved to the same
      // application
      if (firstchild != i)
        children[i]->OnMouseMove(oldx - this->x, oldx - this->y, newx - this->x,
                                 newy - this->y);
      break;
    }
}
void CompositeWidget::OnKeyDown(char str) {
  if (focussedChild != 0)
    focussedChild->OnKeyDown(str);
}
void CompositeWidget::OnKeyUp(char str) {
  if (focussedChild != 0)
    focussedChild->OnKeyUp(str);
}

bool CompositeWidget::AddChild(Widget *child) {
  if (numChildren >= 100)
    return false;
  children[numChildren++] = child;
  return true;
}

} // namespace gui
} // namespace myos
