#include "_protocols.h"

const Protocol* protocols[] = {
    &protocol_retekess_td112,
    &protocol_retekess_td157,
};

const size_t protocols_count = COUNT_OF(protocols);
