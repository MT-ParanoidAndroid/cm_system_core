/*
 *  linux/fs/proc/mlog.c
 *
 *  Copyright (C) 2010  by FIHTDC HenryMCWang
 *
 */

#include <stdio.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdarg.h>
#include "mlog.h"

//Div2-SW2-BSP, APR abnormal power on log, HenryMCWang, +
#define APR_COUNTER_DEBUG   1
#define ABNORMAL_POWER_ON_LOG_ROOT  "/data/fih_statistics"
#define ABNORMAL_POWER_ON_LOG_STAT  ABNORMAL_POWER_ON_LOG_ROOT"/abnormal_power_on.log"
#define KERNEL_PANIC_COUNT_TEXT     "Kernel_Panic_Count:"
#define MODEM_REBOOT_COUNT_TEXT     "Modem_Panic_Count:"
#define POWER_CUT_COUNT_TEXT        "Power_Cut_Count:"
#define UNKNOWN_RESET_COUNT_TEXT    "Unknown_Reset_Count:"

#define POWER_CUT_BIT           0x01000000UL
#define MODEM_FATAL_ERROR_BIT   0x10000000UL
#define KERNEL_PANIC_BIT        0x20000000UL
#define UNKNOWN_RESET_BIT       0x40000000UL

static unsigned int power_on_cause = 0;
//Div2-SW2-BSP, APR abnormal power on log, HenryMCWang, -

void _LOGK(const char *fmt, ...)
{
	char buf[128];
	int fd;
	fd = open( "/dev/kmsg", O_WRONLY );
	
	if(fd >=0)
	{
		int len;
		va_list ap;
		va_start(ap, fmt);
	
		vsnprintf(buf, sizeof(buf), fmt, ap);
		len = strlen(buf);
		write(fd, buf, len);        
		close(fd);    
	}
}

static int
unix_read(int  fd, void*  buff, int  len)
{
	int  ret;
	do { ret = read(fd, buff, len); } while (ret < 0 && errno == EINTR);
	return ret;
}

static int
unix_write(int  fd, const void*  buff, int  len)
{
	int  ret;
	do { ret = write(fd, buff, len); } while (ret < 0 && errno == EINTR);
	return ret;
}

//Div2-SW2-BSP, APR abnormal power on log, HenryMCWang, +
#define MAX_POWERONCAUSE_LENGTH     16L
static void
do_log_poweroncause(void)
{
    char  buff[MAX_POWERONCAUSE_LENGTH];
    int   fd, ret, len;

    fd = open("/proc/poweroncause",O_RDONLY);
    if (fd >= 0) {
        int  ret;
        memset(buff, 0, sizeof(buff));
        ret = unix_read(fd, buff, (MAX_POWERONCAUSE_LENGTH - 1));
        sscanf(buff, "%x", &power_on_cause);
        close(fd);
    }
}

