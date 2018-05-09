//
// Created by rune on 2018/4/9.
//

#ifndef MQDEMO_UTILS_H
#define MQDEMO_UTILS_H

void die(const char *fmt, ...);
extern void die_on_error(int x, char const *context);
extern void die_on_amqp_error(amqp_rpc_reply_t x, char const *context);

extern void amqp_dump(void const *buffer, size_t len);

extern uint64_t now_microseconds(void);
extern void microsleep(int usec);

#endif //MQDEMO_UTILS_H
