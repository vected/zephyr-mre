#define DT_DRV_COMPAT custom_tiff_display

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(tiff_display, CONFIG_DISPLAY_LOG_LEVEL);

struct tiff_display_config {
    uint16_t width;
    uint16_t height;
    const char *framebuffer_path;
};

struct tiff_display_data {
    struct k_mutex lock;
};

static int tiff_display_init(const struct device *dev)
{
    //    const struct tiff_display_config *config = dev->config;
    struct tiff_display_data *data = dev->data;

    (void)k_mutex_init(&data->lock);

    return 0;
}

static int tiff_display_write(const struct device *dev, const uint16_t x,
                              const uint16_t y,
                              const struct display_buffer_descriptor *desc,
                              const void *buf)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(buf);

    // const struct tiff_display_config *config = dev->config;

    // const uint32_t screen_width  = config->width;
    // const uint32_t screen_pitch  = screen_width * BYTES_PER_PIXEL;
    // const uint32_t update_x      = x;
    // const uint32_t update_y      = y;
    // const uint32_t update_width  = desc->width;
    // const uint32_t update_height = desc->height;
    // const uint32_t update_pitch  = desc->pitch * BYTES_PER_PIXEL;

    LOG_DBG("Display Write at (%d, %d): %d x %d", x, y, desc->width, desc->height);

    int err = -ENOSYS;

    return 0;
}

static int tiff_display_clear(const struct device *dev)
{
    ARG_UNUSED(dev);

    // const struct tiff_display_config *config = dev->config;

    // const uint32_t framebuffer   = config->framebuffer0;
    // const uint32_t screen_width  = config->width;
    // const uint32_t screen_pitch  = screen_width * BYTES_PER_PIXEL;
    // const uint32_t screen_height = config->height;
    // const uint32_t screen_size   = screen_height * screen_pitch;

    int err = -ENOSYS;

    return 0;
}

static void tiff_display_get_capabilities(const struct device *dev,
                                          struct display_capabilities *capabilities)
{
    const struct tiff_display_config *config = dev->config;

    *capabilities = (struct display_capabilities){
        .x_resolution            = config->width,
        .y_resolution            = config->height,
        .supported_pixel_formats = PIXEL_FORMAT_ARGB_8888,
        .current_pixel_format    = PIXEL_FORMAT_ARGB_8888,
        .current_orientation     = DISPLAY_ORIENTATION_NORMAL,
    };
}

static const struct display_driver_api tiff_display_api = {
    .write            = tiff_display_write,
    .clear            = tiff_display_clear,
    .get_capabilities = tiff_display_get_capabilities,
};

#define TIFF_DISPLAY_WIDTH(n)            DT_INST_PROP(n, width)
#define TIFF_DISPLAY_HEIGHT(n)           DT_INST_PROP(n, height)
#define TIFF_DISPLAY_FRAMEBUFFER_PATH(n) DT_INST_PROP(n, framebuffer_path)

#define TIFF_DISPLAY_DEFINE(inst)                                                 \
    BUILD_ASSERT(TIFF_DISPLAY_WIDTH(inst) <= UINT16_MAX);                         \
    BUILD_ASSERT(TIFF_DISPLAY_HEIGHT(inst) <= UINT16_MAX);                        \
                                                                                  \
    static const struct tiff_display_config tiff_display_config_##inst = {        \
        .width            = TIFF_DISPLAY_WIDTH(inst),                             \
        .height           = TIFF_DISPLAY_HEIGHT(inst),                            \
        .framebuffer_path = TIFF_DISPLAY_FRAMEBUFFER_PATH(inst),                  \
    };                                                                            \
                                                                                  \
    static struct tiff_display_data tiff_display_data_##inst;                     \
                                                                                  \
    DEVICE_DT_INST_DEFINE(inst, tiff_display_init, NULL,                          \
                          &tiff_display_data_##inst, &tiff_display_config_##inst, \
                          POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY,              \
                          &tiff_display_api);

DT_INST_FOREACH_STATUS_OKAY(TIFF_DISPLAY_DEFINE)
