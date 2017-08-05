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
 * \file   raft.cpp
 *
 * \author Sandeep Prakash
 *
 * \date   Mar 15, 2017
 *
 * \brief  
 *
 ******************************************************************************/

/********************************** INCLUDES **********************************/
#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

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
#include "utils/thread-pool.hpp"
#include "utils/tcp-listener.hpp"
#include "utils/tcp-server.hpp"
#include "utils/logger.hpp"
#include "protos/c++/communication.pb.h"

/********************************* CONSTANTS **********************************/

/*********************************** MACROS ***********************************/
#define RAFT_DEFAULT_LISTEN_PORT (8888)

#define RAFT_MAX_PEERS           (10)

#define _200_OK ("HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n")

#define _500_INTERNAL_SERVER_ERROR ("HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n")

/******************************** ENUMERATIONS ********************************/

/************************* STRUCTURE/UNION DATA TYPES *************************/
typedef struct _RAFT_ARGS_X
{
   in_port_t listenPort;

   in_port_t peerPorts[RAFT_MAX_PEERS];

   uint32_t numPorts;
} RAFT_ARGS_X;

typedef struct _RAFT_MSG_HDR_X
{
   uint32_t ui_msg_id;

   uint32_t ui_msg_pay_len;

   uint8_t uca_reserved[56];
} RAFT_MSG_HDR_X;

typedef struct _RAFT_MSG_HEARTBEAT_X
{
   RAFT_MSG_HDR_X x_hdr;

   uint32_t ui_bitmask;
} RAFT_MSG_HEARTBEAT_X;

/************************ STATIC FUNCTION PROTOTYPES **************************/
static void raft_get_opts_from_args (int argc, char **argv,
      RAFT_ARGS_X *px_raft_args);

static void event_log_cbk(int severity, const char *msg);

static void onMessage (client_ctxt *client, uint8_t *message, uint32_t length, void *this_);

static void onConnection (client_ctxt *client, void *this_);

static void raft_print_usage (struct option *long_opt,
                              const char **long_opt_description,
                              uint32_t ui_opts_count,
                              char **argv);

static bool raft_is_opts_from_args_valid (RAFT_ARGS_X *px_raft_args);

/****************************** LOCAL FUNCTIONS *******************************/
using namespace std;

static Logger &log = Logger::getInstance();

static void event_log_cbk(int severity, const char *msg) {

}

static void raft_print_usage (struct option *long_opt,
                              const char **long_opt_description,
                              uint32_t ui_opts_count,
                              char **argv) {
   uint32_t ui_opts_idx = 0;
   printf("Usage: %s [OPTIONS]\n", argv[0]);
   for (ui_opts_idx = 0; ui_opts_idx < ui_opts_count; ui_opts_idx++)
   {
      if (NULL == long_opt[ui_opts_idx].name)
      {
         break;
      }
      char temp[20] = {0};
      strcat(temp, "--");
      strcat(temp, long_opt[ui_opts_idx].name);

      printf("  -%c %-20s (OR %22s=%-20s) - %s\n",
            long_opt[ui_opts_idx].val,
            long_opt[ui_opts_idx].name,
            temp,
            long_opt[ui_opts_idx].name,
            long_opt_description[ui_opts_idx]);
   }
   printf("\n");
}

static bool raft_is_opts_from_args_valid (RAFT_ARGS_X *px_raft_args) {
   printf ("Entered Peer Ports: %d\n", px_raft_args->numPorts);
   return true;
}

