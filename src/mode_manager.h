#ifndef MANAGER_H_
#define MANAGER_H_

#define NORMAL 0x12
#define BLUE   0x13
#define OFF    0x14

/* Devuelve 0 si todo fue bien, o un código de error negativo */
int init_button(void);

extern struct k_msgq but_q;

#endif 