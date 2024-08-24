#include <via/stdio.h>

int math(char* option, float num1, float num2) {
    if (option == "%c", 'a') {
        float output = num1 + num2;
        printf(output);
        printf("> ");
    }
    else if (option == "%c", '-s') {
        float output = num1 - num2;
        printf(output);
        printf("> ");
    }
    else if (option == "%c", '-m') {
        float output = num1 * num2;
        printf(output);
        printf("> ");
    }
    else if (option == "%c", '-d') {
        float output = num1 / num2;
        printf(output);
        printf("> ");
    }
    else if (option == "%s", '--add') {
        float output = num1 + num2;
        printf(output);
        printf("> ");
    }
    else if (option == "%s", '--subtract') {
        float output = num1 - num2;
        printf(output);
        printf("> ");
    }
    else if (option == "%s", '--multiply') {
        float output = num1 * num2;
        printf(output);
        printf("> ");
    }
    else if (option == "%s", '--divide') {
        float output = num1 / num2;
        printf(output);
        printf("> ");
    }
    else {
        printf("Invalid Operation!");
    }
}
