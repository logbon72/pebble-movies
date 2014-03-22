#include <pebble.h>
#include "pbmovies.h"
#include "movies.h"


#define MOVIE_FLD_LENGTH_ID 7
#define MOVIE_FLD_LENGTH_TITLE 25
#define MOVIE_FLD_LENGTH_GENRE 17
#define MOVIE_FLD_LENGTH_USER_RATING 6
#define MOVIE_FLD_LENGTH_CRITC_RATING 3
#define MOVIE_FLD_LENGTH_RUNTIME 4
#define MOVIE_FLD_LENGTH_RATED 6

//id,title,genre,user_rating,rated,critic_rating,runtime                    
#define MOVIE_FLD_IDX_ID 0
#define MOVIE_FLD_IDX_TITLE 1
#define MOVIE_FLD_IDX_GENRE 2
#define MOVIE_FLD_IDX_USER_RATING 3
#define MOVIE_FLD_IDX_RATED 4
#define MOVIE_FLD_IDX_CRITC_RATING 5
#define MOVIE_FLD_IDX_RUNTIME 6

static struct MovieRecord {
    char id[MOVIE_FLD_LENGTH_ID];
    char title[MOVIE_FLD_LENGTH_TITLE];
    char genre[MOVIE_FLD_LENGTH_GENRE];
    char userRating[MOVIE_FLD_LENGTH_USER_RATING];
    char criticRating[MOVIE_FLD_LENGTH_CRITC_RATING];
    char runtime[MOVIE_FLD_LENGTH_RUNTIME];
    char rated[MOVIE_FLD_LENGTH_RATED];
} currentMovie;


const char *labelRated = "Rated";
const char *labelPercent = "%";
const char *labelCritics = "Critics";
const char *labelUsers = "Users";
const char *labelMin = " min";

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


    get_data_at(MOVIES_LIST, movieIndex, MOVIE_FLD_IDX_ID, currentMovie.id, MOVIE_FLD_LENGTH_ID);
    get_data_at(MOVIES_LIST, movieIndex, MOVIE_FLD_IDX_TITLE, currentMovie.title, MOVIE_FLD_LENGTH_TITLE);
    get_data_at(MOVIES_LIST, movieIndex, MOVIE_FLD_IDX_GENRE, currentMovie.genre, MOVIE_FLD_LENGTH_GENRE);
    get_data_at(MOVIES_LIST, movieIndex, MOVIE_FLD_IDX_CRITC_RATING, currentMovie.criticRating, MOVIE_FLD_LENGTH_CRITC_RATING);
    get_data_at(MOVIES_LIST, movieIndex, MOVIE_FLD_IDX_USER_RATING, currentMovie.userRating, MOVIE_FLD_LENGTH_USER_RATING);
    get_data_at(MOVIES_LIST, movieIndex, MOVIE_FLD_IDX_RUNTIME, currentMovie.runtime, MOVIE_FLD_LENGTH_RUNTIME);
    get_data_at(MOVIES_LIST, movieIndex, MOVIE_FLD_IDX_RATED, currentMovie.rated, MOVIE_FLD_LENGTH_RATED);
    

    text_layer_set_text(moviesUI.titleTxt, currentMovie.title);
    text_layer_set_text(moviesUI.genreTxt, currentMovie.genre);
    text_layer_set_text(moviesUI.criticRatingTxt, currentMovie.criticRating);
    text_layer_set_text(moviesUI.userRatingTxt, currentMovie.userRating);
    text_layer_set_text(moviesUI.runtimeTxt, currentMovie.runtime);
    text_layer_set_text(moviesUI.ratedTxt, currentMovie.rated);
}


