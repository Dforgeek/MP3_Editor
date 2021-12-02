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

void changeFrame(FILE *file, char prop_name[4], unsigned char *newValue) {
    int doneFlag = 0;
    FILE *tmpFile = fopen("tmp.mp3", "wb");
    FRAMEHEADER tmp;
    unsigned char mainHeader[10], *tmpValue;
    char c;
    fread(mainHeader, 1, 10, file);
    for (int i=0; i<10; i++){
        fputc(mainHeader[i], tmpFile);
    }//исправить, сайз должен обновляться
    unsigned int frameSize;
    do {
        fread(&tmp, sizeof(FRAMEHEADER), 1, file);
        frameSize = (tmp.rSize[0] << 21) + (tmp.rSize[1] << 14) + (tmp.rSize[2] << 7) + tmp.rSize[3];
        if (frameSize == 0) {
            printf("Invalid input.\n");
            fclose(file);
            remove("tmp.mp3");
            return;
        }
        tmpValue = malloc(frameSize);
        if (strcmp(prop_name, tmp.name) == 0) {
            fseek(file, 0, SEEK_CUR + frameSize);
            int s = strlen(newValue);
            tmp.rSize[3] = s % 128;
            s /= 128;
            tmp.rSize[2] = s % 128;
            s /= 128;
            tmp.rSize[1] = s % 128;
            s /= 128;
            tmp.rSize[0] = s % 128;
            fprintf(tmpFile, "%s", tmp.name);
            fprintf(tmpFile, "%s", tmp.rSize);
            fprintf(tmpFile, "%s", tmp.flags);
            fprintf(tmpFile, "%s", newValue);
            doneFlag = 1;
        } else {
            fread(tmpValue, 1, frameSize, file);
            fprintf(tmpFile, "%s", tmp.name);
            fprintf(tmpFile, "%s", tmp.rSize);
            fprintf(tmpFile, "%s", tmp.flags);
            fprintf(tmpFile, "%s", tmpValue);
        }
    } while (doneFlag == 0);

    do {
        c = getc(file);
        fputc(c, tmpFile);
    } while (c != EOF);//не записывает, почему?


    fseek(file, 0, SEEK_SET);
    fseek(tmpFile, 0, SEEK_SET);
    do {
        c = getc(tmpFile);
        fputc(c, file);
    } while (c != EOF);
    free(tmpValue);
    fclose(file);
    remove("tmp.mp3");
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

        if (strcmp(arg2, "--set=")!=0) {
            printf("Invalid second argument.\n");
            return 1;
        }
        if (strcmp(arg3, "--value=")!=0) {
            printf("Invalid third argument.\n");
            return 1;
        }
        //пишем самую жопную функцию
        changeFrame(oldFile, argv[2] + 6, argv[3] + 8);

    }
    return 0;
}