static void
update_exception_counter(const char*  path)
{
    char buf[512];
    int exception_stat_fd;
    unsigned int kernel_panic_count = 0;
    unsigned int modem_panic_count = 0;
    unsigned int power_cut_count = 0;
    unsigned int unknown_reset_count = 0;
    int len = 0;
    int ll = 0;
#if APR_COUNTER_DEBUG
    int fd;
    char buf2[512];
    fd = open("/dev/kmsg", O_RDWR, 0755);
#endif

#if APR_COUNTER_DEBUG
        memset(buf2, 0, sizeof(buf2));
#endif
    memset(buf, 0, sizeof(buf));
    /* create log file if needed */
    exception_stat_fd = open(path, O_RDWR, 0755);
    if (exception_stat_fd < 0) 
    {
        //File not existed
        exception_stat_fd = open(path,O_RDWR|O_CREAT|O_TRUNC,0755);
        len = sprintf(buf, KERNEL_PANIC_COUNT_TEXT " %u\n", kernel_panic_count);
        len += sprintf(&buf[len], MODEM_REBOOT_COUNT_TEXT " %u\n", modem_panic_count);
        len += sprintf(&buf[len], POWER_CUT_COUNT_TEXT " %u\n", power_cut_count);
        len += sprintf(&buf[len], UNKNOWN_RESET_COUNT_TEXT" %u\n", unknown_reset_count);
        if(write(exception_stat_fd, buf, len) == 0)
        {
        //error msg
            #if APR_COUNTER_DEBUG
            sprintf(buf2, "Failed to write %s\n", path);
            write(fd, buf2, strlen(buf2));
            #endif
        }
    }
    else    //File already existed
    {
        ll = read(exception_stat_fd, buf, sizeof(buf));
        if ( ll == 0)
        {
            //err msg
            #if APR_COUNTER_DEBUG
            sprintf(buf2, "Failed to read %s\n", path);
            write(fd, buf2, strlen(buf2));
            #endif
        }
        else
        {
            sscanf(buf, "%*s %u %*s %u %*s %u %*s %u",
               &kernel_panic_count,
               &modem_panic_count,
               &power_cut_count,
               &unknown_reset_count);

            lseek(exception_stat_fd, 0L, SEEK_SET); //seek to the beginning

            if (power_on_cause & POWER_CUT_BIT)
            {
                power_cut_count++;
            }
            if (power_on_cause & MODEM_FATAL_ERROR_BIT)
            {
                modem_panic_count++;
            }
            if (power_on_cause & KERNEL_PANIC_BIT)
            {
                kernel_panic_count++;
            }
            if (power_on_cause & UNKNOWN_RESET_BIT)
            {
                unknown_reset_count++;
            }

            len = sprintf(buf, KERNEL_PANIC_COUNT_TEXT " %u\n", kernel_panic_count);
            len += sprintf(&buf[len], MODEM_REBOOT_COUNT_TEXT " %u\n", modem_panic_count);
            len += sprintf(&buf[len], POWER_CUT_COUNT_TEXT " %u\n", power_cut_count);
            len += sprintf(&buf[len], UNKNOWN_RESET_COUNT_TEXT" %u\n", unknown_reset_count);

            if(write(exception_stat_fd, buf, len) == 0)
            {
            //error msg
            #if APR_COUNTER_DEBUG
            sprintf(buf2, "Failed to write %s\n", path);
            write(fd, buf2, strlen(buf2));
            #endif
            }
        }

    }
}

void do_reboot_count_file()
{
    int ret;

    do {ret=mkdir(ABNORMAL_POWER_ON_LOG_ROOT,0777);}while (ret < 0 && errno == EINTR);
    update_exception_counter(ABNORMAL_POWER_ON_LOG_STAT);
}
//Div2-SW2-BSP, APR abnormal power on log, HenryMCWang, -

static int
unix_ioctl(int  fd, unsigned int cmd, unsigned long arg)
{
	int  ret;
	ret = ioctl(fd, cmd, arg);
	return ret;
}


static void
read_buffer(void * buff, int len)
{
	int fd = 0;
	
	fd = open("/proc/mlog", O_RDONLY);
	if (fd >= 0)
	{
		int  ret;
		ret = unix_read(fd, buff, len);
		close(fd);
	}
}

static void
set_offset(unsigned long offset)
{
	int fd = 0;
	int ret;
	
	// for special reason, we set offset + 2
	offset += 3;
	
	fd = open("/proc/mlog", O_RDONLY);
	if (fd >= 0)
	{
		ret = unix_ioctl(fd, 0, offset);
		close(fd);
	}
}

static void
set_blk_avail(int dbg_region)
{
	int fd ;
	int ret;
	
	fd = open("/proc/mlog", O_RDONLY);
	if (fd >= 0)
	{
		if(dbg_region == OEMDBG_0_LOG)
		{
			ret = unix_ioctl(fd, 0, 0);
		}
		else if(dbg_region == OEMDBG_1_LOG)
		{
			ret = unix_ioctl(fd, 0, 1);
		}
			
		close(fd);
	}
}

static void
set_smem_log_flag(void)
{
	int fd ;
	int ret;
	
	fd = open("/proc/mlog", O_RDONLY);
	if (fd >= 0)
		ret = unix_ioctl(fd, 0, 2);

	close(fd);
}

void do_read_oemdbg_header(void * buff, int size)
{
	set_offset(OEMDBG_HDR_ADDR);
	read_buffer(buff,  size);
}

void do_read_header(void * buff, int size, unsigned long block_addr)
{
	set_offset(block_addr);
	read_buffer(buff, size);
}

