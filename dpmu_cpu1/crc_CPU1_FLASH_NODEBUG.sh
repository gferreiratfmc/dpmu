#!/bin/bash

#  create firmware with CRC

#  change to the current batch directory
#  so the relative paths works independent of the caller
LOC=${PWD}
cd ${LOC}

cd ..
cd generate_firmware

TOOLS_PATH=${PWD}
PROJECT_PATH=${LOC}

GENERATOR=${TOOLS_PATH}/generate_firmware
OBJCOPY=/usr/bin/objcopy
TARGET=${PROJECT_PATH}/CPU1_FLASH_NODEBUG
APPL=dpmu_cpu1

#  generate binary
${OBJCOPY} -I ihex -O binary --gap-fill=0xFF ${TARGET}/${APPL}.lsb ${TARGET}/${APPL}.lsbbin
${OBJCOPY} -I ihex -O binary --gap-fill=0xFF ${TARGET}/${APPL}.msb ${TARGET}/${APPL}.msbbin

#  check for double files and new created one
dir ${TARGET}/*.hex ${TARGET}/*.bin* ${TARGET}/*.lsb ${TARGET}/*.msb 

echo 
echo

#  CRC
${GENERATOR} -i ${TARGET}/${APPL}.lsbbin -j ${TARGET}/${APPL}.msbbin -o ${TARGET}/${APPL}.crc -D --od1018_1 793  --od1018_2 65617665
#  if you do not know what 793 and 65617665 means check cobl_user.c


cd ${TARGET}/..
dir ${TARGET}

pause

CAN_UPDATER_PATH="/mnt/c/Users/gferreira/OneDrive - Digicorner/git_dpmu_python/a055-can-updater"
CAN_UPDATER_TOOL=CANUpdaterEmotasOri.py

cd "${CAN_UPDATER_PATH}"

echo '''python CANUpdaterEmotasOri.py 1 "C:\Users\gferreira\workspace_endurance_cllc\dpmu_cpu1\CPU1_FLASH_NODEBUG\dpmu_cpu1.crc"'''

python3 ${CAN_UPDATER_TOOL} 1 ${TARGET}/${APPL}

cd ${TARGET}/..