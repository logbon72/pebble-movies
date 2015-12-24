#include <pebble.h>
#include <pebble_fonts.h>
#include "pbmovies.h"
#include "showtimes.h"
#include"color.h"
#include "preloader.h"

static const char* typeDigital = "Digital";
static const char* typeDigital3D = "3D";
static const char* typeIMAX = "IMAX 3D";
//static const char* sectionHeader = "Available Showtimes";
#ifndef PBL_RECT
static const char* typeDigitalQr = "Digital | QR";
static const char* typeDigital3DQr = "3D | QR";
static const char* typeIMAXQr = "IMAX 3D | QR";
#endif

#define SHOWTIME_TYPE_DIGITAL '0'
#define SHOWTIME_TYPE_3D '1'
#define SHOWTIME_TYPE_IMAX '2'

#define SHOWTIME_CAN_BUY '1'
#define SHOWTIME_CANT_BUY '0'

#define MAX_SHOWTIMES_COUNT 30

#define SHOWTIME_FLD_LENGTH_ID 8
#define SHOWTIME_FLD_LENGTH_TYPE 2
#define SHOWTIME_FLD_LENGTH_TIME 8
#define SHOWTIME_FLD_LENGTH_LINK 2

#define SHOWTIME_FLD_IDX_ID 0
#define SHOWTIME_FLD_IDX_TYPE 1
#define SHOWTIME_FLD_IDX_TIME 2
#define SHOWTIME_FLD_IDX_LINK 3

struct Showtime {
    char id[SHOWTIME_FLD_LENGTH_ID];
    char type[SHOWTIME_FLD_LENGTH_TYPE];
    char time[SHOWTIME_FLD_LENGTH_TIME];
    char link[SHOWTIME_FLD_LENGTH_LINK];
};

//static struct Showtime showtimes[MAX_SHOWTIMES_COUNT];

static struct ShowtimesUIScreen {
    Window* window;
    MenuLayer *menuLayer;
    GBitmap *canBuyIcon;
    GBitmap *cantBuyIcon;
    uint8_t total;

} showtimesUI;

static struct Showtime get_showtime_at(uint8_t row) {
    struct Showtime showtime;
    get_data_at(SHOWTIMES_BUFFER, row, SHOWTIME_FLD_IDX_ID,
            showtime.id, SHOWTIME_FLD_LENGTH_ID);
    get_data_at(SHOWTIMES_BUFFER, row, SHOWTIME_FLD_IDX_LINK,
            showtime.link, SHOWTIME_FLD_LENGTH_LINK);
    get_data_at(SHOWTIMES_BUFFER, row, SHOWTIME_FLD_IDX_TIME,
            showtime.time, SHOWTIME_FLD_LENGTH_TIME);
    get_data_at(SHOWTIMES_BUFFER, row, SHOWTIME_FLD_IDX_TYPE,
            showtime.type, SHOWTIME_FLD_LENGTH_TYPE);

    return showtime;
}

static uint16_t menu_num_sections(MenuLayer *menu_layer, void *data) {
    return 1;
}

static uint16_t menu_num_rows(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return showtimesUI.total;
}

static int16_t menu_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    // This is a define provided in pebble.h that you may use for the default height
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void menu_draw_header(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
    // Determine which section we're working with
    if (section_index == 0) {
        graphics_draw_text(ctx, "Showtimes", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), layer_get_bounds(cell_layer), GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    }

}

