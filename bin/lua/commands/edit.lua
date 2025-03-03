local editor = AV.settings.editor
Command {
	id = "toggle-undo-redo-jump",
	run = function() editor.undoRedoJump = not editor.undoRedoJump end,
	checked = function() return editor.undoRedoJump end,
}
Command {
	id = "toggle-time-based-copy",
	run = function() editor.timeBasedCopy = not editor.timeBasedCopy end,
	checked = function() return editor.timeBasedCopy end,
}
Command {
	id = "run-script",
	run = function() AV.runScript("pack-info", "dumpSongInfo") end,
}