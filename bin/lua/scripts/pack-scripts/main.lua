loadLibrary("io")
loadLibrary("math")

function dumpBreakdown()
	local packDir = [[D:\Games\StepMania 5.1 DEV\Songs\DCS3 Tournament]]
	local file = io.open([[D:\Games\StepMania 5.1 DEV\Songs\DCS3 Tournament\breakdown.txt]], "w")
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