void do_read_content(void * buff, int offset, int size, int block_addr)
{
	set_offset(block_addr + sizeof(LogBlock_Hdr) + offset);
	read_buffer(buff, size);
}

void do_set_oemdbg_blk_avail(int dbg_region)
{
	set_blk_avail(dbg_region);
}

void do_write_mlog_file(int block, int file_length)
{
	int fd = 0;
	char buff[1024];
	int offset = 0;
	int buffer_read_len = 0;
	char * file_path;
	unsigned long block_addr;
	
	switch(block)
	{
		case OEMDBG_0_LOG:
			file_path = OEMDBG_0_LOG_PATH;
			block_addr = OEMDBG_0_ADDR;
			fd = open(file_path,O_RDWR|O_CREAT|O_TRUNC,0755);
			break;
		case OEMDBG_1_LOG:
			file_path = OEMDBG_1_LOG_PATH;
			block_addr = OEMDBG_1_ADDR;
			fd = open(file_path,O_RDWR|O_CREAT|O_TRUNC,0755);
			break;
		case OEMDBG_EXCEPTION_LOG:
			file_path = EXCEPTION_LOG_PATH;
			block_addr = EXCEPTION_LOG_ADDR;
			fd = open(file_path,O_RDWR|O_CREAT|O_TRUNC,0755);
			break;
		case OEMDBG_F3_CRS_LOG:
			file_path = F3_CRS_LOG_PATH;
			block_addr = F3_CRS_ADDR;
			fd = open(file_path,O_RDWR|O_CREAT|O_TRUNC,0755);
			break;
		case OEMDBG_F3_F3_LOG:
			file_path = F3_F3_LOG_PATH;
			block_addr = F3_F3_ADDR;
			fd = open(file_path,O_RDWR|O_CREAT|O_TRUNC,0755);
			break;
		case OEMDBG_F3_ID_LOG:
			file_path = F3_ID_LOG_PATH;
			block_addr = F3_ID_ADDR;
			fd = open(file_path,O_RDWR|O_CREAT|O_TRUNC,0755);
			break;
		case OEMDBG_SMEM_SLEEP_VOTE_LOG:
			file_path = SMEM_LOG_SLEPP_VOTERS_PATH;
			block_addr = SLEEP_VECTOR_ADDR;
			fd = open(file_path,O_RDWR|O_CREAT|O_TRUNC,0755);
			break;
		case OEMDBG_SMEM_LOG_EVENTS_LOG:
			file_path = SMEM_LOG_EVENTS_PATH;
			block_addr = EVT_ADDR;
			fd = open(file_path,O_RDWR|O_CREAT|O_TRUNC,0755);
			break;
		case OEMDBG_SMEM_LOG_IDX_LOG:
			file_path = SMEM_LOG_INDEX_PATH;
			block_addr = IDX_ADDR;
			fd = open(file_path,O_RDWR|O_CREAT|O_TRUNC,0755);
			break;
		case OEMDBG_SMEM_POWER_LOG_EVENTS_LOG:
			file_path = SMEM_LOG_POWER_EVENTS_PATH;
			block_addr = POWER_EVT_ADDR;
			fd = open(file_path,O_RDWR|O_CREAT|O_TRUNC,0755);
			break;
		case OEMDBG_SMEM_POWER_LOG_IDX_LOG:
			file_path = SMEM_LOG_POWER_INDEX_PATH;
			block_addr = POWER_IDX_ADDR;
			fd = open(file_path,O_RDWR|O_CREAT|O_TRUNC,0755);
			break;
		case OEMDBG_SMEM_STATIC_LOG_EVENTS_LOG:
			file_path = SMEM_LOG_STATIC_EVENTS_PATH;
			block_addr = STATIC_EVT_ADDR;
			fd = open(file_path,O_RDWR|O_CREAT|O_TRUNC,0755);
			break;
		case OEMDBG_SMEM_STATIC_LOG_IDX_LOG:
			file_path = SMEM_LOG_STATIC_INDEX_PATH;
			block_addr = STATIC_IDX_ADDR;
			fd = open(file_path,O_RDWR|O_CREAT|O_TRUNC,0755);
			break;
		default :
			_LOGK("[mlog] Unknown block(%d)\n", block);
	}

	if(fd <0)
	{
		_LOGK("[mlog] Open %s fail\n", file_path);
		return ;
	}

	if (fd >= 0)
	{
		for (;;)
		{
			if(file_length < sizeof(buff) + offset)
			{
				if(file_length < sizeof(buff))
					buffer_read_len = file_length;
				else
					buffer_read_len = file_length - offset;
			}
			else
			{
				buffer_read_len = sizeof(buff);
			}
			
			do_read_content(buff, offset, buffer_read_len, block_addr);

			offset += buffer_read_len;

			unix_write(fd, buff, buffer_read_len);
		
			if(offset >= file_length)
			break;
		}

		close(fd);
	}
}

