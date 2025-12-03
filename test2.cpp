#include "libs/SerialInterface.h"
// extern "C"{
#include "libs/dosfs.h"
// }
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring> // for memset
#include <fstream>
#include <iostream>

int main(int _argc, char *_argv[]) {
    uint8_t scratch[SECTOR_SIZE], scratch2[SECTOR_SIZE];
    uint32_t pstart, psize, i;
    uint8_t pactive, ptype;
    VOLINFO volInfo;
    DIRINFO dirInfo;
    DIRENT dirEntry;
    uint32_t cache;
    FILEINFO fileInfo;
    uint8_t *readFileBuf;

    // if (_argc < 2) {
    //     printf("Usage: %s [image_file]\n", _argv[0]);
    //     return -1;
    // }

    // Attach user-specified image file
    if (InitDosFsSerialConnection(DEFAULT_PORT, 115200)) {
        printf("Failed to create serial connection");
        return -1;
    }

    // Obtain pointer to first partition on first (only) unit
    pstart = DFS_GetPtnStart(0, scratch, 0, &pactive, &ptype, &psize);
    if (pstart == 0xffffffff) {
        printf("Cannot find first partition\n");
        return -1;
    }

    printf("Partition 0 start sector 0x%-08.8lX active %-02.2hX type %-02.2hX "
           "size %-08.8lX\n",
           pstart, pactive, ptype, psize);

    if (DFS_GetVolInfo(0, scratch, pstart, &volInfo)) {
        printf("Error getting volume information\n");
        return -1;
    }
    printf("Volume label '%-11.11s'\n", volInfo.label);
    printf("%d sector/s per cluster, %d reserved sector/s, volume total %d "
           "sectors.\n",
           volInfo.secperclus, volInfo.reservedsecs, volInfo.numsecs);
    printf("%d sectors per FAT, first FAT at sector #%d, root dir at #%d.\n",
           volInfo.secperfat, volInfo.fat1, volInfo.rootdir);
    printf("(For FAT32, the root dir is a CLUSTER number, FAT12/16 it is a "
           "SECTOR number)\n");
    printf("%d root dir entries, data area commences at sector #%d.\n",
           volInfo.rootentries, volInfo.dataarea);
    printf("%d clusters (%d bytes) in data area, filesystem IDd as ",
           volInfo.numclusters,
           volInfo.numclusters * volInfo.secperclus * SECTOR_SIZE);
    if (volInfo.filesystem == FAT12)
        printf("FAT12.\n");
    else if (volInfo.filesystem == FAT16)
        printf("FAT16.\n");
    else if (volInfo.filesystem == FAT32)
        printf("FAT32.\n");
    else
        printf("[unknown]\n");

    //------------------------------------------------------------
    // Directory enumeration test
    dirInfo.scratch = scratch;
    //	if (DFS_OpenDir(&volInfo, "", &dirInfo)) {
    //		printf("Error opening root directory\n");
    //		return -1;
    //	}
    if (DFS_OpenDir(&volInfo, (uint8_t *)"/MYDIR1", &dirInfo)) {
        printf("error opening subdirectory\n");
        return -1;
    }
    while (!DFS_GetNext(&volInfo, &dirInfo, &dirEntry)) {
        if (dirEntry.name[0])
            printf("file: '%-11.11s'\n", dirEntry.name);
    }

    //------------------------------------------------------------
    // Unlink test
    //	cache = 0;
    //	printf("*** FAT BEFORE ***\n");
    //  	for (i=0;i<volInfo.numclusters;i++) {
    //		printf("entry %-08.8x, %-08.8X\n", i, DFS_GetFAT(&volInfo,
    // sector, &cache, i));
    //	}
    //	if (DFS_UnlinkFile(&volInfo, "MYDIR1/SUBDIR1.2/README.TXT", sector)) {
    //		printf("error unlinking file\n");
    //	}
    //	printf("*** FAT AFTER ***\n");
    //  	for (i=0;i<volInfo.numclusters;i++) {
    //		printf("entry %-08.8x, %-08.8X\n", i, DFS_GetFAT(&volInfo,
    // sector, &cache, i));
    //	}

    // TODO: uncomment this
    //------------------------------------------------------------
    // File write test
    // if (DFS_OpenFile(&volInfo, (uint8_t *)"MYDIR1/WRTEST.TXT", DFS_WRITE,
    //                  scratch, &fileInfo)) {
    //     printf("error opening file\n");
    //     return -1;
    // }
    // for (i = 0; i < 18; i++) {
    //     std::memset(scratch2, 128 + i, SECTOR_SIZE);
    //     DFS_WriteFile(&fileInfo, scratch, scratch2, &cache, SECTOR_SIZE / 2);
    //     std::cout << "wrote " << cache << " bytes\n";
    //     std::memset(scratch2 + 256, 255 - i, SECTOR_SIZE / 2);
    //     DFS_WriteFile(&fileInfo, scratch, scratch2 + 256, &cache,
    //                   SECTOR_SIZE / 2);
    //     std::cout << "wrote " << cache << " bytes\n";
    // }
    // sprintf((char *)scratch2, "test string at the end...");
    // DFS_WriteFile(&fileInfo, scratch, scratch2, &cache,
    //               strlen((char *)scratch2));
    // std::cout << "wrote " << cache << " bytes\n";

    //------------------------------------------------------------
    // File read test
    printf("Readback test\n");
    if (DFS_OpenFile(&volInfo, (uint8_t *)"MYDIR1/WRTEST.TXT", DFS_READ,
                     scratch, &fileInfo)) {
        printf("error opening file\n");
        return -1;
    }
    readFileBuf = (uint8_t *)malloc(fileInfo.filelen + 512);
    std::memset(readFileBuf, 0xaa, fileInfo.filelen + 512);

    std::cout << "before: " << (std::uintptr_t)readFileBuf << "\n";
    DFS_ReadFile(&fileInfo, scratch, readFileBuf, &i, fileInfo.filelen);
    std::cout << "after: " << (std::uintptr_t)readFileBuf << "\n";
    printf("read complete %d bytes (expected %d) pointer %d\n", i,
           fileInfo.filelen, fileInfo.pointer);

    std::ofstream outFile("out.txt", std::ios::binary);
    outFile.write(reinterpret_cast<const char *>(readFileBuf),
                  fileInfo.filelen);

    return 0;
}
