AV.loadLibrary("io")
AV.loadLibrary("string")
AV.loadLibrary("table")
AV.loadLibrary("math")

test_path = [[D:\Dev\ArrowVortex\test\data\osu\Dear Brave (4key, OSZ)\372359 Kano - Dear Brave.osz]]

local modesTable =
{
	[4] = "dance-single",
	[6] = "dance-solo",
	[7] = "kb7-single",
	[8] = "dance-double"
}

function toBpm(spb)
	local bpm = 60.0 / spb
	local rounded = math.floor(bpm + 0.5)
	if math.abs(bpm - rounded) < 0.001 then
		return rounded
	end
	return bpm
end

function toNoteCol(x, ncols)
	local col = 1 + math.floor(x * ncols / 512)
	return math.max(1, math.min(col, ncols))
end

function processTempo(sim, timingPoints)
	local tempo = sim.tempo
	local tp = timingPoints[1]
	if tp[1] <= 0 then
		tempo.offset = tp[1]
	else
		tempo.offset = tp[2] - math.fmod(tp[1], tp[2])
	end
	tempo:setBpm(0, toBpm(tp[2]))
	AV.logInfo("BPM: " .. toBpm(tp[2]))
	for i = 2, #timingPoints do
		tp = timingPoints[i]
		tempo:setBpm(tempo:timeToRow(tp[1]), toBpm(tp[2]))
	end
end

function processFirstChart(sim, chart, file, ncols)

	local general = file.General or {}
	sim.music = general.AudioFilename

	local meta = file.Metadata or {}
	sim.title = meta.Title
	sim.titleTranslit = meta.TitleUnicode
	sim.artist = meta.Artist
	sim.artistTranslit = meta.ArtistUnicode
	sim.credit = meta.Creator

	local events = file.Events or {}
	if events.background then
		sim.background = events.background
		sim.banner = events.background
	end

	processTempo(sim, file.TimingPoints)
	processChart(chart, file, ncols)
end

function processChart(chart, file, ncols)
	local tempo = getTempo()
	local diff = file.Difficulty or {}
	chart.meter = math.floor(tonumber(diff.OverallDifficulty))
	for i, n in ipairs(file.HitObjects) do
		if #n == 2 then
			chart:addStep(toNoteCol(n[2], ncols), tempo:timeToRow(n[1]))
		else
			chart:addHold(toNoteCol(n[2], ncols), tempo:timeToRow(n[1]), tempo:timeToRow(n[3]))
		end
	end
end

function parseMs(t)
	return tonumber(t) * 0.001
end

function parseKeyValues(section, line)
	local key, value = line:match("^(%w+)%s*:%s*(.+)$")
	if key and value then
		section[key] = value
	end
end

function parseEvents(section, line)
	local background = line:match('^0,0,"(.+)"')
	if background then
		section.background = background
	end
end

function parseTimingPoints(section, line)
	local t, bl = line:match("^(%d+),([%d%.]+),")
	if t and bl then
		table.insert(section, {parseMs(t), parseMs(bl)})
	end
end

function parseHitObjects(section, line)
	local x, t, nt, et = line:match("^(%d+),%d+,(%d+),(%d+),%d+,(%d+)")
	if nt == "1" then
		table.insert(section, {parseMs(t), tonumber(x)})
	elseif nt == "128" then
		table.insert(section, {parseMs(t), tonumber(x), parseMs(et)})
	end
end

local osuSectionParsers =
{
	General = parseKeyValues,
	Metadata = parseKeyValues,
	Difficulty = parseKeyValues,
	Events = parseEvents,
	TimingPoints = parseTimingPoints,
	HitObjects = parseHitObjects,
}

function parseOsuFile(path)
	local result = {}
	local section = {}
	local parseLine = nil
	for l in io.lines(path) do
		l = l:gsub("//.*", "") -- remove comments
		l = l:match("^%s*(.-)%s*$") -- trim whitespace
		if l ~= "" then
			local sectionKey = l:match("^%[(%w+)%]$")
			local parser = osuSectionParsers[sectionKey]
			if sectionKey and parser then
				section = {}
				result[sectionKey] = section
				parseLine = parser
			elseif parseLine then
				parseLine(section, l)
			end
		end
	end
	return result
end

function parseSongDir(sim, dir)
	local firstChart = true
	for filename in AV.listFiles(dir, false, "osu") do
		local path = dir .. "/" .. filename
		local file = parseOsuFile(path)
		if file.General.Mode == "3" then
			local ncols = tonumber(file.Difficulty.CircleSize)
			local mode = modesTable[ncols]
			if mode then
				local chart = sim:addChart(mode)
				if firstChart then
					processFirstChart(sim, chart, file, ncols)
					firstChart = false
				else
					processChart(chart, file, ncols)
				end
			end
		end
	end
end

function loadOsu(args)
	local sim = args.simfile
	if AV.isDirectory(args.path) then
		parseSongDir(sim, args.path)
	elseif string.find(path:lower(), "%.osz$") then
		 = AV.unzipFile(path)
		parseSongDir(sim, unzipDir)
	else
		parseSongDir(sim, AV.getParentDirectory(path))
	end
end