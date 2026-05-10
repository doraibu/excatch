#ifndef EXCATCH_H
#define EXCATCH_H

#include "raii.h"
#include "exceptions.h"

#define guard(name, alloc_expr) RAII_VAR name = alloc_expr

#define throw(msg, code) exc_throw(msg, code)

#define try TRY
#define catch(err_var) \
    CATCH { \
        int err_var = _err;

#define end_try } }

#endif
