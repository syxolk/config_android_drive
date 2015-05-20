#ifndef COMM_COMM_H
#define COMM_COMM_H

#include <stdint.h>
#ifndef F_CPU
#include <unistd.h>
#endif
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct up;
struct down; 

extern void finalize_up(struct up*);
extern void finalize_down(struct down*);

extern bool check_up(struct up*);
extern bool check_down(struct down*);

#ifdef __cplusplus
}
#endif

#endif // COMM_COMM_H
