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

#include <pigpiod_if2.h>

/*

This programs transmits Wiegand codes on a pair of GPIO.  It is
intended to be used to test Wiegand decoding software.

REQUIRES

Nothing

TO BUILD

gcc -Wall -pthread -o tx_WD tx_WD.c -lpigpiod_if2

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

*/

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

int optGpioGreen = -1;
int optGpioWhite = -1;
int optSize = 26;
int optPulseLen = 100;
int optBitGap = 1000;
int optMsgGap = 20000;
int optRepeats = 1;
char *optHost   = NULL;
char *optPort   = NULL;

int wid0, wid1;

static uint64_t getNum(char *str, int *err)
{
   uint64_t val;
   char *endptr;

   *err = 0;

   val = strtoll(str, &endptr, 0);
   if (*endptr) {*err = 1; val = -1;}
   return val;
}

static void initOpts(int argc, char *argv[])
{
   int opt, err, i;

   while ((opt = getopt(argc, argv, "g:w:s:r:m:i:l:h:p:")) != -1)
   {
      switch (opt)
      {
         case 'g':
            i = getNum(optarg, &err);
            if ((i >= 0) && (i <= 31)) optGpioGreen = i;
            else fatal("invalid -g option (%s)", optarg);
            break;

         case 'w':
            i = getNum(optarg, &err);
            if ((i >= 0) && (i <= 31)) optGpioWhite = i;
            else fatal("invalid -w option (%s)", optarg);
            break;

         case 's':
            i = getNum(optarg, &err);
            if ((i >= 8) && (i <= 63)) optSize = i;
            else fatal("invalid -s option (%s)", optarg);
            break;

         case 'r':
            i = getNum(optarg, &err);
            if ((i >= 1) && (i <= 65535)) optRepeats = i;
            else fatal("invalid -r option (%s)", optarg);
            break;

         case 'm':
            i = getNum(optarg, &err);
            if ((i >= 1000) && (i <= 65000)) optMsgGap = i;
            else fatal("invalid -m option (%s)", optarg);
            break;

         case 'i':
            i = getNum(optarg, &err);
            if ((i >= 50) && (i <= 20000)) optBitGap = i;
            else fatal("invalid -i option (%s)", optarg);
            break;

         case 'l':
            i = getNum(optarg, &err);
            if ((i >= 5) && (i <= 400)) optPulseLen = i;
            else fatal("invalid -s option (%s)", optarg);
            break;

         case 'h':
            optHost = malloc(sizeof(optarg)+1);
            if (optHost) strcpy(optHost, optarg);
            break;

         case 'p':
            optPort = malloc(sizeof(optarg)+1);
            if (optPort) strcpy(optPort, optarg);
            break;

        default: /* '?' */
           usage();
           exit(-1);
        }
    }
}

void send_wiegand_code(
   int pi, long long code, int bits)
{
   int i, pos=0;
   char chain[128];

   chain[pos++]= 255;
   chain[pos++] = 0; /*Start of wave block. */

   for (i=0; i<bits; i++)
   {
      if (code & ((uint64_t)1<<(bits-1-i)))
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

int main(int argc, char *argv[])
{
   int pi, arg, err;
   long long code;

   initOpts(argc, argv);

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

      for (arg=optind; arg<argc; arg++)
      {
         code = getNum(argv[arg], &err);
         printf("sending %llu/%d\n", code, optSize);
         send_wiegand_code(pi, code, optSize);
         time_sleep(1.0);
      }

      wave_delete(pi, wid0);
      wave_delete(pi, wid1);

      pigpio_stop(pi);
   }
   return 0;
}

