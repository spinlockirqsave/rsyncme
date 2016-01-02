///  @file      rm_error.c
///  @brief     Errors print/log.
///  @author    peterg
///  @version   0.1.1
///  @date      02 Jan 2016 02:30 PM
///  @copyright LGPL


#include "rm_error.h"


void rm_err(const char *s)
{
        assert(s != NULL);
        fprintf(stderr, "%s\n", s);
        return;
}

void rm_perr(const char *s)
{
        assert(s != NULL);
        fprintf(stderr, "%s %s\n",
                        s, strerror(errno));
        return;
}

void rm_perr_abort(const char *s)
{
        assert(s != NULL);
        fprintf(stderr, "%s %s\n",
                s, strerror(errno));
        exit(1);
}
