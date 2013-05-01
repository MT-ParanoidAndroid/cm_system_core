#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <linux/fb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/file.h>

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 108
#endif

#define LOCAL_SOCKET  "/data/fmsocket"

#define FM_VOLUME_LEVEL_MAX     100
#define FM_VOLUME_LEVEL_DEFAULT 50

#define OPTION_MASK_VOL 0x01   // Adjust Volume
#define OPTION_MASK_PWR 0x02   // Enable/Disable Chip
#define OPTION_MASK_FW  0x04   // Loading/unLoading Firmware

struct sockaddr_un 
{
    sa_family_t sun_family;
    char        sun_path[UNIX_PATH_MAX];
};

typedef struct 
{
   int mask;
   int vol;
   int enable;
   int load_fw;

} fmOptions;

static int fm_rfkill_id = -1;
static char *fm_rfkill_state_path = NULL;

int sendCommand(char* cmd)
{
    struct sockaddr_un addr;
    int ret = 0;

    int sockid = socket(PF_UNIX, SOCK_STREAM, 0); // aslo known as PF_LOCAL
    if (SOCKET_ERROR == sockid)
    {
        printf("%s(%d): client- socket failed with %d\n", __FILE__, __LINE__, errno);
        return ret;
    }

    addr.sun_family = PF_UNIX;
    sprintf(addr.sun_path, LOCAL_SOCKET);

    ret = connect(sockid, (struct sockaddr*) &addr, sizeof(addr));
    if (SOCKET_ERROR == ret)
    {
        close(sockid);
        printf("%s(%d): client- connect failed with %d\n", __FILE__, __LINE__, errno);
        return ret;
    }

    ret = send(sockid, cmd, strlen(cmd), 0);
    if (SOCKET_ERROR == ret)
    {
        printf ("%s(%d): client- send(%s) failed with %d\n",__FILE__,__LINE__,cmd,errno);
    }

    close(sockid);
    return ret;
}

static int fm_init_rfkill()
{
    char path[64];
    char buf[16];
    int fd;
    int sz;
    int id;

    for (id = 0; ; id++)
    {
        snprintf(path, sizeof(path), "/sys/class/rfkill/rfkill%d/type", id);

        fd = open(path, O_RDONLY);
        if (fd < 0)
        {
            printf("open(%s) failed: %s (%d)\n", path, strerror(errno), errno);
            return -1;
        }

        sz = read(fd, &buf, sizeof(buf));
        close(fd);

        if (sz >= 2 && memcmp(buf, "fm", 2) == 0)
        {
            fm_rfkill_id = id;
            break;
        }
    }
 
    asprintf(&fm_rfkill_state_path, "/sys/class/rfkill/rfkill%d/state", fm_rfkill_id);

    return 0;
}

int getOptions(int argc, char* argv[], fmOptions* op)
{
    int ret = -1;
    int vol = 0;
    int i = 0;

    for (i = 0; i < argc; ++i)
    {
        if (sscanf(argv[i], "v%d", &vol))
        {
            printf("[fm app] get vol=%d\n", vol);

            op->mask |= OPTION_MASK_VOL;
            op->vol = vol;
            ret = 0;
        }

        if (!strcmp(argv[i], "enable"))
        {
            printf("[fm app] Ready to power on chip\r\n");
            op->mask |= OPTION_MASK_PWR;
            op->enable = 1;
            ret = 0;
        }

        if (!strcmp(argv[i], "disable"))
        {
            printf("[fm app] Ready to power off chip\r\n");
            op->mask |= OPTION_MASK_PWR;
            op->enable = 0;
            ret = 0;
        }

        if (!strcmp(argv[i], "load_fw"))
        {
            printf("[fm app] Ready to load firmware\r\n");
            op->mask |= OPTION_MASK_FW;
            op->load_fw = 1;
            ret = 0;
        }

        if (!strcmp(argv[i], "unload_fw"))
        {
            printf("[fm app] Ready to unload firmware\r\n");
            op->mask |= OPTION_MASK_FW;
            op->load_fw = 0;
            ret = 0;
        }
    }

    return ret;
}

int ti102_write (int address, char v0)
{
    char str[128];   
    sprintf(str, "hcitool cmd 0x3f 0x15 0x%x 0x00 0x%x", address, v0);
    return sendCommand(str);
}

int volumeTable[]=
{
	0x00,

	0x1f,
	0x2f,
	0x3f,
	0x4f, 
	0x5f,

	0x6f,
	0x7f,
	0x8f,
	0x9f,
	0xaf,

	0xbf,
	0xcf,
	0xdf,
	0xef,
	0xff
};

int runOptions(fmOptions* op)
{
    int mask = op->mask;

    while (mask)
    {
        if (mask & OPTION_MASK_PWR)
        {
            printf("[fm app] run OPTION_MASK_PWR\r\n");

            mask &= ~OPTION_MASK_PWR;

            if (fm_rfkill_id == -1)
            {
                if (fm_init_rfkill())
                {
                    printf("[fm app] Init rfkill fail\r\n");
                    return -1;
                }
            }
            asprintf(&fm_rfkill_state_path, "echo %d > /sys/class/rfkill/rfkill%d/state", op->enable, fm_rfkill_id);
            sendCommand(fm_rfkill_state_path);
        }

        if (mask & OPTION_MASK_FW)
        {
            printf("[fm app] run OPTION_MASK_FW\r\n");

            mask &= ~OPTION_MASK_FW;

            if (op->load_fw == 1)
            {
                sendCommand("bttest load_fw");
            }
            else
                sendCommand("bttest unload_fw");
        }

        if (mask & OPTION_MASK_VOL)
        {
            printf("[fm app] run OPTION_MASK_VOL\r\n");

            mask &= (~OPTION_MASK_VOL);
            //op->vol = CLAMP (op->vol,0,sizeof(volumeTable)-1);
            if (op->vol < 0)
                op->vol = 0;
            if (op->vol > sizeof(volumeTable)-1)
                op->vol = sizeof(volumeTable)-1;

            printf("[fm app] vol=%d %d\n", op->vol, volumeTable[op->vol]);

            ti102_write(0xf8, volumeTable[op->vol]);
        }
    }

    return 0;
}

void printHelp()
{
    printf("Usage: fm_app [ OPTIONS ]\n");
    printf("  -v, --volume=level            Volume level (0-%d, default is %d)\n",
                                                        FM_VOLUME_LEVEL_MAX,
                                                        FM_VOLUME_LEVEL_DEFAULT);
}

int fmappsckt_main(int argc, char *argv[])
{
    int ret = -1;
    fmOptions op;
    memset(&op, 0, sizeof(op));

    if (argc >= 2)
    {
        ret = getOptions(argc-1, &argv[1], &op);
    }

    if (ret)
    {
        printHelp();
    }
    else
    {
        ret = runOptions(&op);
    }

    return ret;
}
