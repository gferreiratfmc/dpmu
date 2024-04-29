/* main - firmware image creator
 *
 * Copyright (c) 2012 emtas GmbH
 *-------------------------------------------------------------------
 * SVN  $Date: 2016-05-09 11:24:14 +0200 (Mo, 09. Mai 2016) $
 * SVN  $Rev: 13750 $
 * SVN  $Author: ro $
 *-------------------------------------------------------------------
 *
 * $Id: main.c 13750 2016-05-09 09:24:14Z ro $
 * $Date: 2016-05-09 11:24:14 +0200 (Mo, 09. Mai 2016) $
 *
 */

/********************************************************************/
/**
 * \file
 * \brief main entry point
 *
 * Creates CRC and the complete firmware file
 */

/* standard includes
 --------------------------------------------------------------------------*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sys/time.h>
#include <getopt.h>    /* for getopt_long; POSIX standard getopt is in unistd.h */
/* header of project specific types
 ---------------------------------------------------------------------------*/
#include <cobl_type.h>
#include <cobl_crc.h>

/* constant definitions
 ---------------------------------------------------------------------------*/
#define  DEF_CFG_BLK_SIZE		0x100 /* default config block size */

/* local defined data types
 ---------------------------------------------------------------------------*/
/** Config Block */
#pragma pack(2)
typedef struct {
	uint32_t			/* this is a fixed length C99 data type */
		sizeOfApplication; /* 32 bit */
	uint16_t crc; /* 16 bit */
} config_t; 

typedef struct {
	uint32_t			/* this is a fixed length C99 data type */
		sizeOfApplication; /* 32 bit */
	uint16_t crc; /* 16 bit */
	uint16_t reserved1; /* for 4Byte alignment devices */
	uint32_t od1018Vendor; /* 1018:1 Vendor */
	uint32_t od1018Product;	/* 1018:2 productcode */
	uint32_t od1018Version;	/* 1018:3 version */
	uint32_t swVersion;	/* sw version - customer specific */
} configEnhanced_t; 
#pragma pack()


/* list of external used functions, if not in headers
 ---------------------------------------------------------------------------*/

/* list of global defined functions
 ---------------------------------------------------------------------------*/

/* list of local defined functions
 ---------------------------------------------------------------------------*/
static void checkOptions(int argc, char ** argv);
static void printVersion(void);
static void createFile(void);
static void loadBinary(void);
static void loadDSPBinary(void);
static void calcCrc(void);

/* external variables
 ---------------------------------------------------------------------------*/

/* global variables
 ---------------------------------------------------------------------------*/
char ifileName[255]; /**< input file name */
size_t ifileSize; /**< file size */

char i2fileName[255]; /**< input file name */

char ofileName[255]; /**< output file name */
size_t ofileSize; /**< file size */


unsigned char * pBinary;
size_t binarySize;


unsigned char * pConfig; /**< pointer to config block */
size_t configSize; /**< size of config block */
configEnhanced_t tmpConfig; /* for parameter set */

unsigned char fDSP; /**< DSP or not */
unsigned char fRandom; /**< random data in config block */
unsigned char fConfigOnly; /**< output file us only the config block */
unsigned char fEnhancedConfig; /**< more than only crc */

/* local defined variables
 ---------------------------------------------------------------------------*/

void ausgabe(void)
{
	if (errno != 0) {
		printf("Error\n");
	}
}



/***************************************************************************/
/**
 * \brief main entry
 *
 * \params
 *       nothing
 * \results
 *       exit value
 */

int main(int argc, char ** argv)
{
	atexit(ausgabe);

	/* defaults */
	configSize = DEF_CFG_BLK_SIZE;

	checkOptions(argc, argv);

	loadBinary();
	if (fDSP != 0) {
		loadDSPBinary();
	}

	calcCrc();

	createFile();

	return EXIT_SUCCESS;
}


