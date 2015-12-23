#include <pebble.h>
#include <stdio.h>
#include <pebble_fonts.h>
#include "pbmovies.h"
#include "movies.h"
#include "preloader.h"

#define CRTICS_TEXT_SIZE 13
#define RUNTIME_TEXT_SIZE 8
#define USERS_RATING_TEXT_SIZE 13

#define UI_TITLE_HEIGHT 80
#define UI_Y_SPACING 2
#define UI_RUNTIME_HEIGHT 20
#define UI_RATING_HEIGHT 18
#define UI_INC_Y(y, h) (y+= h+UI_Y_SPACING)
#define UI_RUNTIME_WIDTH 60
#define UI_CRITICS_WIDTH 65


const char *labelUsers = "Users";
char *criticTextWithPercent;
char *runtimeTextBuffer;
char *usersRatingBuffer;

static struct MovieUIScreen {
    Window *window;
    TextLayer *titleTxt;
    Layer *titleUnderline;
    TextLayer *runtimeTxt;
    TextLayer *ratedTxt;
    TextLayer *criticRatingLabelTxt;
    TextLayer *userRatingLabelTxt;
    TextLayer *genreTxt;
    ActionBarLayer *actionBar;

    GBitmap *downIcon;
    GBitmap *upIcon;
    GBitmap *selectIcon;
    uint8_t total;
    uint8_t currentIndex;
    char *currentTheatreId;
    enum MovieUIMode currentMode;
} moviesUI;

static void set_movie_at_index(uint8_t movieIndex) {
    get_data_at(MOVIES_BUFFER, movieIndex, MOVIE_FLD_IDX_ID, movie.id, MOVIE_FLD_LENGTH_ID);
    get_data_at(MOVIES_BUFFER, movieIndex, MOVIE_FLD_IDX_TITLE, movie.title, MOVIE_FLD_LENGTH_TITLE);
    get_data_at(MOVIES_BUFFER, movieIndex, MOVIE_FLD_IDX_GENRE, movie.genre, MOVIE_FLD_LENGTH_GENRE);
    get_data_at(MOVIES_BUFFER, movieIndex, MOVIE_FLD_IDX_CRITC_RATING, movie.criticRating, MOVIE_FLD_LENGTH_CRITC_RATING);
    get_data_at(MOVIES_BUFFER, movieIndex, MOVIE_FLD_IDX_USER_RATING, movie.userRating, MOVIE_FLD_LENGTH_USER_RATING);
    get_data_at(MOVIES_BUFFER, movieIndex, MOVIE_FLD_IDX_RUNTIME, movie.runtime, MOVIE_FLD_LENGTH_RUNTIME);
    get_data_at(MOVIES_BUFFER, movieIndex, MOVIE_FLD_IDX_RATED, movie.rated, MOVIE_FLD_LENGTH_RATED);
}

void set_visibility_text_layer(TextLayer *txtLayer, _Bool hidden) {
    Layer *l = text_layer_get_layer(txtLayer);
    layer_set_hidden(l, hidden);
}

static void set_current(uint8_t movieIndex) {
    if (movieIndex >= moviesUI.total) {
        movieIndex = 0;
    }

    moviesUI.currentIndex = movieIndex;

    //should icons be shown?
    if (movieIndex > 0) {
        action_bar_layer_set_icon(moviesUI.actionBar, BUTTON_ID_UP, moviesUI.upIcon);
    } else {
        action_bar_layer_clear_icon(moviesUI.actionBar, BUTTON_ID_UP);
    }

    if (movieIndex < moviesUI.total - 1) {
        action_bar_layer_set_icon(moviesUI.actionBar, BUTTON_ID_DOWN, moviesUI.downIcon);
    } else {
        action_bar_layer_clear_icon(moviesUI.actionBar, BUTTON_ID_DOWN);
    }

    set_movie_at_index(movieIndex);
    text_layer_set_text(moviesUI.titleTxt, movie.title);
    text_layer_set_text(moviesUI.genreTxt, movie.genre);

    bool hideCritic = (movie.criticRating[0] == '0' && movie.criticRating[1] == '\0');
    set_visibility_text_layer(moviesUI.criticRatingLabelTxt, hideCritic);
    if (!hideCritic) {
        snprintf(criticTextWithPercent, CRTICS_TEXT_SIZE, "critics: %s%%", movie.criticRating);
        text_layer_set_text(moviesUI.criticRatingLabelTxt, criticTextWithPercent);
    }


    bool hideUserRating = (movie.userRating[0] == '0' && movie.userRating[1] == '\0');
    set_visibility_text_layer(moviesUI.userRatingLabelTxt, hideUserRating);
    if (!hideUserRating) {
        snprintf(usersRatingBuffer, USERS_RATING_TEXT_SIZE, "users: %s", movie.userRating);
        text_layer_set_text(moviesUI.userRatingLabelTxt, usersRatingBuffer);
    }


    snprintf(runtimeTextBuffer, RUNTIME_TEXT_SIZE, "%s min", movie.runtime);
    text_layer_set_text(moviesUI.runtimeTxt, runtimeTextBuffer);
    text_layer_set_text(moviesUI.ratedTxt, movie.rated);

    //APP_LOG(APP_LOG_LEVEL_INFO, "Critic Rating: %s", currentMovie.criticRating);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    if (movie.id) {
        if (moviesUI.currentMode == MovieUIModeMovies) {
            //APP_LOG(APP_LOG_LEVEL_DEBUG, "Next, get movie theatres for movie ID: %s", currentMovie.id);
            if (send_message_with_string(PB_MSG_OUT_GET_MOVIE_THEATRES, APP_KEY_MOVIE_ID, movie.id, 0, NULL)) {
                preloader_init();
            }
        } else {
            //APP_LOG(APP_LOG_LEVEL_DEBUG, "Next, get showtimes for Movie ID: %s and TheatreID: %s", currentMovie.id, moviesUI.currentTheatreId);
            load_showtimes_for_movie_theatre();
        }
    }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
    if (moviesUI.currentIndex > 0) {
        set_current(moviesUI.currentIndex - 1);
    }
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    if (moviesUI.currentIndex < moviesUI.total - 1) {
        set_current(moviesUI.currentIndex + 1);
    }
}

