#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <linux/input.h>
#include <sys/ioctl.h>

#define MAX_DEVICES 16

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s /dev/input/eventX1 /dev/input/eventX2 ...\n", argv[0]);
        return 1;
    }

    if (argc - 1 > MAX_DEVICES) {
        fprintf(stderr, "Error: Too many devices. Max is %d.\n", MAX_DEVICES);
        return 1;
    }

    int num_devices = argc - 1;
    struct pollfd fds[MAX_DEVICES];
    char device_names[MAX_DEVICES][256];

    // Открываем все устройства
    for (int i = 0; i < num_devices; i++) {
        int fd = open(argv[i + 1], O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            perror("Failed to open device");
            return 1;
        }

        // Получаем имя устройства
        if (ioctl(fd, EVIOCGNAME(sizeof(device_names[i])), device_names[i]) < 0) {
            strcpy(device_names[i], "Unknown");
        }

        fds[i].fd = fd;
        fds[i].events = POLLIN;
    }

    printf("Monitoring %d devices. Press Ctrl+C to exit.\n", num_devices);

    while (1) {
        // Ждем события
        int ret = poll(fds, num_devices, -1);
        if (ret < 0) {
            perror("poll failed");
            break;
        }

        // Проверяем устройства с событиями
        for (int i = 0; i < num_devices; i++) {
            if (fds[i].revents & POLLIN) {
                struct input_event ev;
                
                // Читаем все доступные события
                while (read(fds[i].fd, &ev, sizeof(ev)) == sizeof(ev)) {
                    printf("[%s] type: %d, code: %d, value: %d\n", 
                           device_names[i], ev.type, ev.code, ev.value);
                }
            }
        }
    }

    // Закрываем устройства
    for (int i = 0; i < num_devices; i++) {
        close(fds[i].fd);
    }

    return 0;
}