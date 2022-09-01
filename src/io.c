#include "io.h"

#include "CPU/interrupt.h"
#include "CPU/timer.h"
#include "utils/log.h"
#include "utils/macro.h"

void write_io(u16 address, u8 data)
{
    if (IN_RANGE(address, TIMER_DIV, TIMER_TAC)) {
        write_timer(address, data);
        return;
    }

    switch (address) {
    case IF_ADDRESS:
        write_interrupt(IF_ADDRESS, data);
        break;

    default:
        log_err("IO write: " HEX, address);
    }
}

u8 read_io(u16 address)
{
    if (IN_RANGE(address, TIMER_DIV, TIMER_TAC)) {
        return read_timer(address);
    }

    switch (address) {
    case IF_ADDRESS:
        return read_interrupt(IF_ADDRESS);

    default:
        log_err("IO write: " HEX, address);
        return 0;
    }
}
