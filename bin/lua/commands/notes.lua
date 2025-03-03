for v in AV.quantizations do
	Command {
		id = "notes-select-quantization-" .. v,
		run = function() AV.selectNotes("quantization:" .. v) end,
	}
end
Command {
	id = "notes-select-steps",
	run = function() AV.selectNotes("type:step") end,
}
Command {
	id = "notes-select-mines",
	run = function() AV.selectNotes("type:mine") end,
}
Command {
	id = "notes-select-holds",
	run = function() AV.selectNotes("type:hold") end,
}
Command {
	id = "notes-select-rolls",
	run = function() AV.selectNotes("type:roll") end,
}
Command {
	id = "notes-select-all",
	run = function() AV.selectNotes("type:any") end,
}
Command {
	id = "notes-convert-to-mine",
}
Command {
	id = "notes-convert-to-fake",
}
Command {
	id = "notes-convert-to-lift",
}
Command {
	id = "notes-convert-to-step",
}
Command {
	id = "notes-toggle-hold-roll",
}
Command {
	id = "notes-mirror-horizontally",
	run = function() AV.mirrorNotes(true, false) end,
}
Command {
	id = "notes-mirror-vertically",
	run = function() AV.mirrorNotes(false, true) end,
}
Command {
	id = "notes-mirror-both",
	run = function() AV.mirrorNotes(true, true) end,
}
Command {
	id = "notes-expand-2-to-1",
	run = function() AV.scaleNotes(2, 1) end,
}
Command {
	id = "notes-expand-3-to-2",
	run = function() AV.scaleNotes(3, 2) end,
}
Command {
	id = "notes-expand-4-to-3",
	run = function() AV.scaleNotes(4, 3) end,
}
Command {
	id = "notes-compress-1-to-2",
	run = function() AV.scaleNotes(1, 2) end,
}
Command {
	id = "notes-compress-2-to-3",
	run = function() AV.scaleNotes(2, 3) end,
}
Command {
	id = "notes-compress-3-to-4",
	run = function() AV.scaleNotes(3, 4) end,
}