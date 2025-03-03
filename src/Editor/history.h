#pragma once

#include <Core/String.h>
#include <Core/Input.h>
#include <Core/ByteStream.h>

namespace Vortex {

struct History : public InputHandler
{
	struct Bindings
	{
		Simfile* simfile;
		Chart* chart;
		Tempo* tempo;
	};

	typedef uint EditId;

	typedef void (*ReleaseFunc)(ReadStream& in, bool hasBeenApplied);
	typedef String(*ApplyFunc)(ReadStream& in, Bindings bound, bool undo, bool redo);

	static void create();
	static void destroy();

	virtual EditId addCallback(ApplyFunc apply, ReleaseFunc release = nullptr) = 0;

	virtual void addEntry(EditId id, const void* data, uint size) = 0;
	virtual void addEntry(EditId id, const void* data, uint size, Tempo* targetTempo) = 0;
	virtual void addEntry(EditId id, const void* data, uint size, Chart* targetChart) = 0;

	virtual void startChain() = 0;
	virtual void finishChain(String message) = 0;
	
	virtual void onFileOpen(Simfile* simfile) = 0;
	virtual void onFileClosed() = 0;
	virtual void onFileSaved() = 0;

	virtual bool hasUnsavedChanges() const = 0;
};

extern History* gHistory;

}; // namespace Vortex
