del .\bin\*.log /q
del .\bin\*.ilk /q
del .\bin\*.pdb /q

rmdir .\build\vs\.vs /s /q
rmdir .\build\vs\x64 /s /q

del .\build\vs\*.aps /q
del .\build\vs\*.log /q
del .\build\vs\*.hint /q

rmdir .\lib\freetype\obj\x64 /s /q
del .\lib\freetype\obj\*.* /q

rmdir .\lib\libmad\obj\x64 /s /q
del .\lib\libmad\obj\*.* /q

rmdir .\lib\libvorbis\obj\x64 /s /q
del .\lib\libvorbis\obj /q

rmdir .\lib\lua\obj\x64 /s /q
del .\lib\lua\obj\*.* /q