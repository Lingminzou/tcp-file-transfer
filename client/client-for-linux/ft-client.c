#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>

#include <netdb.h>  
#include <net/if.h>  
#include <arpa/inet.h> 
#include <sys/ioctl.h>

#include<sys/stat.h>
#include<fcntl.h>
#include <unistd.h>

#include <pthread.h>

//#define TLOG_LEVEL TLOG_LEVEL_INFO
#define TLOG_LEVEL TLOG_LEVEL_DEBUG
#define TLOG_TAG "ft-client"
#include "tlog.h"

#define SERVER_CMD_PORT   8899

void (* listen_callback)() = NULL;
extern int download_file(char* path);
extern int push_file(char* path);

#define IP_SIZE     16

static const char* eth_list[] = {
    "eth0",
    "wlan0",
    "eth1",
    "wlan1",
};

static char local_ip[IP_SIZE] = {0};
static char broad_ip[IP_SIZE] = {0};

static char cmd_line[128] = {0};

static pthread_t tid;

/*
 *   SIOCGIFADDR
 *   SIOCGIFNETMASK
 *   SIOCGIFBRDADDR
 *   //SIOCGIFHWADDR
 */
int get_eth_ip_info(const char *eth_inf, int cmd, char *ip)
{
    int sd;  
    struct sockaddr_in sin;  
    struct ifreq ifr;  

    sd = socket(AF_INET, SOCK_DGRAM, 0);  
    if (-1 == sd)  
    {  
        tlog_e("socket error: %s", strerror(errno));
        return -1;        
    }  

    strncpy(ifr.ifr_name, eth_inf, IFNAMSIZ);  
    ifr.ifr_name[IFNAMSIZ - 1] = 0;  

    // if error: No such device
    if (ioctl(sd, cmd, &ifr) < 0)
    {  
        tlog_e("get %s ioctl error: %s", ifr.ifr_name, strerror(errno));
        close(sd);
        return -1;
    }

    memcpy(&sin, &ifr.ifr_addr, sizeof(sin));

    snprintf(ip, IP_SIZE, "%s", inet_ntoa(sin.sin_addr));

    close(sd);

    return 0; 
}

/* find the available eth */
int get_eth_ip_info_in_list(int cmd, char *ip)
{
    uint8_t i;
    static const char *peth = NULL;

    if(NULL == peth)
    {
        for(i = 0x00; i != sizeof(eth_list)/sizeof(eth_list[0]); i++)
        {
            if(0x00 == get_eth_ip_info(eth_list[i], cmd, ip))
            {
                peth = eth_list[i];
                tlog_i("get the available eth %s", eth_list[i]);
                return 0x00;
            }
        }

        if(i == sizeof(eth_list)/sizeof(eth_list[0]))
        {
            tlog_e("None available eth!!!");
            return -1;
        }
    }
    else
    {
        return get_eth_ip_info(peth, cmd, ip);
    }

    return 0x00;
}

void* udp_bcast_thread(void* arg)
{
	int send_count;
    int sock_fd;
    struct sockaddr_in addr_bcast;
	int dest_len = sizeof(struct sockaddr_in);
	int opt = 1;
	
	char *cmd_line = (char*) arg;
	
	if(get_eth_ip_info_in_list(SIOCGIFBRDADDR, broad_ip) < 0)
	{
        tlog_e("get eth bcast addr error!!!");

        return NULL;
	}
	
	tlog_i("bcast ip: %s", broad_ip);

	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	
	if(sock_fd < 0)
    {
        tlog_e("create bcast socket error!!!");

        return NULL;
    }
    else
    {
        tlog_i("create bcast socket sucessful!");
    }
	
	memset(&addr_bcast, 0, sizeof(addr_bcast));
    addr_bcast.sin_family = AF_INET;
    addr_bcast.sin_port =  htons(SERVER_CMD_PORT);
    addr_bcast.sin_addr.s_addr = inet_addr(broad_ip);
	
	/* enable broadcast */
    setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));
	
	tlog_d("broadcast cmd: %s", cmd_line);
	
	while(1)
	{
		send_count = sendto(sock_fd, cmd_line, strlen(cmd_line), 0, (struct sockaddr *)&addr_bcast, dest_len);
		
		if(send_count < 0)
		{
			tlog_e("bcast send error!!!, send_count: %d", send_count);
			
			break;
		}
		
		sleep(1);
	}
	
	return NULL;
}

void callback()
{
	void *tret;
	
	tlog_i("cancel broadcast thread!!");
	
	pthread_cancel(tid);

	pthread_join(tid, &tret);
}

#ifdef DEBUG
#include "backtrace.c"
#endif

static const char* optString = "p:g:h?";

int main(int argc, char *argv[])
{
    char ft_opt = 'g';
    int opt = 0;
    int file_size = 0;

    char *pfile_path = NULL;
    char *pfile_name = NULL;

#ifdef DEBUG
    RegisterSignalForBacktrace();
#endif

    do
    {
        opt = getopt(argc, argv, optString);

        switch(opt)
        {
        case 'p':
            ft_opt = 'p';
            pfile_path = optarg;
            break;
        case 'g':
            ft_opt = 'g';
            pfile_path = optarg;
            break;
        case 'h':
        case '?':
            break;
        default:
            break;
        }

    }while(-1 != opt);
 
    if(NULL == pfile_path)
    {
        tlog_e("Please enter \'p\' or \'g\' and file path!!");
        return -1;
    }

    pfile_name = rindex(pfile_path, '/');

    if (NULL == pfile_name)
    {
        pfile_name = pfile_path;
    }
    else
    {
        pfile_name++;
    }

	tlog_i("file path: %s, file name: %s", pfile_path, pfile_name);

	if(strlen(pfile_name) <= 0)
	{
		tlog_e("Please enter file name!!!");
		return -1;
	}
	
    memset(cmd_line, 0, sizeof(cmd_line));

    if('p' == ft_opt)
    {
        strcat(cmd_line, "opt:p");
    }
    else
    {
	    strcat(cmd_line, "opt:g");
    }

	strcat(cmd_line, ";name:");
	
	strcat(cmd_line, pfile_name);
	
	strcat(cmd_line, ";ip:");
	
	if(0 != get_eth_ip_info_in_list(SIOCGIFADDR, local_ip))
	{
		tlog_e("get local ip file!!!");
		return -1;
	}
	
	strcat(cmd_line, local_ip);
	
	tlog_i("local ip: %s", local_ip);

    if('p' == ft_opt)
    {
        struct stat buf;

        if(0 != stat(pfile_path, &buf))
        {
            tlog_e("get file info error!!!");
        }
        else
        {
            file_size = buf.st_size;
        }
    }

    tlog_i("file size: %d", file_size);

    strcat(cmd_line, ";size:");

    sprintf(&cmd_line[strlen(cmd_line)], "%d", file_size);

	listen_callback = callback;
	
	if(0 != pthread_create(&tid, NULL, udp_bcast_thread, cmd_line))
	{
		tlog_e("create thread file!!!");
		return -1;		
	}

    if('p' == ft_opt)
    {
        if(push_file(pfile_path) < 0)
        {
            return -1;
        }
    }
    else
    {
        if(download_file(pfile_path) < 0)
	    {
		    return -1;
        }
	}
	
	return 0;
}
