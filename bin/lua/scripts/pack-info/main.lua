AV.loadLibrary("io")
AV.loadLibrary("math")

function dumpChartBreakdowns()
	local file = io.open(packDir .. "\\breakdowns.txt", "w")
	for i, song in ipairs(listDirectories(packDir, false)) do
		openSimfile(packDir .. "/" .. song)
		local numCharts = getNumCharts()
		for j = 1, numCharts do
			local chart = getChart(j)
			file:write(getSongTitle() .. ";" .. chart:getDifficulty() .. " " .. chart:getMeter() .. ";")
			local measures = {}
			local firstMeasure = math.maxinteger
			local lastMeasure = 0
			for noterow in chart:getNotesByRow() do
				local m = math.floor(noterow.row / 192)
				firstMeasure = math.min(firstMeasure, m)
				lastMeasure = math.max(lastMeasure, m)
				for k, note in ipairs(noterow) do
					if note.type ~= NOTE_TYPE_MINE then
						measures[m] = (measures[m] or 0) + 1
					end
				end
			end
			for k = firstMeasure, lastMeasure do
				file:write((measures[k] or 0) .. " ")
			end
			file:write("\n")
		end
	end
	file:close()
end

function dumpSongInfo()
	local packDir = AV.askOpenDirectory("Choose pack directory")
	local file = io.open(packDir .. "\\songs.txt", "w")
	for song in AV.listDirectories(packDir, false) do
		local sim = AV.loadSimfile(packDir .. "/" .. song)
		local tempo = sim.tempo
		file:write(math.floor(tempo:getBpm(0)) .. ";" .. sim.title .. ";")
		file:write(sim.credit .. ";pack;")
		local diffs = ""
		local endRow = 0
		for chart in sim.charts do
			if diffs == "" then
				diffs = chart.meter
			else
				diffs = diffs .. "/" .. chart.meter
			end
			endRow = math.max(endRow, chart.endRow)
		end
		local endTime = tempo:rowToTime(endRow)
		local minutes = math.floor(endTime / 60)
		local seconds = math.floor(endTime - minutes * 60)
		file:write(diffs .. ";" .. minutes .. ":" .. seconds .. "\n")
	end
	file:close()
end