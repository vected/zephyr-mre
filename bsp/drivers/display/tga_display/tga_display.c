#define DT_DRV_COMPAT custom_tga_display

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(tga_display, CONFIG_DISPLAY_LOG_LEVEL);

struct tga_display_config {
    uint16_t width;
    uint16_t height;
    const char *framebuffer_path;
    const enum display_pixel_format pixel_format;
    uint8_t *framebuffer;
    const uint8_t *tga_header;
    size_t tga_header_size;
};

struct tga_display_data {
    struct k_mutex lock;
};

static int tga_display_init(const struct device *dev)
{
    struct tga_display_data *data = dev->data;

    (void)k_mutex_init(&data->lock);

    LOG_INF("Display init");

    return 0;
}

int fs_write_all(struct fs_file_t *zfp, const void *ptr, size_t size)
{
    const uint8_t *data = (const uint8_t *)ptr;

    while (size != 0) {
        ssize_t ret = fs_write(zfp, data, size);
        if (ret < 0) {
            return ret;
        }
        if (ret == 0) {
            return -EIO;
        }
        size -= ret;
        data += ret;
    }

    return 0;
}

static int tga_display_write(const struct device *dev, const uint16_t x,
                             const uint16_t y,
                             const struct display_buffer_descriptor *desc,
                             const void *buf)
{
    const struct tga_display_config *config = dev->config;
    struct tga_display_data *data           = dev->data;

    int err = 0;

    k_mutex_lock(&data->lock, K_FOREVER);

    const uint32_t bytes_per_pixel = DISPLAY_BITS_PER_PIXEL(config->pixel_format) / 8;
    const uint32_t screen_width    = config->width;
    const uint32_t screen_pitch    = screen_width * bytes_per_pixel;
    const uint32_t update_x        = x;
    const uint32_t update_y        = y;
    const uint32_t update_width    = desc->width;
    const uint32_t update_height   = desc->height;
    const uint32_t update_pitch    = desc->pitch * bytes_per_pixel;

    LOG_INF("Display write at (%d, %d): %d x %d", x, y, desc->width, desc->height);
    const uint8_t *pixel_buf = (const uint8_t *)buf;

    for (uint32_t pos_y = 0; pos_y < update_height; pos_y++) {
        memcpy(config->framebuffer + (pos_y + update_y) * screen_pitch + update_x * bytes_per_pixel,
               pixel_buf + pos_y * update_pitch,
               update_width * bytes_per_pixel);
    }

    if (desc->frame_incomplete) {
        goto end;
    }

    LOG_INF("Frame finished, writing to disk ...");

    struct fs_file_t file;
    fs_file_t_init(&file);

    err = fs_open(&file, config->framebuffer_path, FS_O_WRITE | FS_O_CREATE | FS_O_TRUNC);
    if (0 != err) {
        LOG_ERR("Failed to open framebuffer (%d)", err);
        goto end;
    }

    err = fs_write_all(&file, config->tga_header, config->tga_header_size);
    if (0 != err) {
        LOG_ERR("Failed to write tga header into framebuffer (%d)", err);
        goto close;
    }

    err = fs_write_all(&file, config->framebuffer, config->width * config->height * (DISPLAY_BITS_PER_PIXEL(config->pixel_format) / 8));
    if (0 != err) {
        LOG_ERR("Failed to write data into framebuffer (%d)", err);
        goto close;
    }

close:
    fs_close(&file);

    if (0 == err) {
        LOG_INF("Frame writing finished");
    }

end:
    k_mutex_unlock(&data->lock);
    return err;
}

static int tga_display_clear(const struct device *dev)
{
    const struct tga_display_config *config = dev->config;
    struct tga_display_data *data           = dev->data;

    k_mutex_lock(&data->lock, K_FOREVER);

    LOG_INF("Display clear");

    memset(config->framebuffer, 0, config->width * config->height * (DISPLAY_BITS_PER_PIXEL(config->pixel_format) / 8));

    k_mutex_unlock(&data->lock);

    return 0;
}

