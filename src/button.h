#ifndef BUTTON_H_
#define BUTTON_H_

/* Button Modes */
enum button_mode {
    SHORT_PRESS,
    LONG_PRESS
};

/* Message Queue ID */
extern struct k_msgq but_q;

#endif /* BUTTON_H_ */