void do_mlog()
{
	Oemdbg_Hdr p_oemdbg_header;
	LogBlock_Hdr pLogBlock_header;

	_LOGK("[mlog] do_log_poweroncause()\n");
	do_log_poweroncause();
	
	_LOGK("[mlog] do_mlog()\n");
	do_read_oemdbg_header(&p_oemdbg_header, sizeof(Oemdbg_Hdr));
	
	_LOGK("[mlog] p_oemdbg_header.magic_number = %x\n", p_oemdbg_header.magic_number);
	_LOGK("[mlog] p_oemdbg_header.blk0_avail = %x\n", p_oemdbg_header.blk0_avail);
	_LOGK("[mlog] p_oemdbg_header.blk1_avail = %x\n", p_oemdbg_header.blk1_avail);
	_LOGK("[mlog] p_oemdbg_header.current_blk_idx = %x\n", p_oemdbg_header.current_blk_idx);
	_LOGK("[mlog] p_oemdbg_header.log_status = %x\n", p_oemdbg_header.log_status);
	_LOGK("[mlog] p_oemdbg_header.smem_log_flag = %x\n", p_oemdbg_header.smem_log_flag);
	_LOGK("[mlog] p_oemdbg_header.powerOnCause = %x\n", p_oemdbg_header.powerOnCause);
	
	if(p_oemdbg_header.magic_number == 0x1234)
	{
		// No logs
		_LOGK("[mlog] p_oemdbg_header.magic_number == %x, normal boot, do nothing\n", p_oemdbg_header.magic_number);
	}
	else if(p_oemdbg_header.magic_number == 0x5678)
	{
		_LOGK("[mlog] p_oemdbg_header.magic_number == %x, modem error boot,  do fatal error logs\n", p_oemdbg_header.magic_number);
		_LOGK("[mlog] p_oemdbg_header.log_status == %d\n", p_oemdbg_header.log_status);
		
		if(p_oemdbg_header.log_status == 0)
		{
			// store oemdbg_0, set blk0_avail = 1
			do_read_header(&pLogBlock_header, sizeof(LogBlock_Hdr), OEMDBG_0_ADDR);
			_LOGK("[mlog] OEMDBG_0_LOG, type = %x, length = %x\n", pLogBlock_header.type, pLogBlock_header.length);
			if(pLogBlock_header.length > 0 && pLogBlock_header.length < OEMDBG_0_SIZE)
				do_write_mlog_file(OEMDBG_0_LOG, pLogBlock_header.length);
			do_set_oemdbg_blk_avail(OEMDBG_0_LOG);
		}
		else if(p_oemdbg_header.log_status == 1)
		{
			// store oemdbg_1, set blk1_avail = 1
			do_read_header(&pLogBlock_header, sizeof(LogBlock_Hdr), OEMDBG_1_ADDR);
			_LOGK("[mlog] OEMDBG_1_LOG, type = %x, length = %x\n", pLogBlock_header.type, pLogBlock_header.length);
			if(pLogBlock_header.length > 0 && pLogBlock_header.length < OEMDBG_1_SIZE)
				do_write_mlog_file(OEMDBG_1_LOG, pLogBlock_header.length);
			do_set_oemdbg_blk_avail(OEMDBG_1_LOG);
		}
		else if(p_oemdbg_header.log_status == 2)
		{
			// store dbg block 1
			do_read_header(&pLogBlock_header, sizeof(LogBlock_Hdr), OEMDBG_1_ADDR);
			_LOGK("[mlog] OEMDBG_1_LOG, type = %x, length = %x\n", pLogBlock_header.type, pLogBlock_header.length);
			if(pLogBlock_header.length > 0 && pLogBlock_header.length < OEMDBG_1_SIZE)
				do_write_mlog_file(OEMDBG_1_LOG, pLogBlock_header.length);

			// store dbg block 0			
			do_read_header(&pLogBlock_header, sizeof(LogBlock_Hdr), OEMDBG_0_ADDR);
			_LOGK("[mlog] OEMDBG_0_LOG, type = %x, length = %x\n", pLogBlock_header.type, pLogBlock_header.length);
			if(pLogBlock_header.length > 0 && pLogBlock_header.length < OEMDBG_1_SIZE)
				do_write_mlog_file(OEMDBG_0_LOG, pLogBlock_header.length);

			// set blk1_avail = 1, set blk0_avail = 1
			do_set_oemdbg_blk_avail(OEMDBG_1_LOG);
			do_set_oemdbg_blk_avail(OEMDBG_0_LOG);
		}
		else if(p_oemdbg_header.log_status == 3)
		{
			// store dbg block 0			
			do_read_header(&pLogBlock_header, sizeof(LogBlock_Hdr), OEMDBG_0_ADDR);
			_LOGK("[mlog] OEMDBG_0_LOG, type = %x, length = %x\n", pLogBlock_header.type, pLogBlock_header.length);
			if(pLogBlock_header.length > 0 && pLogBlock_header.length < OEMDBG_0_SIZE)
				do_write_mlog_file(OEMDBG_0_LOG, pLogBlock_header.length);

			// store dbg block 1
			do_read_header(&pLogBlock_header, sizeof(LogBlock_Hdr), OEMDBG_1_ADDR);
			_LOGK("[mlog] OEMDBG_1_LOG, type = %x, length = %x\n", pLogBlock_header.type, pLogBlock_header.length);
			if(pLogBlock_header.length > 0 && pLogBlock_header.length < OEMDBG_1_SIZE)
				do_write_mlog_file(OEMDBG_1_LOG, pLogBlock_header.length);

			// set blk0_avail = 1, set blk1_avail = 1
			do_set_oemdbg_blk_avail(OEMDBG_0_LOG);
			do_set_oemdbg_blk_avail(OEMDBG_1_LOG);
		}
		
		
		if(p_oemdbg_header.smem_log_flag == 0x1234)
		{
			_LOGK("[mlog] p_oemdbg_header.smem_log_flag == %x, modem error boot,  save exception & F3 & smem logs\n", p_oemdbg_header.smem_log_flag);
			
			do_read_header(&pLogBlock_header, sizeof(LogBlock_Hdr), EXCEPTION_LOG_ADDR);
			_LOGK("[mlog] OEMDBG_EXCEPTION_LOG, type = %x, length = %x\n", pLogBlock_header.type, pLogBlock_header.length);
			if(pLogBlock_header.length > 0 && pLogBlock_header.length < EXCEPTION_LOG_SIZE)
				do_write_mlog_file(OEMDBG_EXCEPTION_LOG, pLogBlock_header.length);
	
			do_read_header(&pLogBlock_header, sizeof(LogBlock_Hdr), F3_CRS_ADDR);
			_LOGK("[mlog] OEMDBG_F3_CRS_LOG, type = %x, length = %x\n", pLogBlock_header.type, pLogBlock_header.length);
			if(pLogBlock_header.length > 0 && pLogBlock_header.length < F3_CRS_SIZE)
				do_write_mlog_file(OEMDBG_F3_CRS_LOG, pLogBlock_header.length);
			
			do_read_header(&pLogBlock_header, sizeof(LogBlock_Hdr), F3_F3_ADDR);
			_LOGK("[mlog] OEMDBG_F3_F3_LOG, type = %x, length = %x\n", pLogBlock_header.type, pLogBlock_header.length);
			if(pLogBlock_header.length > 0 && pLogBlock_header.length < F3_F3_SIZE)
				do_write_mlog_file(OEMDBG_F3_F3_LOG, pLogBlock_header.length);
			
			do_read_header(&pLogBlock_header, sizeof(LogBlock_Hdr), F3_ID_ADDR);
			_LOGK("[mlog] OEMDBG_F3_ID_LOG, type = %x, length = %x\n", pLogBlock_header.type, pLogBlock_header.length);
			if(pLogBlock_header.length > 0 && pLogBlock_header.length < F3_ID_SIZE)
				do_write_mlog_file(OEMDBG_F3_ID_LOG, pLogBlock_header.length);
			
			do_read_header(&pLogBlock_header, sizeof(LogBlock_Hdr), SLEEP_VECTOR_ADDR);
			_LOGK("[mlog] OEMDBG_SMEM_SLEEP_VOTE_LOG, type = %x, length = %x\n", pLogBlock_header.type, pLogBlock_header.length);
			if(pLogBlock_header.length > 0 && pLogBlock_header.length < SLEEP_VECTOR_SIZE)
				do_write_mlog_file(OEMDBG_SMEM_SLEEP_VOTE_LOG, pLogBlock_header.length);
			
			do_read_header(&pLogBlock_header, sizeof(LogBlock_Hdr), EVT_ADDR);
			_LOGK("[mlog] OEMDBG_SMEM_LOG_EVENTS_LOG, type = %x, length = %x\n", pLogBlock_header.type, pLogBlock_header.length);
			if(pLogBlock_header.length > 0 && pLogBlock_header.length < EVT_SIZE)
				do_write_mlog_file(OEMDBG_SMEM_LOG_EVENTS_LOG, pLogBlock_header.length);
			
			do_read_header(&pLogBlock_header, sizeof(LogBlock_Hdr), IDX_ADDR);
			_LOGK("[mlog] OEMDBG_SMEM_LOG_IDX_LOG, type = %x, length = %x\n", pLogBlock_header.type, pLogBlock_header.length);
			if(pLogBlock_header.length > 0 && pLogBlock_header.length < IDX_SIZE)
				do_write_mlog_file(OEMDBG_SMEM_LOG_IDX_LOG, pLogBlock_header.length);
	
			do_read_header(&pLogBlock_header, sizeof(LogBlock_Hdr), POWER_EVT_ADDR);
			_LOGK("[mlog] OEMDBG_SMEM_POWER_LOG_EVENTS_LOG, type = %x, length = %x\n", pLogBlock_header.type, pLogBlock_header.length);
			if(pLogBlock_header.length > 0 && pLogBlock_header.length < POWER_EVT_SIZE)
				do_write_mlog_file(OEMDBG_SMEM_POWER_LOG_EVENTS_LOG, pLogBlock_header.length);
	
			do_read_header(&pLogBlock_header, sizeof(LogBlock_Hdr), POWER_IDX_ADDR);
			_LOGK("[mlog] OEMDBG_SMEM_POWER_LOG_IDX_LOG, type = %x, length = %x\n", pLogBlock_header.type, pLogBlock_header.length);
			if(pLogBlock_header.length > 0 && pLogBlock_header.length < POWER_IDX_SIZE)
				do_write_mlog_file(OEMDBG_SMEM_POWER_LOG_IDX_LOG, pLogBlock_header.length);
	
			do_read_header(&pLogBlock_header, sizeof(LogBlock_Hdr), STATIC_EVT_ADDR);
			_LOGK("[mlog] OEMDBG_SMEM_STATIC_LOG_EVENTS_LOG, type = %x, length = %x\n", pLogBlock_header.type, pLogBlock_header.length);
			if(pLogBlock_header.length > 0 && pLogBlock_header.length < STATIC_EVT_SIZE)
				do_write_mlog_file(OEMDBG_SMEM_STATIC_LOG_EVENTS_LOG, pLogBlock_header.length);
	
			do_read_header(&pLogBlock_header, sizeof(LogBlock_Hdr), STATIC_IDX_ADDR);
			_LOGK("[mlog] OEMDBG_SMEM_STATIC_LOG_IDX_LOG, type = %x, length = %x\n", pLogBlock_header.type, pLogBlock_header.length);
			if(pLogBlock_header.length > 0 && pLogBlock_header.length < STATIC_IDX_SIZE)
				do_write_mlog_file(OEMDBG_SMEM_STATIC_LOG_IDX_LOG, pLogBlock_header.length);
				
			set_smem_log_flag();
		}
	}
	else
	{
		_LOGK("[mlog] Exception magic number = %d, do nothing\n", p_oemdbg_header.magic_number);
	}
}
