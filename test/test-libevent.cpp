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

/* Required by event.h. */
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

/********************************* CONSTANTS **********************************/

/*********************************** MACROS ***********************************/

/******************************** ENUMERATIONS ********************************/

/************************* STRUCTURE/UNION DATA TYPES *************************/

/************************ STATIC FUNCTION PROTOTYPES **************************/
static void event_log_cbk(int severity, const char *msg);

static void event_listen_cbk(evutil_socket_t fd, short events, void *arg);

/****************************** LOCAL FUNCTIONS *******************************/
static void event_log_cbk(int severity, const char *msg) {

}

static void event_listen_cbk(evutil_socket_t fd, short events, void *arg) {
   printf ("New connection!!!");
}

int main () {
   int listen_fd;
   struct sockaddr_in listen_addr;
   int reuseaddr_on;
   event *listen_event = NULL;

   event_base *base = event_base_new();
   event_set_log_callback(event_log_cbk);

   listen_fd = socket(AF_INET, SOCK_STREAM, 0);
   if (listen_fd < 0)
      err(1, "listen failed");
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

   listen_event = event_new(base, listen_fd, EV_READ, event_listen_cbk, NULL);

   struct timeval to = {0xFFFFFFFF, 0xFFFFFFFF};
   event_add(listen_event, &to);

   event_base_dispatch(base);
   return 0;
}
