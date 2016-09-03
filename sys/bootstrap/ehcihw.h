#ifndef _ehcihw_h
#define _ehcihw_h

struct ehcibase {
	uint8_t caplength;
	uint8_t res1;
	uint16_t hciversion;
	uint32_t hcsparams;
	uint32_t hccparams;
	uint64_t hcsp_portroute;
} __packed;

struct ehciopreg {
	uint32_t usbcmd;
	uint32_t usbsts;
	uint32_t usbintr;
	uint32_t frindex;
	uint32_t ctrldssegment;
	uint32_t periodiclistbase;
	uint32_t asynclistaddr;
	uint32_t res1[9];
	uint32_t configflags;
	uint32_t portsc[0];
} __packed;

#endif