/***************************************************************************/
static void checkOptions(
		int argc,
		char ** argv
	)
{
int c;
unsigned long u32value;
unsigned long reqOptions = 0;

#define OD1018_1  1001
#define OD1018_2  1002
#define OD1018_3  1003
#define SWVERSION 1004
/*
    struct option {
               const char *name;
               int         has_arg;
               int        *flag;
               int         val;
           };
*/
static struct option long_options[] = {
        {"od1018_1",  1, NULL, OD1018_1},
        {"od1018_2",  1, NULL, OD1018_2},
        {"od1018_3",  1, NULL, OD1018_3},
        {"swversion", 1, NULL, SWVERSION},
        {"help",      0, NULL, 'h'},
        {NULL,        0, NULL, 0}
    };
int option_index;

    opterr = 0;

	fDSP = 0;
	fRandom = 0;
	fConfigOnly = 0;

	memset(&tmpConfig, 0xFF, sizeof(tmpConfig));
	fEnhancedConfig = 0;

    /*
     * V .. Version
     * h .. help
     * o .. output file
     * i .. input file
     * j .. 2nd input file (DSP)
	 * D .. DSP (16bit)
	 * R .. fill config block with random data 
	 * C .. output file is only config block
	 *
	 * additional long options
     */
    while ((c = getopt_long (argc, argv, "Vho:i:j:c:DRC",
				long_options, &option_index
			)) != -1) {
      switch (c)
        {
		case OD1018_1:
            if (optarg) {
				u32value = strtoul(optarg, NULL, 0);
			}
			tmpConfig.od1018Vendor = u32value;
			fEnhancedConfig = 1;
			break;
		case OD1018_2:
            if (optarg) {
				u32value = strtoul(optarg, NULL, 0);
			}
			tmpConfig.od1018Product = u32value;
			fEnhancedConfig = 1;
			break;
		case OD1018_3:
            if (optarg) {
				u32value = strtoul(optarg, NULL, 0);
			}
			tmpConfig.od1018Version = u32value;
			fEnhancedConfig = 1;
			break;
		case SWVERSION:
            if (optarg) {
				u32value = strtoul(optarg, NULL, 0);
			}
			tmpConfig.swVersion = u32value;
			fEnhancedConfig = 1;
			break;
		case 0:
            //printf ("option %s", long_options[option_index].name);
            if (optarg) {
				u32value = strtoul(optarg, NULL, 0);
			}
			printf("internal Error - Long Options\n");
			exit(-100);
			break;
        case 'V':
        	printVersion();
        	exit(0);
          break;
        case 'o':
        	strncpy(ofileName, optarg, 255);
        	reqOptions |= 1 <<0;
          break;
        case 'i':
        	strncpy(ifileName, optarg, 255);
        	reqOptions |= 1 << 1;
          break;
        case 'j':
        	strncpy(i2fileName, optarg, 255);
        	reqOptions |= 1 << 2;
          break;
        case 'c':
	  	/* accept different number formats */
			configSize = strtol(optarg, NULL, 0);
          break;
        case 'D':
			fDSP = 1;
          break;
        case 'R':
			fRandom = 1;
		  break;
		case 'C':
			fConfigOnly = 1;
		  break;
		case 'h':
        default:
        	printVersion();
        	printf(" -V            .. print version information\n");
        	printf(" -h            .. this help\n");
        	printf(" -o <filename> .. output file name\n");
        	printf(" -i <filename> .. input file name\n");
        	printf(" -j <filename> .. name of 2nd input file (DSP/MSB)\n");
        	printf(" -c <size>     .. config block size in in dec or hex (0x%0x Bytes)\n",
				(unsigned int)configSize);
        	printf(" -D            .. DSP (size are words)\n");
        	printf(" -R            .. fill config block with random values\n");
        	printf(" -C            .. output config block only\n");
			
			printf("\n");
			printf(" additional long options to set additional appl parameters\n");
			printf("\n");
			printf(" --od1018_1=<0x1018:1> .. CiA VendorID\n");
			printf(" --od1018_2=<0x1018:2> .. ProductCode\n");
			printf(" --od1018_3=<0x1018:3> .. SoftwareVersion (CiA301)\n");
			printf(" --swversion=<version> .. SoftwareVersion (Customer, 32bit)\n");
			
        	exit(-1);
        }
    }

	if (fDSP == 0) {
		if (reqOptions != 0x0003) {
			printf("Error! Required options failed.\n");
			exit(-2);
		}
	} else {
		if (reqOptions != 0x0007) {
			printf("Error! Required options failed.\n");
			exit(-2);
		}
	}

	if (fEnhancedConfig == 0) {
		if (configSize < sizeof(config_t)) {
			printf("Error: Config block size too small(min. %ld)!\n", sizeof(config_t));
			exit(-13);
		}
	} else {
		if (configSize < sizeof(configEnhanced_t)) {
			printf("Error: Config block size too small for additional parameter (min. %ld)!\n", 
															sizeof(configEnhanced_t));
			exit(-14);
		}
	}

	if (fDSP == 0) {
		printf("Config Block size: %ld Bytes\n", configSize);
		printf("Input file: %s\n", ifileName);
	} else {
		printf("Config Block size: %ld Words\n", configSize);
		printf("Input file (LSB): %s\n", ifileName);
		printf("Input file (MSB): %s\n", i2fileName);
	}
	
	if (fEnhancedConfig != 0) {
		printf("0x1018:1 : 0x%08xu\n", tmpConfig.od1018Vendor);
		printf("0x1018:2 : 0x%08xu\n", tmpConfig.od1018Product);
		printf("0x1018:3 : 0x%08xu\n", tmpConfig.od1018Version);
		printf("swversion: 0x%08xu\n", tmpConfig.swVersion);
	}

	printf("Output file: %s\n", ofileName);
}

