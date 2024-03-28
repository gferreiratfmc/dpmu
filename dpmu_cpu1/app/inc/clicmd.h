/*
 * clicmd.h
 *
 *  Created on: 5 apr. 2022
 *      Author: us
 */

#ifndef CLICMD_H_
#define CLICMD_H_


struct CliCmd
{
    const char *name;    // command string
    const char *args;    // command arguments
    void (*pfunc)();     // command function
    const char *descr;   // description
};


#endif /* CLICMDS_H_ */
