function removeHigherDiffs(args)
	
	if not args.path then
		logError("Missing --path arg, call with path to song pack")
		return
	end
	local packDir = args.path

	for i, song in ipairs(listDirectories(packDir, false)) do
		openSimfile(packDir .. "/" .. song)
		local numCharts = getNumCharts()
		local title = getSongTitle()

		local hasLowers = false
		local hasHighers = false
		for j = 1, numCharts do
			if getChart(j):getMeter() > 10 then
				hasHighers = true
			else
				hasLowers = true
			end
		end

		if hasLowers and hasHighers then
			logInfo("Trimming " .. title .. "...")
			for j = numCharts, 1, -1 do
				local diff = getChart(j):getMeter()
				if diff > 10 then
					logInfo("-> removing " .. diff)
					removeChart(j)
				end
			end
			saveSimfile()
		else
			logInfo("Skipping " .. title)
		end
	end
end