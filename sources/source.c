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
char desc_str[] = "\r\nITProject RFID WiegandController 1.0.0.0 <br>  Компания ООО АйТиПроект-программные решения <br> (ITProject-software solutions), 2018. Все права защищены.\r\n";
char cyr_str[sizeof(desc_str)];
void  solve_are_bytes_in_hex(  struct mg_connection *nc, int ev, void *ev_data);
void solve_ok_req(  struct mg_connection *nc, int ev, void *ev_data);
void solve_wrong_data_names(  struct mg_connection *nc, int ev, void *ev_data);
void solve_wrong_data_ports(  struct mg_connection *nc, int ev, void *ev_data);
void solve_wrong_bytes_legth(  struct mg_connection *nc, int ev, void *ev_data);
void solve_wrong_protocol_req(  struct mg_connection *nc, int ev, void *ev_data);


int is_hex_str(char* input, int* num) {
  int is_hex = 1;
  int i = 0;
  char ch;
  ch = *input;

  // counting of hexadecimal numbers 
  while (ch != '\0') {
    ch = input[i];
    if (isxdigit(ch)) {
      ++(*num);
    } 
    else if (ch == '\0') break;
    else {
      is_hex = 0;
    }
    i++;
  }
  return is_hex;
}


void hex_binary(const char* input, char * res) {
   char binary[16][5] = { "0000", "0001", "0010", "0011", "0100", "0101","0110", "0111", "1000", "1001", "1010", "1011", "1100", "1101", "1110","1111" };
   char digits[] = "0123456789abcdef";


   res[0] = '\0';
   int p = 0;
   int value = 0;
   while (input[p])
   {
      const char *v = strchr(digits, tolower(input[p]));
      if (v[0] > 96) {
         value = v[0] - 87;
      }
      else {
         value = v[0] - 48;
      }
      if (v) {
         strcat(res, binary[value]);
      }
      p++;
   }
}
int parity_check_2(char* num) {
   int counter = 0;
   for (size_t i = 0; i < strlen(num); i++)
   {
      if (num[i] == '1')
      {
         ++counter;
      }
   }
   if (counter % 2 == 0)
   {
      return 1;
   }
   return 0;
}
int parity_check_1(char* num) {
   int counter = 0;
   for (size_t i = 0; i < strlen(num); i++)
   {
      if (num[i] == '1')
      {
         ++counter;
      }
   }
   if (counter % 2 == 0)
   {
      return 0;
   }
   return 1;
}

int binaryToDecimal(char* n)
{
   int dec_value = 0;
   // Initializing base value to 1, i.e 2^0 
   int base = 1;

   int len = strlen(n);
   for (int i = len - 1;i >= 0;i--)
   {
      if (n[i] == '1')
         dec_value += base;
      base = base * 2;
   }
   return dec_value;
}
char* bin_conv(char* s) {
   
   size_t l = strlen(s);
   char* r = (char*)malloc((l + 1) * sizeof(char));
   r[l] = '\0';
   int i;
   for (i = 0; i < l; i++) {
      r[i] = s[l - 1 - i];
   }
   return r;
}


int get_dec_code(char* hex) {
   char *bin = malloc(sizeof(char) * 25);
   char *bin_part_1 = malloc(sizeof(char) * 13);
   char *bin_part_2 = calloc(13, sizeof(char));

   hex_binary(hex, bin);

   for (size_t i = 0; i < 12; i++)
   {
      bin_part_1[i] = bin[i];
   }
   bin_part_1[12] = '\0';
   for (size_t i = 12; i < 25; i++)
   {
      bin_part_2[i-12] = bin[i];
   }
   bin_part_2[12] = '\0';
   char *res_bin = malloc(sizeof(char) * 27);
   char parity[2];
   snprintf(parity, sizeof(parity), "%d", parity_check_1(bin_part_1));
   strcpy(res_bin, parity );
   strcat(res_bin, bin_part_1);
   strcat(res_bin, bin_part_2);
   snprintf(parity, sizeof(parity), "%d", parity_check_2(bin_part_2));
   strcat(res_bin, parity);

   int result = binaryToDecimal(res_bin);
   free(bin); free(res_bin);
   free(bin_part_1); free(bin_part_2);
   return  result;
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
       char* splitted_array = NULL;
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
        int is_hex_err = 0;
        int numofhex = 0;
        int is_hex = 0;
        is_hex = is_hex_str( hexstring, &numofhex);
        if (numofhex != 6  && is_hex == 1)
        {
           solve_wrong_bytes_legth(nc, ev, ev_data);
           strcpy(req_to_log, "400 check bytes length");
        }else if( is_hex == 0){
                 solve_are_bytes_in_hex(nc, ev, ev_data);  
                 strcpy(req_to_log, "400 check are bytes in 6 lengthed hex");
        }else {
                if (is_hex_err == 0 && optGpioGreen >= 0 && optGpioWhite >= 0 
                        && is_data0_in_req && is_data1_in_req  )
                {

                   send_data(optGpioGreen, optGpioWhite, hexstring);
                   solve_ok_req(nc, ev, ev_data);
                   strcpy(req_to_log, "200 OK");
                }
               else if(optGpioGreen < 0 || optGpioWhite < 0  ){
                    solve_wrong_data_ports(nc, ev, ev_data);
                    strcpy(req_to_log, "400 data0=x & data1 = y: x and y values are not valid");
           
                    } else  {    
                          solve_wrong_data_names(nc, ev, ev_data);
                          strcpy(req_to_log, "400 check data0=X&data1=Y data names ");
                        }
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

   
  void send_data(int optGpioGreen, int optGpioWhite, char* hexcode)
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
            
           
            printf("sending hex code %s\n", hexcode);
            send_wiegand_code(pi, get_dec_code (  hexcode), optSize);  
       
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
              mg_printf(nc, "%s", cyr_str);
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