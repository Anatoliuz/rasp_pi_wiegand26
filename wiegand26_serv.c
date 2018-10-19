/*
tx_WD.c
2015-11-25
Public Domain
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include <netinet/in.h> 
#include <sys/socket.h>

#include <pigpiod_if2.h>
#include <yuarel.h>
 
#include "mongoose.h"
#include <locale.h>

#define PORT 8000 
#define version 0.1
#define BYTES_NUM 3
static const char *s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;

/*

This programs transmits Wiegand codes on a pair of GPIO.  It is
intended to be used to test Wiegand decoding software.

REQUIRES

Nothing

TO BUILD

gcc -Wall -pthread -o tx_WD tx_WD.c -lpigpiod_if2
gcc -Wall -pthread  wiegand26_serv.c mongoose.* -o serv  -lpigpiod_if2 -lyuarel
http_client.c -o serv 


TO RUN

sudo pigpiod # If the daemon is not already running

then

./tx_WD -gGPIO -wGPIO [options] {code}+

./tx_WD -? for options

E.g.

./tx_WD -g5 -w6 -s37 12345 67890 123 899999

NOTE

This code sets the used GPIO to be outputs.  If you intend to feed the
signals into a program under test it is probably best to use different
GPIO in the two programs and connect them by wires.


gcc -Wall -pthread  wiegand26_serv.c mongoose.* -o serv  -lpigpiod_if2 -lyuarel
*/


int optGpioGreen = -1;
int optGpioWhite = -1;
int optSize = 26;
int optPulseLen = 50;
int optBitGap = 500;
int optMsgGap = 20000;
int optRepeats = 1;
char *optHost   = NULL;
char *optPort   = NULL;

int wid0, wid1;

static uint64_t getNum(char *str, int *err);
void usage();
void fatal(char *fmt, ...);
char* get_full_url(struct http_message *hm, struct mg_connection *nc);
char* get_uri(struct http_message *hm);
char* get_query(struct http_message *hm);
struct yuarel_param * parse_params(char url_string[]);
void send_wiegand_code(
   int pi, unsigned int code, int bits);
char desc_str[] = "\r\nITProject RFID NetduinoRelay %g <br>  Компания ООО АйТиПроект-программные решения (ITProject-software solutions), <br>2018. Все права защищены.\r\n";
char cyr_str[sizeof(desc_str)];

void solve_empty_req(struct mg_connection *nc, int ev, void *ev_data){
             UTF8ToWin1251(desc_str, cyr_str, sizeof(desc_str));
             char addr[32];
            mg_send_response_line(nc, 200,
                                   "Content-Type: text/html\r\n"
                                   "Connection: close");
              mg_printf(nc, cyr_str, version);
              nc->flags |= MG_F_SEND_AND_CLOSE;
       
}

