#ifndef REACTOR_TIME_SERVER_H_
#define REACTOR_TIME_SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include "test_common.h"

const static char http_error_hdr[]  = "HTTP/1.1 404 Not Found\r\nContent-type: text/html\r\n\r\n";
const static char http_error_html[] = "<html><body><h1>FILE NOT FOUND</h1></body></html>";
const static char http_html_hdr[]   = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";
const static char http_image_hdr[]  = "HTTP/1.1 200 OK\r\nContent-Type: image/gif\r\n\r\n";

#define PEND_CONNECTIONS 100  
#define SRV_PORT 8000        // port number of web server
#define BUF_LEN 1024

reactor::Reactor g_reactor;

const size_t kBufferSize = 1024;
char g_read_buffer[kBufferSize];
char g_write_buffer[kBufferSize];

class HTTPRequestHandler : public reactor::EventHandler
{
public:
    HTTPRequestHandler(reactor::handle_t handle) :
        EventHandler(),
        m_handle(handle)
    {}
    
	virtual reactor::handle_t GetHandle() const
    {
        return m_handle;
    }

    virtual void HandleRead()
    {
        memset(g_read_buffer, 0, sizeof(g_read_buffer));
        int len = recv(m_handle, g_read_buffer, kBufferSize, 0);
        if (len > 0)
        {
            if (strncasecmp("GET", g_read_buffer, 3) == 0)
            {
				// Process the web request
				char *file_path = g_read_buffer + 5;
				file_path = strchr(file_path, ' ');
				*file_path = '\0';
				file_path = g_read_buffer + 5;

				int fd = open(file_path, O_RDONLY, S_IREAD | S_IWRITE);
				if(fd == -1)
				{
					printf("Failed to openi file %s.\n", file_path);
					write(m_handle, http_error_hdr, strlen(http_error_hdr));
					write(m_handle, http_error_html, strlen(http_error_html));
				}
				else
				{
					printf("File %s is being sent.\n", file_path);
					if(strstr(file_path, ".jpg")!=NULL || strstr(file_path, ".png")!=NULL)
						write(m_handle, http_image_hdr, strlen(http_image_hdr));
					else
						write(m_handle, http_html_hdr, strlen(http_html_hdr));

					char send_buf[BUF_LEN];
					int buf_len = 1;

					while(buf_len > 0)
					{
						buf_len = read(fd, send_buf, BUF_LEN);
						if(buf_len > 0)
						{
							write(m_handle, send_buf, buf_len);
						}
					}
				}

				close(fd);
				close(m_handle);
				g_reactor.RemoveHandler(this);
				delete this;
            }
        }
        else
        {
            ReportSocketError("recv");
        }
    }

    virtual void HandleError()
    {
        fprintf(stderr, "client %d closed\n", m_handle);
        close(m_handle);
        g_reactor.RemoveHandler(this);
        delete this;
    }

private:
    reactor::handle_t m_handle; 
};

class HTTPServer : public reactor::EventHandler
{
public:
    HTTPServer(unsigned short port) :
        EventHandler(),
        m_port(port)
    {}

    bool Start()
    {
        m_handle = socket(AF_INET, SOCK_STREAM, 0);
        if(!IsValidHandle(m_handle))
        {
            ReportSocketError("socket");
            return false;
        }

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(m_port);
        addr.sin_addr.s_addr = INADDR_ANY;

		int yes = 1;
		if(setsockopt(m_handle, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
		{
			perror("setsockopt failed\n");
			return false;
		}

        if(bind(m_handle, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            ReportSocketError("bind");
            return false;
        }

        if(listen(m_handle, 10) < 0)
        {
            ReportSocketError("listen");
            return false;
        }
        return true;
    }

    virtual reactor::handle_t GetHandle() const
    {
        return m_handle;
    }

    virtual void HandleRead()
    {
        struct sockaddr addr;
        socklen_t addrlen = sizeof(addr);
        reactor::handle_t handle = accept(m_handle, &addr, &addrlen);
        if (!IsValidHandle(handle))
        {
            ReportSocketError("accept");
        }
        else
        {
            HTTPRequestHandler * handler = new HTTPRequestHandler(handle);
            if (g_reactor.RegisterHandler(handler, reactor::kReadEvent) != 0)
            {
                fprintf(stderr, "error: register handler failed\n");
                delete handler;
            }
        }
    }

private:
    reactor::handle_t     m_handle; 
    unsigned short        m_port;   
};

int main(int argc, char ** argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "usage: %s port\n", argv[0]);
        return EXIT_FAILURE;
    }

    HTTPServer server(atoi(argv[1]));
    if (!server.Start())
    {
        fprintf(stderr, "start server failed\n");
        return EXIT_FAILURE;
    }

    while (1)
    {
        g_reactor.RegisterHandler(&server, reactor::kReadEvent);
        g_reactor.HandleEvents(100);
    }
    return EXIT_SUCCESS;
}

#endif 
