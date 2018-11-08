/**
* \file
*		Ring Buffer library
*/

#include "mqtt_common.h"
#include "ringbuf.h"
#include "string.h"

/**
* \brief init a RINGBUF object
* \param r pointer to a RINGBUF object
* \param buf pointer to a byte array
* \param size size of buf
* \return 0 if successfull, otherwise failed
*/
int16_t  RINGBUF_Init(RINGBUF *r, uint8_t* buf, uint32_t size)
{
	if(r == NULL || buf == NULL || size < 2) return -1;
	
	r->p_o = r->p_r = r->p_w = buf;
	r->fill_cnt = 0;
	r->size = size;
	//memset(r->buf, 0, sizeof(r->buf));
	return 0;
}
/**
* \brief put a character into ring buffer
* \param r pointer to a ringbuf object
* \param c character to be put
* \return 0 if successfull, otherwise failed
*/
int16_t  RINGBUF_Put(RINGBUF *r, uint8_t c)
{
	//uint8_t i = 0;
	if(r->fill_cnt>=r->size)return -1;		// ring buffer is full, this should be atomic operation
	

	r->fill_cnt++;							// increase filled slots count, this should be atomic operation

	
	*r->p_w++ = c;							// put character into buffer
	
	if(r->p_w >= r->p_o + r->size)			// rollback if write pointer go pass
		r->p_w = r->p_o;					// the physical boundary
//	if(c=='\n')
//	{
//		while(i<5)
//		{
//			if(!(r->buf[i]))
//			{
//				r->buf[i] = r->fill_cnt-1;
//				break;
//			}
//		}
//	}
	return 0;
}

int16_t  RINGBUF_Put_str(RINGBUF *r, uint8_t* str, uint8_t len)
{
	r->fill_cnt +=len;
	if(r->fill_cnt>=r->size)
	{
		r->fill_cnt -=len;
		return -1;
	}
	if((r->p_o + r->size)<(r->p_w+len))
	{
		uint8_t len1 = r->p_o + r->size - r->p_w;
		memcpy(r->p_w, str, len1);
		r->p_w = r->p_o;
		uint8_t len2 = len -len1;
		memcpy(r->p_w, str + len1, len2);
		r->p_w += len2;
	}else{
		memcpy(r->p_w, str, len);
		r->p_w += len;
	}
	return 0;

}
/**
* \brief get a character from ring buffer
* \param r pointer to a ringbuf object
* \param c read character
* \return 0 if successfull, otherwise failed
*/
int16_t  RINGBUF_Get(RINGBUF *r, uint8_t* c)
{
	if(r->fill_cnt<=0)return -1;				// ring buffer is empty, this should be atomic operation
	

	r->fill_cnt--;								// decrease filled slots count

	
	*c = *r->p_r++;								// get the character out
	if(r->p_r >= r->p_o + r->size)				// rollback if write pointer go pass
		r->p_r = r->p_o;						// the physical boundary
	return 0;
}

//get a string to buffer
int16_t  RINGBUF_Get_str(RINGBUF *r, uint8_t* str)
{
	if(r->fill_cnt<=0)return -1;				// ring buffer is empty, this should be atomic operation

	if((r->p_o + r->size)<(r->p_r + r->fill_cnt))
	{
		uint8_t len1 = r->p_o + r->size - r->p_r;
		memcpy(str, r->p_r, len1);
		r->p_r = r->p_o;
		uint8_t len2 = r->fill_cnt -len1;
		memcpy(str + len1, r->p_r, len2);
		r->p_r += len2;
	}else{
		memcpy(str, r->p_r, r->fill_cnt);
		r->p_r += r->fill_cnt;
	}
	r->fill_cnt = 0;

	return 0;
}

//according '\r' or '\n' find string
uint16_t  RINGBUF_find_location(RINGBUF *r)
{
	uint16_t size = 0;
	uint16_t tmp = 0;
	if(r->fill_cnt>0)
	{
		if((*(r->p_w)=='\n')||(*(r->p_w)=='\r'))
		{
			size = r->fill_cnt;
			return size;
		}else{
			//while((*(r->p_r)=='\n')||(*(r->p_r)=='\r'))
				//r->p_r++;
			while(r->p_r!=r->p_w)
			{
				r->p_r++;
				tmp++;
				if(*(r->p_r)=='\n')
				{
					size += tmp;
					tmp = 0;
				}
			}
			r->p_r -= size;
			r->p_r -= tmp;
			return size;
		}
	}
	return 0;
}


