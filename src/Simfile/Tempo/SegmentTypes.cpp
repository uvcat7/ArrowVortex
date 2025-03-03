#include <Precomp.h>

#include <Simfile/Tempo/SegmentTypes.h>

namespace AV {

// =====================================================================================================================
// Persistent segments.

const BpmChangeSegment BpmChangeSegment::fallback = { 120 };

const ScrollSegment ScrollSegment::fallback = { 1 };

const SpeedSegment SpeedSegment::fallback = { 1, 0, 0 };

const TimeSignatureSegment TimeSignatureSegment::fallback = { 4, 4 };

const TickCountSegment TickCountSegment::fallback = { 1 };

const ComboSegment ComboSegment::fallback = { 1, 1 };

// =====================================================================================================================
// Isolated segments.

const StopSegment StopSegment::none = { 0 };

const DelaySegment DelaySegment::none = { 0 };

const WarpSegment WarpSegment::none = { 0 };

const FakeSegment FakeSegment::none = { 0 };

const LabelSegment LabelSegment::none = {};

} // namespace AV
