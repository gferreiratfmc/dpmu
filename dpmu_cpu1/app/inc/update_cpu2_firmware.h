/*
 * update_cpu2_firmware.h
 *
 *  Created on: 8 de mar de 2024
 *      Author: Gustavo Luiz Ferreira gustavo.ferreira2@technipfmc.com
 */



#ifndef APP_INC_UPDATE_CPU2_FIRMWARE_H_
#define APP_INC_UPDATE_CPU2_FIRMWARE_H_

#include "serial.h"

void executeCPU2FirmwareUpdate(struct Serial *cli_serial );
void startCPU2FirmwareUpdate();
bool cpu2FirmwareReady();


#endif /* APP_INC_UPDATE_CPU2_FIRMWARE_H_ */
