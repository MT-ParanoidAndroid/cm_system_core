/*
 *  linux/fs/proc/mlog.h
 *
 *  Copyright (C) 2010  by FIHTDC HenryMCWang
 *
 */
 
#define MLOG_ROOT				"/data/efslog/OEMDBG_LOG" //SW2-5-1-MP-DbgCfgTool-07*
#define OEMDBG_0_LOG_PATH		MLOG_ROOT"/FIH_modem_oemdbg_0_log.txt"
#define OEMDBG_1_LOG_PATH		MLOG_ROOT"/FIH_modem_oemdbg_1_log.txt"
#define EXCEPTION_LOG_PATH		MLOG_ROOT"/FIH_modem_err_data_index01_log00.txt"
#define F3_CRS_LOG_PATH			MLOG_ROOT"/FIH_modem_err_f3_index01.crs"
#define F3_F3_LOG_PATH			MLOG_ROOT"/FIH_modem_err_f3_index01.f3"
#define F3_ID_LOG_PATH			MLOG_ROOT"/FIH_modem_err_f3_index01.id"
#define SMEM_LOG_SLEPP_VOTERS	MLOG_ROOT"/FIH_log_sleep_voters_01.bin"
#define SMEM_LOG_EVENTS			MLOG_ROOT"/FIH_log_events_01.bin"
#define SMEM_LOG_INDEX			MLOG_ROOT"/FIH_log_index_01.bin"
#define SMEM_LOG_POWER_EVENTS	MLOG_ROOT"/FIH_power_events_01.bin"
#define SMEM_LOG_POWER_INDEX	MLOG_ROOT"/FIH_power_index_01.bin"
#define SMEM_LOG_STATIC_EVENTS	MLOG_ROOT"/FIH_smem_log_static_events_01.bin"
#define SMEM_LOG_STATIC_INDEX	MLOG_ROOT"/FIH_smem_log_static_index_01.bin"

typedef struct
{
	unsigned magic_number:16;
	unsigned blk0_avail :1;
	unsigned blk1_avail :1;
	unsigned current_blk_idx:1;
	unsigned log_status:2; 
	//0:only save blk0; 
	//1:only save blk1; 
	//2:both save blk0 & blk1; blk0 first
	//3:both save blk0 & blk1; blk1 first
	unsigned smem_log_flag:16;
	unsigned powerOnCause:32;
}Oemdbg_Hdr;

typedef enum
{
	  OEMDBG_0_LOG = 0,
	  OEMDBG_1_LOG,
	  OEMDBG_EXCEPTION_LOG,
	  OEMDBG_F3_CRS_LOG,
	  OEMDBG_F3_F3_LOG,
	  OEMDBG_F3_ID_LOG,
	  OEMDBG_SMEM_SLEEP_VOTE_LOG,
	  OEMDBG_SMEM_LOG_EVENTS_LOG,
	  OEMDBG_SMEM_LOG_IDX_LOG,
	  OEMDBG_SMEM_POWER_LOG_EVENTS_LOG,  
	  OEMDBG_SMEM_POWER_LOG_IDX_LOG,
	  OEMDBG_SMEM_STATIC_LOG_EVENTS_LOG,  
	  OEMDBG_SMEM_STATIC_LOG_IDX_LOG,
	  OEMDBG_MAX_LOG_NUM
}oemdbg_crash_log_type;
//--------------------------------------------------------------------------------------------------------
typedef struct
{
	unsigned type:32;
	unsigned length:32;
}LogBlock_Hdr;

//Define the size for every block==================
//Total 1+50+50+20+50+140+1+1+1+1+40+1+3=399
#define OEMDBG_HDR_SIZE		0X400	//1KB
#define OEMDBG_0_SIZE		0XC800    //50KB
#define OEMDBG_1_SIZE		0XC800    //50KB
#define EXCEPTION_LOG_SIZE	0X5000    //20KB
#define F3_CRS_SIZE			0XC800    //50KB
#define F3_F3_SIZE				0X23000  //140KB
#define F3_ID_SIZE				0X400	//1KB
#define SLEEP_VECTOR_SIZE		0X400	//1KB
#define EVT_SIZE				0XA000	//40KB
#define IDX_SIZE				0X400	//1KB
#define POWER_EVT_SIZE		0XA000	//40KB
#define POWER_IDX_SIZE		0X400	//1KB
#define STATIC_EVT_SIZE		0XC00	//3KB
#define STATIC_IDX_SIZE		0X400	//1KB

//Define the address for every block===========================================
#define OEMDBG_HDR_ADDR		0
#define OEMDBG_0_ADDR			OEMDBG_HDR_ADDR+OEMDBG_HDR_SIZE    //50KB
#define OEMDBG_1_ADDR			OEMDBG_0_ADDR+OEMDBG_0_SIZE    //50KB
#define EXCEPTION_LOG_ADDR		OEMDBG_1_ADDR+OEMDBG_1_SIZE    //20KB
#define F3_CRS_ADDR				EXCEPTION_LOG_ADDR+EXCEPTION_LOG_SIZE    //50KB
#define F3_F3_ADDR				F3_CRS_ADDR+F3_CRS_SIZE    //140KB
#define F3_ID_ADDR				F3_F3_ADDR+F3_F3_SIZE    //1KB
#define SLEEP_VECTOR_ADDR		F3_ID_ADDR+F3_ID_SIZE    //1KB
#define EVT_ADDR					SLEEP_VECTOR_ADDR+SLEEP_VECTOR_SIZE    //40KB
#define IDX_ADDR					EVT_ADDR+EVT_SIZE    //1KB
#define POWER_EVT_ADDR			IDX_ADDR+IDX_SIZE    //1KB
#define POWER_IDX_ADDR			POWER_EVT_ADDR+POWER_EVT_SIZE    //40KB
#define STATIC_EVT_ADDR			POWER_IDX_ADDR+POWER_IDX_SIZE    //1KB
#define STATIC_IDX_ADDR			STATIC_EVT_ADDR+STATIC_EVT_SIZE    //3KB

#define OEMDBG_0_LOG_PATH					MLOG_ROOT"/FIH_modem_oemdbg_0_log.txt"
#define OEMDBG_1_LOG_PATH					MLOG_ROOT"/FIH_modem_oemdbg_1_log.txt"
#define EXCEPTION_LOG_PATH					MLOG_ROOT"/FIH_modem_err_data_index01_log00.txt"
#define F3_CRS_LOG_PATH						MLOG_ROOT"/FIH_modem_err_f3_index01.crs"
#define F3_F3_LOG_PATH						MLOG_ROOT"/FIH_modem_err_f3_index01.f3"
#define F3_ID_LOG_PATH						MLOG_ROOT"/FIH_modem_err_f3_index01.id"
#define SMEM_LOG_SLEPP_VOTERS_PATH			MLOG_ROOT"/FIH_log_sleep_voters_01.bin"
#define SMEM_LOG_EVENTS_PATH				MLOG_ROOT"/FIH_log_events_01.bin"
#define SMEM_LOG_INDEX_PATH					MLOG_ROOT"/FIH_log_index_01.bin"
#define SMEM_LOG_POWER_EVENTS_PATH		MLOG_ROOT"/FIH_power_events_01.bin"
#define SMEM_LOG_POWER_INDEX_PATH			MLOG_ROOT"/FIH_power_index_01.bin"
#define SMEM_LOG_STATIC_EVENTS_PATH			MLOG_ROOT"/FIH_smem_log_static_events_01.bin"
#define SMEM_LOG_STATIC_INDEX_PATH			MLOG_ROOT"/FIH_smem_log_static_index_01.bin"
