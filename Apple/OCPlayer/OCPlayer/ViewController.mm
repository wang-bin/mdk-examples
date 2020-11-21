//
//  ViewController.m
//  OCPlayer
//
//  Created by WangBin on 2020/11/5.
//

#import "ViewController.h"
@import MetalKit;
#include <mdk/Player.h>
using namespace MDK_NS;

@interface ViewController ()<MTKViewDelegate>
{
    MTKView* _mkv;
    Player _player;
    id<MTLCommandQueue> _cmdQueue;
}
@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    auto dev = MTLCreateSystemDefaultDevice();
    _cmdQueue = [dev newCommandQueue];

    _mkv = [[MTKView alloc] init];
    _mkv.device = dev;

// setup player first to ensure renderer is ready in MTKViewDelegate:mtkView:drawableSizeWillChange
    MetalRenderAPI ra;
    ra.device = (__bridge void*)dev;
    ra.cmdQueue = (__bridge void*)_cmdQueue;
    ra.opaque = (__bridge void*)_mkv;
    ra.currentRenderTarget = [](const void* opaque){
        auto view = (__bridge MTKView*)opaque;
        return (__bridge const void*)view.currentDrawable.texture;
    };
    _player.setRenderAPI(&ra);
    _player.setVideoDecoders({"VT", "FFmpeg"});
    _player.setLoop(-1);

#ifdef DECODER_DRIVEN
    _mkv.enableSetNeedsDisplay = YES;
    __weak typeof(_mkv) view = _mkv;
    _player.setRenderCallback([view](void*) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [view setNeedsDisplay:YES];
        });
    });
#endif

    //_mkv = (MTKView*)self.view;
    _mkv.delegate = self;
    _mkv.clearColor = MTLClearColorMake(0.0, 0.5, 1.0, 1.0);
    [self.view addSubview:_mkv];
    _mkv.translatesAutoresizingMaskIntoConstraints = NO;
    [_mkv.leftAnchor constraintEqualToAnchor:self.view.leftAnchor].active = YES;
    [_mkv.rightAnchor constraintEqualToAnchor:self.view.rightAnchor].active = YES;
    [_mkv.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor].active = YES;
    [_mkv.topAnchor constraintEqualToAnchor:self.view.topAnchor].active = YES;
    // Do any additional setup after loading the view.
}

-(void)viewDidAppear
{
    _player.setMedia("http://huan.mediacdn.cedock.net/ts/ceshi20151014/ceshi1.ts");
    _player.setState(State::Playing);
}

-(void)viewWillDisappear
{
    _player.setState(State::Stopped);
}

- (void)setRepresentedObject:(id)representedObject {
    [super setRepresentedObject:representedObject];

    // Update the view, if already loaded.
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size
{
    _player.setVideoSurfaceSize(size.width, size.height);
}


- (void)drawInMTKView:(nonnull MTKView *)view
{
    _player.renderVideo();
    id<MTLCommandBuffer> cmdBuf = [_cmdQueue commandBuffer];
    [cmdBuf presentDrawable:_mkv.currentDrawable];
    [cmdBuf commit];
}


@end
