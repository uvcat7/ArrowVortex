#pragma once

#include <Core/GuiContext.h>
#include <Core/Vector.h>

namespace Vortex {

class RowLayout : public GuiWidget {
   public:
    virtual ~RowLayout();

    RowLayout(GuiContext* gui, int spacing);

    void onUpdateSize();
    void onArrange(recti r);
    void onTick();
    void onDraw();

    void add(GuiWidget* w);
    void addBlank();

    RowLayout& row(bool expand = false);
    RowLayout& col(bool expand = false);
    RowLayout& col(int w, bool expand = false);

    GuiWidget** begin();
    GuiWidget** end();

    template <typename T>
    T* add() {
        T* w = new T(gui_);
        add(w);
        return w;
    }

    template <typename T>
    T* add(const std::string& label) {
        add<WgLabel>()->text.set(label);
        return add<T>();
    }

    template <typename T>
    T* addH(int height) {
        T* w = new T(gui_);
        w->setHeight(height);
        add(w);
        return w;
    }

   protected:
    struct Row;
    struct Col;

    Vector<Row> row_list_;
    Vector<GuiWidget*> widget_list_;
    int row_spacing_;
};

};  // namespace Vortex