/***************************************************************************/
static void printVersion(void)
{
	printf("Firmware Image Creator\n");
	printf(" $Date: 2016-05-09 11:24:14 +0200 (Mo, 09. Mai 2016) $\n");
	printf(" $Rev: 13750 $\n");
}

/***************************************************************************/
/*
* calcCrc - calculate CRC checksum
*  
* This function creates the config block, too.
*/
static void calcCrc(void)
{
config_t * pConfigStruct;
configEnhanced_t * pConfigEStruct;

	if (fDSP == 1) {
		configSize *= 2;
	}

	pConfig = malloc(configSize);
	if (pConfig == NULL) {
		printf("Error: malloc() for %ld Bytes\n", configSize);
		exit(-12);
	}

	memset(pConfig, 0xff, configSize);

	if (fRandom == 1) {
	unsigned long int i;
	unsigned char * ptr;

		/*
		* gettimeofday() is obsolete, but mingw dont know clock_gettime.
		*
		{
		struct timespec spec;
		clock_gettime(CLOCK_REALTIME, &spec);
		printf("t %ld\n", spec->tv_nsec);
		}
		*
		*/

		{
		struct timeval  tv;
			gettimeofday(&tv, NULL);
			srand(tv.tv_usec);
		}
		ptr = (unsigned char *)pConfig;

		for (i = 0; i < configSize; i++) {
			ptr[i] = (unsigned char)rand(); /* 0..0x7FFF */
		}
	}

	pConfigStruct = (void*)pConfig;
	if (fDSP == 0) {
		pConfigStruct->sizeOfApplication = binarySize;
	} else {
		pConfigStruct->sizeOfApplication = binarySize / 2;
	}
	pConfigStruct->crc = 0;

	crcInitCalculation();

	pConfigStruct->crc = crcCalculation(pBinary, CRC_START_VALUE, binarySize);	
	printf("Length : 0x%08xu\n", pConfigStruct->sizeOfApplication); 
	printf("CRC sum: 0x%04xu\n", pConfigStruct->crc); 

	if (fEnhancedConfig != 0) {
		pConfigEStruct = (void*)pConfig;

		pConfigEStruct->od1018Vendor = tmpConfig.od1018Vendor;
		pConfigEStruct->od1018Product = tmpConfig.od1018Product;
		pConfigEStruct->od1018Version = tmpConfig.od1018Version;
		pConfigEStruct->swVersion = tmpConfig.swVersion;
	}
	
}


/***************************************************************************/
static void createFile(void)
{
FILE * ofileFD;
size_t writtenSize;

printf("create file %s\n", ofileName);

	ofileFD = fopen(ofileName, "wb+");
	if (ofileFD == NULL) {
		printf("Error opening file for writing!\n");
		exit(-3);
	}

	writtenSize = fwrite(pConfig, 1, configSize, ofileFD );
	if(writtenSize != configSize) {
		printf("Error on writing config data\n");
		exit(-11);
	}


	if (fConfigOnly != 1) {
		writtenSize = fwrite(pBinary, 1, binarySize, ofileFD );

		if(writtenSize != binarySize) {
			printf("Error on writing application data\n");
			exit(-11);
		}
	}
	fclose(ofileFD);
}


