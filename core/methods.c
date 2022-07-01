#include "methods.h"

#include <Python.h>
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

typedef struct _user
{
    u_int64_t id;
    char name[6];
    char surname[10];
    char email[30];
    char password[8];
} user;

static PyObject *create_base(PyObject *self, PyObject *args)
{
  if (mkdir("./_pyb/bazica", 0777) == -1)
  {
    printf("Error: %s\n", strerror(errno));
    PyErr_SetString(PyExc_TypeError, "Error occured while creating database!");
    return NULL;
  }

  FILE *user_schema_stream = fopen("./_pyb/bazica/user_schema.bin", "wb");
  FILE *schema_definition_stream = fopen("./_pyb/bazica/schema_definition.bin", "wb");
  user *usr = calloc(1, sizeof(*usr));

  for (int i = 0; i < 1000; i++) {
    usr->id = i+1;
    strcpy(usr->name, "Janez");
    strcpy(usr->surname, "Sedeljsak");
    strcpy(usr->email, "janez.sedeljsak@gmail.com");
    strcpy(usr->password, "test123");

    fwrite(usr, sizeof(*usr), 1, user_schema_stream);
  }

  fclose(user_schema_stream);
  fclose(schema_definition_stream);
  Py_RETURN_TRUE;
}
