function dumpInfo(args)
	showInfo("DUMPING ARGS:")
	for k, v in pairs(args) do
		showInfo(k .. ": " .. v)
	end
end