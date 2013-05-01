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

#ifndef closesocket
#define closesocket(sockid) close(sockid)
#endif

#ifndef ioctlsocket
#define ioctlsocket ioctl
#endif

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 108
#endif

#define LOCAL_SOCKET  "/data/skylocalsocket"

struct sockaddr_un 
{
    sa_family_t sun_family;
    char        sun_path[UNIX_PATH_MAX];
};


int recv_check(int sockid, int msecToWait)
{
   int ret;
   int len;
   fd_set rfd;
   struct timeval to;
   
   FD_ZERO( &rfd );
   FD_SET( sockid, &rfd );
   to.tv_sec = msecToWait/1000;
   to.tv_usec = (msecToWait%1000)*1000;
   
   ret = select( sockid+1, &rfd, NULL, NULL, &to );
   if (SOCKET_ERROR==ret)
   {
      fprintf( stderr, "%s(%d): select failed with %d\n",__FILE__,__LINE__,errno );
      return 0;
   }
   
   if (!FD_SET(sockid, &rfd))
   {
      return 0;
   }
   
   ret = ioctlsocket(sockid, FIONREAD, &len);
   if (SOCKET_ERROR==ret)
   {
      fprintf( stderr, "%s(%d): ioctlsocket(FIONREAD) failed with %d\n",__FILE__,__LINE__,errno );
      return 0;
   }
   return len;
}


int server_proc( int sockid2, char*cmdline );
int try_client( char*cmdline[] );


void createFQCFile()
{
	char *filename = "/data/FQC_mode";
	char file_data[2];

	file_data[0] = '0';
	file_data[1] = '\n';
    /*
	int fd = creat(filename, 0777);	
	write(fd, file_data, sizeof(file_data));
	close(fd);
    */
	int fd2 = creat("/data/wifitestmode", 0777);	
	write(fd2, file_data, sizeof(file_data));
	close(fd2);
}

int runit_main(int argc, char *argv[])
{
    int sockid;  // main socket id
    int sockid2; // socket id to
    int runit=1;
    int addr2_len=0;
    struct sockaddr_un addr = {0};
    struct sockaddr_un addr2 = {0};
    createFQCFile();

    // client
    if (argc>=2 )
    {
       return try_client(&argv[1]);
    }
    
    unlink(LOCAL_SOCKET);
    
    //fprintf( stderr, "%s(%d): argc=%d argv[1]=%s\n",__FILE__,__LINE__,argc,argv[1] );

    // prepare main socket    
    sockid = socket(PF_UNIX, SOCK_STREAM, 0); // aslo known as PF_LOCAL
    if (SOCKET_ERROR==sockid)
    {
        fprintf(stderr, "%s(%d): socket failed with %d\n",__FILE__,__LINE__,errno);
        return 0;
    }

    addr.sun_family = PF_UNIX;
    sprintf(addr.sun_path, LOCAL_SOCKET);
    
    if (SOCKET_ERROR==bind(sockid, (struct sockaddr*)&addr, sizeof(addr)))
    {
        closesocket(sockid);
        sockid = SOCKET_ERROR;
        fprintf(stderr, "%s(%d): bind %s failed with %d\n", __FILE__, __LINE__, addr.sun_path,errno );
        return -1;
    }

    // start service
    while (runit)
    {
        int cmd_len = 0;
        int num_recv = 0;
        char *cmdline = 0;
    
        if (SOCKET_ERROR==listen(sockid, 2))
        {
            closesocket(sockid);
            sockid = SOCKET_ERROR;
            fprintf( stderr, "%s(%d): listen failed with %d\n",__FILE__,__LINE__,errno);
            return -1;
        }
        
        sockid2 = accept(sockid, (struct sockaddr*) &addr2, &addr2_len);
        if (SOCKET_ERROR==sockid2)
        {
            closesocket(sockid);
            sockid = SOCKET_ERROR;
            unlink(LOCAL_SOCKET);
            fprintf( stderr, "%s(%d): accept failed with %d\n",__FILE__,__LINE__,errno);
            return -1;
        }
        
        // service the local socket
        cmd_len = recv_check( sockid2, 3000 );
        fprintf( stderr, "%s(%d): cmd_len=%d\n",__FILE__,__LINE__,cmd_len );
        
        if (SOCKET_ERROR==cmd_len || 0==cmd_len )
        {
           closesocket( sockid2 );
           continue;
        }
        
        // read command
        cmdline = (char*) malloc(1+cmd_len);
        num_recv = recv( sockid2, cmdline, cmd_len, 0 );
        
        fprintf(stderr, "%s(%d): num_recv=%d\n", __FILE__, __LINE__, num_recv);
        
        if (SOCKET_ERROR==num_recv||cmd_len!=num_recv)
        {
           free(cmdline);
           closesocket( sockid2 );
           continue;
        }
        
        cmdline[cmd_len] = 0;
        
        fprintf(stderr, "%s(%d): cmd_len=%d cmdline(%s)\n", __FILE__,__LINE__,cmd_len,cmdline );
        
        if ('e'==cmdline[0]&&'x'==cmdline[1]&&'i'==cmdline[2]&&'t'==cmdline[3] )
        {
            runit = 0;
        }
        else if ('c'==cmdline[0]&&'m'==cmdline[1]&&'d'==cmdline[2]&&':'==cmdline[3])
        {
            // using fork()
            int pid = 0;
            
            pid = fork();
            
            if( pid == 0 )
            {                             
                runit = server_proc( sockid2, &cmdline[4] );
                
                _exit(0);
            }
        }
        else if( 'r'==cmdline[0] && 'e'==cmdline[1] && 't'==cmdline[2] && ':'==cmdline[3] )
        {
        }
        
		free(cmdline);
        closesocket( sockid2 );
    }

    closesocket(sockid);
    unlink(LOCAL_SOCKET);

    return 0;
}

