#pragma once

#include <Dialogs/Dialog.h>

#include <Core/QuadBatch.h>
#include <Core/Draw.h>
#include <Core/Vector.h>
#include <Core/WidgetsLayout.h>

namespace Vortex {

class DialogDancingBot : public EditorDialog {
   public:
    enum Foot { LEFT = 0, RIGHT = 1, BOTH = 2 };

    ~DialogDancingBot();
    DialogDancingBot();

    void onUpdateSize() override;
    void onDraw() override;

    void onChanges(int changes) override;

    void onDoFootswitchesChanged();
    void onDoCrossoversChanged();

   private:
    void myCreateWidgets();
    void myPushArrow(QuadBatchTC* batch, int x, int y);
    vec2i myGetDrawPos(vec2i colRow);
    void myAssignFeetToNotes();
    void myGetFeetPositions(vec3f* out, int player);

    Vector<int> myPadLayout;
    Vector<uint32_t> myFeetBits;
    BatchSprite myPadSpr[6];
    BatchSprite myFeetSpr[2];
    Texture myPadTex, myFeetTex;
    bool myDoFootswitches, myDoCrossovers;
};

};  // namespace Vortex
