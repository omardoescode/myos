#pragma once

#include "common/graphicscontext.h"
#include "common/types.h"
#include "drivers/keyboard.h"
namespace myos {
namespace gui {
class Widget : public myos::drivers::KeyboardEventHandler {
protected:
  Widget *parent;
  common::int32_t x;
  common::int32_t y;
  common::int32_t w;
  common::int32_t h;
  common::uint32_t r;
  common::uint32_t g;
  common::uint32_t b;
  bool focussable;

public:
  Widget(Widget *parent, common::int32_t x, common::int32_t y,
         common::int32_t w, common::int32_t h, common::uint32_t r,
         common::uint32_t g, common::uint32_t b);
  ~Widget();
  virtual void GetFocus(Widget *widget);
  virtual void ModelToScreen(common::int32_t &x, common::int32_t &y);
  virtual void Draw(common::GraphicsContext *fc);
  virtual void OnMouseDown(common::int32_t x, common::int32_t y,
                           common::uint8_t button);
  virtual void OnMouseUp(common::int32_t x, common::int32_t y,
                         common::uint8_t button);
  virtual void OnMouseMove(common::int32_t oldx, common::int32_t oldy,
                           common::int32_t newx, common::int32_t newy);

  virtual bool ContainsCoordinates(common::uint32_t x, common::uint32_t y);
};

class CompositeWidget : public Widget {
private:
  Widget *children[100];
  int numChildren;

  Widget *focussedChild;

public:
  CompositeWidget(Widget *parent, common::int32_t x, common::int32_t y,
                  common::int32_t w, common::int32_t h, common::uint32_t r,
                  common::uint32_t g, common::uint32_t b);
  ~CompositeWidget();

  virtual void GetFocus(Widget *widget) override;
  virtual bool AddChild(Widget *child);

  virtual void Draw(common::GraphicsContext *fc) override;
  virtual void OnMouseDown(common::int32_t x, common::int32_t y,
                           common::uint8_t button) override;
  virtual void OnMouseUp(common::int32_t x, common::int32_t y,
                         common::uint8_t button) override;
  virtual void OnMouseMove(common::int32_t oldx, common::int32_t oldy,
                           common::int32_t newx, common::int32_t newy) override;

  virtual void OnKeyDown(char) override;
  virtual void OnKeyUp(char) override;
};

} // namespace gui
} // namespace myos