/***************************************************************************/
static void loadBinary(void)
{
FILE * ifileFD;
size_t size;
size_t osize;
int ret;
size_t readcount;

	ifileFD = fopen(ifileName, "rb");
	if (ifileFD == NULL) {
		printf("Error opening file (%s) for reading!\n", ifileName);
		exit(-5);
	}

	ret = fseek(ifileFD, 0, SEEK_END);
	if (ret != 0) {
		printf("Error seeking input file!\n");
		exit(-6);
	}

	size = ftell(ifileFD);

	if (size < 1) {
		printf("Error: empty input file!\n");
		exit(-7);
	}

	osize = size;
	if (fDSP != 0) {
		osize *= 2;
	}

	pBinary = malloc(osize);
	if (pBinary == NULL) {
		printf("Error: malloc() for %ld Bytes\n", osize);
		exit(-8);
	}

	binarySize = osize;

	ret = fseek(ifileFD, 0, SEEK_SET);
	if (ret != 0) {
		printf("Error seeking input file!\n");
		exit(-9);
	}

	readcount = 0;
	while (readcount < size) {
		readcount += fread(pBinary, 1, size - readcount, ifileFD);
		if (readcount != size) {
			if (ferror(ifileFD) != 0) {
				printf("Error reading input file (%ld/%ld)\n", readcount, size);
				exit(-10);
			}
			if (feof(ifileFD) != 0) {
				printf("Error reading input file - End Of File (%ld/%ld)\n", readcount, size);
				exit(-10);
			}
		}
	}

	fclose(ifileFD);
}

/*
* loadBinary loaded the LSB in the first bytes of the buffer
* loadDSPBinary load the MSB and correct LSB and MSB
*
*/
static void loadDSPBinary(void)
{
FILE * i2fileFD;
size_t size;
size_t osize;
int ret;
size_t readcount;
unsigned char * pTempBinary;
size_t i; 

	i2fileFD = fopen(i2fileName, "rb");
	if (i2fileFD == NULL) {
		printf("Error opening file (%s) for reading!\n", i2fileName);
		exit(-25);
	}

	ret = fseek(i2fileFD, 0, SEEK_END);
	if (ret != 0) {
		printf("Error seeking input file!\n");
		exit(-26);
	}

	size = ftell(i2fileFD);

	if (size != binarySize/2) {
		printf("Error: wrong size of 2nd input file!\n");
		exit(-27);
	}

	osize = size * 2; /* end size */

	pTempBinary = malloc(size); /* temporary buffer */
	if (pTempBinary == NULL) {
		printf("Error: malloc() for %ld Bytes\n", size);
		exit(-28);
	}

	ret = fseek(i2fileFD, 0, SEEK_SET);
	if (ret != 0) {
		printf("Error seeking 2nd input file!\n");
		exit(-29);
	}

	readcount = 0;
	while (readcount < size) {
		readcount += fread(pTempBinary, 1, size - readcount, i2fileFD);
		if (readcount != size) {
			if (ferror(i2fileFD) != 0) {
				printf("Error reading 2nd input file (%ld/%ld)\n", readcount, size);
				exit(-30);
			}
			if (feof(i2fileFD) != 0) {
				printf("Error reading 2nd input file - End Of File (%ld/%ld)\n", readcount, size);
				exit(-30);
			}
		}
	}

	/* mix first and second file
	 * 1..first, 2..second, e..empty
	 * first file - pBinary      |1111eeee|
	 * second file - pTempBinary |2222|
	 * 
	 * create - pBinary          |12121212| (little endian)
	 * start copy from right
	 */
	i = size;
	do {
		i--; /* array index:  size - 1 .. 0 */
		pBinary[i * 2 + 1] = pTempBinary[i];
		pBinary[i * 2] = pBinary[i];
	} while (i > 0);

	free (pTempBinary);
	fclose(i2fileFD);
}

