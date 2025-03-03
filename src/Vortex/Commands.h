#pragma once

#include <Precomp.h>

namespace AV {

class Command
{
public:
	struct State { bool enabled, checked; };
	Command(const char* name);
	virtual ~Command();
	virtual void run() const = 0;
	virtual State getState() const = 0;
	const char* const id;
};

// Global command functions.
namespace Commands
{
	void initialize(const XmrDoc& settings);
	void deinitialize();

	// Returns the command associated with the given id, or null if it is not found.
	Command* find(stringref id, bool intrinsicsOnly);
};

} // namespace AV
