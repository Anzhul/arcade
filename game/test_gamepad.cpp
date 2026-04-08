#include <linux/joystick.h>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <sys/ioctl.h>

int main() {
    // Try joystick API
    printf("=== Testing /dev/input/js0 (joystick API) ===\n");
    int fd = open("/dev/input/js0", O_RDONLY);
    if (fd < 0) {
        printf("Could not open /dev/input/js0\n");
    } else {
        char name[128] = "Unknown";
        ioctl(fd, JSIOCGNAME(sizeof(name)), name);
        unsigned char axes = 0, buttons = 0;
        ioctl(fd, JSIOCGAXES, &axes);
        ioctl(fd, JSIOCGBUTTONS, &buttons);
        printf("Controller: %s\n", name);
        printf("Axes: %d, Buttons: %d\n", axes, buttons);
        close(fd);
    }

    // Use evdev instead - more reliable for USB gamepads
    printf("\n=== Testing /dev/input/event4 (evdev API) ===\n");
    printf("Press buttons and D-pad. Ctrl+C to quit.\n\n");

    fd = open("/dev/input/event4", O_RDONLY);
    if (fd < 0) {
        printf("Could not open /dev/input/event4\n");
        return 1;
    }

    char name[128] = "Unknown";
    ioctl(fd, EVIOCGNAME(sizeof(name)), name);
    printf("Device: %s\n\n", name);

    struct input_event ev;
    while (read(fd, &ev, sizeof(ev)) == sizeof(ev)) {
        if (ev.type == EV_KEY) {
            printf("BUTTON code=%d %s\n", ev.code, ev.value ? "PRESSED" : "RELEASED");
        } else if (ev.type == EV_ABS) {
            printf("AXIS code=%d value=%d\n", ev.code, ev.value);
        }
    }

    close(fd);
    return 0;
}
