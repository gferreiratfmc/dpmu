
MEMORY
{
   /* BEGIN is used for the "boot to Flash" bootloader mode   */
   BEGIN            : origin = 0x080000, length = 0x000002
   BOOT_RSVD        : origin = 0x000002, length = 0x0001AF     /* Part of M0, BOOT rom will use this for stack */
   RAMM0            : origin = 0x0001B1, length = 0x00024F
   RAMM1            : origin = 0x000400, length = 0x0003F8     /* on-chip RAM block M1 */
// RAMM1_RSVD       : origin = 0x0007F8, length = 0x000008     /* Reserve and do not use for code as per the errata advisory "Memory: Prefetching Beyond Valid Memory" */
   RAMD0            : origin = 0x00C000, length = 0x000800
   RAMD1            : origin = 0x00C800, length = 0x000800
   RAMLS03           : origin = 0x008000, length = 0x002000
   RAMLS4           : origin = 0x00A000, length = 0x000800
   RAMLS5           : origin = 0x00A800, length = 0x000800
   RAMLS6           : origin = 0x00B000, length = 0x000800
   RAMLS7           : origin = 0x00B800, length = 0x000800
   RAMGS0           : origin = 0x00D000, length = 0x001000
   RAMGS1           : origin = 0x00E000, length = 0x001000
   RAMGS2           : origin = 0x00F000, length = 0x001000
   RAMGS3           : origin = 0x010000, length = 0x001000
   RAMGS4           : origin = 0x011000, length = 0x001000
   RAMGS5           : origin = 0x012000, length = 0x001000
   RAMGS6           : origin = 0x013000, length = 0x001000
   RAMGS7           : origin = 0x014000, length = 0x001000
   RAMGS8           : origin = 0x015000, length = 0x001000
   RAMGS9           : origin = 0x016000, length = 0x001000
   RAMGS10          : origin = 0x017000, length = 0x001000
   RAMGS11          : origin = 0x018000, length = 0x001000
   RAMGS12          : origin = 0x019000, length = 0x001000
   RAMGS13          : origin = 0x01A000, length = 0x001000
   RAMGS14          : origin = 0x01B000, length = 0x001000
   RAMGS15          : origin = 0x01C000, length = 0x000FF8
//   RAMGS15_RSVD     : origin = 0x01CFF8, length = 0x000008     /* Reserve and do not use for code as per the errata advisory "Memory: Prefetching Beyond Valid Memory" */

   /* Flash sectors */
   FLASH0           : origin = 0x080002, length = 0x001FFE  /* on-chip Flash */
   FLASH1           : origin = 0x082000, length = 0x002000  /* on-chip Flash */

   CPU1TOCPU2RAM   : origin = 0x03A000, length = 0x000800
   CPU2TOCPU1RAM   : origin = 0x03B000, length = 0x000800
   CPUTOCMRAM      : origin = 0x039000, length = 0x000800
   CMTOCPURAM      : origin = 0x038000, length = 0x000800

   CANA_MSG_RAM     : origin = 0x049000, length = 0x000800
   CANB_MSG_RAM     : origin = 0x04B000, length = 0x000800

   RESET            : origin = 0x3FFFC0, length = 0x000002

   /* The following are definitions for BOOT mode */
   Z1_OTP_BOOTPINCONFIG : origin = 0x078008, length = 00000004
   Z1_OTP_BOOTDEF_LOW   : origin = 0x07800C, length = 00000002
   Z1_OTP_BOOTDEF_HIGH  : origin = 0x07800E, length = 00000002
}

SECTIONS
{
   codestart           : > BEGIN, ALIGN(8)
   .text               : >> FLASH0 | FLASH1, ALIGN(8)
   .cinit              : > FLASH1, ALIGN(8)
   .switch             : > FLASH1, ALIGN(8)
   .reset              : > RESET, TYPE = DSECT /* not used, */
   .stack              : > RAMM1

   .init_array      : > FLASH1, ALIGN(8)
   .bss             : > RAMLS5
   .bss:output      : > RAMLS4
   .bss:cio         : > RAMLS5
   .data            : > RAMLS5
   .sysmem          : > RAMLS5
   /* Initalized sections go in Flash */
   .const           : > FLASH1, ALIGN(8)

   ramgs0 : > RAMGS0, type=NOINIT
   ramgs1 : > RAMGS1, type=NOINIT

   command : > RAMM0, type=NOINIT

   MSGRAM_CPU1_TO_CPU2 : > CPU1TOCPU2RAM, type=NOINIT
   MSGRAM_CPU2_TO_CPU1 : > CPU2TOCPU1RAM, type=NOINIT
   MSGRAM_CPU_TO_CM    : > CPUTOCMRAM, type=NOINIT
   MSGRAM_CM_TO_CPU    : > CMTOCPURAM, type=NOINIT

   /* The following are definitions for BOOT mode */
   z1_otpBootPinConfig : > Z1_OTP_BOOTPINCONFIG
   z1_otpBootDef_Low   : > Z1_OTP_BOOTDEF_LOW
   z1_otpBootDef_High  : > Z1_OTP_BOOTDEF_HIGH

   DataBufferSection : > RAMM0, ALIGN(8)

   GROUP
   {
	   .TI.ramfunc
	   { -l F2838x_C28x_FlashAPI.lib}

   } LOAD = FLASH1,
	 RUN = RAMLS03,
	 LOAD_START(RamfuncsLoadStart),
	 LOAD_SIZE(RamfuncsLoadSize),
	 LOAD_END(RamfuncsLoadEnd),
	 RUN_START(RamfuncsRunStart),
	 RUN_SIZE(RamfuncsRunSize),
	 RUN_END(RamfuncsRunEnd),
	 ALIGN(8)

}

/*
//===========================================================================
// End of file.
//===========================================================================
*/
