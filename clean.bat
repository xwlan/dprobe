del /S *.ilk
del /S *.exp
del /S *.map
del /S *.bsc

del bin\x86\debug\*.lib
del bin\x64\debug\*.lib
del bin\x86\release\*.lib
del bin\x64\release\*.lib

copy dprobe.db bin\x86\debug\dprobe.db
copy dprobe.db bin\x64\debug\dprobe.db
copy dprobe.db bin\x86\release\dprobe.db
copy dprobe.db bin\x64\release\dprobe.db