void solve_wiegand_request(struct mg_connection *nc, int ev, void *ev_data){

             struct http_message *hm = (struct http_message *) ev_data;

               struct yuarel_param* parsed_params;
                unsigned int bytearray[BYTES_NUM];

                printf("Full url %s\n",  get_full_url(hm, nc));

                parsed_params =   parse_params( get_full_url(hm, nc) );
                for (int i = 0; i < 3; ++i)
            {
                 printf("\t%s: %s\n", parsed_params[i].key, parsed_params[i].val);
                 if (strcmp(parsed_params[i].key, "data0") == 0)
                 {
                   optGpioGreen = atoi(parsed_params[i].val);
                   printf("green: %d\n", optGpioGreen);
       
                 }
                 else if(strcmp(parsed_params[i].key, "data1") == 0)
                 {
                   optGpioWhite = atoi(parsed_params[i].val);
                   printf("white: %d\n", optGpioWhite);
       
                 }
                 else if(strcmp(parsed_params[i].key, "code") == 0)
                 {
                       char *hexstring = parsed_params[i].val;
                      int i;
                      uint8_t str_len = strlen(hexstring);
         
                      for (i = 0; i < (str_len / 2); i++) {
                           sscanf(hexstring + 2*i, "%02x", &bytearray[i]);
                           printf("bytearray %d: %02x\n", i, bytearray[i]);
                      }
                 }
            }
                
                if (optGpioGreen >= 0 && optGpioWhite >= 0)
                {
                        send_data(optGpioGreen, optGpioWhite, bytearray);
                        mg_send_response_line(nc, 200,
                                   "Content-Type: text/html\r\n"
                                   "Connection: close");
                        mg_printf(nc, "HTTP/1.0 200 OK\r\n\r\nOK");
                        nc->flags |= MG_F_SEND_AND_CLOSE;
                }
                else{
                    mg_send_response_line(nc, 400,
                                   "Content-Type: text/html\r\n"
                                   "Connection: close");
                        mg_printf(nc, "HTTP/1.0 400 err\r\n\r\ncheck ports");
                   nc->flags |= MG_F_SEND_AND_CLOSE;
                }

               

}
static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
        struct http_message *hm = (struct http_message *) ev_data;
         switch (ev) {
           case MG_EV_ACCEPT: {
             char addr[32];
             mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                                 MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
             printf("%p: Connection from %s\r\n", nc, addr);
             break;
           }
           case MG_EV_HTTP_REQUEST: {
                    if (mg_vcmp(&hm->uri, "/") == 0) {
                       solve_empty_req(nc, ev, ev_data);
                       printf("IMHERE\n");
                       break;
                       }
                    else if (mg_vcmp(&hm->uri, "/wiegand26") == 0) {
                         solve_wiegand_request(nc, ev, ev_data);
                                                printf("wiegand\n");
                         break;
                       }
                   
                  
            
       
           //                if(strcmp(get_uri(hm) , "/favicon.ico") == 0 ){
           //                mg_printf(nc, "\r\nBA");
           //                mbuf_remove(&nc->recv_mbuf, nc->recv_mbuf.len);
           //                nc->flags |= MG_F_SEND_AND_CLOSE;
           //                break;
           //             }
           //       /*
             /*if (strcmp( get_uri ( hm ), "/wiegand26") != 0 && strcmp(get_uri(hm) , "/") != 0){
                mg_printf(nc, "\r\nCHECK PROTOCOL NAME\r\n");
                nc->flags |= MG_F_SEND_AND_CLOSE;
                break;
             }
       */

            // parsed_params =   parse_params( get_full_url(hm, nc) );
           /*  
            for (int i = 0; i < 3; ++i)
            {
                 printf("\t%s: %s\n", parsed_params[i].key, parsed_params[i].val);
                 if (strcmp(parsed_params[i].key, "data0") == 0)
                 {
                   optGpioGreen = atoi(parsed_params[i].val);
                   printf("green: %d\n", optGpioGreen);
       
                 }
                 else if(strcmp(parsed_params[i].key, "data1") == 0)
                 {
                   optGpioWhite = atoi(parsed_params[i].val);
                   printf("white: %d\n", optGpioWhite);
       
                 }
                 else if(strcmp(parsed_params[i].key, "code") == 0)
                 {
                       char *hexstring = parsed_params[i].val;
                      int i;
                      uint8_t str_len = strlen(hexstring);
         
                      for (i = 0; i < (str_len / 2); i++) {
                           sscanf(hexstring + 2*i, "%02x", &bytearray[i]);
                           printf("bytearray %d: %02x\n", i, bytearray[i]);
                      }
                 }
            }
            */
              if (optGpioGreen >= 0 && optGpioWhite >= 0)
                {
//                  mg_printf(nc, "\r\nOK\r\n");
//                  send_data(optGpioGreen, optGpioWhite, bytearray);
//                   mbuf_remove(&nc->recv_mbuf, nc->recv_mbuf.len);
//                   nc->flags |= MG_F_SEND_AND_CLOSE;
//                 break;
//       
                }
                else{
                 //mg_printf(nc, "\r\nCHECK data0=x && data1=\r\n");
               //  nc->flags |= MG_F_SEND_AND_CLOSE;
                // break;
                }
         //    nc->flags |= MG_F_SEND_AND_CLOSE;
           }
           case MG_EV_CLOSE: {
             printf("%p: Connection closed\r\n", nc);
             break;
           }
             default:
                 break;
         }
       }
       
    


int main(int argc, char *argv[])
{
  struct mg_mgr mgr;
  struct mg_connection *nc;

  mg_mgr_init(&mgr, NULL);
  printf("Starting web server on port %s\n", s_http_port);
  nc = mg_bind(&mgr, s_http_port, ev_handler);
  if (nc == NULL) {
    printf("Failed to create listener\n");
    return 1;
  }

  mg_set_protocol_http_websocket(nc);

  for (;;) {
    mg_mgr_poll(&mgr, 1000);
  }
  mg_mgr_free(&mgr);

   return 0;
}

   
  void send_data(int optGpioGreen, int optGpioWhite, unsigned int* bytes)
  {
            
            
                    int pi, arg, err;
                    long long code;
                
                
                   if ((optGpioGreen < 0) ||
                       (optGpioWhite < 0) ||
                       (optGpioGreen == optGpioWhite))
                   {
                      fprintf(stderr, "You must specify GPIO A and GPIO B.\n");
                      exit(0);
                    }
            
               pi = pigpio_start(optHost, optPort); /* Connect to Pi. */
            
               if (pi >= 0)
               {
                  set_mode(pi, optGpioGreen, PI_OUTPUT);
                  set_mode(pi, optGpioWhite, PI_OUTPUT);
            
                  wave_add_new(pi);
            
                  wave_add_generic(pi, 2, (gpioPulse_t[]){{0, 1<<optGpioGreen, optPulseLen},
                                                          {1<<optGpioGreen, 0, optBitGap}});
                  wid0 = wave_create(pi);
            
                  wave_add_generic(pi, 2, (gpioPulse_t[]){{0, 1<<optGpioWhite, optPulseLen},
                                                          {1<<optGpioWhite, 0, optBitGap}});
                  wid1 = wave_create(pi); 
                  
                
                  for (int index = 0; index < BYTES_NUM; index++) {
                      printf("sending %d/%d\n", bytes[index], optSize);
                     send_wiegand_code(pi,  bytes[index], optSize);
                     time_sleep(1.0);
                  }
              }
                  wave_delete(pi, wid0);
                  wave_delete(pi, wid1);
                  pigpio_stop(pi);
}

