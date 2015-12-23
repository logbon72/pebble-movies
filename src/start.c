#include <pebble.h>
#include "pbmovies.h"
#include "home.h"

#define DAYS_TO_BROWSE 5
#define SECTIONS_COUNT 1
#define SECONDS_IN_DAY 86400

static SimpleMenuSection sections[SECTIONS_COUNT];
static SimpleMenuItem menu_days[DAYS_TO_BROWSE];
//static char WEEK_DAYS[7][10] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
static char WEEK_DAYS[DAYS_TO_BROWSE - 2][10]; //today and tomorrow already sorted
static char WEEK_DATES[DAYS_TO_BROWSE - 2][7]; //today and tomorrow already sorted
static char TodayText[7];
static char TomorrowText[7];

//static const char *MENU_FORMAT_DATE = "%b %e";
//static const char *MENU_FORMAT_WDAY = "%b %e";

static int dateOffset = 0;

int get_date_offset() {
    return dateOffset;
}

static struct StartScreen {
    Window *window;
    //TextLayer *text;
    SimpleMenuLayer *menu_layer;
} startScreen;

static struct tm *addday(time_t *time, int d) {
    *time += d * SECONDS_IN_DAY;
    return localtime(time);
}

static struct tm *tminc(time_t *x) {
    return addday(x, 1);
}

static void menu_day_selected(int index, void *ctx) {
    // Here we just change the subtitle to a literal string
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "You selected date offset %d", index);
    dateOffset = index;
    time_t t = time(0);
    strftime(CurrentDateStr, sizeof (CurrentDateStr), "%A, %b %e", addday(&t, index));
    home_screen_init();
}

static void start_screen_load(Window *window) {
    Layer *windowLayer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(windowLayer);

    //get local time
    time_t rawtime;
    time(&rawtime);
    //    struct tm *currentTime;
    //    currentTime = localtime(&rawtime);

    //today's format
    strftime(TodayText, sizeof (TodayText), "%b %e", localtime(&rawtime));

    int i = 0;
    menu_days[i++] = (SimpleMenuItem){
        .title = "Today",
        .callback = menu_day_selected,
        .subtitle = TodayText,
    };

    strftime(TomorrowText, sizeof (TomorrowText), "%b %e", tminc(&rawtime));

    menu_days[i++] = (SimpleMenuItem){
        .title = "Tomorrow",
        .callback = menu_day_selected,
        .subtitle = TomorrowText
    };


    struct tm *currentTime;

    for (; i < DAYS_TO_BROWSE; i++) {
        int x = i - 2;
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "Current Day before: %d", currentTime->tm_mday);
        currentTime = tminc(&rawtime);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "Current Day after: %d", currentTime->tm_mday);
        strftime(WEEK_DAYS[x], sizeof (WEEK_DAYS[x]), "%A", currentTime);
        strftime(WEEK_DATES[x], sizeof (WEEK_DATES[x]), "%b %e", currentTime);

        menu_days[i] = (SimpleMenuItem){
            .title = WEEK_DAYS[x],
            .callback = menu_day_selected,
            .subtitle = WEEK_DATES[x],
        };
    }

    sections[0] = (SimpleMenuSection){
        .title = PBL_IF_RECT_ELSE("Select Day", NULL),
        .num_items = DAYS_TO_BROWSE,
        .items = menu_days,
    };


    startScreen.menu_layer = simple_menu_layer_create(bounds, window, sections, SECTIONS_COUNT, NULL);
#if defined(PBL_COLOR)
    MenuLayer *menuLayer = simple_menu_layer_get_menu_layer(startScreen.menu_layer);
    menu_layer_set_normal_colors(menuLayer, THEME_COLOR_MENU_NORMAL_BACKGROUND, THEME_COLOR_MENU_NORMAL_FOREGROUND);
    menu_layer_set_highlight_colors(menuLayer, THEME_COLOR_MENU_HIGHLIGHT_BACKGROUND, THEME_COLOR_MENU_HIGHLIGHT_FOREGROUND);
#endif
    layer_add_child(windowLayer, simple_menu_layer_get_layer(startScreen.menu_layer));
}

static void start_screen_unload() {
    simple_menu_layer_destroy(startScreen.menu_layer);
    if (startScreen.window) {
        window_destroy(startScreen.window);
    }
}

void start_screen_init() {
    startScreen.window = window_create();

    window_set_window_handlers(startScreen.window, (WindowHandlers) {
        .load = start_screen_load,
        .unload = start_screen_unload,
    });

    const bool animated = true;
    window_stack_push(startScreen.window, animated);

}

