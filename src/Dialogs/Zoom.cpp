#include <Dialogs/Zoom.h>

#include <Core/Widgets.h>
#include <Core/WidgetsLayout.h>

#include <Editor/View.h>

namespace Vortex {

enum Actions {
    ACT_ZOOM,
    ACT_SCALE,
};

DialogZoom::~DialogZoom() = default;

DialogZoom::DialogZoom() {
    setTitle("ZOOM");
    myCreateWidgets();
}

void DialogZoom::myCreateWidgets() {
    myLayout.row().col(64).col(70).col(70);

    WgSlider* slider = myLayout.add<WgSlider>("Zoom");
    slider->value.bind(&myZoomLevel);
    slider->onChange.bind(this, &DialogZoom::onAction,
                          static_cast<int>(ACT_ZOOM));
    slider->setRange(1.0, 19.0);
    slider->setTooltip("Zoom Level");

    WgSpinner* spinner = myLayout.add<WgSpinner>();
    spinner->value.bind(&myZoomLevel);
    spinner->setPrecision(2, 2);
    spinner->setRange(1.0, 19.0);
    spinner->setStep(1.0);
    spinner->onChange.bind(this, &DialogZoom::onAction,
                           static_cast<int>(ACT_ZOOM));

    slider = myLayout.add<WgSlider>("Scale");
    slider->value.bind(&myScaleLevel);
    slider->onChange.bind(this, &DialogZoom::onAction,
                          static_cast<int>(ACT_SCALE));
    slider->setRange(1.0, 10.0);
    slider->setTooltip("Note Scale");

    spinner = myLayout.add<WgSpinner>();
    spinner->value.bind(&myScaleLevel);
    spinner->setPrecision(2, 2);
    spinner->setRange(1.0, 10.0);
    spinner->setStep(0.25);
    spinner->onChange.bind(this, &DialogZoom::onAction,
                           static_cast<int>(ACT_SCALE));
}

void DialogZoom::onTick() {
    myZoomLevel = gView->getZoomLevel() + 3;
    myScaleLevel = gView->getScaleLevel();
    EditorDialog::onTick();
}

void DialogZoom::onAction(int id) {
    switch (id) {
        case ACT_ZOOM: {
            gView->setZoomLevel(myZoomLevel - 3);
        } break;
        case ACT_SCALE: {
            gView->setScaleLevel(myScaleLevel);
        } break;
    }
}
};  // namespace Vortex
