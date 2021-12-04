#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#pragma pack(push, 1)
typedef struct {
    char name[4];
    char rSize[4];
    char flags[2];
} FRAMEHEADER;
#pragma pack(pop)

void getFrame(FILE *file, char prop_name[4]) {
    FRAMEHEADER tmp;
    unsigned char mainHeader[10];
    fread(mainHeader, 1, 10, file);
    unsigned int frameSize;
    while (1) {
        fread(&tmp, sizeof(FRAMEHEADER), 1, file);
        frameSize = (tmp.rSize[0] << 21) + (tmp.rSize[1] << 14) + (tmp.rSize[2] << 7) + tmp.rSize[3];
        if (frameSize == 0) {
            printf("Error in \"--get\" function: No such prop_name.\n");
            fclose(file);
            return;
        }
        unsigned char *value = malloc(frameSize);
        if (strcmp(prop_name, tmp.name) == 0) {
            printf("%s:", prop_name);
            for (int i = 0; i < frameSize; i++) {
                value[i] = fgetc(file);
                printf("%c", value[i]);
            }
            printf("\n");
            return;
        } else {
            for (int i = 0; i < frameSize; i++) {
                value[i] = fgetc(file);
            }
        }
    }
}

void showFrames(FILE *file) {
    FRAMEHEADER tmp;
    unsigned int valueMaxLen = 0, framesCnt = 0;
    unsigned char mainHeader[10];
    fread(mainHeader, 1, 10, file);
    unsigned int frameSize;
    while (1) {
        fread(&tmp, sizeof(FRAMEHEADER), 1, file);
        frameSize = (tmp.rSize[0] << 21) + (tmp.rSize[1] << 14) + (tmp.rSize[2] << 7) + tmp.rSize[3];
        if (frameSize == 0) {
            break;
        } else {
            framesCnt++;
        }
        if (frameSize + 7 > valueMaxLen) {
            valueMaxLen = frameSize + 7;
        }
        unsigned char *value = malloc(frameSize);
        for (int i = 0; i < frameSize; i++) {
            value[i] = getc(file);
        }
    };
    fseek(file, 0, SEEK_SET);
    printf("|");
    for (int i = 0; i < valueMaxLen; i++) {
        printf("-");
    }
    printf("|\n");
    fread(mainHeader, 1, 10, file);
    for (int i = 0; i < framesCnt; i++) {
        fread(&tmp, sizeof(FRAMEHEADER), 1, file);
        frameSize = (tmp.rSize[0] << 21) + (tmp.rSize[1] << 14) + (tmp.rSize[2] << 7) + tmp.rSize[3];
        unsigned char *value = malloc(frameSize);
        printf("|%d)%s:", i + 1, tmp.name);
        for (int j = 0; j < frameSize; j++) {
            value[j] = getc(file);
            printf("%c", value[j]);
        }
        for (unsigned int j = frameSize; j < valueMaxLen - 7; j++) {
            printf(" ");
        }
        printf("|\n");
    }

    printf("|");
    for (int i = 0; i < valueMaxLen; i++) {
        printf("-");
    }
    printf("|\n");
}

