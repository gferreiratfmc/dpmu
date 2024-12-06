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
set TARGET=%PROJECT_PATH%CPU1_FLASH_NODEBUG
set APPL=dpmu_cpu1

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
%GENERATOR% -i "%TARGET%\%APPL%.lsbbin" -j "%TARGET%\%APPL%.msbbin" -o "%TARGET%\%APPL%.crc" -D --od1018_1 793  --od1018_2 65617665
rem if you do not know what 793 and 65617665 means check cobl_user.c


cd "%TARGET%\.."
dir "%TARGET%"

pause

taskkill /IM python.exe /F

cd "C:\Users\gferreira\OneDrive - Digicorner\git_dpmu_python\a055-can-updater\"

python CANUpdaterEmotasOri.py 1 "C:\Users\gferreira\OneDrive - Digicorner\git_endurance\dpmu\dpmu_cpu1\CPU1_FLASH_NODEBUG\dpmu_cpu1.crc"

pause

REM cd "C:\Users\gferreira\OneDrive - Digicorner\git_dpmu_python\a055-dpmu-python-can"
REM start "python.exe" "dpmu_canopenLowVoltage2.py"
REM start "python.exe" "dpmu_canopenNormalVoltage.py"
cd "C:\Users\gferreira\OneDrive - Digicorner\dpmu_python_can"
start "python.exe" "dpmu_canopenNormalVoltage.py"

cd "%TARGET%\.."