#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>

#include <app_version.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "gui.h"

LOG_MODULE_REGISTER(app, CONFIG_APP_LOG_LEVEL);

int main(void)
{
    LOG_INF("My Test Application %s (git: %s)", APP_VERSION_STRING, STRINGIFY(APP_BUILD_VERSION));

    gui_run();

    return 0;
}
