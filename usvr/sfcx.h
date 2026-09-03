#ifndef __xenon_sfcx_h
#define __xenon_sfcx_h

//Registers
#define SFCX_CONFIG				(0x00)
#define SFCX_STATUS 			(0x04)
#define SFCX_COMMAND			(0x08)
#define SFCX_ADDRESS			(0x0C)
#define SFCX_DATA				(0x10)
#define SFCX_LOGICAL 			(0x14)
#define SFCX_PHYSICAL			(0x18)
#define SFCX_DATAPHYADDR		(0x1C)
#define SFCX_SPAREPHYADDR		(0x20)
#define SFCX_MMC_IDENT			(0xFC)

//Commands for Command Register
#define PAGE_BUF_TO_REG			(0x00) 			//Read page buffer to data register
#define REG_TO_PAGE_BUF			(0x01) 			//Write data register to page buffer
#define LOG_PAGE_TO_BUF			(0x02) 			//Read logical page into page buffer
#define PHY_PAGE_TO_BUF			(0x03) 			//Read physical page into page buffer
#define WRITE_PAGE_TO_PHY		(0x04) 			//Write page buffer to physical page
#define BLOCK_ERASE				(0x05) 			//Block Erase
#define DMA_LOG_TO_RAM			(0x06) 			//DMA logical flash to main memory
#define DMA_PHY_TO_RAM			(0x07) 			//DMA physical flash to main memory
#define DMA_RAM_TO_PHY			(0x08) 			//DMA main memory to physical flash
#define UNLOCK_CMD_0			(0x55) 			//Unlock command 0
#define UNLOCK_CMD_1			(0xAA) 			//Unlock command 1

//Config Register bitmasks
#define CONFIG_DBG_MUX_SEL  	(0x7C000000)	//Debug MUX Select
#define CONFIG_DIS_EXT_ER   	(0x2000000)		//Disable External Error (Pre Jasper?)
#define CONFIG_CSR_DLY      	(0x1FE0000)		//Chip Select to Timing Delay
#define CONFIG_ULT_DLY      	(0x1F800)		//Unlocking Timing Delay
#define CONFIG_BYPASS       	(0x400)			//Debug Bypass
#define CONFIG_DMA_LEN      	(0x3C0)			//DMA Length in Pages
#define CONFIG_FLSH_SIZE    	(0x30)			//Flash Size (Pre Jasper)
#define CONFIG_WP_EN        	(0x8)			//Write Protect Enable
#define CONFIG_INT_EN       	(0x4)			//Interrupt Enable
#define CONFIG_ECC_DIS      	(0x2)			//ECC Decode Disable : TODO: make sure this isn't disabled before reads and writes!!
#define CONFIG_SW_RST       	(0x1)			//Software reset

//Status Register bitmasks
#define STATUS_MASTER_ABOR		(0x4000)		// DMA master aborted if not zero
#define STATUS_TARGET_ABOR		(0x2000)		// DMA target aborted if not zero
#define STATUS_ILL_LOG      	(0x800)			//Illegal Logical Access
#define STATUS_PIN_WP_N     	(0x400)			//NAND Not Write Protected
#define STATUS_PIN_BY_N     	(0x200)			//NAND Not Busy
#define STATUS_INT_CP       	(0x100)			//Interrupt
#define STATUS_ADDR_ER      	(0x80)			//Address Alignment Error
#define STATUS_BB_ER        	(0x40)			//Bad Block Error
#define STATUS_RNP_ER       	(0x20)			//Logical Replacement not found
#define STATUS_ECC_ER       	(0x1C)			//ECC Error, 3 bits, need to determine each
#define STATUS_WR_ER        	(0x2)			//Write or Erase Error
#define STATUS_BUSY         	(0x1)			//Busy
#define STATUS_ECC_ERROR		(0x10)			// controller signals unrecoverable ECC error when (!((stat&0x1c) < 0x10))
#define STATUS_DMA_ERROR		(STATUS_MASTER_ABOR|STATUS_TARGET_ABOR)
#define STATUS_ERROR			(STATUS_ILL_LOG|STATUS_ADDR_ER|STATUS_BB_ER|STATUS_RNP_ER|STATUS_ECC_ERROR|STATUS_WR_ER|STATUS_MASTER_ABOR|STATUS_TARGET_ABOR)
#define STSCHK_WRIERA_ERR(sta)	((sta & STATUS_WR_ER) != 0)
#define STSCHK_ECC_ERR(sta)		(!((sta & STATUS_ECC_ER) < 0x10))
#define STSCHK_DMA_ERR(sta)		((sta & (STATUS_DMA_ERROR) != 0)

//Page bitmasks
#define PAGE_VALID          	(0x4000000)
#define PAGE_PID            	(0x3FFFE00)

#define NAND_DEVICE_BASE		(0x7FEAC000)

// status ok or status ecc corrected
//#define SFCX_SUCCESS(status) (((int) status == STATUS_PIN_BY_N) || ((int) status & STATUS_ECC_ER))
// define success as no ecc error and no bad block error
#define SFCX_SUCCESS(status) ((status&STATUS_ERROR)==0)



#endif
