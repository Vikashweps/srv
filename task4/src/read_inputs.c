#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <string.h>
#include <sys/ioctl.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s /dev/input/eventX\n", argv[0]);
        return 1;
    }

    const char *device_path = argv[1];

    // Проверка прав доступа
    if (access(device_path, R_OK) != 0) {
        perror("Access check failed");
        return 1;
    }

    // Открытие устройства
    int fd = open(device_path, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }

    /*  ioctl для получения информации */
    char device_name[256];
    if (ioctl(fd, EVIOCGNAME(sizeof(device_name)), device_name) < 0) {
        perror("Failed to get device name");
    } else {
        printf("Device name: %s\n", device_name);
    }

    char device_phys[256];
    if (ioctl(fd, EVIOCGPHYS(sizeof(device_phys)), device_phys) < 0) {
        perror("Failed to get physical path");
    } else {
        printf("Physical path: %s\n", device_phys);
    }

    printf("Reading events from %s:\n", device_path);

    struct input_event ev;
    while (1) {
        // Чтение события
        ssize_t bytes = read(fd, &ev, sizeof(ev));
        if (bytes != sizeof(ev)) {
            perror("Failed to read event");
            break;
        }

        // Вывод полей структуры (Задание 1)
        printf("type: %d, code: %d, value: %d\n", ev.type, ev.code, ev.value);
    }

    close(fd);
    return 0;
}