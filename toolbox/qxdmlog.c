#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

int qxdmlog_main(int argc, char *argv[])
{
    int fd, r, bytes = 0;
    char devname[] = "/sys/devices/platform/DIAG.0/qxdm2sd";
    char buffer[8];
    
    fd = open(devname, O_WRONLY);
    
    if(fd < 0)
    {
        fprintf(stderr,"failed to open qxdm2sd - %s\n",
            strerror(errno));
        return -1;
    }
    
    if (!strcmp(argv[1],"start"))
    {
        bytes = sprintf(buffer, "%d\n", 1);
        r = write(fd, buffer, bytes);

        if (r != bytes)
        {
            fprintf(stderr,"failed to write qxdm2sd (%d) %s\n", r, strerror(errno));
            return -1;
        }
        else
        {
        	printf("Start QXDM log to SD.\n");
        }
    }
    else if (!strcmp(argv[1],"stop"))
    {
        bytes = sprintf(buffer, "%d\n", 0);
        r = write(fd, buffer, bytes);
        
        if(r != bytes)
        {
            fprintf(stderr,"failed to write qxdm2sd (%d) %s\n", r, strerror(errno));
            return -1;
        }
        else
        {
        	printf("Stop QXDM log to SD.\n");
        }
    }
    else if (!strcmp(argv[1],"fstart"))
    {
        bytes = sprintf(buffer, "%d\n", 2);
        r = write(fd, buffer, bytes);
        
        if(r != bytes)
        {
            fprintf(stderr,"failed to write qxdm2sd (%d) %s\n", r, strerror(errno));
            return -1;
        }
        else
        {
        	printf("Start capture filter to SD.\n");
        }
    }
    else if (!strcmp(argv[1],"fstop"))
    {
        bytes = sprintf(buffer, "%d\n", 3);
        r = write(fd, buffer, bytes);
        
        if(r != bytes)
        {
            fprintf(stderr,"failed to write qxdm2sd (%d) %s\n", r, strerror(errno));
            return -1;
        }
        else
        {
        	printf("Stop capture filter to SD.\n");
        }
    }
    
    close(fd);
	
	return 0;
}
