// Tweak.xm — Dylib tu tim zip1.zip .. zip9.zip trong bundle (Payload)
//            va giai nen tung file ra thu muc Documents cua app.
#import <Foundation/Foundation.h>
#include <notify.h>
#include "zipcore.h"

#define ZIP_FIRST 1
#define ZIP_LAST  9

// Tim file co ten dung 'name' trong bundlePath (quet ca 1 cap thu muc con).
// Tra ve duong dan tuyet doi hoac nil.
static NSString *find_zip_in_bundle(NSFileManager *fm, NSString *bundlePath, NSString *name) {
    // cap goc
    NSString *cand = [bundlePath stringByAppendingPathComponent:name];
    if ([fm fileExistsAtPath:cand]) return cand;

    // 1 cap thu muc con
    NSArray *top = [fm contentsOfDirectoryAtPath:bundlePath error:nil];
    for (NSString *entry in top) {
        NSString *sub = [bundlePath stringByAppendingPathComponent:entry];
        BOOL isDir = NO;
        if ([fm fileExistsAtPath:sub isDirectory:&isDir] && isDir) {
            NSString *c2 = [sub stringByAppendingPathComponent:name];
            if ([fm fileExistsAtPath:c2]) return c2;
        }
    }
    return nil;
}

static void run_extract(void) {
    @autoreleasepool {
        NSFileManager *fm = [NSFileManager defaultManager];
        NSString *bundlePath = [[NSBundle mainBundle] bundlePath];

        // Thu muc Documents cua app
        NSString *docs = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,
                            NSUserDomainMask, YES) firstObject];
        if (!docs) {
            docs = [NSHomeDirectory() stringByAppendingPathComponent:@"Documents"];
        }
        [fm createDirectoryAtPath:docs withIntermediateDirectories:YES attributes:nil error:nil];

        int found = 0, ok = 0;

        // Lan luot tim zip1.zip .. zip9.zip va giai nen ra Documents
        for (int i = ZIP_FIRST; i <= ZIP_LAST; i++) {
            NSString *name = [NSString stringWithFormat:@"zip%d.zip", i];
            NSString *zipPath = find_zip_in_bundle(fm, bundlePath, name);
            if (!zipPath) {
                NSLog(@"[UnzipDylib] Khong thay %@ trong bundle", name);
                continue;
            }

            found++;
            NSLog(@"[UnzipDylib] Giai '%@' -> '%@'", zipPath, docs);
            int rc = zip_extract(zipPath.UTF8String, docs.UTF8String);
            NSLog(@"[UnzipDylib] %@ ket qua: rc=%d", name, rc);
            if (rc == 0) ok++;
        }

        NSLog(@"[UnzipDylib] Hoan tat: tim thay %d/%d file, giai nen thanh cong %d",
              found, ZIP_LAST - ZIP_FIRST + 1, ok);

        notify_post("com.user.unzipdylib.done");
    }
}

// Chay khi dylib duoc nap (rat som) -> hoan sang main queue cho app on dinh.
__attribute__((constructor))
static void unzipdylib_init(void) {
    dispatch_async(dispatch_get_main_queue(), ^{
        run_extract();
    });
}