static void movie_click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
    window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

static void movies_screen_unload() {
    text_layer_destroy(moviesUI.criticRatingLabelTxt);
    text_layer_destroy(moviesUI.genreTxt);
    text_layer_destroy(moviesUI.ratedTxt);
    text_layer_destroy(moviesUI.runtimeTxt);
    text_layer_destroy(moviesUI.titleTxt);
    text_layer_destroy(moviesUI.userRatingLabelTxt);

    layer_destroy(moviesUI.titleUnderline);
    gbitmap_destroy(moviesUI.downIcon);
    gbitmap_destroy(moviesUI.upIcon);
    gbitmap_destroy(moviesUI.selectIcon);

    action_bar_layer_destroy(moviesUI.actionBar);
    free(MOVIES_BUFFER);
    free(criticTextWithPercent);
    free(runtimeTextBuffer);
    free(usersRatingBuffer);
    if (moviesUI.window) {
        window_destroy(moviesUI.window);
    }
}

static void draw_outline_around(Layer *layer, GContext* ctx) {
    GRect lb = layer_get_bounds(layer);
    //graphics_draw_line(ctx, GPoint(lb.origin.x- 2, lb.size.h + lb.origin.y + 1), GPoint(lb.origin.x + lb.size.w + 2, lb.size.h + lb.origin.y + 1));
    graphics_context_set_stroke_color(ctx, THEME_COLOR_BACKGROUND_PRIMARY);
    graphics_context_set_stroke_width(ctx, 3);
    graphics_draw_line(ctx, GPoint(lb.origin.x, lb.origin.y), GPoint(lb.origin.x+lb.size.w, lb.origin.y));
}

