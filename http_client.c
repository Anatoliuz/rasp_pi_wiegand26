/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 *
 * This program fetches HTTP URLs.
 */

#include "mongoose.h"
#include <yuarel.h>

#define APP_VERSION 0.1
static const char *s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;


int parse_data(char* uurl){
  int p;
  struct yuarel url;
  char *parts[3];
  struct yuarel_param *params;
  params = malloc(3 * sizeof(struct yuarel_param));

  char url_string[] = "http://192.168.15.116:8000/wiegand26?data0=0&data2=1&code=AABBCC";

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
}

char* get_query(struct http_message *hm){
  char *query_string = malloc(sizeof(char) * (int) hm->query_string.len + 1);        
    for(int i = 0; i < (int) hm->query_string.len; ++i){
        query_string[i] =  hm->query_string.p[i];
    }
    query_string[hm->query_string.len] = '\0';
    return query_string;
}

int get_g_port_num(char* query){
    for (int i = 0; query[i] != '\0'; ++i)
    {
      /* code */
    }

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
  char *url = malloc(sizeof(char) * 1000);    

  char start[] = "http://";

  strcat( url, start );

  char addr[32];
  mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                          MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
  strcat( url, addr );
  strcat( url, get_uri(hm) );
  strcat( url, "?" );

  strcat( url, get_query(hm) );

  printf("full URL:%s\n",  url);
  return url;
}
static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
  switch (ev) {
    case MG_EV_ACCEPT: {
      char addr[32];
      mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                          MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
      printf("%p: Connection from %s\r\n", nc, addr);
      break;
    }
    case MG_EV_HTTP_REQUEST: {
      struct http_message *hm = (struct http_message *) ev_data;
      char addr[32];
      mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                          MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
      printf("%p: %.*s %.*s\r\n", nc, (int) hm->method.len, hm->method.p,
             (int) hm->uri.len, hm->uri.p);

      mg_send_response_line(nc, 200,
                            "Content-Type: text/html\r\n"
                            "Connection: close");



      if (strcmp( get_uri ( hm ), "/wiegand26") != 0){
         mg_printf(nc, "\r\n<h1>CHECK protocol URI NAME wiegand26 ONLY supported</h1>\r\n");
         break;
      }
    
      char temp[] = "http://1http://192.168.15.116:8000/wiegand26?data0=0&data2=1&code=AABBCC";
      parse_data("");
      mg_printf(nc,
                "\r\n<h1>Hello, %s FROM APP VER. %g !</h1>\r\n"
                "You asked for %.*s\r\n",
                addr, APP_VERSION, (int) hm->query_string.len,  hm->query_string.p);
      nc->flags |= MG_F_SEND_AND_CLOSE;

      break;
    }
    case MG_EV_CLOSE: {
      printf("%p: Connection closed\r4\n", nc);
      break;
    }
  }
}

int main(int argc, char *argv[]) {
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
