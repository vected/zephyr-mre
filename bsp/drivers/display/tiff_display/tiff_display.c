#define DT_DRV_COMPAT custom_tiff_display

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(tiff_display, CONFIG_DISPLAY_LOG_LEVEL);

struct tiff_display_config {
    uint16_t width;
    uint16_t height;
    const char *framebuffer_path;
    const enum display_pixel_format pixel_format;
};

struct tiff_display_data {
    struct k_mutex lock;
};

static int tiff_display_init(const struct device *dev)
{
    const struct tiff_display_config *config = dev->config;
    struct tiff_display_data *data           = dev->data;

    (void)k_mutex_init(&data->lock);

    LOG_INF("Display init");

    int err = 0;

    struct fs_file_t file;
    fs_file_t_init(&file);

    err = fs_open(&file, config->framebuffer_path, FS_O_WRITE | FS_O_CREATE);
    if (0 != err) {
        LOG_ERR("Failed to create framebuffer");
        goto end;
    }

    err = fs_truncate(&file, (uint32_t)config->width * (uint32_t)config->height * (DISPLAY_BITS_PER_PIXEL(config->pixel_format) / 8));
    if (0 != err) {
        LOG_ERR("Failed to initialize framebuffer");
        goto close;
    }

close:
    fs_close(&file);

end:
    return err;
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

static int tiff_display_write(const struct device *dev, const uint16_t x,
                              const uint16_t y,
                              const struct display_buffer_descriptor *desc,
                              const void *buf)
{
    const struct tiff_display_config *config = dev->config;
    struct tiff_display_data *data           = dev->data;

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

    int err = 0;

    struct fs_file_t file;
    fs_file_t_init(&file);

    err = fs_open(&file, config->framebuffer_path, FS_O_WRITE);
    if (0 != err) {
        LOG_ERR("Failed to open framebuffer (%d)", err);
        goto end;
    }

    for (uint32_t pos_y = 0; pos_y < update_height; pos_y++) {
        k_msleep(100);
        LOG_INF("Writing line %d", pos_y + update_y);
        err = fs_seek(&file, (pos_y + update_y) * screen_pitch + update_x * bytes_per_pixel, FS_SEEK_SET);
        if (0 != err) {
            LOG_ERR("Failed to seek to position in framebuffer (%d)", err);
            goto close;
        }

        err = fs_write_all(&file, pixel_buf + pos_y * update_pitch, update_width * bytes_per_pixel);
        if (0 != err) {
            LOG_ERR("Failed to write data into framebuffer (%d)", err);
            goto close;
        }
    }

close:
    fs_close(&file);

end:
    k_mutex_unlock(&data->lock);
    return err;
}

static int tiff_display_clear(const struct device *dev)
{
    const struct tiff_display_config *config = dev->config;
    struct tiff_display_data *data           = dev->data;

    k_mutex_lock(&data->lock, K_FOREVER);

    LOG_INF("Display clear");

    int err = 0;

    struct fs_file_t file;
    fs_file_t_init(&file);

    err = fs_open(&file, config->framebuffer_path, FS_O_WRITE | FS_O_TRUNC);
    if (0 != err) {
        LOG_ERR("Failed to open and truncate framebuffer");
        goto end;
    }

    err = fs_truncate(&file, (uint32_t)config->width * (uint32_t)config->height * (DISPLAY_BITS_PER_PIXEL(config->pixel_format) / 8));
    if (0 != err) {
        LOG_ERR("Failed to write zeros to framebuffer");
        goto close;
    }

close:
    fs_close(&file);

end:
    k_mutex_unlock(&data->lock);
    return err;
}

static void tiff_display_get_capabilities(const struct device *dev,
                                          struct display_capabilities *capabilities)
{
    const struct tiff_display_config *config = dev->config;

    *capabilities = (struct display_capabilities){
        .x_resolution            = config->width,
        .y_resolution            = config->height,
        .supported_pixel_formats = config->pixel_format,
        .current_pixel_format    = config->pixel_format,
        .current_orientation     = DISPLAY_ORIENTATION_NORMAL,
    };

    LOG_INF("Display get capabilities");
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
        .pixel_format     = PIXEL_FORMAT_ARGB_8888,                               \
    };                                                                            \
                                                                                  \
    static struct tiff_display_data tiff_display_data_##inst;                     \
                                                                                  \
    DEVICE_DT_INST_DEFINE(inst, tiff_display_init, NULL,                          \
                          &tiff_display_data_##inst, &tiff_display_config_##inst, \
                          POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY,              \
                          &tiff_display_api);

DT_INST_FOREACH_STATUS_OKAY(TIFF_DISPLAY_DEFINE)
