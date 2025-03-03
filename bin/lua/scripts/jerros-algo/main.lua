-- Load librarys.
loadLibrary("io");
loadLibrary("string")
loadLibrary("math")
loadLibrary("table")

packDir = [[D:\Installed Games\StepMania 5.1\Songs\Easy As Pie 1]]

-- Open the files for the Titles and the Chart Data.
local path = [[C:\Users\Bram\Desktop\TestOutput\RegressTree Data.txt]];
local file_data = io.open(path, "w")

if not file_data then
	showError("Unable to create note table file_data...")
	return;
end

local path = [[C:\Users\Bram\Desktop\TestOutput\RegressTree Titles.txt]];
local file_titles = io.open(path, "w")

if not file_titles then
	showError("Unable to create note table file...")
	return;
end

-- Rounding function.
function round(num, numDecimalPlaces)
	local mult = 10^(numDecimalPlaces or 0)
	return math.floor(num * mult + 0.5) / mult
end

-- Get percentile of a table
function percentile(t, p)
  local indxMax = #t

  table.sort(t)

  local indxPrctl = round(p * indxMax , 0)
  local prctl = t[indxPrctl]
  
  return prctl
end

local foobar = 1

function processSong()
	local numCharts = getNumCharts()
	
	-- If there are no charts to export, quit out early.
	if numCharts == 0 then
		showInfo("I can't export notes because there are no charts...")
		return;
	end
	
	-- Write notes of each chart to the output file.
	for i = 1,numCharts do
	
		local chart = getChart(i)
		local tempo = chart:getTempo()
	
		-- Write properties if it's a Single chart.
		if chart:getGameMode() == "Dance Single" then
			local FirstNoteFound = 0
			local TotalSteps = 0
			local StepTable = {}
			local TimeTable = {}
			--local MeassureTime = 1/(chart:getBPM(1)/60) * 4
			local RegionTime = 1
			local StepsInMeassure = {0}
			local MeassureNPSprctles = {0,0,0,0,0,0,0,0,0}
			local MeassureNPS = {}
			local CurrentMeassure = 0
			local Jumps = 0
			local Hands = 0
			local Quads = 0
			
			-- Find the time of the first and last note
			for noterow in chart:getNotesByRow() do
				local rowTime = tempo:rowToTime(noterow.row)
				
				-- if foobar < 100 then
				-- 	showInfo("rowTime(" .. noterow.row .. ") = " .. rowTime)
				-- 	foobar = foobar + 1
				-- end
				
				-- Get number of jumps, hands and quads
				if #noterow == 4 then
					Quads = Quads+1
				elseif #noterow == 3 then
					Hands = Hands+1
				elseif #noterow == 2 then
					Jumps = Jumps+1
				end
				
				for i,note in ipairs(noterow) do
					if note.type ~= NOTE_TYPE_MINE then
						if FirstNoteFound ~= 1 then
							FirstNoteFound = 1
							FirstNoteTime = rowTime
						end
						TotalSteps = TotalSteps+1
						LastNoteTime = rowTime
						StepTable[TotalSteps] = note.col
						TimeTable[TotalSteps] = LastNoteTime
						
						-- Get number of Steps in each meassure
						if math.max(math.ceil((LastNoteTime - FirstNoteTime) / RegionTime),1) > CurrentMeassure then
							CurrentMeassure = CurrentMeassure+1
							StepsInMeassure[CurrentMeassure] = 0
						end
						StepsInMeassure[CurrentMeassure] = StepsInMeassure[CurrentMeassure]+1
					end
				end
			end
			
			-- Compensate number of jumps, hands and quads
			Jumps = Jumps - Quads - Hands
			Hands = Hands - Quads
			local TotalAllJumps = Jumps + Quads + Hands
			
			-- Get Density of each meassure
			for n = 1,#StepsInMeassure do
				MeassureNPS[n] = StepsInMeassure[n]/RegionTime
			end
			for n = 1,9 do
				MeassureNPSprctles[n] = percentile(MeassureNPS,n/10)
			end
			
			-- Get number of tech patterns
			local StepsString = table.concat(StepTable)
			local CrossOvers = select(2, StepsString:gsub("1242", ""))
			CrossOvers = CrossOvers + select(2, StepsString:gsub("4212", ""))
			CrossOvers = CrossOvers + select(2, StepsString:gsub("1343", ""))
			CrossOvers = CrossOvers + select(2, StepsString:gsub("4313", ""))
			
			local AfroNovers = select(2, StepsString:gsub("1241", ""))
			AfroNovers = AfroNovers + select(2, StepsString:gsub("4214", ""))
			AfroNovers = AfroNovers + select(2, StepsString:gsub("1341", ""))
			AfroNovers = AfroNovers + select(2, StepsString:gsub("4314", ""))
			
			local FootSwitches = select(2, StepsString:gsub("1334", ""))
			FootSwitches = FootSwitches + select(2, StepsString:gsub("4331", ""))
			FootSwitches = FootSwitches + select(2, StepsString:gsub("1224", ""))
			FootSwitches = FootSwitches + select(2, StepsString:gsub("4221", ""))
			
			local TotalTech = CrossOvers + AfroNovers + FootSwitches
			local SongLength = LastNoteTime - FirstNoteTime			
			local AvgNPS = TotalSteps/(LastNoteTime-FirstNoteTime)			-- Get the average NPS of the chart.
			
			file_data:write(chart:getMeter() .. "," .. SongLength .. "," .. AvgNPS .. "," .. TotalSteps .. "," .. CrossOvers .. ","  .. AfroNovers .. ","  .. FootSwitches .. ","  .. TotalTech .. ","  .. Jumps .. ","  .. Hands .. ","  .. Quads .. "," .. TotalAllJumps .. "," .. MeassureNPSprctles[1] .. ","  .. MeassureNPSprctles[2] .. ","  .. MeassureNPSprctles[3] .. ","  .. MeassureNPSprctles[4] .. ","  .. MeassureNPSprctles[5] .. ","  .. MeassureNPSprctles[6] .. ","  .. MeassureNPSprctles[7] .. ","  .. MeassureNPSprctles[8] .. ","  .. MeassureNPSprctles[9] .. "\n")
			file_titles:write(getSongTitle() .. "\n")
		end
	end
end

function processPack()
	showInfo("processing pack:" .. packDir)
	for i, song in ipairs(getSubdirectories(packDir, false)) do
		openSimfile(packDir .. "/" .. song)
		processSong()
	end
	showInfo("processing done!")
end