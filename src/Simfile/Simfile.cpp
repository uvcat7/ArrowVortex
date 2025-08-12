#include <Simfile/Simfile.h>

#include <Simfile/Chart.h>
#include <Simfile/Tempo.h>

namespace Vortex {

Simfile::Simfile()
    : format(SIM_NONE),
      previewStart(0.0),
      previewLength(0.0),
      isSelectable(true),
      tempo(new Tempo) {}

Simfile::~Simfile() {
    for (auto chart : charts) {
        delete chart;
    }
    delete tempo;
}

void Simfile::sanitize() {
    for (int i = 0; i < charts.size(); ++i) {
        auto chart = charts[i];
        if (chart->style == nullptr) {
            std::string desc = chart->description();
            HudWarning("%s is missing a style, ignoring chart.", desc.c_str());
            charts.erase(i--);
            delete chart;
        } else {
            chart->sanitize();
        }
    }
    tempo->sanitize();
}

};  // namespace Vortex
