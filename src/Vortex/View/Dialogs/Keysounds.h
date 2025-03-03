#pragma once

#include <Vortex/View/EditorDialog.h>
#include <Core/Graphics/Draw.h>

namespace AV {

class DialogKeysounds : public EditorDialog
{
public:
	void onUpdateSize() override;
	void tick(bool enabled) override;
	void draw(bool enabled) override;

	~DialogKeysounds();
	DialogKeysounds();

private:
	struct KeysoundList;

	shared_ptr<KeysoundList> myList;
};

} // namespace AV
