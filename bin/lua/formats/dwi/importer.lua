AV.loadLibrary("io")
AV.loadLibrary("string")
AV.loadLibrary("table")

function toRow(key) -- DWI uses 4 subdivisions per beat
	return tonumber(key) * AV.rowsPerBeat * 0.25
end

function toDifficulty(str)
	if str == "BEGINNER" then
		return "beginner"
	elseif str == "BASIC" then
		return "easy"
	elseif str == "ANOTHER" then
		return "medium"
	elseif str == "MANIAC" then
		return "hard"
	elseif str == "SMANIAC" then
		return "challenge"
	end
	return "edit"
end

function parseBpmChanges(tempo, str)
	for key, bpm in string.gmatch(str, "([%d%.]+)=([%d%.%-]+),?") do
		tempo:setBpm(toRow(key), tonumber(bpm))
	end
end

function parseStops(tempo, str)
	for key, stop in string.gmatch(str, "([%d%.]+)=([%d%.%-]+),?") do
		tempo:setStop(toRow(key), tonumber(stop) * 0.001)
	end
end

function parseDisplayBpm(tempo, str)
	if str == "*" then
		tempo:setDisplayBpm("random")
	else
		lo, hi = str:match("([%d%.%-]+)%.%.([%d%.%-]+)")
		if lo and hi then
			tempo:setDisplayBpm("custom", tonumber(lo), tonumber(hi))
		else
			local bpm = tonumber(str)
			tempo:setDisplayBpm("custom", bpm, bpm)
		end
	end
end

function columnMapping(padNr, solo)
	local D, L, U, R, UL, UR
	if solo then
		local i = 1+(padNr-1)*6
		L, UL, D, U, UR, R = i, i+1, i+2, i+3, i+4, i+5
	else
		local i = 1+(padNr-1)*4
		L, D, U, R = i, i+1, i+2, i+3
	end
	local mapping = {
		["0"] = {},
		["1"] = {D, L},
		["2"] = {D},
		["3"] = {D, R},
		["4"] = {L},
		["5"] = {},
		["6"] = {R},
		["7"] = {U, L},
		["8"] = {U},
		["9"] = {U, R},
		["A"] = {U, D},
		["B"] = {L, R},
		["C"] = {UL},
		["D"] = {UR},
		["E"] = {L, UL},
		["F"] = {UL, D},
		["G"] = {UL, U},
		["H"] = {UL, R},
		["I"] = {L, UR},
		["J"] = {D, UR},
		["K"] = {U, UR},
		["L"] = {UR, R},
		["M"] = {UL, UR},
	}
	return mapping
end

function parseRow(notes, i, chart, row, mapping, holds)
	local nexti
	-- <..> = row with multiple symbols
	local symbols = notes:match("^<([%w!]+)>", i)
	if symbols then
		nexti = i + string.len(symbols) + 2
	else
		-- X!Y = X symbols, of which Y start holds
		symbols = notes:match("^(%w!%w)", i)
		if symbols then
			nexti = i + string.len(symbols)
		else
			-- otherwise row is single symbol
			symbols = notes:sub(i,i)
			nexti = i + 1
		end
	end
	-- early out on empty row
	if symbols == "0" then
		return nexti
	end
	-- parse notes/holds
	local notes = {}
	local startHold = false
	for c in symbols:gmatch(".") do
		if c == "!" then
			startHold = true
		else
			for icol, col in ipairs(mapping[c]) do
				if startHold then
					holds[col] = row
					notes[col] = nil
				elseif holds[col] then
					chart:addHold(col, holds[col], row)
					holds[col] = nil
				else
					notes[col] = row
				end
			end
		end
	end
	for col, row in pairs(notes) do
		chart:addStep(col, row)
	end
	return nexti
end

quantizations =
{
	["("] = 12, [")"] = 24, -- 1/16 steps
	["["] = 8,  ["]"] = 24, -- 1/24 steps
	["{"] = 3,  ["}"] = 24, -- 1/64 steps
	["`"] = 1,  ["'"] = 24, -- 1/192 steps
}

function parseNotes(chart, notes, mapping)
	local quant = 24 -- 1/8 steps
	local row = 0
	local i = 1
	local holds = {}
	while i <= string.len(notes) do
		local c = notes:sub(i, i)
		if quantizations[c] then
			quant = quantizations[c]
			i = i + 1
		else
			i = parseRow(notes, i, chart, row, mapping, holds)
			row = row + quant
		end
	end
end

function parseChart(sim, str, numPads, solo, style)
	-- format is DIFF:METER:NOTES[:NOTES2]
	local fields = {}
	for f in str:gmatch("([^:]+)") do
		table.insert(fields, f)
	end
	if #fields ~= 2 + numPads then
		error(str .. " has an unexpected number of fields: " .. #fields)
	end
	local chart = sim:addChart(style)
	chart.difficulty = toDifficulty(fields[1])
	chart.meter = tonumber(fields[2])
	for i = 1, numPads do
		local mapping = columnMapping(i, solo)
		parseNotes(chart, fields[2 + i], mapping)
	end
end

function parse(sim, key, val)
	if key == "SINGLE" then
		parseChart(sim, val, 1, false, "dance-single")
	elseif key == "DOUBLE" then
		parseChart(sim, val, 2, false, "dance-double")
	elseif key == "COUPLE" then
		parseChart(sim, val, 2, false, "dance-couple")
	elseif key == "SOLO" then
		parseChart(sim, val, 1, true, "dance-solo")
	elseif key == "TITLE" then
		sim.title = val
	elseif key == "ARTIST" then
		sim.artist = val
	elseif key == "FILE" then
		sim.music = val
	elseif key == "BPM" then
		sim.tempo:setBpm(0, tonumber(val))
	elseif key == "GAP" then
		sim.tempo.offset = tonumber(val) * -0.001
	elseif key == "CHANGEBPM" then
		parseBpmChanges(sim.tempo, val)
	elseif key == "FREEZE" then
		parseStops(sim.tempo, val)
	elseif key == "GENRE" then
		sim.songGenre = val
	elseif key == "DISPLAYBPM" then
		parseDisplayBpm(sim.tempo, val)
	end
end

function loadDwi(args)
	local t = {}
	for l in io.lines(args.path) do
		l = l:gsub("//.*", "") -- remove comments
		l = l:match("^%s*(.-)%s*$") -- trim whitespace
		if l ~= "" then table.insert(t, l) end
	end
	t = table.concat(t)
	for k, v in t:gmatch("#([^:]+):([^;]+);") do
		parse(args.simfile, k, v)
	end
end