static void tga_display_get_capabilities(const struct device *dev,
                                         struct display_capabilities *capabilities)
{
    const struct tga_display_config *config = dev->config;

    *capabilities = (struct display_capabilities){
        .x_resolution            = config->width,
        .y_resolution            = config->height,
        .supported_pixel_formats = config->pixel_format,
        .current_pixel_format    = config->pixel_format,
        .current_orientation     = DISPLAY_ORIENTATION_NORMAL,
    };

    LOG_INF("Display get capabilities");
}

static const struct display_driver_api tga_display_api = {
    .write            = tga_display_write,
    .clear            = tga_display_clear,
    .get_capabilities = tga_display_get_capabilities,
};

#define TGA_DISPLAY_WIDTH(n)            DT_INST_PROP(n, width)
#define TGA_DISPLAY_HEIGHT(n)           DT_INST_PROP(n, height)
#define TGA_DISPLAY_FRAMEBUFFER_PATH(n) DT_INST_PROP(n, framebuffer_path)

#define TGA_PIXEL_FORMAT                PIXEL_FORMAT_ARGB_8888

#define TGA_HEADER(WIDTH, HEIGHT)                                                                \
    {                                                                                            \
        0,                                       /* no image id */                               \
        0,                                       /* no palette */                                \
        2,                                       /* color */                                     \
        0, 0, 0, 0, 0,                           /* palette data */                              \
        0, 0, 0, 0,                              /* zero point coordinates */                    \
        (WIDTH) & 0xff, ((WIDTH) >> 8) & 0xff,   /* width */                                     \
        (HEIGHT) & 0xff, ((HEIGHT) >> 8) & 0xff, /* height */                                    \
        32,                                      /* bits per pixel */                            \
        0x28,                                    /* 8 bit alpha, left to right, top to bottom */ \
    }

#define TGA_DISPLAY_DEFINE(inst)                                                                                                                     \
    BUILD_ASSERT(TGA_DISPLAY_WIDTH(inst) <= UINT16_MAX);                                                                                             \
    BUILD_ASSERT(TGA_DISPLAY_HEIGHT(inst) <= UINT16_MAX);                                                                                            \
                                                                                                                                                     \
    static uint8_t tga_display_framebuffer##inst[TGA_DISPLAY_WIDTH(inst) * TGA_DISPLAY_HEIGHT(inst) * DISPLAY_BITS_PER_PIXEL(TGA_PIXEL_FORMAT) / 8]; \
    static uint8_t tga_header##inst[] = TGA_HEADER(TGA_DISPLAY_WIDTH(inst), TGA_DISPLAY_HEIGHT(inst));                                               \
                                                                                                                                                     \
    static const struct tga_display_config tga_display_config_##inst = {                                                                             \
        .width            = TGA_DISPLAY_WIDTH(inst),                                                                                                 \
        .height           = TGA_DISPLAY_HEIGHT(inst),                                                                                                \
        .framebuffer_path = TGA_DISPLAY_FRAMEBUFFER_PATH(inst),                                                                                      \
        .pixel_format     = TGA_PIXEL_FORMAT,                                                                                                        \
        .framebuffer      = tga_display_framebuffer##inst,                                                                                           \
        .tga_header       = tga_header##inst,                                                                                                        \
        .tga_header_size  = sizeof(tga_header##inst),                                                                                                \
    };                                                                                                                                               \
                                                                                                                                                     \
    static struct tga_display_data tga_display_data_##inst;                                                                                          \
                                                                                                                                                     \
    DEVICE_DT_INST_DEFINE(inst, tga_display_init, NULL,                                                                                              \
                          &tga_display_data_##inst, &tga_display_config_##inst,                                                                      \
                          POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY,                                                                                 \
                          &tga_display_api);

DT_INST_FOREACH_STATUS_OKAY(TGA_DISPLAY_DEFINE)
