// made by Theo Daniel Lapus

#include <via/stdio.h>
#include "../../user/properties.h"

// Implemented by Kap while Refining this command to run properly
void intToStr(int num, char *str, int base) 
{
    // Check for valid base
    if (base < 2 || base > 16) {
        str[0] = '\0'; // Set an empty string for invalid base
        return;
    }

    // Handle zero explicitly, otherwise empty string is printed
    if (num == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }

    char temp[32]; // Temporary buffer to hold the number in reverse
    int i = 0; // Index for temp

    // Process individual digits
    while (num != 0) {
        int remainder = num % base;
        temp[i++] = (remainder > 9) ? (remainder - 10) + 'A' : remainder + '0';
        num = num / base;
    }

    temp[i] = '\0'; // Null-terminate the string

    // Reverse the string in temp and copy it to str
    int j;
    for (j = 0; j < i; j++) {
        str[j] = temp[i - j - 1];
    }
    str[j] = '\0';
}

// lmao i love this
// - Kap Petrov
void info() 
{
    // Fucking retarded approach to inttostr();
    char idkwhyweneedthis[32];

    printf("\nHello, ");
    printf(nameOfUser);
    printf("\n");

    if (ageOfUser <= 3 && !(ageOfUser >= 3)) 
    {
        intToStr(ageOfUser, idkwhyweneedthis, 10);

        printf("You are googoo gaga ");
        printf(idkwhyweneedthis);
        printf(" years old\n");
    }
    else if (ageOfUser >= 3 && !(ageOfUser >= 12))
    {
        printf("you are kid ");
        printf(idkwhyweneedthis);
        printf(" years old\n");
    }
    else if (ageOfUser >= 12 && !(ageOfUser >= 18))
    {
        printf("you are teen ");
        printf(idkwhyweneedthis);
        printf(" years old\n");
    }
    else if (ageOfUser >= 18 && !(ageOfUser >= 65))
    {
        printf("you are 'I pay taxes' ");
        printf(idkwhyweneedthis);
        printf(" years old\n");
    }
    else if (ageOfUser >= 65 && !(ageOfUser >= 90))
    {
        printf("you are oldie ");
        printf(idkwhyweneedthis);
        printf(" years old\n");
    }
    else if (ageOfUser >= 90 && !(ageOfUser >= 116))
    {
        printf("dude.... ");
        printf(idkwhyweneedthis);
        printf(" years old... how...?\n");
    }
    else if (ageOfUser >= 116 && !(ageOfUser >= 9999999999))
    {
        printf("impossible.... ");
        printf(idkwhyweneedthis);
        printf("\n");
    }
    else if (ageOfUser >= 9999999999)
    {
        printf("Yahaha! You found me!\n");
    }
}