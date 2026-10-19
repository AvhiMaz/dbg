#define TB_IMPL

#include "tui.h"
#include "../thirdparty/termbox2.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disasm.h"

typedef struct {
    char  *text;
    size_t size;
    int    lines;
} Pane;

typedef struct {
    const char *path;
    size_t      len;
    Pane        header;
    Pane        sections;
    Pane        symbols;
    Pane        insns;
    int         top;
} Tui;

static FILE *pane_open(Pane *p) {
    FILE *out = open_memstream(&p->text, &p->size);

    if (out == NULL) {
        perror("open_memstream failed");
    }

    return out;
}

static void pane_close(Pane *p, FILE *out) {
    fclose(out);

    p->lines = 0;
    for (size_t i = 0; i < p->size; i++) {
        if (p->text[i] == '\n') {
            p->lines++;
        }
    }
}

static int load(Tui *t, const unsigned char *buf, size_t len,
                const Elf64_Ehdr *eh) {

    FILE *out;

    if ((out = pane_open(&t->header)) == NULL) {
        return 0;
    }
    fprintf(out, "size:       %zu\n", len);
    elf_print_header(out, eh);
    pane_close(&t->header, out);

    if ((out = pane_open(&t->sections)) == NULL) {
        return 0;
    }
    elf_print_sections(out, buf, len, eh);
    pane_close(&t->sections, out);

    if ((out = pane_open(&t->symbols)) == NULL) {
        return 0;
    }
    elf_print_symbols(out, buf, len, eh);
    pane_close(&t->symbols, out);

    if ((out = pane_open(&t->insns)) == NULL) {
        return 0;
    }
    disasm_print(out, buf, len, eh);
    pane_close(&t->insns, out);

    return 1;
}

static void unload(Tui *t) {
    free(t->header.text);
    free(t->sections.text);
    free(t->symbols.text);
    free(t->insns.text);
}

static void draw_box(int x, int y, int w, int h, uintattr_t fg,
                     const char *title) {

    for (int i = x + 1; i < x + w - 1; i++) {
        tb_set_cell(i, y, 0x2500, fg, TB_DEFAULT);
        tb_set_cell(i, y + h - 1, 0x2500, fg, TB_DEFAULT);
    }

    for (int j = y + 1; j < y + h - 1; j++) {
        tb_set_cell(x, j, 0x2502, fg, TB_DEFAULT);
        tb_set_cell(x + w - 1, j, 0x2502, fg, TB_DEFAULT);
    }

    tb_set_cell(x, y, 0x250c, fg, TB_DEFAULT);
    tb_set_cell(x + w - 1, y, 0x2510, fg, TB_DEFAULT);
    tb_set_cell(x, y + h - 1, 0x2514, fg, TB_DEFAULT);
    tb_set_cell(x + w - 1, y + h - 1, 0x2518, fg, TB_DEFAULT);

    int room = w - 6;

    if (room > 0) {
        tb_printf(x + 2, y, fg, TB_DEFAULT, " %.*s ", room, title);
    }
}

static void draw_text(int x, int y, int w, int h, const Pane *p, int skip) {
    const char *s = p->text;
    int         row = 0;

    if (w <= 0 || h <= 0 || s == NULL) {
        return;
    }

    while (*s != '\0' && row < skip + h) {
        const char *end = strchr(s, '\n');

        if (end == NULL) {
            end = s + strlen(s);
        }

        int n = (int)(end - s);

        if (row >= skip) {
            tb_printf(x, y + row - skip, TEXT, TB_DEFAULT, "%.*s",
                      n < w ? n : w, s);
        }

        row++;
        s = *end == '\0' ? end : end + 1;
    }
}

static void draw_pane(int x, int y, int w, int h, uintattr_t fg,
                      const char *title, const Pane *p, int skip) {
    draw_box(x, y, w, h, fg, title);
    draw_text(x + 2, y + 1, w - 4, h - 2, p, skip);
}

static int fit(const Pane *p, int max) {
    return p->lines + 2 < max ? p->lines + 2 : max;
}

static int insn_rows(void) {
    return tb_height() - 3;
}

static void draw_status(const Tui *t) {
    int  y = tb_height() - 1;
    char info[64];

    for (int x = 0; x < tb_width(); x++) {
        tb_set_cell(x, y, ' ', TB_DEFAULT, GRAY);
    }

    snprintf(info, sizeof info, "line %d/%d  %zu bytes", t->top + 1,
             t->insns.lines, t->len);

    tb_printf(1, y, DARK, GRAY,
              "j/k scroll  ctrl-d/u page  g/G top/end  q quit");
    tb_printf(tb_width() - (int)strlen(info) - 1, y, DARK, GRAY, "%s", info);
}

static void draw(const Tui *t) {
    int  w = tb_width();
    int  h = tb_height() - 1;
    int  left = w * 2 / 5;
    int  header = fit(&t->header, h / 3);
    int  sections = fit(&t->sections, (h - header) / 2);
    int  symbols = h - header - sections;
    char title[128];

    snprintf(title, sizeof title, "[4] Instructions  %s", t->path);

    tb_clear();
    draw_pane(0, 0, left, header, BLUE, "[1] Header", &t->header, 0);
    draw_pane(0, header, left, sections, PURPLE, "[2] Sections", &t->sections,
              0);
    draw_pane(0, header + sections, left, symbols, YELLOW, "[3] Symbols",
              &t->symbols, 0);
    draw_pane(left, 0, w - left, h, GREEN, title, &t->insns, t->top);
    draw_status(t);
    tb_present();
}

static void scroll(Tui *t, int delta) {
    int last = t->insns.lines - insn_rows();

    t->top += delta;

    if (t->top > last) {
        t->top = last;
    }

    if (t->top < 0) {
        t->top = 0;
    }
}

static int handle_key(Tui *t, const struct tb_event *ev) {
    int page = insn_rows();

    if (ev->ch == 'q') {
        return 0;
    }

    if (ev->ch == 'j' || ev->key == TB_KEY_ARROW_DOWN) {
        scroll(t, 1);
    } else if (ev->ch == 'k' || ev->key == TB_KEY_ARROW_UP) {
        scroll(t, -1);
    } else if (ev->key == TB_KEY_CTRL_D) {
        scroll(t, page / 2);
    } else if (ev->key == TB_KEY_CTRL_U) {
        scroll(t, -page / 2);
    } else if (ev->ch == 'g') {
        scroll(t, -t->insns.lines);
    } else if (ev->ch == 'G') {
        scroll(t, t->insns.lines);
    }

    return 1;
}

int tui_run(const char *path, const unsigned char *buf, size_t len,
            const Elf64_Ehdr *eh) {

    Tui             t = {.path = path, .len = len};
    struct tb_event ev;

    if (!load(&t, buf, len, eh)) {
        unload(&t);
        return 1;
    }

    if (tb_init() != TB_OK) {
        unload(&t);
        return 1;
    }

    tb_set_output_mode(TB_OUTPUT_256);
    draw(&t);

    while (1) {
        int rc = tb_poll_event(&ev);

        if (rc == TB_ERR_POLL && tb_last_errno() == EINTR) {
            continue;
        }

        if (rc != TB_OK) {
            break;
        }

        if (ev.type == TB_EVENT_KEY && !handle_key(&t, &ev)) {
            break;
        }

        if (ev.type == TB_EVENT_RESIZE) {
            scroll(&t, 0);
        }

        draw(&t);
    }

    tb_shutdown();
    unload(&t);

    return 0;
}
