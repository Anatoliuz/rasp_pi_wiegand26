#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h> 
#include <sys/socket.h>
#include <time.h>
#include <locale.h>
#include <pigpiod_if2.h>
#include <yuarel.h>
 
#include "mongoose.h"

#define PORT 8000 
#define version 0.1
#define BYTES_NUM 3

static const char *s_http_port = "8000";


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

char* get_full_url(struct http_message *hm, struct mg_connection *nc);

void solve_empty_req(struct mg_connection *nc, int ev, void *ev_data);
struct yuarel_param * parse_params(char url_string[]);
void send_wiegand_code(
  int pi, unsigned int code, int bits);
char desc_str[] = "\r\nITProject RFID NetduinoRelay %g <br>  Компания ООО АйТиПроект-программные решения <br> (ITProject-software solutions), 2018. Все права защищены.\r\n";
char cyr_str[sizeof(desc_str)];
void  solve_are_bytes_in_hex(  struct mg_connection *nc, int ev, void *ev_data);
void solve_ok_req(  struct mg_connection *nc, int ev, void *ev_data);
void solve_wrong_data_names(  struct mg_connection *nc, int ev, void *ev_data);
void solve_wrong_data_ports(  struct mg_connection *nc, int ev, void *ev_data);
void solve_wrong_bytes_legth(  struct mg_connection *nc, int ev, void *ev_data);
void solve_wrong_protocol_req(  struct mg_connection *nc, int ev, void *ev_data);

char* split_hex_str(char *hexstring, char** pEnd){
   char* splitted;
    splitted = malloc(3*sizeof(char));  
    int i = 0;
    while(*pEnd != '\0' && i < 2) {
      splitted[i] = **pEnd;
     
      (*pEnd)++;
      ++i;
    } 
    splitted[2] = '\0';
    return splitted;
}


