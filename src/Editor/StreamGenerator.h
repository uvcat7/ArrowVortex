#pragma once

#include <Editor/Common.h>

namespace Vortex {

struct StreamGenerator {
    StreamGenerator();

    vec2i feetCols;
    int maxColRep;
    int maxBoxRep;
    bool allowBoxes;
    bool startWithRight;
    float patternDifficulty;

    void generate(int startRow, int endRow, SnapType spacing);
};

};  // namespace Vortex
