#ifndef BLEMAIN_H
#define BLEMAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  Public Functions
 */
extern void ble_stack_init(void);
extern void gap_params_init(void);
extern void gatt_init(void);
extern void services_init(void);
extern void conn_params_init(void);
extern void peer_manager_init(void);
extern void advertising_start();

#ifdef __cplusplus
}
#endif
#endif /* BLEMAIN_H */