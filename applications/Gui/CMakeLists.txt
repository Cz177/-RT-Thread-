SET(SOURCES components/ui_comp_list.c
    components/ui_comp.c
    screens/ui_Screenmain.c
    screens/ui_Screenadd.c
    screens/ui_Screenfind.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    ui_events.c
    fonts/ui_font_Font1.c
    fonts/ui_font_Font2.c)

add_library(ui ${SOURCES})
