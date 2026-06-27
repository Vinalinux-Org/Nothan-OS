#ifndef _NOTHAN_INPUT_H
#define _NOTHAN_INPUT_H

struct input_ops {
	int (*read_key)(void);  /* returns keycode, or -1 if none pending */
	/*
	 * Fill the current absolute pointer state in the touch panel's RAW
	 * landscape coordinates (x: 0..799, y: 0..479) and pressed (0/1).
	 * Returns 1 if a pointer device is present, 0 otherwise.
	 */
	int (*get_pointer)(int *x, int *y, int *pressed);
};

/*
 * /dev/input0 read protocol when a pointer backend is registered: each read
 * returns one fixed 5-byte record = { x_lo, x_hi, y_lo, y_hi, pressed },
 * carrying the latest raw pointer state (non-blocking).
 */
#define INPUT_POINTER_RECORD	5

void input_register_ops(struct input_ops *ops);

#endif /* _NOTHAN_INPUT_H */
