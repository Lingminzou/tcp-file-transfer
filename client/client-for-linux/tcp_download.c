#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define TLOG_LEVEL TLOG_LEVEL_INFO
#define TLOG_TAG "tcp_download"
#include "tlog.h"

#define QUEUE_LENGTH  1
#define DATA_PORT     8999

static char file_recv_buff[64 * 1024] = {0};

extern void (* listen_callback)();

static int new_file(const char *path)
{
	int file_fd = -1;
	
	if(0 == access(path, F_OK))
	{
		if(0 != remove(path))
		{
			tlog_e("remove file: %s , error!!!", path);
			
			return -1;
		}
	}
	
	file_fd = open(path, O_RDWR | O_CREAT, 0644);
	
	if(file_fd < 0)
	{
		tlog_e("creat file: %s , error!!!", path);
		
		return -1;
	}
	
	return file_fd;
}

static int transfer_file(int file_fd)
{
    int sock_fd;
	int conn_fd;
    struct sockaddr_in addr_serv;
	struct sockaddr_in client_addr;
	socklen_t addr_len = sizeof(struct sockaddr_in);
	
	int recv_count = 0;
	
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(sock_fd < 0)
    {
        tlog_e("transfer_file create socket error!!!");

        return -1;
    }
    else
    {
        tlog_i("transfer_file create socket sucessful!");
    }

    memset(&addr_serv, 0, sizeof(addr_serv));
    addr_serv.sin_family = AF_INET;
    addr_serv.sin_port =  htons(DATA_PORT);
    addr_serv.sin_addr.s_addr = htons(INADDR_ANY);
	
	if(bind(sock_fd, (struct sockaddr *)&addr_serv, sizeof(struct sockaddr_in)) < 0)
	{
        tlog_e("transfer_file bind fail!!!");

        goto transfer_file_fail;		
	}
	else
	{
		tlog_i("transfer_file bind sucessful!");
	}
	
	if(listen(sock_fd, QUEUE_LENGTH) < 0)
	{
        tlog_e("transfer_file listen fail!!!");

        goto transfer_file_fail;
	}
	else
	{
		tlog_i("start listen......");
	}

	conn_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &addr_len);
	
	if(conn_fd < 0)
	{
		tlog_e("transfer_file accept fail!!!");
		
		goto transfer_file_fail;
	}
	else
	{
		tlog_i("accept client ip: %s  port: %d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
	}
	
	if(NULL != listen_callback)
	{
		listen_callback();
	}

    while(1)
    {
        recv_count = recv(conn_fd, file_recv_buff, sizeof(file_recv_buff), 0);
		
		//tlog_d("recv recv_count: %d", recv_count);

        if(recv_count < 0)
        {
            tlog_e("transfer_file recv error!!");

            goto transfer_file_fail;
        }
        else if(recv_count == 0)
        {
			tlog_i("transfer file sucessful, break!!");
			
			break;
        }
		else
		{
			if(write(file_fd, file_recv_buff, recv_count) < 0)
			{
				tlog_e("transfer_file write file error!!");
				
				goto transfer_file_fail;
			}
		}
    }

	close(file_fd);
	close(sock_fd);
	
	return 0;
	
transfer_file_fail:

	close(file_fd);
	close(sock_fd);
	
	return -1;
}

int download_file(char* path)
{
	tlog_i("download file name: %s", path);
	
	int file_fd = new_file(path);
	
	if(file_fd < 0)
	{
		tlog_e("create file error!!!");
		
		return -1;
	}

	if(transfer_file(file_fd) < 0)
	{
		tlog_e("transfer file error!!!");
		
		return -1;
	}
	
	return 0;
}
