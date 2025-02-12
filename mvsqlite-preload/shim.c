#define _GNU_SOURCE

#include <stdlib.h>
#include <dlfcn.h>
#include <stdio.h>
#include <pthread.h>
#include <sqlite3.h>
#include <assert.h>

extern void init_mvsqlite(void);
extern void init_mvsqlite_connection(sqlite3 *db);

typedef int (*sqlite3_initialize_fn)(void);
typedef int (*sqlite3_open_v2_fn)(
    const char *filename,   /* Database filename (UTF-8) */
    sqlite3 **ppDb,            /* OUT: SQLite db handle */
    int flags,              /* Flags */
    const char *zVfs        /* Name of VFS module to use */
);

static sqlite3_open_v2_fn real_sqlite3_open_v2 = NULL;
static int mvsqlite_enabled = 0;

void mvsqlite_global_init(void) {
    mvsqlite_enabled = 1;
}

static void bootstrap(void) {
    real_sqlite3_open_v2 = dlsym(RTLD_NEXT, "sqlite3_open_v2");
    if (real_sqlite3_open_v2 == NULL) {
        fprintf(stderr, "Failed to find real sqlite3_open_v2\n");
        exit(1);
    }

    if(mvsqlite_enabled) {
        init_mvsqlite();
    }
}

static pthread_once_t vfs_init = PTHREAD_ONCE_INIT;

int sqlite3_open_v2(
    const char *filename,   /* Database filename (UTF-8) */
    sqlite3 **ppDb,            /* OUT: SQLite db handle */
    int flags,              /* Flags */
    const char *zVfs        /* Name of VFS module to use */
) {
    int ret;

    pthread_once(&vfs_init, bootstrap);
    ret = real_sqlite3_open_v2(filename, ppDb, flags, zVfs);
    if(ret == SQLITE_OK && mvsqlite_enabled) {
        init_mvsqlite_connection(*ppDb);
    }
    return ret;
}

int sqlite3_open(
    const char *filename,   /* Database filename (UTF-8) */
    sqlite3 **ppDb          /* OUT: SQLite db handle */
) {
    return sqlite3_open_v2(filename, ppDb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
}

int sqlite3_open16(
    const void *filename,   /* Database filename (UTF-16) */
    sqlite3 **ppDb          /* OUT: SQLite db handle */
) {
    abort();
}
