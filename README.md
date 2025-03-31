# uevent
> A epoll event lib from [OpenWrt libubox](https://github.com/openwrt/libubox.git)

## Build
```bash
$ make clean && make

```

## Run

### Server
```bash
$ ./example-server
New connection
Block read: ABCDEFGH, bytes: 9
Block read: 0123456789, bytes: 11

```

### Client
```bash
$ ./example-client
Connection established
Input message to send (or 'quit' to exit), end with 'ENTER'.
ABCDEFGH
Wrote 9 bytes to server.
Read 9 bytes from server:
ABCDEFGH
0123456789
Wrote 11 bytes to server.
Read 11 bytes from server:
0123456789

```