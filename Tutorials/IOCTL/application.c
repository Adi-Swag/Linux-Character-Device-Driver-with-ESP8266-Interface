#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define BUF_SIZE 100


#define MY_IOCTL_BASE 'a'
#define CMD_CLEAR            _IO(MY_IOCTL_BASE, 1)
#define CMD_GET_BUFFER_SIZE  _IOR(MY_IOCTL_BASE, 2, int)
#define CMD_SET_APPEND_MODE  _IOW(MY_IOCTL_BASE, 3, int)
#define CMD_RESET_OFFSET     _IO(MY_IOCTL_BASE, 4)

char write_buf[BUF_SIZE];
char read_buf[BUF_SIZE];

int main()
{
    int fd;
    int device_no;
    char device_path[50];
    char option;

    printf("Enter device number (e.g., 0 for /dev/device_0): ");
    scanf("%d", &device_no);

    snprintf(device_path, sizeof(device_path), "/dev/device_%d", device_no);

    fd = open(device_path, O_RDWR);
    if (fd < 0) {
        perror("Cannot open device");
        return -1;
    }

    printf("Opened %s successfully!\n", device_path);

    while (1) {
        printf("\n====== MENU ======\n");
        printf("1. Write\n");
        printf("2. Read\n");
        printf("3. Clear Buffer (ioctl)\n");
        printf("4. Get Buffer Size (ioctl)\n");
        printf("5. Set Append Mode (ioctl)\n");
        printf("6. Reset Offset (ioctl)\n");
        printf("7. Exit\n");
        printf("==================\n");

        scanf(" %c", &option);

        switch(option) {

        case '1':
            printf("Enter string: ");
            getchar();
            fgets(write_buf, BUF_SIZE, stdin);
            write_buf[strcspn(write_buf, "\n")] = '\0';
            lseek(fd, 0, SEEK_SET);
            if (write(fd, write_buf, strlen(write_buf)) < 0)
                perror("Write failed");
            else
                printf("Write successful!\n");
            break;

        case '2': {
            memset(read_buf, 0, BUF_SIZE);

            lseek(fd, 0, SEEK_SET);  // safe read from start

            int ret = read(fd, read_buf, BUF_SIZE - 1);
            if (ret < 0)
                perror("Read failed");
            else {
                read_buf[ret] = '\0';
                printf("Data (%d bytes): %s\n", ret, read_buf);
            }
            break;
        }

        case '3':
            ioctl(fd, CMD_CLEAR);
            printf("Buffer cleared\n");
            break;

        case '4': {
            int size = 0;
            if (ioctl(fd, CMD_GET_BUFFER_SIZE, &size) < 0)
                perror("ioctl GET_BUFFER_SIZE failed");
            else
                printf("Buffer Size = %d\n", size);
            break;
        }

        case '5': {
            int mode;
            printf("Enter append mode (0 = OFF, 1 = ON): ");
            scanf("%d", &mode);

            if (ioctl(fd, CMD_SET_APPEND_MODE, &mode) < 0)
                perror("ioctl SET_APPEND_MODE failed");
            else
                printf("Append mode set to %d\n", mode);
            break;
        }

        case '6':
            ioctl(fd, CMD_RESET_OFFSET);
            printf("Offset reset\n");
            break;

        case '7':
            close(fd);
            printf("Exiting...\n");
            return 0;

        default:
            printf("Invalid option\n");
        }
    }

    return 0;
}