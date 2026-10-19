#define TB_IMPL

#include "tui.h"
#include "../thirdparty/termbox2.h"

int tui_run(void) {

    struct tb_event ev;

    if (tb_init() != TB_OK) {
        return 1;
    }

    tb_printf(0, 1, TB_WHITE, 0, "hello from termbox");
    tb_present();

    while (tb_poll_event(&ev) == TB_OK) {
        if (ev.type == TB_EVENT_KEY && ev.ch == 'q') {
            break;
        }
    }

    tb_shutdown();

    return 0;
}
