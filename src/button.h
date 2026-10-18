#ifndef BUTTON_H_
#define BUTTON_H_

#define SHORT_PRESS 0X10
#define LONG_PRESS  0X11

/* Devuelve 0 si todo fue bien, o un código de error negativo */

extern struct k_msgq but_q;

#endif /* BUTTON_H_ */