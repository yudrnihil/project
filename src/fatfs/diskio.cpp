/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2014        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h"		/* FatFs lower layer API */
//#include "usbdisk.h"	/* Example: Header file of existing USB MSD control module */
//#include "atadrive.h"	/* Example: Header file of existing ATA harddisk control module */
#include "../SDCard.h"		/* Example: Header file of existing MMC/SDC contorl module */

/* Definitions of physical drive number for each drive */
//#define ATA		0	/* Example: Map ATA harddisk to physical drive 0 */
//#define MMC		1	/* Example: Map MMC/SD card to physical drive 1 */
//#define USB		2	/* Example: Map USB MSD to physical drive 2 */

#define SD_CARD 0
#define FLASH_SECTOR_SIZE 512
u16 FLASH_SECTOR_COUNT=2048*12;
#define FLASH_BLOCK_SIZE 8;

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	return 0;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat;
	uint8_t result;

	switch (pdrv) {
		case SD_CARD :
			result = SD_Init();
			break;
		default:
			result = 1;
	}
	if(result){
		return STA_NOINIT;
	}
	else return 0;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address in LBA */
	UINT count		/* Number of sectors to read */
)
{
	DRESULT res;
	uint8_t result;

	if(!count){
		return RES_PARERR;
	}
	switch (pdrv) {
		case SD_CARD:
			result = SD_ReadDisk(buff, sector, count);
			while(result){
				SD_Init();
				result = SD_ReadDisk(buff, sector, count);
			}
			break;
		default:
			result = 1;
	}
	if (result == 0x00){
		return RES_OK;
	}
	else{
		return RES_ERROR;
	}
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT res;
	uint8_t result;

	if(!count){
		return RES_PARERR;
	}

	switch (pdrv) {
		case SD_CARD:
			result = SD_WriteDisk((u8*)buff,sector,count);
			while(result){
				SD_Init();
				result = SD_WriteDisk((u8*)buff, sector, count);
			}
			break;
		default:
			result = 1;
	}

	if (result == 0x00){
		return RES_OK;
	}
	else{
		return RES_ERROR;
	}
}
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

#if _USE_IOCTL
DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;
	int result;

	if(pdrv == SD_CARD){
		switch(cmd){
			case CTRL_SYNC:
				res = RES_OK;
				break;
			case GET_SECTOR_SIZE:
				*(DWORD*)buff = 512;
				res = RES_OK;
				break;
			case GET_BLOCK_SIZE:
				*(WORD*)buff = SDCardInfo.CardBlockSize;
				res = RES_OK;
				break;
			case GET_SECTOR_COUNT:
				*(DWORD*)buff = SDCardInfo.CardCapacity/512;
				res = RES_OK;
				break;
			default:
				res = RES_PARERR;
				break;
		}
	}
	else res = RES_ERROR;
	return res;
}
#endif

DWORD get_fattime(void){
	return 0;
}

void* ff_memalloc(UINT size){
	return new u8[size];
}

void ff_memfree(void* ptr){
	delete [] (char*)ptr;
}
