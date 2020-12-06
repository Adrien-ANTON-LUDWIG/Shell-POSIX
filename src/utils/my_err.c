#include "my_err.h"

#include <err.h>

void my_err(int err_code, struct major *mj, char *message)
{
    major_free(mj);
    errx(err_code, "%s", message);
}
