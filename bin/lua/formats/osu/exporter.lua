AV.loadLibrary("io")
AV.loadLibrary("string")
AV.loadLibrary("table")
AV.loadLibrary("math")

function section(f, name)
	f:write("\n[" .. name .. "]\n")
end

function line(f, str)
	f:write(str, "\n")
end

function alt(first, alternative)
	if first:len() > 0 then
		return first
	end
	return alternative
end

function writeTimingPoints(chart, f)
	local events = chart:getTempo():getTimingEvents()
	for i, event in ipairs(events) do
		local msPerBeat = 60000.0 / event.bpm
		local msTime = math.floor(event.startTime * 1000.0)
		line(f, msTime .. "," .. msPerBeat .. ",4,1,0,100,1,0")
	end
end

function writeHitObjects(chart, f)
	local tempo = chart:getTempo()
	local width = 512.0 / chart:getNumColumns()
	for noterow in chart:getNotesByRow() do
		for i, note in ipairs(noterow) do
			local x = math.floor((note.column - 0.5) * width)
			local msTime = math.floor(tempo:rowToTime(noterow.row) * 1000.0)
			local s = x .. ",192," .. msTime
			if note.length > 0 then
				msEndTime = math.floor(tempo:rowToTime(noterow.row + note.length) * 1000.0)
				line(f, s .. ",128,0," .. msEndTime .. ":0:0:0:0:")
			else
				line(f, s .. ",1,0,0:0:0:0:")
			end
		end
	end
end

function writeChart(chart, f)
	line(f, "osu file format v14")

	section(f, "General")
	line(f, "AudioFilename: " .. getMusicPath())
	line(f, "Mode: 3");

	section(f, "Metadata")
	line(f, "Title:" .. alt(getTransliteratedSongTitle(), getSongTitle()))
	line(f, "TitleUnicode:" .. getSongTitle())
	line(f, "Artist:" .. alt(getTransliteratedSongArtist(), getSongArtist()))
	line(f, "ArtistUnicode:" .. getSongArtist())
	line(f, "Creator:" .. alt(chart:getStepAuthor(), "Unknown"))
	line(f, "Version:" .. chart:getDifficulty())

	section(f, "Difficulty")
	line(f, "HPDrainRate:5")
	line(f, "CircleSize:" .. chart:getNumColumns())
	line(f, "OverallDifficulty:" .. chart:getMeter())
	line(f, "ApproachRate:5")
	line(f, "SliderMultiplier:1")
	line(f, "SliderTickRate:1")

	section(f, "Events")
	line(f, "//Background and Video inputs");
	local bg = getBackgroundPath()
	if (bg:len() > 0) then
		line(f, "0,0,\"" .. bg .. "\",0,0")
	end
	line(f, "//Break Periods")
	line(f, "//Storyboard Layer 0 (Background)")
	line(f, "//Storyboard Layer 1 (Fail")
	line(f, "//Storyboard Layer 2 (Pass")
	line(f, "//Storyboard Layer 3 (Foreground)")
	line(f, "//Storyboard Sound Samples")

	section(f, "TimingPoints")
	writeTimingPoints(chart, f)

	section(f, "HitObjects")
	writeHitObjects(chart, f)	
end

function saveOsu(args)
	let sim = args.simfile
	for chart in sim.chart do
		local filename = sim.filename .. " [" .. chart:getDifficulty() .. "].osu"
		local file = io.open(args.directory .. "/" .. filename, "w")
		writeChart(chart, file)
		file:close()
	end
end