static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    if (currentMovie.id) {
        if (moviesUI.currentMode == MovieUIModeMovies) {
            APP_LOG(APP_LOG_LEVEL_DEBUG, "Next, get movie theatres for movie ID: %s", currentMovie.id);
        } else {
            APP_LOG(APP_LOG_LEVEL_DEBUG, "Next, get showtimes for Movie ID: %s and TheatreID: %s", currentMovie.id, moviesUI.currentTheatreId);
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
    text_layer_destroy(moviesUI.criticRatingTxt);
    text_layer_destroy(moviesUI.genreTxt);
    text_layer_destroy(moviesUI.percentLabelTxt);
    text_layer_destroy(moviesUI.ratedTxt);
    text_layer_destroy(moviesUI.runtimeTxt);
    text_layer_destroy(moviesUI.titleTxt);
    text_layer_destroy(moviesUI.userRatingLabelTxt);
    text_layer_destroy(moviesUI.userRatingTxt);
    text_layer_destroy(moviesUI.minTxt);

    gbitmap_destroy(moviesUI.downIcon);
    gbitmap_destroy(moviesUI.upIcon);
    gbitmap_destroy(moviesUI.selectIcon);

    action_bar_layer_destroy(moviesUI.actionBar);
}

static void draw_line_under_layer(Layer *layer, GContext* ctx) {
    GRect lb = layer_get_bounds(layer);
    graphics_draw_line(ctx, GPoint(lb.origin.x, lb.size.h + lb.origin.y), GPoint(lb.origin.x + lb.size.w, lb.size.h + lb.origin.y));
}

static void draw_outline_around(Layer *layer, GContext* ctx) {
    GRect lb = layer_get_bounds(layer);
    graphics_draw_rect(ctx, lb);
}

#define MOVIE_UI_TITLE_HEIGHT 40
#define MOVIE_UI_Y_SPACING 2
#define MOVIE_UI_RUNTIME_HEIGHT 20
#define MOVIE_UI_RATING_HEIGHT 20
#define MOVIE_UI_INC_Y(y, h) (y+= h+MOVIE_UI_Y_SPACING)

void movies_screen_load(Window *window) {
    Layer *windowLayer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(windowLayer);

    uint16_t paddingSide = 10;
    uint16_t yPos = 5;
    uint16_t allowedWth = bounds.size.w - paddingSide - 25;

    //title
    moviesUI.titleTxt = text_layer_create(GRect(paddingSide, bounds.origin.y + yPos, allowedWth, MOVIE_UI_TITLE_HEIGHT));
    text_layer_set_font(moviesUI.titleTxt, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text_alignment(moviesUI.titleTxt, GTextAlignmentCenter);
    Layer *titleLayer = text_layer_get_layer(moviesUI.titleTxt);
    layer_set_update_proc(titleLayer, draw_line_under_layer);
    layer_add_child(windowLayer, titleLayer);

    MOVIE_UI_INC_Y(yPos, MOVIE_UI_TITLE_HEIGHT);


    moviesUI.runtimeTxt = text_layer_create(GRect(paddingSide, yPos, 40, MOVIE_UI_RUNTIME_HEIGHT));
    text_layer_set_font(moviesUI.runtimeTxt, fonts_get_system_font(FONT_KEY_BITHAM_18_LIGHT_SUBSET));
    text_layer_set_text_alignment(moviesUI.runtimeTxt, GTextAlignmentRight);
    layer_add_child(windowLayer, text_layer_get_layer(moviesUI.runtimeTxt));

    moviesUI.minTxt = text_layer_create(GRect(paddingSide + 40 + 1, yPos, 40, MOVIE_UI_RUNTIME_HEIGHT));
    text_layer_set_font(moviesUI.minTxt, fonts_get_system_font(FONT_KEY_BITHAM_18_LIGHT_SUBSET));
    text_layer_set_text(moviesUI.minTxt, labelMin);
    layer_add_child(windowLayer, text_layer_get_layer(moviesUI.minTxt));

    //rated text
    moviesUI.ratedTxt = text_layer_create(GRect(allowedWth - 40, yPos, 40, MOVIE_UI_RUNTIME_HEIGHT));
    text_layer_set_font(moviesUI.ratedTxt, fonts_get_system_font(FONT_KEY_BITHAM_18_LIGHT_SUBSET));
    text_layer_set_text_alignment(moviesUI.ratedTxt, GTextAlignmentCenter);
    text_layer_set_background_color(moviesUI.ratedTxt, GColorBlack);
    text_layer_set_text_color(moviesUI.ratedTxt, GColorWhite);
    layer_add_child(windowLayer, text_layer_get_layer(moviesUI.ratedTxt));

    MOVIE_UI_INC_Y(yPos, MOVIE_UI_RUNTIME_HEIGHT);

    //rated label
    moviesUI.userRatingLabelTxt = text_layer_create(GRect(0, yPos, 70, MOVIE_UI_RATING_HEIGHT));
    text_layer_set_background_color(moviesUI.userRatingLabelTxt, GColorBlack);
    text_layer_set_text_color(moviesUI.userRatingLabelTxt, GColorWhite);
    text_layer_set_text(moviesUI.userRatingLabelTxt, labelUsers);
    text_layer_set_font(moviesUI.userRatingLabelTxt, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    layer_add_child(windowLayer, text_layer_get_layer(moviesUI.userRatingLabelTxt));
    //rating itself
    moviesUI.userRatingTxt = text_layer_create(GRect(71, yPos, 40, MOVIE_UI_RATING_HEIGHT));
    text_layer_set_font(moviesUI.userRatingTxt, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    Layer *userRatingLayer = text_layer_get_layer(moviesUI.userRatingTxt);
    layer_set_update_proc(userRatingLayer, draw_outline_around);
    layer_add_child(windowLayer, userRatingLayer);


    MOVIE_UI_INC_Y(yPos, MOVIE_UI_RATING_HEIGHT);
    MOVIE_UI_INC_Y(yPos, 5);

    //metacrtic label
    moviesUI.criticRatingLabelTxt = text_layer_create(GRect(0, yPos, 70, MOVIE_UI_RATING_HEIGHT));
    text_layer_set_background_color(moviesUI.criticRatingLabelTxt, GColorBlack);
    text_layer_set_text_color(moviesUI.criticRatingLabelTxt, GColorWhite);
    text_layer_set_text(moviesUI.criticRatingLabelTxt, labelCritics);
    text_layer_set_font(moviesUI.criticRatingLabelTxt, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    layer_add_child(windowLayer, text_layer_get_layer(moviesUI.criticRatingLabelTxt));

    //metacritic itself
    moviesUI.criticRatingTxt = text_layer_create(GRect(71, yPos, 30, MOVIE_UI_RATING_HEIGHT));
    text_layer_set_font(moviesUI.criticRatingTxt, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    Layer *criticRatingLayer = text_layer_get_layer(moviesUI.criticRatingTxt);
    layer_set_update_proc(criticRatingLayer, draw_outline_around);
    layer_add_child(windowLayer, criticRatingLayer);

    //the percentage
    moviesUI.percentLabelTxt = text_layer_create(GRect(101, yPos, 10, MOVIE_UI_RATING_HEIGHT));
    text_layer_set_background_color(moviesUI.percentLabelTxt, GColorBlack);
    text_layer_set_text_color(moviesUI.percentLabelTxt, GColorWhite);
    text_layer_set_text(moviesUI.percentLabelTxt, labelPercent);
    text_layer_set_font(moviesUI.percentLabelTxt, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    layer_add_child(windowLayer, text_layer_get_layer(moviesUI.percentLabelTxt));

    MOVIE_UI_INC_Y(yPos, MOVIE_UI_RATING_HEIGHT);

    //next is the genre
    moviesUI.genreTxt = text_layer_create(GRect(paddingSide, yPos, allowedWth, bounds.size.h - yPos));
    text_layer_set_text_alignment(moviesUI.genreTxt, GTextAlignmentCenter);
    layer_add_child(windowLayer, text_layer_get_layer(moviesUI.genreTxt));

    moviesUI.actionBar = action_bar_layer_create();
    action_bar_layer_add_to_window(moviesUI.actionBar, moviesUI.window);

    //icons
    moviesUI.upIcon = gbitmap_create_with_resource(RESOURCE_ID_ICON_A_BAR_UP_WHITE);
    moviesUI.downIcon = gbitmap_create_with_resource(RESOURCE_ID_ICON_A_BAR_DOWN_WHITE);

    if (moviesUI.currentMode == MovieUIModeMovies) {
        moviesUI.selectIcon = gbitmap_create_with_resource(RESOURCE_ID_ICON_A_BAR_THEATRE_BLACK);
    } else {
        moviesUI.selectIcon = gbitmap_create_with_resource(RESOURCE_ID_ICON_A_BAR_SHOWTIME_BLACK);
    }

    action_bar_layer_set_icon(moviesUI.actionBar, BUTTON_ID_SELECT, moviesUI.selectIcon);
    action_bar_layer_set_click_config_provider(moviesUI.actionBar, movie_click_config_provider);

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
    });


    const bool animated = true;
    window_stack_push(moviesUI.window, animated);
}

