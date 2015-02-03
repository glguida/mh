#ifndef _ukern_usrentry_h
#define _ukern_usrentry_h

#include <machine/uk/types.h>

void __usrentry_setup(struct usrentry *ue, vaddr_t ip);
void __usrentry_enter(void *frame);


void usrentry_setup(struct usrentry *ue, vaddr_t ip);
void usrentry_enter(struct usrentry *ue);
struct usrentry *usrentry_leave(void);

#endif
