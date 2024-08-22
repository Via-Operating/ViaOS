#include <via/stdio.h>

int math(char* option, int num1, int num2) {
    if (option == '-a') {
        int output = num1 + num2;
        printf(output);
        printf("> ");
    }
    else if (option == '-s') {
        int output = num1 - num2;
        printf(output);
        printf("> ");
    }
    else if (option == '-m') {
        int output = num1 * num2;
        printf(output);
        printf("> ");
    }
    else if (option == '-d') {
        int output = num1 / num2;
        printf(output);
        printf("> ");
    }
    else if (option == '--add') {
        int output = num1 + num2;
        printf(output);
        printf("> ");
    }
    else if (option == '--subtract') {
        int output = num1 - num2;
        printf(output);
        printf("> ");
    }
    else if (option == '--multiply') {
        int output = num1 * num2;
        printf(output);
        printf("> ");
    }
    else if (option == '--divide') {
        int output = num1 / num2;
        printf(output);
        printf("> ");
    }
    else {
        printf("Invalid Operation!");
    }
}
