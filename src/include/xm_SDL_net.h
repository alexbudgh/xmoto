#ifndef __XM_SDL_NET_H__
#define __XM_SDL_NET_H__

#if ENABLE_NETWORKING
#error "SDL_net has been removed. Networking is currently disabled."
#else
/* Stub types so net headers compile when networking is disabled */
typedef void *TCPsocket;
typedef void *UDPsocket;
typedef void *SDLNet_SocketSet;
struct UDPpacket { int channel; unsigned char *data; int len; int maxlen; int status; };
struct IPaddress { unsigned int host; unsigned short port; };
#endif

#endif
