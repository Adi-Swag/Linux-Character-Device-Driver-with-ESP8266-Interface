#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

#define BUF_SIZE 100

char write_buf[BUF_SIZE];
char read_buf[BUF_SIZE];

int main()
{
    int fd;
    int device_no;
    char device_path[50];
    char option;

    // 🔹 Choose device
    printf("Enter device number (e.g., 0 for /dev/device_0): ");
    scanf("%d", &device_no);

    snprintf(device_path, sizeof(device_path), "/dev/device_%d", device_no);

    fd = open(device_path, O_RDWR);
    if (fd < 0) {
        perror("Cannot open device file");
        return -1;
    }

    printf("Opened %s successfully!\n", device_path);

    while (1) {
        printf("\n**** Please Enter the Option ****\n");
        printf("1. Write\n");
        printf("2. Read\n");
        printf("3. Exit\n");
        printf("********************************\n");

        scanf(" %c", &option);

        switch (option) {

        case '1': {
            printf("Enter string: ");

            getchar(); // clear newline
            fgets(write_buf, BUF_SIZE, stdin);

            // remove newline
            write_buf[strcspn(write_buf, "\n")] = '\0';

            // 🔥 overwrite mode (like echo > file)
            lseek(fd, 0, SEEK_SET);

            ssize_t ret = write(fd, write_buf, strlen(write_buf));
            if (ret < 0) {
                perror("Write failed");
            } else {
                printf("Write successful (%ld bytes)\n", ret);
            }
            break;
        }

        case '2': {
            printf("Reading...\n");

            memset(read_buf, 0, BUF_SIZE);

            // 🔥 reset offset before read
            lseek(fd, 0, SEEK_SET);

            ssize_t ret = read(fd, read_buf, BUF_SIZE - 1);

            if (ret < 0) {
                perror("Read failed");
            } else {
                read_buf[ret] = '\0';  // ensure string
                printf("Data (%ld bytes): %s\n", ret, read_buf);
            }
            break;
        }

        case '3':
            close(fd);
            printf("Exiting...\n");
            return 0;

        default:
            printf("Invalid option!\n");
        }
    }

    return 0;
}