void changeFrame(FILE *file, char prop_name[4], unsigned char *newValue, char *arg1) {
    unsigned char mainHeader[10], *tmpValue;
    char c;
    int doneFlag = 0;
    unsigned int frameSize, metaDataSize;
    FILE *tmpFile = fopen("tmp.mp3", "wb+");
    FRAMEHEADER tmp;

    fread(mainHeader, 1, 10, file);
    metaDataSize = (mainHeader[6] << 21) + (mainHeader[7] << 14) + (mainHeader[8] << 7) + (mainHeader[9]);
    fseek(tmpFile, 10, SEEK_SET);

    do {
        fread(&tmp, sizeof(FRAMEHEADER), 1, file);
        frameSize = (tmp.rSize[0] << 21) + (tmp.rSize[1] << 14) + (tmp.rSize[2] << 7) + tmp.rSize[3];
        tmpValue = malloc(frameSize);
        if (strcmp(prop_name, tmp.name) == 0) {
            fseek(file, frameSize, SEEK_CUR);
            int s = strlen(newValue);
            if (frameSize > s) {
                metaDataSize = metaDataSize - (frameSize - s);
                mainHeader[9] = metaDataSize % 128;
                metaDataSize /= 128;
                mainHeader[8] = metaDataSize % 128;
                metaDataSize /= 128;
                mainHeader[7] = metaDataSize % 128;
                metaDataSize /= 128;
                mainHeader[6] = metaDataSize % 128;
            } else if (s > frameSize) {
                metaDataSize = metaDataSize + (s - frameSize);
                mainHeader[9] = metaDataSize % 128;
                metaDataSize /= 128;
                mainHeader[8] = metaDataSize % 128;
                metaDataSize /= 128;
                mainHeader[7] = metaDataSize % 128;
                metaDataSize /= 128;
                mainHeader[6] = metaDataSize % 128;
            }

            tmp.rSize[3] = s % 128;
            s /= 128;
            tmp.rSize[2] = s % 128;
            s /= 128;
            tmp.rSize[1] = s % 128;
            s /= 128;
            tmp.rSize[0] = s % 128;
            fwrite(&tmp, sizeof(FRAMEHEADER), 1, tmpFile);
            fwrite(newValue, 1, strlen(newValue), tmpFile);
            doneFlag = 1;
        } else {
            fread(tmpValue, 1, frameSize, file);
            fwrite(&tmp, sizeof(FRAMEHEADER), 1, tmpFile);
            fwrite(tmpValue, 1, frameSize, tmpFile);
        }
    } while (doneFlag == 0);
//    if (doneFlag == 0) {
//        printf("Invalid input.\n");
//        free(tmpValue);
//        fclose(file);
//        remove("tmp.mp3");
//        return;
//    }
    c = getc(file);
    while (!feof(file)) {
        fputc(c, tmpFile);
        c = getc(file);

    }
    fseek(tmpFile, 0, SEEK_SET);
    fwrite(mainHeader, 1, 10, tmpFile);
    free(tmpValue);
    fseek(tmpFile, 0, SEEK_SET);
    file = freopen(arg1, "wb", file);

    c = getc(tmpFile);
    while (!feof(tmpFile)) {
        fputc(c, file);
        c = getc(tmpFile);

    }
    fclose(tmpFile);
    fclose(file);

}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 4) {
        printf("Wrong amount of arguments.\n");
        return 1;
    }
    char arg1[100];
    sscanf(argv[1], "%11s", arg1);
    if (strcmp(arg1, "--filepath=") != 0) {
        printf("Invalid first argument.\n");
        return 1;
    }
    FILE *oldFile = fopen(argv[1] + 11, "rb");
    if (oldFile == NULL) {
        printf("Unable to open the mp3 file.\n");
        return 1;
    }
    char arg2[100];
    if (argc == 3) {
        sscanf(argv[2], "%6s", arg2);
        if (strcmp(arg2, "--show") == 0) {

            showFrames(oldFile);

        } else if (strcmp(arg2, "--get=") == 0) {

            getFrame(oldFile, argv[2] + 6);

        } else {
            printf("Invalid second argument1.\n");
            return 1;
        }
    } else if (argc == 4) {
        char arg3[100];
        sscanf(argv[2], "%6s", arg2);
        sscanf(argv[3], "%8s", arg3);

        if (strcmp(arg2, "--set=") != 0) {
            printf("Invalid second argument.\n");
            return 1;
        }
        if (strcmp(arg3, "--value=") != 0) {
            printf("Invalid third argument.\n");
            return 1;
        }
        //пишем самую жопную функцию
        changeFrame(oldFile, argv[2] + 6, argv[3] + 8, argv[1] + 11);

    }
    return 0;
}