// default num of params is 3: two channels and data
struct yuarel_param * parse_params(char url_string[] ){
  int p;
  int num_params = 3;
  struct yuarel url;

  struct yuarel_param *params;
  params = malloc(num_params * sizeof(struct yuarel_param));

  if (-1 == yuarel_parse(&url, url_string)) {
    fprintf(stderr, "Could not parse url!\n");
    return 1;
  }

   printf("scheme:\t%s\n", url.scheme);
  printf("host:\t%s\n", url.host);
  printf("port:\t%d\n", url.port);
  printf("path:\t%s\n", url.path);
  printf("query:\t%s\n", url.query);
  printf("fragment:\t%s\n", url.fragment);


  p = yuarel_parse_query(url.query, '&', params, 3);
  while (p-- > 0) {
   printf("\t%s: %s\n", params[p].key, params[p].val);
  }
  return params;
}

void send_wiegand_code(
   int pi, unsigned int  code, int bits)
{
   int i, pos=0;
   char chain[128];

   chain[pos++]= 255;
   chain[pos++] = 0; /*Start of wave block. */

   for (i=0; i<bits; i++)
   {
      if (code & ((uint8_t)1<<(bits-1-i)))
         chain[pos++] = wid1;
      else
         chain[pos++] = wid0;
   }

   chain[pos++] = 255;
   chain[pos++] = 2; /* Delay optMsgGap. */
   chain[pos++] = optMsgGap & 255;
   chain[pos++] = (optMsgGap >> 8) & 255;

   chain[pos++] = 255;
   chain[pos++] = 1; /* Repeat optRepeats. */
   chain[pos++] = optRepeats & 255;
   chain[pos++] = (optRepeats >> 8) & 255;

   wave_chain(pi, chain, pos);

   while (wave_tx_busy(pi)) time_sleep(0.05);
}




char* get_query(struct http_message *hm){
  char *query_string = malloc(sizeof(char) * (int) hm->query_string.len + 1);        
    for(int i = 0; i < (int) hm->query_string.len; ++i){
        query_string[i] =  hm->query_string.p[i];
    }
    query_string[hm->query_string.len] = '\0';
    return query_string;
}



char* get_uri(struct http_message *hm){
  char *uri = malloc(sizeof(char) * (int) hm->uri.len + 1);    
      for(int i = 0; i < (int) hm->uri.len; ++i){
        uri[i] =  hm->uri.p[i];
    }
    uri[hm->uri.len] = '\0';
    return uri;
}

char* get_full_url(struct http_message *hm, struct mg_connection *nc){
  char *url = calloc(sizeof(char) , 1000);    

  char start[] = "http://";

  strcat( url, start );

  char addr[32];
  mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                          MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
  printf("  Addr:%s\n", addr);
  strcat( url, addr );
  strcat( url, get_uri(hm) );
  strcat( url, "?" );

  strcat( url, get_query(hm) );

  return url;
}

void fatal(char *fmt, ...)
{
   char buf[128];
   va_list ap;

   va_start(ap, fmt);
   vsnprintf(buf, sizeof(buf), fmt, ap);
   va_end(ap);

   fprintf(stderr, "%s\n", buf);

   fflush(stderr);

   exit(EXIT_FAILURE);
}

void usage()
{
   fprintf(stderr, "\n" \
      "Usage: tx_WD [OPTION] ...\n" \
      "   -g value, gpio green (0), 0-31,            default None\n" \
      "   -w value, gpio while (1), 0-31,            default None\n" \
      "   -s value, code size, 8-63,                 default 26\n" \
      "   -r value, repeats, 1-65535,                default 1\n" \
      "   -l value, pulse length, 5-400 micros,      default 50\n" \
      "   -i value, pulse interval, 50-20000 micros, default 500\n" \
      "   -g value, message gap, 1000-65000 micros,  default 20000\n" \
      "   -h string, host name,                      default NULL\n" \
      "   -p value, socket port, 1024-32000,         default 8888\n" \
      "EXAMPLE\n" \
      "tx_WD -g10 -w12  2345 1234\n" \
      "   Transmit (26 bit) Wiegand codes on GPIO 10/12.\n\n");
}
static uint64_t getNum(char *str, int *err)
{
   uint64_t val;
   char *endptr;

   *err = 0;

   val = strtoll(str, &endptr, 0);
   if (*endptr) {*err = 1; val = -1;}
   return val;
}

