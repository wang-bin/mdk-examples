#import <Cocoa/Cocoa.h>

#ifndef APP_BUNDLE
#import "AppDelegate.h"
#endif

int main(int argc, const char * argv[]) {
#ifdef APP_BUNDLE
    return NSApplicationMain(argc, argv);
#else
    NSApplication *app = [NSApplication sharedApplication];
    AppDelegate *delegate = [AppDelegate new];
    app.delegate = delegate;
    [app run];
#endif
}
