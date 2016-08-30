#define VGA_VIDEO_MEM_BASE	0x0b8000
#define VGA_VIDEO_MEM_LENGTH	0x004000

#define VGA_REG_BASE		0x3c0
#define VGA_REG_LENGTH		0x020

#define VGA_FONT_SIZE		256
#define VGA_FONT_HEIGHT		32

/* Sequencer */
#define VGA_SEQ_ADDR_REG       	0x3c4
#define VGA_SEQ_DATA_REG	0x3c5

/* Sequencer subregisters */
#define VGA_SEQ_CLOCK_MODE	0x01
#define VGA_SEQ_MAP		0x02
#define VGA_SEQ_FONT		0x03
#define VGA_SEQ_MODE		0x04

/* GFX */
#define VGA_GFX_ADDR_REG	0x3ce
#define VGA_GFX_DATA_REG	0x3cf

/* GFX Subregisters */
#define VGA_GFX_MAP		0x04

#define VGA_GFX_MODE		0x05
#define VGA_GFX_MODE_SHIFT256	0x40
#define VGA_GFX_MODE_SHIFT	0x20
#define VGA_GFX_MODE_HOSTOE	0x10
#define VGA_GFX_MODE_READ0	0x00
#define VGA_GFX_MODE_READ1	0x08
#define VGA_GFX_MODE_WRITE0	0x00
#define VGA_GFX_MODE_WRITE1	0x01
#define VGA_GFX_MODE_WRITE2	0x02
#define VGA_GFX_MODE_WRITE3	0x03

#define VGA_GFX_MISC		0x06
#define VGA_GFX_MISC_GFX	0x01
#define VGA_GFX_MISC_CHAINOE	0x02
#define VGA_GFX_MISC_A0TOBF	0x00
#define VGA_GFX_MISC_A0TOAF	0x04
#define VGA_GFX_MISC_B0TOB7	0x08
#define VGA_GFX_MISC_B8TOBF	0x0c


/* CRTC */
#define VGA_CRT_ADDR_REG	0x3d4
#define VGA_CRT_DATA_REG	0x3d5

/* CRTC Subregisters */
#define VGA_CRT_MAX_SCAN_LINE	0x09
#define VGA_CRT_CURSOR_START	0x0a
#define VGA_CRT_CURSOR_END	0x0b
#define VGA_CRT_CURSOR_HIGH	0x0e
#define VGA_CRT_CURSOR_LOW	0x0f

/* Attribute registers */
#define VGA_ATTR_ADDR_DATA_REG	0x3c0
#define VGA_ATTR_DATA_READ_REG	0x3c1

/* Attribute subregisters */
#define VGA_ATTR_MODE		0x10
#define VGA_ATTR_ENABLE		0x20

#define VGA_INPUT_STATUS_1_REG	0x3da
