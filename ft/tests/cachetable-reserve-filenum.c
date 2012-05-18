/* -*- mode: C; c-basic-offset: 4 -*- */
#ident "$Id$"
#ident "Copyright (c) 2007-2011 Tokutek Inc.  All rights reserved."
#include "includes.h"
#include "test.h"


// Mostly copied from cachetable-fd-test.c
//
// Added logic to exercise filenum reservation
//
// NOTE: Attempting to release a reserved filenum will cause a crash.

static void
cachetable_reserve_filenum_test (void) {
    const int test_limit = 1;
    int r;
    CACHETABLE ct;
    r = toku_create_cachetable(&ct, test_limit, ZERO_LSN, NULL_LOGGER); assert(r == 0);
    char fname1[] = __SRCFILE__ "test1.dat";
    unlink(fname1);
    CACHEFILE cf;
    r = toku_cachetable_openf(&cf, ct, fname1, O_RDWR|O_CREAT, S_IRWXU|S_IRWXG|S_IRWXO); assert(r == 0);

    int fd1 = toku_cachefile_get_and_pin_fd(cf); assert(fd1 >= 0);
    toku_cachefile_unpin_fd(cf);

    // test set to good fd succeeds
    char fname2[] = __SRCFILE__ "test2.data";
    unlink(fname2);
    int fd2 = open(fname2, O_RDWR | O_CREAT, S_IRWXU|S_IRWXG|S_IRWXO); assert(fd2 >= 0 && fd1 != fd2);
    r = toku_cachefile_set_fd(cf, fd2, fname2); assert(r == 0);
    assert(toku_cachefile_get_and_pin_fd(cf) == fd2);
    toku_cachefile_unpin_fd(cf);

    // test set to bogus fd fails
    int fd3 = open(DEV_NULL_FILE, O_RDWR); assert(fd3 >= 0);
    r = close(fd3); assert(r == 0);
    r = toku_cachefile_set_fd(cf, fd3, DEV_NULL_FILE); assert(r != 0);
    assert(toku_cachefile_get_and_pin_fd(cf) == fd2);
    toku_cachefile_unpin_fd(cf);

    // test the filenum functions
    FILENUM fn = toku_cachefile_filenum(cf);
    CACHEFILE newcf = 0;
    r = toku_cachefile_of_filenum(ct, fn, &newcf);
    assert(r == 0 && cf == newcf);

    // test a bogus filenum
    fn.fileid++;
    r = toku_cachefile_of_filenum(ct, fn, &newcf);
    assert(r == ENOENT);

    // now exercise filenum reservation
    FILENUM fn2 = {0};
    r = toku_cachetable_reserve_filenum(ct, &fn, FALSE, fn2);
    CKERR(r);
    toku_cachetable_unreserve_filenum (ct, fn);

    r = toku_cachetable_reserve_filenum(ct, &fn2, TRUE, fn);
    CKERR(r);
    assert(fn2.fileid == fn.fileid);
    r = toku_cachetable_reserve_filenum(ct, &fn2, TRUE, fn);
    CKERR2(r,EEXIST);
    toku_cachetable_unreserve_filenum (ct, fn);

    r = toku_cachetable_reserve_filenum(ct, &fn2, TRUE, fn);
    CKERR(r);
    assert(fn2.fileid == fn.fileid);
    toku_cachetable_unreserve_filenum (ct, fn);

    r = toku_cachefile_close(&cf, 0, FALSE, ZERO_LSN); assert(r == 0);
    r = toku_cachetable_close(&ct); assert(r == 0 && ct == 0);
}

int
test_main(int argc, const char *argv[]) {
    default_parse_args(argc, argv);
    toku_os_initialize_settings(verbose);

    cachetable_reserve_filenum_test();
    return 0;
}