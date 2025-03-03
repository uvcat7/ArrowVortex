#pragma once

#include <Vortex/View/EditorDialog.h>

#include <Core/Interface/Widgets/VerticalTabs.h>

#include <Vortex/Edit/StreamGenerator.h>

namespace AV {

class DialogGenerateNotes : public EditorDialog
{
public:
	~DialogGenerateNotes();
	DialogGenerateNotes();

private:

	void myInitWidgets();
	void myGenerateNotes();

	struct Form;
	Form* myForm = nullptr;

	StreamGenerator myStream;
};

} // namespace AV
