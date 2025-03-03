local view = AV.settings.view
local status = AV.settings.status
Command {
	id = "show-shortcuts",
}
Command {
	id = "show-command-line",
}
Command {
	id = "show-message-log",
}
Command {
	id = "show-debug-log",
}
Command {
	id = "show-about",
}
Command {
	id = "toggle-show-waveform",
	run = function() view.showWaveform = not view.showWaveform end,
	checked = function() return view.showWaveform end,
}
Command {
	id = "toggle-show-beat-lines",
	run = function() view.showBeatLines = not view.showBeatLines end,
	checked = function() return view.showBeatLines end,
}
Command {
	id = "toggle-show-tempo-boxes",
	run = function() view.showTempoBoxes = not view.showTempoBoxes end,
	checked = function() return view.showTempoBoxes end,
}
Command {
	id = "toggle-show-tempo-help",
	run = function() view.showTempoHelp = not view.showTempoHelp end,
	checked = function() return view.showTempoHelp end,
}
Command {
	id = "toggle-show-column-background",
	run = function() view.showColumnBg = not view.showColumnBg end,
	checked = function() return view.showColumnBg end,
}
Command {
	id = "toggle-show-notes",
	run = function() view.showNotes = not view.showNotes end,
	checked = function() return view.showNotes end,
}
Command {
	id = "toggle-show-passed-notes",
	run = function() view.showPassedNotes = not view.showPassedNotes end,
	checked = function() return view.showPassedNotes end,
}
Command {
	id = "toggle-reverse-scroll",
	run = function() view.reverseScroll = not view.reverseScroll end,
	checked = function() return view.reverseScroll end,
}
Command {
	id = "toggle-column-colors",
	run = function() view.columnColors = not view.columnColors end,
	checked = function() return view.columnColors end,
}
Command {
	id = "enable-time-based-scroll",
	run = function() view.isTimeBased = true end,
	checked = function() return view.isTimeBased end,
}
Command {
	id = "enable-beat-based-scroll",
	run = function() view.isTimeBased = false end,
	checked = function() return not view.isTimeBased end,
}
Command {
	id = "minimap-show-notes",
	run = function() view.minimapMode = "notes" end,
	checked = function() return view.minimapMode == "notes" end,
}
Command {
	id = "minimap-show-density",
	run = function() view.minimapMode = "density" end,
	checked = function() return view.minimapMode == "density" end,
}
Command {
	id = "background-hide",
	run = function() view.backgroundBrightness = 0 end,
}
Command {
	id = "background-brightness-up",
	run = function() view.backgroundBrightness = view.backgroundBrightness + 10 end,
}
Command {
	id = "background-brightness-down",
	run = function() view.backgroundBrightness = view.backgroundBrightness - 10 end,
}
Command {
	id = "background-mode-stretch",
	run = function() view.backgroundMode = "stretch" end,
	checked = function() return view.backgroundMode == "stretch" end,
}
Command {
	id = "background-mode-letterbox",
	run = function() view.backgroundMode = "letterbox" end,
	checked = function() return view.backgroundMode == "letterbox" end,
}
Command {
	id = "background-mode-crop",
	run = function() view.backgroundMode = "crop" end,
	checked = function() return view.backgroundMode == "crop" end,
}
Command {
	id = "toggle-status-chart",
	run = function() status.showChart = not status.showChart end,
	checked = function() return status.showChart end,
}
Command {
	id = "toggle-status-snap",
	run = function() status.showSnap = not status.showSnap end,
	checked = function() return status.showSnap end,
}
Command {
	id = "toggle-status-bpm",
	run = function() status.showBpm = not status.showBpm end,
	checked = function() return status.showBpm end,
}
Command {
	id = "toggle-status-row",
	run = function() status.showRow = not status.showRow end,
	checked = function() return status.showRow end,
}
Command {
	id = "toggle-status-beat",
	run = function() status.showBeat = not status.showBeat end,
	checked = function() return status.showBeat end,
}
Command {
	id = "toggle-status-measure",
	run = function() status.showMeasure = not status.showMeasure end,
	checked = function() return status.showMeasure end,
}
Command {
	id = "toggle-status-time",
	run = function() status.showTime = not status.showTime end,
	checked = function() return status.showTime end,
}
Command {
	id = "toggle-status-timing-mode",
	run = function() status.showTimingMode = not status.showTimingMode end,
	checked = function() return status.showTimingMode end,
}