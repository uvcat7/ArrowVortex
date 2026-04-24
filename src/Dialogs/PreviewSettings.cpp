#include <Dialogs/PreviewSettings.h>

#include <Editor/NotefieldPreview.h>

namespace Vortex {
enum Actions {
    ACT_TOGGLE,
    ACT_MODE,
    ACT_BEAT,
    ACT_REVERSE,
    ACT_TRANS_NOTES,
    ACT_TRANS_REGIONS,
    ACT_CMOD_TOGGLE,
    ACT_CMOD_VALUE,
    ACT_XMOD_TOGGLE,
    ACT_XMOD_VALUE,
    ACT_CROP_TOGGLE
};

DialogPreviewSettings::~DialogPreviewSettings() = default;

DialogPreviewSettings::DialogPreviewSettings() {
    setTitle("PREVIEW SETTINGS");
    myCreateWidgets();
}

void DialogPreviewSettings::myCreateWidgets() {
    myLayout.row().col(190);
    WgCheckbox* check = myLayout.add<WgCheckbox>();
    check->text.set("Enabled");
    check->value.bind(&isEnabled_);
    check->onChange.bind(this, &DialogPreviewSettings::onAction,
                         static_cast<int>(ACT_TOGGLE));
    check->setTooltip(
        "If enabled, shows a second playfield with seperate settings");

    myLayout.row().col(194);
    myLayout.add<WgSeperator>();

    WgCycleButton* preset = myLayout.add<WgCycleButton>();
    preset->onChange.bind(this, &DialogPreviewSettings::onAction,
                          static_cast<int>(ACT_MODE));
    preset->value.bind(&drawMode_);
    preset->addItem("C-mod");
    preset->addItem("X-mod");
    preset->addItem("Variable");
    preset->setTooltip("Draw mode used for preview");

    myLayout.add<WgSeperator>();
    myLayout.row().col(190);

    check = myLayout.add<WgCheckbox>();
    check->text.set("Beat Lines");
    check->value.bind(&isBeatLines_);
    check->onChange.bind(this, &DialogPreviewSettings::onAction,
                         static_cast<int>(ACT_BEAT));

    check = myLayout.add<WgCheckbox>();
    check->text.set("Reverse Scroll");
    check->value.bind(&isReverse_);
    check->onChange.bind(this, &DialogPreviewSettings::onAction,
                         static_cast<int>(ACT_REVERSE));

    check = myLayout.add<WgCheckbox>();
    check->text.set("Transparent Fake Notes");
    check->value.bind(&isFakeNotes_);
    check->onChange.bind(this, &DialogPreviewSettings::onAction,
                         static_cast<int>(ACT_TRANS_NOTES));

    check = myLayout.add<WgCheckbox>();
    check->text.set("Transparent Fake Regions");
    check->value.bind(&isFakeRegions_);
    check->onChange.bind(this, &DialogPreviewSettings::onAction,
                         static_cast<int>(ACT_TRANS_REGIONS));

    myLayout.row().col(194);
    myLayout.add<WgSeperator>();
    myLayout.row().col(90).col(100);

    check = myLayout.add<WgCheckbox>();
    check->text.set("C-mod");
    check->value.bind(&isCModEnabled_);
    check->onChange.bind(this, &DialogPreviewSettings::onAction,
                         static_cast<int>(ACT_CMOD_TOGGLE));
    check->setTooltip(
        "If enabled, a line will be draw where the view would end in C-mod");

    WgSpinner* spinner = myLayout.add<WgSpinner>();
    spinner->value.bind(&cmod_);
    spinner->setPrecision(0, 0);
    spinner->setRange(50, 3000);
    spinner->setStep(50.0);
    spinner->onChange.bind(this, &DialogPreviewSettings::onAction,
                           static_cast<int>(ACT_CMOD_VALUE));

    check = myLayout.add<WgCheckbox>();
    check->text.set("X-mod");
    check->value.bind(&isXModEnabled_);
    check->onChange.bind(this, &DialogPreviewSettings::onAction,
                         static_cast<int>(ACT_XMOD_TOGGLE));
    check->setTooltip(
        "If enabled, a line will be draw where the view would end in X-mod");

    spinner = myLayout.add<WgSpinner>();
    spinner->value.bind(&xmod_);
    spinner->setPrecision(2, 2);
    spinner->setRange(0.5, 20.0);
    spinner->setStep(0.5);
    spinner->onChange.bind(this, &DialogPreviewSettings::onAction,
                           static_cast<int>(ACT_XMOD_VALUE));

    myLayout.row().col(190);
    check = myLayout.add<WgCheckbox>();
    check->text.set("Crop Notefield");
    check->value.bind(&isNotesCropped_);
    check->onChange.bind(this, &DialogPreviewSettings::onAction,
                         static_cast<int>(ACT_CROP_TOGGLE));
    check->setTooltip("If enabled, notes past the view line are hidden");
}

void DialogPreviewSettings::onTick() {
    isEnabled_ = gNotefieldPreview->hasEnabled();
    drawMode_ = gNotefieldPreview->getMode();
    isBeatLines_ = gNotefieldPreview->hasShowBeatLines();
    isReverse_ = gNotefieldPreview->hasReverseScroll();
    isFakeNotes_ = gNotefieldPreview->hasTransparentNotes();
    isFakeRegions_ = gNotefieldPreview->hasTransparentRegions();
    isCModEnabled_ = gNotefieldPreview->hasGuideCMod();
    cmod_ = gNotefieldPreview->getGuideCMod();
    isXModEnabled_ = gNotefieldPreview->hasGuideXMod();
    xmod_ = gNotefieldPreview->getGuideXMod();
    isNotesCropped_ = gNotefieldPreview->hasGuideCrop();
    EditorDialog::onTick();
}

void DialogPreviewSettings::onAction(int id) {
    switch (id) {
        case ACT_TOGGLE: {
            gNotefieldPreview->toggleEnabled();
        } break;
        case ACT_MODE: {
            gNotefieldPreview->setMode(drawMode_);
        } break;
        case ACT_BEAT: {
            gNotefieldPreview->toggleShowBeatLines();
        } break;
        case ACT_REVERSE: {
            gNotefieldPreview->toggleReverseScroll();
        } break;
        case ACT_TRANS_NOTES: {
            gNotefieldPreview->toggleTransparentNotes();
        } break;
        case ACT_TRANS_REGIONS: {
            gNotefieldPreview->toggleTransparentRegions();
        } break;
        case ACT_CMOD_TOGGLE: {
            gNotefieldPreview->toggleGuideCMod();
        } break;
        case ACT_CMOD_VALUE: {
            gNotefieldPreview->setGuideCMod(cmod_);
        } break;
        case ACT_XMOD_TOGGLE: {
            gNotefieldPreview->toggleGuideXMod();
        } break;
        case ACT_XMOD_VALUE: {
            gNotefieldPreview->setGuideXMod(xmod_);
        } break;
        case ACT_CROP_TOGGLE: {
            gNotefieldPreview->toggleGuideCrop();
        } break;
    }
}
};  // namespace Vortex
