/*******************************************************************************
 *
 *  BSD 2-Clause License
 *
 *  Copyright (c) 2017, Sandeep Prakash
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

/*******************************************************************************
 * Copyright (c) 2017, Sandeep Prakash <123sandy@gmail.com>
 *
 * \file   test-libevent.cpp
 *
 * \author Sandeep Prakash
 *
 * \date   Apr 10, 2017
 *
 * \brief
 *
 ******************************************************************************/

/********************************** INCLUDES **********************************/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/time.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>

#include <event2/event.h>
#include <event2/util.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include <iostream>
#include <thread>
#include <chrono>
#include "thread-pool.hpp"
#include "logger.hpp"

/********************************* CONSTANTS **********************************/

/*********************************** MACROS ***********************************/

/******************************** ENUMERATIONS ********************************/

/************************* STRUCTURE/UNION DATA TYPES *************************/

/************************ STATIC FUNCTION PROTOTYPES **************************/
static void event_log_cbk(int severity, const char *msg);

static void event_listen_cbk(evutil_socket_t fd, short events, void *arg);

/****************************** LOCAL FUNCTIONS *******************************/
using namespace std;

static Logger &log = Logger::getInstance();

typedef struct _listen_ctxt {
   int listen_fd;
   struct event_base *base;
   struct event *listen_event;
   ThreadPool *listenThread;
   ThreadPool *workerPool;
} listen_ctxt;

typedef struct _client_ctxt {
   listen_ctxt *listenCtxt;
   int client_fd;
   struct event *client_event;
   struct bufferevent *client_bev;
   struct sockaddr_in *px_sock_addr_in;
} client_ctxt;

void * worker_thread_routine (void *arg)
{
   client_ctxt *client = (client_ctxt *) arg;
   LOG << "Running Worker Thread Routine" << std::endl;

   bufferevent_enable (client->client_bev, EV_READ | EV_WRITE);

   return NULL;
}

void * listen_thread_routine (void *arg)
{
   listen_ctxt *listenCtxt = (listen_ctxt *) arg;
   LOG << "Running Listen Thread Routine" << std::endl;

   struct timeval to = {0xFFFFFFFF, 0xFFFFFFFF};
   event_add(listenCtxt->listen_event, &to);

   event_base_dispatch(listenCtxt->base);

   return NULL;
}

static void event_log_cbk(int severity, const char *msg) {

}

static void bufferevent_read_cbk (struct bufferevent *bev, void *ctx) {
   LOG << "Data on socket!!" << std::endl;
   struct evbuffer *buf = evbuffer_new();
   bufferevent_read_buffer(bev, buf);

   size_t readLength = evbuffer_get_length (buf);
   int read = -1;
   uint8_t *data = (uint8_t *) malloc (readLength);

   char *response;
   if ((read = evbuffer_remove(buf, (void *) data, readLength)) != readLength) {
      response = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
   }
   else {
      LOG << "Read " << readLength << " bytes from socket." << std::endl;
      LOG << std::endl << data << std::endl;
      response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
   }
   uint32_t length = strlen (response) + 1;
   bufferevent_write (bev, (const void *) response, length);
}

static void bufferevent_write_cbk (struct bufferevent *bev, void *ctx) {

}

static void bufferevent_event_cbk (struct bufferevent *bev, short what, void *ctx) {

}

static void event_listen_cbk(evutil_socket_t fd, short events, void *arg) {
   listen_ctxt *listenCtxt = (listen_ctxt *) arg;
   socklen_t ui_sock_addr_len = 0;

   LOG << "New connection!!" << std::endl;
   client_ctxt *client = (client_ctxt *) malloc (sizeof (client_ctxt));
   memset (client, 0x00, sizeof (*client));

   client->px_sock_addr_in = (struct sockaddr_in *) malloc (
         sizeof (struct sockaddr_in));
   ui_sock_addr_len = sizeof (*(client->px_sock_addr_in));
   memset (client->px_sock_addr_in, 0x00, ui_sock_addr_len);

   client->client_fd = accept (listenCtxt->listen_fd,
         (struct sockaddr *) client->px_sock_addr_in, &ui_sock_addr_len);
   client->listenCtxt = listenCtxt;
   if (client->client_fd > 0) {
      LOG << "Accepted connection!!" << std::endl;

      evutil_make_socket_nonblocking(client->client_fd);

      client->client_bev = bufferevent_socket_new (listenCtxt->base,
            client->client_fd, 0);
      bufferevent_setcb (client->client_bev, bufferevent_read_cbk,
            bufferevent_write_cbk, bufferevent_event_cbk, client);

      ThreadJob job = ThreadJob (worker_thread_routine, client);
      listenCtxt->workerPool->addJob(job);
   }

   ThreadJob job = ThreadJob (listen_thread_routine, listenCtxt);
   listenCtxt->listenThread->addJob(job);
}

int main () {
   int listen_fd;
   struct sockaddr_in listen_addr;
   int reuseaddr_on;
   struct event *listen_event = NULL;
   listen_ctxt *listenCtxt = NULL;

   ThreadPool *listenThread = new ThreadPool (1);
   ThreadPool *workerPool = new ThreadPool ();

   struct event_base *base = event_base_new();
   event_set_log_callback(event_log_cbk);

   listen_fd = socket(AF_INET, SOCK_STREAM, 0);
   if (listen_fd < 0) {
      printf("socket failed");
      exit (-1);
   }
   memset(&listen_addr, 0, sizeof(listen_addr));
   listen_addr.sin_family = AF_INET;
   listen_addr.sin_addr.s_addr = INADDR_ANY;
   listen_addr.sin_port = htons(8888);
   if (bind(listen_fd, (struct sockaddr *)&listen_addr,
      sizeof(listen_addr)) < 0)
      err(1, "bind failed");
   if (listen(listen_fd, 5) < 0)
      err(1, "listen failed");
   reuseaddr_on = 1;
   setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_on,
       sizeof(reuseaddr_on));

   evutil_make_socket_nonblocking(listen_fd);


   listenCtxt = (listen_ctxt *) malloc (sizeof (listen_ctxt));
   memset (listenCtxt, 0x00, sizeof (*listenCtxt));

   listen_event = event_new(base, listen_fd, EV_READ, event_listen_cbk, listenCtxt);
   listenCtxt->listen_fd = listen_fd;
   listenCtxt->base = base;
   listenCtxt->listen_event = listen_event;
   listenCtxt->listenThread = listenThread;
   listenCtxt->workerPool = workerPool;

   ThreadJob job = ThreadJob (listen_thread_routine, listenCtxt);
   listenThread->addJob(job);

   std::chrono::milliseconds ms(1000);
   while (true) {
      std::this_thread::sleep_for(ms);
   }

   return 0;
}