int server_proc(int sockid2, char*cmdline)
{
   int i;
   int last=0;
   int len=0;
   int cmd_len = strlen(cmdline);
   
   char*pszCmd = (char*) malloc(3*(1+cmd_len));
   
   fprintf( stderr, "%s(%d): input command:\n%s\n",__FILE__,__LINE__,cmdline );

   for( i=0; i<cmd_len; i++ )
   {
      //fprintf( stderr, "%02X", cmdline[i] );
   
      if( '\x1b'==cmdline[i] || 0==cmdline[i] || (i==cmd_len-1) )
      {
         // reverse check if any tab/space in last argument
         int k;
         int has_space=0;
         
         for( k=last; k<i; k++ )
         {
            if( ' '==cmdline[k] || '\t'==cmdline[k] )
            {
               has_space=1;
               break;
            }
         }
         
         // cut
         if( i==cmd_len-1 )
            cmdline[i+1]=0;
         else
            cmdline[i] = 0;
         //fprintf( stderr, "%s(%d): last=%d str=%s\n",__FILE__,__LINE__,last,&cmdline[last] );
         
         // copy last argv
         if( has_space )
         {
            len+=sprintf( &pszCmd[len], "\"%s\" ", &cmdline[last] );
         }
         else
         {
            len+=sprintf( &pszCmd[len], "%s ", &cmdline[last] );
         }
         
         // next
         last = i+1;
         
         //fprintf( stderr, "%s(%d): last=%d, len=%d\n",__FILE__,__LINE__,last,len );
      }
      
   }
   
   fprintf( stderr, "%s(%d): command remixed(%s)\n",__FILE__,__LINE__,pszCmd );
   
   //FILE*stream = popen( "ls -l /system/bin/", "r" );
   FILE*stream = popen( pszCmd, "r" );
   //fprintf( stderr, "%s(%d): stream=%d\n",__FILE__,__LINE__,stream);
   
   if( stream )
   {
      int len=4;
      char buf[4096];
      buf[0] = 'r';
      buf[1] = 'e';
      buf[2] = 't';
      buf[3] = ':';

      while( 1==fread( &buf[len], sizeof(char), 1, stream ))
      {
          if( '\n'==buf[len] )
          {
              buf[len] = 0;
              //LOGI( "%s", buf );
              fprintf( stderr, "%s\n", buf );
              send( sockid2, buf, len, 0 );
              
              len = 4;
              buf[4] = 0;
          }
          else
          {
              len++;
          }
      }
      
      buf[len]=4;
      if( len >4 )
      {
          fprintf( stderr, "%s\n", buf );
          send( sockid2, buf, len, 0 );
      }
      
      
      /*
      while( (len=fread( &buf[4], sizeof(char), sizeof(buf)-4, stream )) > 0)
      {
         buf[0]='r';
         buf[1]='e';
         buf[2]='t';
         buf[3]=':';
         send( sockid2, buf, sizeof(buf), 0 );
      
         //fprintf( stderr, "%c", buf[0] );
         fprintf( stderr, "%s(%d): pipe read %d bytes\n",__FILE__,__LINE__,len );
         
         LOGI( "%s(%d): logi- fread=%d\n",__FILE__,__LINE__,len );
      }
      */
      
      //fprintf( stderr, "%s(%d): len=%d\n",__FILE__,__LINE__,len );
      
      pclose(stream);
   }
   else if( 0==stream )
   {
      fprintf( stderr, "%s(%d): pipe/fork failed with %d\n",__FILE__,__LINE__,errno );
   }
   else
   {
      fprintf( stderr, "%s(%d): wait failed with %d\n",__FILE__,__LINE__,errno );
   }
   
   free(pszCmd);
   
   /*
   //chpid=fork();
   if( 0==chpid )
   {
      close(pipefd[1]); // child - close unused read end
      
      if( -1==execvp( cmdline, arg ))
      {
         fprintf( stderr, "%s(%d): execv failed with %d\n",__FILE__,__LINE__,errno );
      }
      close(pipefd[0]); // close write end
      _exit(0);
   }
   else if( -1==chpid )
   {
   }
   else
   {
      char buf[4];
      int chret=0;
      int nread=0;
      int nwrote=0;
   
      //close(pipefd[1]); // close unused write end
      wait( &chret );
      
      ioctl( pipefd[0], FIONREAD, &nread );
      
      fprintf( stderr, "%s(%d): FIONREAD = %d\n",__FILE__,__LINE__,nread );
      
      ioctl( pipefd[1], FIONREAD, &nwrote );
      
      fprintf( stderr, "%s(%d): FIONREAD = %d\n",__FILE__,__LINE__,nwrote );
      
      //while( read(pipefd[1], buf, 1 ) > 0)
      {
         //write(STDOUT_FILENO, buf, 1);
      }
      
      close(pipefd[0]);
      close(pipefd[1]); // close read end
   }
   */
   return 1;
}


