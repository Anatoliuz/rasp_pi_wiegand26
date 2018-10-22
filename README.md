raspberry pi 3 b+ with http server which receives 3 bytes via http request and transmitts them to wiegand26 reader
=========


Very simple raspberry pi 3 b+ code with http server which receives 3 bytes via http request and transmitts them to wiegand26 reader


*Absolute URL:*
scheme ":" [ "//" ] [ username ":" password "@" ] host [ ":" port ] [ "/" ] [ path ] [ "?" query ] [ "#" fragment ]

*Relative URL:*
path [ "?" query ] 

Parts within `[` and `]` are optional. A minimal URL could look like this:

`a:b` or `/`

Due to the fact that the library isn't copying any strings and instead points
to the parts in the URL string, the first `/` in the path will be replaced with
a null terminator. Therefore, the first slash will be missing in the path.

## To build

```sh
$ gcc -Wall -pthread  wiegand26_serv.c mongoose.* encoding_* -o serv  -lpigpiod_if2 -lyuarel
```

## Try it

Compile the example in `examples/`:

```sh
$ ./serv
```

Run  test:

```sh
$ python test.py
```

Get req example


```sh
$ http://192.168.15.116:8000/wiegand26?data0=5&data1=6&code=AABBCC
```


