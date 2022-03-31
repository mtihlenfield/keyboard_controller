#ifndef __KEYS_H__
#define __KEYS_H__

#include <stdint.h>

enum keys {
    KEY_C1 = 1,
    KEY_CS1 = 2,
    // TODO think about how to do key -> CV conversion
};


int key_matrix_init(void);

int key_matrix_loop(void);

#endif
