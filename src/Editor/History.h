#pragma once

#include <Core/Input.h>
#include <Core/ByteStream.h>

namespace Vortex {

struct History : public InputHandler {
    struct Bindings {
        Simfile* simfile;
        Chart* chart;
        Tempo* tempo;
    };

    typedef uint32_t EditId;

    typedef void (*ReleaseFunc)(ReadStream& in, bool hasBeenApplied);
    typedef std::string (*ApplyFunc)(ReadStream& in, Bindings bound, bool undo,
                                     bool redo);

    static void create();
    static void destroy();

    virtual EditId addCallback(ApplyFunc apply,
                               ReleaseFunc release = nullptr) = 0;

    virtual void addEntry(EditId id, const void* data, uint32_t size) = 0;
    virtual void addEntry(EditId id, const void* data, uint32_t size,
                          Tempo* targetTempo) = 0;
    virtual void addEntry(EditId id, const void* data, uint32_t size,
                          Chart* targetChart) = 0;

    virtual void startChain() = 0;
    virtual void finishChain(std::string message) = 0;

    virtual void onFileOpen(Simfile* simfile) = 0;
    virtual void onFileClosed() = 0;
    virtual void onFileSaved() = 0;

    virtual bool hasUnsavedChanges() const = 0;
};

extern History* gHistory;

};  // namespace Vortex