static void menu_cell_drawer(GContext* ctx, const Layer *cell_layer, MenuIndex *ci, void *data) {
    const char *showtimeType;
    struct Showtime showtime = get_showtime_at(ci->row);

#ifndef PBL_ROUND    
    if (showtime.type[0] == SHOWTIME_TYPE_3D) {
        showtimeType = typeDigital3D;
    } else if (showtime.type[0] == SHOWTIME_TYPE_IMAX) {
        showtimeType = typeIMAX;
    } else {
        showtimeType = typeDigital;
    }
    GBitmap *icon = showtime.link[0] == SHOWTIME_CAN_BUY ? showtimesUI.canBuyIcon :
            showtimesUI.cantBuyIcon;
    menu_cell_basic_draw(ctx, cell_layer, showtime.time, showtimeType, icon);
#endif
#ifndef PBL_RECT
    _Bool canBuy = showtime.link[0] == SHOWTIME_CAN_BUY;
    if (showtime.type[0] == SHOWTIME_TYPE_3D) {
        showtimeType = canBuy ? typeDigital3DQr : typeDigital3D;
    } else if (showtime.type[0] == SHOWTIME_TYPE_IMAX) {
        showtimeType = canBuy ? typeIMAXQr : typeIMAX;
    } else {
        showtimeType = canBuy ? typeDigitalQr : typeDigital;
    }
    menu_cell_basic_draw(ctx, cell_layer, showtime.time, showtimeType, NULL);
#endif


}

static void menu_select_handler(MenuLayer *menu_layer, MenuIndex *ci, void *data) {
    // Use the row to specify which item will receive the select action
    struct Showtime showtime = get_showtime_at(ci->row);

    if (showtime.link[0] == SHOWTIME_CAN_BUY) {
        if (send_message_with_string(PB_MSG_OUT_GET_QR_CODE,
                APP_KEY_SHOWTIME_ID, showtime.id, 0, NULL)) {
            preloader_init();
        }
    }
}

static void showtimes_load(Window *window) {
    Layer *windowLayer = window_get_root_layer(showtimesUI.window);
    GRect bounds = layer_get_bounds(windowLayer);
    //window_set_background_color(window, GColorBlack);

    showtimesUI.menuLayer = menu_layer_create(bounds);

    menu_layer_set_callbacks(showtimesUI.menuLayer, NULL, (MenuLayerCallbacks) {
        .get_num_sections = menu_num_sections,
        .get_num_rows = menu_num_rows,
        .get_header_height = menu_header_height_callback,
        .draw_header = menu_draw_header,
        .draw_row = menu_cell_drawer,
        .select_click = menu_select_handler,
    });

    // Bind the menu layer's click config provider to the window for interactivity
    menu_layer_set_click_config_onto_window(showtimesUI.menuLayer, showtimesUI.window);
#ifndef PBL_BW
    set_menu_color(showtimesUI.menuLayer);
#endif
    // Add it to the window for display
    layer_add_child(windowLayer, menu_layer_get_layer(showtimesUI.menuLayer));
}

static void showtimes_unload(Window *window) {
    menu_layer_destroy(showtimesUI.menuLayer);
    gbitmap_destroy(showtimesUI.canBuyIcon);
    gbitmap_destroy(showtimesUI.cantBuyIcon);
    showtimesUI.total = 0;
    if (showtimesUI.window) {
        window_destroy(showtimesUI.window);
    }
    free(SHOWTIMES_BUFFER);
}

void showtimes_init() {
    preloader_set_hidden(NULL);
    //remove_top_window(2);
    showtimesUI.window = window_create();

    showtimesUI.total = record_count(SHOWTIMES_BUFFER, DELIMITER_RECORD);
    if (showtimesUI.total > MAX_SHOWTIMES_COUNT) {
        showtimesUI.total = MAX_SHOWTIMES_COUNT;
    }

    showtimesUI.canBuyIcon = gbitmap_create_with_resource(RESOURCE_ID_ICON_CAN_BUY);
    showtimesUI.cantBuyIcon = gbitmap_create_with_resource(RESOURCE_ID_ICON_CANT_BUY);

    window_set_window_handlers(showtimesUI.window, (WindowHandlers) {
        .load = showtimes_load,
        .unload = showtimes_unload,
        //.appear = preloader_set_hidden,
    });



    window_stack_push(showtimesUI.window, true);

}