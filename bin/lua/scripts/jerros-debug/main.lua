function main()
	local chart = getChart(1)
	local tempo = chart:getTempo()
	local lastRow = 0
	local FirstNoteTime = 0
	local isFirst = true
	for noterow in chart:getNotesByRow() do
		lastRow = noterow.row
		if isFirst == true then
			FirstNoteTime = tempo:rowToTime(noterow.row)
			isFirst = false
		end
	end
	local LastNoteTime = tempo:rowToTime(lastRow)
	showInfo(FirstNoteTime .. "---" .. LastNoteTime)
end