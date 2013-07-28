/*
 * Copyright (C) 2013 Marcin Chojnacki
 * Copyright (C) 2013 Koushik Dutta
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#define BML_UNLOCK_ALL 0x8A29
#define BOARD_BML_BOOT "/dev/block/bml7"
#define BOARD_BML_RECOVERY "/dev/block/bml8"

int restore_internal(const char* bml, const char* filename)
{
    char buf[4096];
    int dstfd, srcfd, bytes_read, bytes_written, total_read = 0;
    if (filename == NULL)
        srcfd = 0;
    else {
        srcfd = open(filename, O_RDONLY | O_LARGEFILE);
        if (srcfd < 0)
            return 2;
    }
    dstfd = open(bml, O_RDWR | O_LARGEFILE);
    if (dstfd < 0)
        return 3;
    if (ioctl(dstfd, BML_UNLOCK_ALL, 0))
        return 4;
    do {
        total_read += bytes_read = read(srcfd, buf, 4096);
        if (!bytes_read)
            break;
        if (bytes_read < 4096)
            memset(&buf[bytes_read], 0, 4096 - bytes_read);
        if (write(dstfd, buf, 4096) < 4096)
            return 5;
    } while(bytes_read == 4096);
    
    close(dstfd);
    close(srcfd);
    
    return 0;
}

int restore_raw_partition(const char *partition, const char *filename)
{
    if (strcmp(partition, "boot") != 0 && strcmp(partition, "recovery") != 0 && strcmp(partition, "recoveryonly") != 0 && partition[0] != '/')
        return 6;

    int ret = -1;
    if (strcmp(partition, "recoveryonly") != 0) {
        // always restore boot, regardless of whether recovery or boot is flashed.
        // this is because boot and recovery are the same on some samsung phones.
        // unless of course, recoveryonly is explictly chosen (bml8)
        ret = restore_internal(BOARD_BML_BOOT, filename);
        if (ret != 0)
            return ret;
    }

    if (strcmp(partition, "recovery") == 0 || strcmp(partition, "recoveryonly") == 0)
        ret = restore_internal(BOARD_BML_RECOVERY, filename);

    // support explicitly provided device paths
    if (partition[0] == '/')
        ret = restore_internal(partition, filename);
    return ret;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("usage: flashbml partition filename\n");
        return 1;
    }

    printf("flashing %s to %s...\n", argv[2], argv[1]);
    int ret = restore_raw_partition(argv[1], argv[2]);

    if (ret == 0)
        printf("flashing successful!\n");
    else if (ret == 6)
        printf("partition must be: boot, recovery or recoveryonly\n");
    else
        printf("flashing error, code: %d\n", ret);

    return ret;
}