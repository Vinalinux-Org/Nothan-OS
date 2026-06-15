#ifndef _NOTHAN_INPUT_H
#define _NOTHAN_INPUT_H

struct input_ops {
	int (*read_key)(void);  /* returns keycode, or -1 if none pending */
};

void input_register_ops(struct input_ops *ops);

#endif /* _NOTHAN_INPUT_H */