static void movies_screen_load(Window *window) {
    Layer *windowLayer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(windowLayer);

    uint16_t paddingSide = 2;
    uint16_t startY = 3;

    moviesUI.actionBar = action_bar_layer_create();
    action_bar_layer_add_to_window(moviesUI.actionBar, moviesUI.window);
    action_bar_layer_set_background_color(moviesUI.actionBar, THEME_COLOR_BACKGROUND_PRIMARY);
    GRect aBarBounds = layer_get_bounds(action_bar_layer_get_layer(moviesUI.actionBar));
    //remove for now, add later
    action_bar_layer_remove_from_window(moviesUI.actionBar);
    //set allowed width
    uint16_t allowedWidth = bounds.size.w - aBarBounds.size.w;



    moviesUI.runtimeTxt = text_layer_create(GRect(paddingSide, startY, 60, UI_RUNTIME_HEIGHT));
    text_layer_set_font(moviesUI.runtimeTxt, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    //text_layer_set_text_alignment(moviesUI.runtimeTxt, GTextAlignmentRight);
    layer_add_child(windowLayer, text_layer_get_layer(moviesUI.runtimeTxt));

    //rated text
    moviesUI.ratedTxt = text_layer_create(GRect(allowedWidth - 60, startY, 60, UI_RUNTIME_HEIGHT));
    text_layer_set_font(moviesUI.ratedTxt, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(moviesUI.ratedTxt, GTextAlignmentRight);
    layer_add_child(windowLayer, text_layer_get_layer(moviesUI.ratedTxt));

    UI_INC_Y(startY, UI_RUNTIME_HEIGHT);
    UI_INC_Y(startY, 2);

    //title
    GRect titleGrect = GRect(paddingSide, startY, allowedWidth, UI_TITLE_HEIGHT);
    moviesUI.titleTxt = text_layer_create(titleGrect);
    text_layer_set_font(moviesUI.titleTxt, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(moviesUI.titleTxt, GTextAlignmentCenter);
    layer_add_child(windowLayer, text_layer_get_layer(moviesUI.titleTxt));
    text_layer_set_overflow_mode(moviesUI.titleTxt, GTextOverflowModeFill);
    text_layer_enable_screen_text_flow_and_paging(moviesUI.titleTxt, 2);

    moviesUI.titleUnderline = layer_create(GRect(titleGrect.origin.x - 2, titleGrect.origin.y - 2, titleGrect.size.w + 4, titleGrect.size.h + 4));
    layer_set_update_proc(moviesUI.titleUnderline, draw_outline_around);
    layer_add_child(windowLayer, moviesUI.titleUnderline);

    UI_INC_Y(startY, UI_TITLE_HEIGHT);


    //metacrtic label
    moviesUI.criticRatingLabelTxt = text_layer_create(GRect(0, startY, allowedWidth, UI_RATING_HEIGHT));
    text_layer_set_background_color(moviesUI.criticRatingLabelTxt, THEME_COLOR_BACKGROUND_PRIMARY);
    text_layer_set_text_color(moviesUI.criticRatingLabelTxt, GColorWhite);
    text_layer_set_overflow_mode(moviesUI.criticRatingLabelTxt, GTextOverflowModeFill);
    text_layer_set_font(moviesUI.criticRatingLabelTxt, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text_alignment(moviesUI.criticRatingLabelTxt, GTextAlignmentCenter);
    layer_add_child(windowLayer, text_layer_get_layer(moviesUI.criticRatingLabelTxt));

    startY += UI_RATING_HEIGHT;
    //user rating label
    moviesUI.userRatingLabelTxt = text_layer_create(GRect(0, startY, allowedWidth, UI_RATING_HEIGHT));
    text_layer_set_background_color(moviesUI.userRatingLabelTxt, THEME_COLOR_BACKGROUND_PRIMARY);
    text_layer_set_text_color(moviesUI.userRatingLabelTxt, GColorWhite);
    text_layer_set_font(moviesUI.userRatingLabelTxt, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text_alignment(moviesUI.userRatingLabelTxt, GTextAlignmentCenter);
    layer_add_child(windowLayer, text_layer_get_layer(moviesUI.userRatingLabelTxt));

    UI_INC_Y(startY, UI_RATING_HEIGHT);

    //next is the genre
    moviesUI.genreTxt = text_layer_create(GRect(paddingSide, startY, allowedWidth, bounds.size.h - startY));
    text_layer_set_text_alignment(moviesUI.genreTxt, GTextAlignmentCenter);
    text_layer_set_font(moviesUI.genreTxt, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    layer_add_child(windowLayer, text_layer_get_layer(moviesUI.genreTxt));

    action_bar_layer_add_to_window(moviesUI.actionBar, moviesUI.window);
    //icons
    moviesUI.upIcon = gbitmap_create_with_resource(RESOURCE_ID_ICON_A_BAR_UP);
    moviesUI.downIcon = gbitmap_create_with_resource(RESOURCE_ID_ICON_A_BAR_DOWN);

    if (moviesUI.currentMode == MovieUIModeMovies) {
        moviesUI.selectIcon = gbitmap_create_with_resource(RESOURCE_ID_ICON_A_BAR_THEATRE_WHITE);
    } else {
        moviesUI.selectIcon = gbitmap_create_with_resource(RESOURCE_ID_ICON_A_BAR_SHOWTIME_WHITE);
    }

    action_bar_layer_set_icon(moviesUI.actionBar, BUTTON_ID_SELECT, moviesUI.selectIcon);
    action_bar_layer_set_click_config_provider(moviesUI.actionBar, movie_click_config_provider);

    criticTextWithPercent = BUFFER_CREATE(CRTICS_TEXT_SIZE - 1);
    runtimeTextBuffer = BUFFER_CREATE(RUNTIME_TEXT_SIZE - 1);
    usersRatingBuffer = BUFFER_CREATE(USERS_RATING_TEXT_SIZE - 1);
    set_current(moviesUI.currentIndex);
}

//void theatres_screen_initialize(int, enum TheatreUiMode, char*);

void movies_screen_init(int recordCount, enum MovieUIMode mode, char *theatreId) {
    moviesUI.total = (uint8_t) recordCount;
    moviesUI.currentIndex = 0;
    moviesUI.currentMode = mode;
    moviesUI.currentTheatreId = theatreId;

    moviesUI.window = window_create();

    window_set_window_handlers(moviesUI.window, (WindowHandlers) {
        .load = movies_screen_load,
        .unload = movies_screen_unload,
        .appear = preloader_set_hidden,
    });


    const bool animated = true;
    window_stack_push(moviesUI.window, animated);
}

