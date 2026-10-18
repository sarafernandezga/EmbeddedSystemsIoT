#ifndef BUTTON_H_
#define BUTTON_H_

enum button_mode {
    SHORT_PRESS,
    LONG_PRESS
};

extern struct k_msgq but_q;

#endif /* BUTTON_H_ */