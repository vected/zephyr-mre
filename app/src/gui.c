#include "gui.h"

#include <lvgl.h>

void render_test_image(lv_obj_t *parent)
{
    static lv_style_t st;
    lv_style_init(&st);
    lv_style_set_pad_all(&st, 0);
    lv_style_set_radius(&st, 0);
    lv_style_set_bg_color(&st, lv_color_black());
    lv_style_set_border_color(&st, lv_color_white());
    lv_style_set_border_width(&st, 1);

    lv_obj_t *border = lv_obj_create(parent);
    lv_obj_set_size(border, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(border, &st, 0);
    lv_obj_align(border, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *half_border = lv_obj_create(parent);
    lv_obj_set_size(half_border, LV_PCT(50), LV_PCT(50));
    lv_obj_add_style(half_border, &st, 0);
    lv_obj_align(half_border, LV_ALIGN_CENTER, 0, 0);

    lv_coord_t w         = lv_obj_get_width(parent);
    lv_coord_t h         = lv_obj_get_height(parent);
    lv_coord_t grid_size = h / 6;

    for (lv_coord_t y = grid_size; y < h; y += grid_size) {
        lv_obj_t *line = lv_obj_create(parent);
        lv_obj_align(line, LV_ALIGN_TOP_LEFT, 0, y - 1);
        lv_obj_set_size(line, LV_PCT(100), 2);
        lv_obj_add_style(line, &st, 0);
    }

    for (lv_coord_t x = grid_size; x < w; x += grid_size) {
        lv_obj_t *line = lv_obj_create(parent);
        lv_obj_align(line, LV_ALIGN_TOP_LEFT, x - 1, 0);
        lv_obj_set_size(line, 2, LV_PCT(100));
        lv_obj_add_style(line, &st, 0);
    }

    int num_circles = Z_MIN(w, h) / (2 * grid_size);
    for (int i = 1; i <= num_circles; i++) {
        lv_obj_t *circle = lv_obj_create(parent);
        lv_obj_set_style_pad_all(circle, 0, 0);
        lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(circle, 0, 0);
        lv_obj_set_style_border_color(circle, lv_color_white(), 0);
        lv_obj_set_style_border_width(circle, 1, 0);
        lv_obj_align(circle, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_size(circle, 2 * i * grid_size, 2 * i * grid_size);
    }
}

void gui_init(void)
{
    /* Set background color to black transparent */
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(lv_layer_bottom(), LV_OPA_TRANSP, LV_PART_MAIN);

    render_test_image(lv_screen_active());
}
