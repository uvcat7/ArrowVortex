#pragma once

#include <Vortex/Common.h>

#include <Vortex/View/EditorDialog.h>

namespace AV {

namespace DialogMan
{
	void initialize(const XmrDoc& settings);
	void deinitialize();

	// Reqests the opening of the dialog with the given id, if it's currently closed.
	void requestOpen(DialogId id);

	// Returns true if the given dialog is currently open.
	bool dialogIsOpen(DialogId id);
};

} // namespace AV