void write_log(char* url, char* response){
   FILE* fp = fopen("log.txt","a"); 
   fseek(fp, 0L, SEEK_END);

   if(fp == NULL){
       fprintf(stderr, "errno:%s - opening log failed\n ", strerror(errno));
       close(fp);
       exit(1);
   }else{
       if ( ftell(fp) > 16384 ){
            if (remove("log.txt") == 0) {
                 printf("Deleted successfully"); 
                 fp = fopen("log.txt","w");
                 } 
             else
                 printf("Unable to delete the file"); 
       }

        char buff[40];
        struct tm *sTm;
        char* temp;
        time_t now = time (0);
        sTm = gmtime (&now);
    
        strftime (buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", sTm);
        
        temp = calloc(strlen(buff) + strlen(url) + strlen(response) + 60  , sizeof(char));
        strcat(temp, buff);
        strcat(temp, "       ");
        strcat(temp, url);
        strcat(temp, "     response is    ");
        strcat(temp, response);
        strcat(temp, "\n");

        int results = fputs(temp, fp);
        free(temp);

        if (results == EOF) {
                    printf("%s", "failed to write");
            }
        }
    
        fclose(fp);
}

int create_log(){
  FILE *fp = fopen("log.txt","a"); 
}


void solve_wiegand_request(struct mg_connection *nc, int ev, void *ev_data){
       bool is_data0_in_req = 0;
       bool is_data1_in_req = 0;
       struct http_message *hm = (struct http_message *) ev_data;
       char *hexstring;
       struct yuarel_param* parsed_params;
       char* url = get_full_url(hm, nc);
        char* url_to_log = calloc(strlen(url) + 2, sizeof(char));
       strcpy(url_to_log, url);
       char* splitted_array;
       char *splitted_array_ptr;

       parsed_params =   parse_params( url );

       char* req_to_log = malloc(sizeof(char) * 100);
       for (int i = 0; i < 3; ++i)
       {
           if (strcmp(parsed_params[i].key, "data0") == 0)
                 {
                   is_data0_in_req = 1;
                   optGpioGreen = atoi(parsed_params[i].val);
                 }
           else if(strcmp(parsed_params[i].key, "data1") == 0)
                 {
                   is_data1_in_req = 1;
                   optGpioWhite = atoi(parsed_params[i].val);
                 }
            else if(strcmp(parsed_params[i].key, "code") == 0)
                 {
                      hexstring = parsed_params[i].val;
                 }                
       }

        if (strlen(hexstring) == 6){
               char* pEnd;
               bool is_hex_err = 0;
               pEnd = hexstring;
               long int longs[3];
               for (int i = 0; i < 3; ++i)
               {
                   splitted_array = split_hex_str(pEnd, &pEnd);
                   splitted_array_ptr = splitted_array;
                  // printf("splitted %p %s\n",pEnd, splitted_array);
                   longs[i]  = strtol (splitted_array,&splitted_array,16);
                   //printf("%ld\n",  longs[i]);
                    free(&(*splitted_array_ptr));

                   if (  longs[i] <= 0 )
                   {
                     is_hex_err = 1;
                   }
               }
                if (is_hex_err == 0 && optGpioGreen >= 0 && optGpioWhite >= 0 
                        && is_data0_in_req && is_data1_in_req )
                {
                  send_data(optGpioGreen, optGpioWhite, longs);
                   solve_ok_req(nc, ev, ev_data);
                   strcpy(req_to_log, "200 OK");
                }
               else if(optGpioGreen < 0 || optGpioWhite < 0){
                    solve_wrong_data_ports(nc, ev, ev_data);
                    strcpy(req_to_log, "400 data0=x & data1 = y: x and y values are not valid");
           
                    } else  if(is_data0_in_req == 0 || is_data1_in_req == 0) {    
                          solve_wrong_data_names(nc, ev, ev_data);
                          strcpy(req_to_log, "400 check data0=X&data1=Y data names ");
                        }else{ 
                             solve_are_bytes_in_hex(nc, ev, ev_data);  
                             strcpy(req_to_log, "400 check are bytes in hex");
                            }
                    }               

        else {
          solve_wrong_bytes_legth(nc, ev, ev_data);
           strcpy(req_to_log, "400 check bytes length");
        }

       write_log(url_to_log, req_to_log);
       free(&(*parsed_params));
       free(&(*url));
       free(&(*url_to_log));
       free(&(*req_to_log));
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
                       break;
                       }
                    else if (mg_vcmp(&hm->uri, "/wiegand26") == 0) {
                         solve_wiegand_request(nc, ev, ev_data);
                         printf("wiegand\n");
                         break;
                       }
                    else if (mg_vcmp(&hm->uri, "/favicon.ico") == 0) {
                         break;
                       }

                    else if (mg_vcmp(&hm->uri, "/wiegand26") != 0) {
                          solve_wrong_protocol_req(nc, ev, ev_data);
                          printf("wrong protocol\n");
                          break;
                       }
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
  create_log();
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

   
  void send_data(int optGpioGreen, int optGpioWhite, long int* bytes)
  {
              int pi;
        
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
                    send_wiegand_code(pi,  bytes[index], optSize);  
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

            p = yuarel_parse_query(url.query, '&', params, 3);
            while (p-- > 0) {
             //printf("\t%s: %s\n", params[p].key, params[p].val);
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
              char *url = calloc(sizeof(char) , 250);         
              char start[] = "http://";
              strcat( url, start );
              char addr[32];
              mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                                      MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
              char* uri   = get_uri(hm);
              char* query = get_query(hm);
              strcat( url, addr );
              strcat( url, uri );
              strcat( url, "?" );
              strcat( url,  query);
              free(&(*uri));
              free(&(*query));
              return url;
}
void solve_empty_req(struct mg_connection *nc, int ev, void *ev_data){
             UTF8ToWin1251(desc_str, cyr_str, sizeof(desc_str));
            
            mg_send_response_line(nc, 200,
                                   "Content-Type: text/html\r\n"
                                   "Connection: close");
              mg_printf(nc, cyr_str, version);
              nc->flags |= MG_F_SEND_AND_CLOSE;
       
}

void solve_wrong_protocol_req(  struct mg_connection *nc, int ev, void *ev_data){
   mg_send_response_line(nc, 400,
                                   "Content-Type: text/html\r\n"
                                   "Connection: close");
                        mg_printf(nc, "HTTP/1.0 400 err\r\n\r\ncheck protocol name");
                   nc->flags |= MG_F_SEND_AND_CLOSE;
}


void solve_wrong_bytes_legth(  struct mg_connection *nc, int ev, void *ev_data){
   mg_send_response_line(nc, 400,
                                         "Content-Type: text/html\r\n"
                                         "Connection: close");
                              mg_printf(nc, "HTTP/1.0 400 err\r\n\r\ncheck num of bytes");
                               nc->flags |= MG_F_SEND_AND_CLOSE;
}

void solve_wrong_data_ports(  struct mg_connection *nc, int ev, void *ev_data){
  mg_send_response_line(nc, 400,
                                         "Content-Type: text/html\r\n"
                                         "Connection: close");
                              mg_printf(nc, "HTTP/1.0 400 err\r\n\r\ncheck data0 & data1 ports");
                               nc->flags |= MG_F_SEND_AND_CLOSE;
}

void solve_wrong_data_names(  struct mg_connection *nc, int ev, void *ev_data){
  mg_send_response_line(nc, 400,
                                         "Content-Type: text/html\r\n"
                                         "Connection: close");
                              mg_printf(nc, "HTTP/1.0 400 err\r\n\r\nncheck data0 & data1 in req");
                               nc->flags |= MG_F_SEND_AND_CLOSE;
}
void solve_ok_req(  struct mg_connection *nc, int ev, void *ev_data){
  mg_send_response_line(nc, 200,
                                   "Content-Type: text/html\r\n"
                                   "Connection: close");
                        mg_printf(nc, "HTTP/1.0 200 OK\r\n\r\nOK");
                        nc->flags |= MG_F_SEND_AND_CLOSE;

}
void  solve_are_bytes_in_hex(  struct mg_connection *nc, int ev, void *ev_data){
  mg_send_response_line(nc, 400,
                                         "Content-Type: text/html\r\n"
                                         "Connection: close");
                              mg_printf(nc, "HTTP/1.0 400 err\r\n\r\ncheck are bytes in hex");
                               nc->flags |= MG_F_SEND_AND_CLOSE;
}