static void raft_get_opts_from_args (int argc, char **argv,
      RAFT_ARGS_X *px_raft_args)
{
   uint32_t ui_opts_count = 0;
   int             c;
   const char    * short_opt = "ha:b:c:w:s:p:l:";
   struct option   long_opt[] =
   {
      {"help",          no_argument,       NULL, 'h'},
      {"listen-port",   optional_argument, NULL, 'l'},
      {"peer-ports",    required_argument, NULL, 'p'},
      {NULL,            0,                 NULL, 0  }
   };
   const char *long_opt_description[] =
   {
         "Print this help and exit.",
         "(default=2) Listen Port.",
         "(default=<none>) List of comma separated peer port numbers (Max = 10)."
   };

   px_raft_args->listenPort = RAFT_DEFAULT_LISTEN_PORT;

   ui_opts_count = sizeof(long_opt) / sizeof (struct option);
   while ((c = getopt_long(argc, argv, short_opt, long_opt, NULL)) != -1) {
      switch (c) {
      case -1: /* no more arguments */
      case 0: /* long options toggles */
         break;

      case 'h':
         raft_print_usage(long_opt, long_opt_description, ui_opts_count, argv);
         exit (0);
         break;
      case 'l':
         printf("you entered \"%s\"\n", optarg);
         px_raft_args->listenPort = atoi (optarg);
         if (px_raft_args->listenPort < 1024 ||
               px_raft_args->listenPort > 65535) {
            px_raft_args->listenPort = RAFT_DEFAULT_LISTEN_PORT;
         }
         break;
      case 'p':
      {
         printf("you entered \"%s\"\n", optarg);

         std::string s = optarg;
         std::string delimiter = ",";
         size_t pos = 0;
         std::string token;
         while ((pos = s.find(delimiter)) != std::string::npos) {
             token = s.substr(0, pos);
             std::cout << "Peer port: " << token << std::endl;

             px_raft_args->peerPorts [px_raft_args->numPorts] = atoi (token.c_str());
             if (px_raft_args->peerPorts [px_raft_args->numPorts] < 1024 ||
                   px_raft_args->peerPorts [px_raft_args->numPorts] > 65535) {
                printf ("Invalid port range!\n");
                raft_print_usage(long_opt, long_opt_description, ui_opts_count, argv);
                exit (0);
             }
             px_raft_args->numPorts++;

             s.erase(0, pos + delimiter.length());

         }
         std::cout << s << std::endl;
         px_raft_args->peerPorts [px_raft_args->numPorts] = atoi (s.c_str());
         if (px_raft_args->peerPorts [px_raft_args->numPorts] < 1024 ||
               px_raft_args->peerPorts [px_raft_args->numPorts] > 65535) {
            printf ("Invalid port range!\n");
            raft_print_usage(long_opt, long_opt_description, ui_opts_count, argv);
            exit (0);
         }
         px_raft_args->numPorts++;

         break;
      }
      default:
         fprintf(stderr, "%s: invalid option -- %c\n", argv[0], c);
         fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
      };
   };

   if (!raft_is_opts_from_args_valid (px_raft_args)) {
      raft_print_usage(long_opt, long_opt_description, ui_opts_count, argv);
      exit (0);
   }
}

static void onMessage (client_ctxt *client, uint8_t *message, uint32_t length, void *this_)
{
   LOG << "New Heartbeat Message" << std::endl;
   LOG << "Read " << length << " bytes from socket." << std::endl;
}

static void onLocalEvent (void *this_)
{
   client_ctxt *client = (client_ctxt *) this_;
   if (client->disconnected) return;
   TcpServer *server = client->tcpServer;
   LOG << "onLocalEvent" << std::endl;

   RAFT_MSG_HEARTBEAT_X x_hb = {{0}};

   raft::Header *header = new raft::Header();
   header->set_type(raft::MailType::REQUEST);
   header->set_id("Random String");
   header->set_mails(0, raft::Mails::HEARTBEAT);
   raft::Mail *mail = new raft::Mail();
   mail->set_allocated_header(header);

   raft::HeartBeat *hb = new raft::HeartBeat();
   hb->

   bufferevent_write (client->client_bev, (const void *) &x_hb, sizeof (x_hb));

   struct timeval tv;
   tv.tv_sec = 3;
   tv.tv_usec = 0;
   server->newLocalEvent(onLocalEvent, client, tv);
}

static void onConnection (client_ctxt *client, void *this_)
{
   TcpServer *server = (TcpServer *) this_;
   LOG << "New Connection" << std::endl;
   struct timeval tv;

   tv.tv_sec = 3;
   tv.tv_usec = 0;
   server->newLocalEvent(onLocalEvent, client, tv);
}

static void onDisconnect (client_ctxt *client, short what, void *this_)
{
	LOG << "onDisconnect" << std::endl;
}

int main (int argc, char **argv)
{
   uint32_t i = 0;
   int fd = -1;
   struct sockaddr_in *client_addr;

   RAFT_ARGS_X *px_raft_args = (RAFT_ARGS_X *) malloc (sizeof (RAFT_ARGS_X));
   memset (px_raft_args, 0x00, sizeof (*px_raft_args));
   raft_get_opts_from_args (argc, argv, px_raft_args);


   event_set_log_callback(event_log_cbk);

   TcpServer *server = new TcpServer (INADDR_ANY, px_raft_args->listenPort);
   server->OnNewMessage(onMessage, NULL);
   server->OnNewConnection(onConnection, server);
   server->OnPeerDisconnect(onDisconnect, server);
   server->start();

   for (i = 0; i < px_raft_args->numPorts; i++)
   {
      fd = socket(AF_INET, SOCK_STREAM, 0);
      if (fd < 0) {
         printf("socket failed");
         exit (-1);
      }

      printf ("Connecting to %d\n", px_raft_args->peerPorts [i]);

      client_addr = (struct sockaddr_in *) malloc (sizeof (struct sockaddr_in));
      memset (client_addr, 0x00, sizeof (*client_addr));
      client_addr->sin_addr.s_addr = INADDR_ANY;
      client_addr->sin_port = htons(px_raft_args->peerPorts [i]);
      client_addr->sin_family = AF_INET;
      if (connect(fd, (struct sockaddr *) client_addr,
              sizeof(struct sockaddr)) < 0) {
         perror ("Connection failed.\n");
         exit (0);
      }

      printf ("Connection successful.\n");
      server->handleConnection(fd, client_addr);
   }

   std::chrono::milliseconds ms(1000);
   while (true) {
      std::this_thread::sleep_for(ms);
   }

   return 0;

   return 0;
}
