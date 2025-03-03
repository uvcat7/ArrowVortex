local audio = AV.settings.audio
Command {
	id = "toggle-mute-audio",
	run = function() audio.mute = not audio.mute end, 
	checked = function() return audio.mute end,
}
Command {
	id = "toggle-beat-tick",
	run = function() audio.beatTick = not audio.beatTick end,
	checked = function() return audio.beatTick end,
}
Command {
	id = "toggle-note-tick",
	run = function() audio.noteTick = not audio.noteTick end,
	checked = function() return audio.noteTick end,
}
Command {
	id = "music-convert-to-ogg",
	run = AV.convertMusicToOgg,
}
Command {
	id = "music-volume-reset",
	run = function() audio.musicVolume = 100 end,
}
Command {
	id = "music-volume-up",
	run = function() audio.musicVolume = audio.musicVolume + 10 end,
}
Command {
	id = "music-volume-down",
	run = function() audio.musicVolume = audio.musicVolume - 10 end,
}
Command {
	id = "music-speed-reset",
	run = function() audio.musicSpeed = 100 end,
}
Command {
	id = "music-speed-up",
	run = function() audio.musicSpeed = audio.musicSpeed + 10 end,
}
Command {
	id = "music-speed-down",
	run = function() audio.musicSpeed = audio.musicSpeed - 10 end,
}