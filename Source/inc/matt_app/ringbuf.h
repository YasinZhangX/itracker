#ifndef _RING_BUF_H_
#define _RING_BUF_H_

#include "cmsis_os.h"

typedef struct{
	uint8_t* p_o;				/**< Original pointer */
	uint8_t* volatile p_r;		/**< Read pointer */
	uint8_t* volatile p_w;		/**< Write pointer */
	volatile uint32_t fill_cnt;	/**< Number of filled slots */
	uint32_t size;				/**< Buffer size */
	//uint8_t buf[5];
}RINGBUF;

int16_t  RINGBUF_Init(RINGBUF *r, uint8_t* buf, uint32_t size);
uint16_t  RINGBUF_Put_char(RINGBUF *r, uint8_t c);
int16_t  RINGBUF_Put_str(RINGBUF *r, uint8_t* str, uint8_t len);
uint16_t  RINGBUF_Get_char(RINGBUF *r, uint8_t* c);
uint16_t  RINGBUF_find_location(RINGBUF *r);

#endif
