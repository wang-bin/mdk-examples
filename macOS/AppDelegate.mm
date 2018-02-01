//  Created by WangBin on 2017/3/12.
//  Copyright © 2017 wangbin. All rights reserved.

#import "AppDelegate.h"
#include "mdk/Player.h"
#include <cstdlib>

using namespace MDK_NS;

@interface GLLayer : CAOpenGLLayer {
    std::function<void()> draw_cb;
}
@end

@implementation GLLayer
- (void)setDrawCallback:(std::function<void()>)cb
{
    draw_cb = cb;
}

- (id)init
{
    if (self = [super init]) {
        [self setAsynchronous:YES]; // MUST YES?
        [self setNeedsDisplayOnBoundsChange:YES];
        [self setAutoresizingMask:kCALayerWidthSizable|kCALayerHeightSizable];
        [self setBackgroundColor:[NSColor blackColor].CGColor];
    }
    return self;
}

- (void)drawInCGLContext:(CGLContextObj)ctx pixelFormat:(CGLPixelFormatObj)pf
        forLayerTime:(CFTimeInterval)t displayTime:(const CVTimeStamp *)ts
{
    if (!draw_cb) 
        return;
    draw_cb();
    CGLFlushDrawable(ctx);
}

- (CGLPixelFormatObj)copyCGLPixelFormatForDisplayMask:(uint32_t)mask
{
    CGLPixelFormatAttribute attrs[] = {
        kCGLPFAOpenGLProfile, (CGLPixelFormatAttribute) kCGLOGLPVersion_3_2_Core,
        kCGLPFADoubleBuffer,
        kCGLPFAAllowOfflineRenderers,
        kCGLPFABackingStore,
        kCGLPFAAccelerated,
        kCGLPFASupportsAutomaticGraphicsSwitching,
        CGLPixelFormatAttribute(0)
    };
    GLint npix;
    CGLPixelFormatObj pix;
    CGLChoosePixelFormat(attrs, &pix, &npix);
    return pix;
}

- (CGLContextObj)copyCGLContextForPixelFormat:(CGLPixelFormatObj)pf
{
    CGLContextObj ctx = [super copyCGLContextForPixelFormat:pf];
    GLint i = 1;
    CGLSetParameter(ctx, kCGLCPSwapInterval, &i);
    CGLEnable(ctx, kCGLCEMPEngine);
    CGLSetCurrentContext(ctx);
    return ctx;
}
@end

@interface CocoaWindow : NSWindow
@end

@implementation CocoaWindow
- (BOOL)canBecomeMainWindow { return YES; }
- (BOOL)canBecomeKeyWindow { return YES; }
@end

@interface AppDelegate ()
#ifdef APP_BUNDLE
@property /*(weak)*/ IBOutlet NSWindow *window; // osx10.6:  cannot synthesize weak property because the current deployment target does not support weak references
#endif
@end

@implementation AppDelegate
{
    Player *player; // why dtor is not called if not a ptr?
    NSWindow *w;
}

#ifndef APP_BUNDLE
- (void)createWindow {
    const int mask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable| NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
    self->w = [[CocoaWindow alloc] initWithContentRect:NSMakeRect(0,0, 960, 540) styleMask:mask backing:NSBackingStoreBuffered defer:NO];
    [self->w setTitle:@"Cocoa Window. Set environment var 'GL_LAYER=1' to use CAOpenGLLayer"];
    [self->w makeMainWindow];
    [self->w makeKeyAndOrderFront:nil];

    [NSApp activateIgnoringOtherApps:YES];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(resizeSurface) name:NSWindowDidResizeNotification object:nil];

    NSMenu *m = [[NSMenu alloc] initWithTitle:@"AMainMenu"];
    NSMenuItem *item = [m addItemWithTitle:@"Apple" action:nil keyEquivalent:@""];
    NSMenu *sm = [[NSMenu alloc] initWithTitle:@"Apple"];
    [m setSubmenu:sm forItem:item];
    [sm addItemWithTitle: @"quit" action:@selector(terminate:) keyEquivalent:@"q"];
    [NSApp setMenu:m];
#if !__has_feature(objc_arc)
    [m autorelease];
#endif
}
#endif

// called before openFile
- (void)applicationWillFinishLaunching:(NSNotification *)notification {
    player = new Player();
    player->setVideoDecoders({"VideoToolbox", "FFmpeg"});
}

- (BOOL)application:(NSApplication *)sender openFile:(NSString *)filename {
    player->setMedia([filename UTF8String]);
    // stop previous playback
    player->setState(State::Stopped);
    // setState() is async, wait until it's really stopped
    player->waitFor(State::Stopped);
    player->setState(State::Playing);
    return YES;
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    NSArray *args = [[NSProcessInfo processInfo] arguments];
    const int argc = [args count];
    for (int i = 0; i < argc; ++i) {
        std::string v([[args objectAtIndex:i] UTF8String]);
        if (v == "-c:v")
            player->setVideoDecoders({[[args objectAtIndex:(i+1)] UTF8String]});
    }
#ifdef APP_BUNDLE
    self->w = self.window;
#else
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular]; // it's default for bundled app
    atexit_b(^{
        [NSApp setActivationPolicy:NSApplicationActivationPolicyProhibited];
    });
    [self createWindow];
#endif
    // close instead of hide(default)
    NSButton *closeButton = [self->w standardWindowButton:NSWindowCloseButton];
    [closeButton setTarget:self];
    [closeButton setAction:@selector(quit)];

    GLLayer *layer = nullptr;
    const char* e = std::getenv("GL_LAYER");
    if (e && std::atoi(e) == 1) {
        layer = [[GLLayer alloc] init];
        [layer setDrawCallback:[self]{ player->renderVideo(); }];
    }
    NSView *view = self->w.contentView;
    if (layer != nil) {
        [view setLayer:layer];
    } else {
        player->updateNativeWindow((__bridge void *)view);
    }
    player->setVideoSurfaceSize(view.bounds.size.width, view.bounds.size.height);
    if (argc > 1) {
        player->setMedia([[args lastObject] UTF8String]);
    }
    player->setState(State::Playing);
}

- (void) quit { 
    [[NSApplication sharedApplication] terminate:nil]; 
}

- (void)resizeSurface {
#ifdef APP_BUNDLE
    NSSize s = [[self window] frame].size;
#else
    NSSize s = self->w.frame.size;
#endif
    player->setVideoSurfaceSize(s.width, s.height);
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
    player->setState(State::Stopped);
    delete player;
    player = nullptr;
}
@end
