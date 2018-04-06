#include <stdio.h>

int main(int argc, char **argv){
    printf("hello world\n");
    char *temp = argv[1];
    printf("%s", temp);
    return 0;
}