#ifndef DFUTASK_H
#define DFUTASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rak_uart_gsm.h"

/**
 *  Public Functions
 */
void dfu_task(void * pvParameter);
void dfu_settings_init(void);

#ifdef __cplusplus
}
#endif
#endif /* DFUTASK_H */