int try_client(char*cmdline[])
{
    int sockid;
    struct sockaddr_un addr={0};
    char*pszCmd=0;
    int len=0;
    
    sockid = socket( PF_UNIX, SOCK_STREAM, 0 ); // aslo known as PF_LOCAL
    if (SOCKET_ERROR==sockid)
    {
        fprintf( stderr, "%s(%d): client- socket failed with %d\n",__FILE__,__LINE__,errno);
        return 0;
    }
    
    addr.sun_family = PF_UNIX;
    sprintf(addr.sun_path, LOCAL_SOCKET);
    if( SOCKET_ERROR==connect( sockid, (struct sockaddr*) &addr, sizeof(addr)) )
    {
       closesocket( sockid );
       fprintf( stderr, "%s(%d): client- connect failed with %d\n",__FILE__,__LINE__,errno);
       return 0;
    }
    
    // prepare command line
    if( cmdline && cmdline[0] )
    {
       int i, j;
       for( i=0; cmdline[i] && cmdline[i][0]; i++ )
       {
          fprintf( stderr, "%s(%d): cmdline[%d]=(%s) len=%d\n",__FILE__,__LINE__,i,cmdline[i],strlen(cmdline[i]));
          len += 2+strlen(cmdline[i]);
       }
       
       pszCmd = (char*) malloc(len+2+4);
       len = sprintf( pszCmd, "cmd:" );
       for( i=0; cmdline[i] && cmdline[i][0]; i++ )
       {
          len += sprintf( &pszCmd[len], "%s\x1b", cmdline[i] );
       }
       
       pszCmd[--len]=0;
       
       fprintf( stderr, "%s(%d): len=%d pszCmd(%s)\n",__FILE__,__LINE__,len,pszCmd );
    }


    //if( SOCKET_ERROR==send( sockid, cmdline, strlen(cmdline), 0 ))
    if (SOCKET_ERROR==send( sockid, pszCmd, len, 0 ))
    {
       fprintf( stderr, "%s(%d): client- send(%s) failed with %d\n",__FILE__,__LINE__,pszCmd,errno);
    }

    if (pszCmd) free(pszCmd);
    closesocket(sockid);
    return 0;
}








