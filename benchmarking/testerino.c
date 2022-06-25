#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct _user
{
    u_int64_t id;
    char name[6];
    char surname[10];
    char email[30];
    char password[8];
} user;

void print_user_by_id(FILE *stream, int id)
{
    user *usr = calloc(1, sizeof(*usr));
    fseek(stream, sizeof(*usr) * (id-1), SEEK_SET);
    fread(usr, sizeof(*usr), 1, stream);
    printf("id=%ld\nfullname=%s %s\nemail=%s\npassword=%s\n", usr->id, usr->name, usr->surname, usr->email, usr->password);
}

int main(void)
{
    FILE *binfile = fopen("../_pyb/bazica/user_schema.bin", "rb");
    print_user_by_id(binfile, 543);

    return 0;
}