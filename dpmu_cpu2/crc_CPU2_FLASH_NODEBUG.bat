REM create firmware with CRC

rem change to the current batch directory
rem so the relative paths works independent of the caller
set LOC=%~dp0
cd "%LOC%"

cd ..
cd generate_firmware

set TOOLS_PATH= "%CD%"
set PROJECT_PATH=%LOC%

set GENERATOR=%TOOLS_PATH%\generate_firmware.exe
set OBJCOPY=%TOOLS_PATH%\mingw\objcopy
set TARGET=%PROJECT_PATH%CPU2_FLASH_NODEBUG
set APPL=dpmu_cpu2

set CONFIGBLOCK = 8
rem check path
%OBJCOPY% 
%GENERATOR% -?

rem generate binary
%OBJCOPY% -I ihex -O binary --gap-fill=0xFF "%TARGET%\%APPL%.lsb" "%TARGET%\%APPL%.lsbbin"
%OBJCOPY% -I ihex -O binary --gap-fill=0xFF "%TARGET%\%APPL%.msb" "%TARGET%\%APPL%.msbbin"

rem check for double files and new created one
dir "%TARGET%\*.hex" "%TARGET%\*.bin*" "%TARGET%\*.lsb" "%TARGET%\*.msb"

@echo ""
@echo ""

rem CRC
%GENERATOR% -c 8 -i "%TARGET%\%APPL%.lsbbin" -j "%TARGET%\%APPL%.msbbin" -o "%TARGET%\%APPL%.crc" -D
rem if you do not know what 793 and 65617665 means check cobl_user.c


cd "%TARGET%\.."
dir "%TARGET%"

pause

taskkill /IM python.exe /F

cd "C:\Users\gferreira\OneDrive - Digicorner\git_dpmu_python\a055-can-updater\"

python CANUpdaterEmotasOri.py 2 "C:\Users\gferreira\OneDrive - Digicorner\git_endurance\dpmu\dpmu_cpu2\CPU2_FLASH_NODEBUG\dpmu_cpu2.crc"

pause

cd "C:\Users\gferreira\OneDrive - Digicorner\git_dpmu_python\a055-dpmu-python-can"
start "python.exe" "dpmu_canopenLowVoltage2.py"

cd "%TARGET%\.."