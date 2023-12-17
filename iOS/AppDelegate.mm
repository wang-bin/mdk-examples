// http://stackoverflow.com/questions/29160540/how-to-add-view-in-uiwindow
// rotate: http://blog.csdn.net/daiyelang/article/details/16984913

#import "AppDelegate.h"
#import <AssetsLibrary/AssetsLibrary.h>
#import <AVFoundation/AVFoundation.h>
#import <MediaToolbox/MediaToolbox.h>
#include <chrono>
#include <sstream>
#include <queue>
#include <vector>
#include <mdk/Player.h>
#include <mdk/RenderAPI.h>
using namespace std;
using namespace MDK_NS;

// http://www.cnblogs.com/smileEvday/archive/2012/11/16/UIWindow.html
@implementation AppDelegate
{
    Player *pmp;
    //FrameReader::Ptr fr;
    UIView *view;
    UIButton *decButton;
    UIButton *playButton;
    UIButton *stopButton;
    UIButton *logButton;
    UILabel *logView;
    NSString *log;
    UILabel *fpsView;
    UISwitch *hwdecButton;
    chrono::system_clock::time_point t0;
    std::queue<int64_t> t;
    int count;
    int url_now;
    std::vector<std::string> urls;
}

@synthesize window, vc;
#if 1

- (void)buttonPressed:(UIButton *)button
{
    if (url_now < 0) {
        url_now = -1;
        pmp->setMedia(urls[++url_now%urls.size()].data());
    }
    if (button == playButton) {
        pmp->updateNativeSurface((__bridge void* )view);
        if (pmp->state() == State::Playing)
            pmp->set(State::Paused);
        else
            pmp->set(State::Playing);
    } else if (button == stopButton) {
        pmp->updateNativeSurface(nullptr);
        pmp->set(State::Stopped);
        //fr->stop();
        pmp->setMedia(urls[++url_now%urls.size()].data());
    } else if (button == decButton) {
        //fr->stop();
        t0 = chrono::system_clock::now();
        count = 0;
        t = queue<int64_t>();
        //fr->start();
    }
    pmp->setLoop(-1);
}

- (void)hwdecSwitched:(UIControl *)control
{
    if (control == hwdecButton) {
        if (hwdecButton.on == YES) {
            pmp->setDecoders(MediaType::Video, {"VT", "BRAW:gpu=auto:scale=1080", "FFmpeg"});
            //fr->setDecoders(MediaType::Video, {"VT", "FFmpeg"});
        } else {
            pmp->setDecoders(MediaType::Video, {"BRAW:gpu=no:scale=1080", "FFmpeg"});
            //fr->setDecoders(MediaType::Video, {"FFmpeg"});
        }
    }
}
- (NSUInteger)application:(UIApplication *)application supportedInterfaceOrientationsForWindow:(UIWindow *)window

{
    return (UIInterfaceOrientationMaskLandscape);

}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation
{
    return YES;
}

#endif
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    //setenv("GL_IOSURFACE", "1", 1);
    [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback error:nil];
	window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
	vc = [[UIViewController alloc] init];

    //[window addSubview:vc.view]; // crash on ios10: Applications are expected to have a root view controller at the end of application launch
    [window setRootViewController:vc];
    // Override point for customization after application launch.
    [self.window makeKeyAndVisible];

    [vc setView:[[[UIView alloc] initWithFrame:[[UIScreen mainScreen] applicationFrame]] autorelease]];
    UIView *videoView = [[[UIView alloc] initWithFrame:[[UIScreen mainScreen] bounds]] autorelease];
    //NSLog(@"mainScreen scale: %f, contentScaleFactor: %f", [[UIScreen mainScreen] scale], [videoView contentScaleFactor]);

    [[vc view] addSubview:videoView];
    [[vc view] setBackgroundColor:[UIColor blackColor]];
    [[vc view] setUserInteractionEnabled:YES];

    logView = [[[UILabel alloc] init] autorelease];
    logView.frame = CGRectMake(0, 0, [UIScreen mainScreen].bounds.size.width, [UIScreen mainScreen].bounds.size.height*2/3);
    logView.textColor = [UIColor whiteColor];
    logView.backgroundColor = [[UIColor grayColor] colorWithAlphaComponent:0.5];
    logView.textAlignment = NSTextAlignmentLeft;
    logView.font = [UIFont systemFontOfSize:10];
    logView.lineBreakMode = NSLineBreakByWordWrapping;
    logView.numberOfLines = 0;
    [[vc view] addSubview:logView];

    fpsView = [[[UILabel alloc] init] autorelease];
    const int kLogW = 88;
    const int kLogH = 26;
    fpsView.frame = CGRectMake([UIScreen mainScreen].bounds.size.width - kLogW, 0, kLogW, kLogH);
    fpsView.textColor = [UIColor redColor];
    fpsView.backgroundColor = [[UIColor greenColor] colorWithAlphaComponent:0.5];
    fpsView.font = [UIFont systemFontOfSize:18];
    fpsView.numberOfLines = 0;
    [[vc view] addSubview:fpsView];

    const int btnH = 28, btnW = 100;
    const int btnY = [UIScreen mainScreen].bounds.size.height - btnH - 168;
    int btnX = 20;
    playButton = [[UIButton alloc] initWithFrame:CGRectMake(btnX, btnY, btnW, btnH)];
    btnX += btnW+10;
    [playButton setBackgroundColor:[UIColor blueColor]];
    [playButton setTitle:@"Play/Pause" forState:UIControlStateNormal];
    [playButton addTarget:self action:@selector(buttonPressed:) forControlEvents: UIControlEventTouchUpInside];
    [[vc view] addSubview:playButton];
    stopButton = [[UIButton alloc] initWithFrame:CGRectMake(btnX, btnY, btnW, btnH)];
    btnX += btnW+10;
    [stopButton setBackgroundColor:[UIColor blueColor]];
    [stopButton setTitle:@"Stop" forState:UIControlStateNormal];
    [stopButton addTarget:self action:@selector(buttonPressed:) forControlEvents: UIControlEventTouchUpInside];
    [[vc view] addSubview:stopButton];
#if 0
    logButton = [[UIButton alloc] initWithFrame:CGRectMake(btnX, btnY, btnW, btnH)];
    btnX += btnW+10;
    [logButton setBackgroundColor:[UIColor blueColor]];
    [logButton setTitle:@"Log" forState:UIControlStateNormal];
    [logButton addTarget:self action:@selector(buttonPressed:) forControlEvents: UIControlEventTouchUpInside];
    [[vc view] addSubview:logButton];
#endif
    decButton = [[UIButton alloc] initWithFrame:CGRectMake(btnX, btnY, btnW, btnH)];
    btnX += btnW+10;
    [decButton setBackgroundColor:[UIColor blueColor]];
    [decButton setTitle:@"Decode" forState:UIControlStateNormal];
    [decButton addTarget:self action:@selector(buttonPressed:) forControlEvents: UIControlEventTouchUpInside];
    [[vc view] addSubview:decButton];

    hwdecButton = [[UISwitch alloc] initWithFrame:CGRectMake(btnX, btnY, btnW, btnH)];
    btnX += btnW+10;
    hwdecButton.on = NO;
    [hwdecButton addTarget:self action:@selector(hwdecSwitched:) forControlEvents:UIControlEventValueChanged];
    [[vc view] addSubview:hwdecButton];

    static std::once_flag init_flag;
#if 0
    std::call_once(init_flag, [=]{
        //NSString * dir = [NSTemporaryDirectory() stringByAppendingString:@"/player.log"];
        NSString * dir = [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject] stringByAppendingString:@"/player.log"];
        freopen([dir UTF8String], "w", stderr);
        freopen([dir UTF8String], "w", stdout);
        NSLog(@"log dir: %@", dir);
    });
#endif
    log = @"MDK log:\n";
#if 0
    setLogHandler([=](LogLevel level, const char* msg){
        @autoreleasepool {
        NSString *s = [NSString stringWithUTF8String: msg];
        NSString *trimed = [s stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
        NSLog(@"mdk:%@", trimed);
        char       buf[80] = {};
        time_t     now = time(0);
        strftime(buf, sizeof(buf), "%m-%d %T", localtime(&now));
        static const char level_name[] = {'I', 'W'};
        string ss(std::snprintf(nullptr, 0, "%c %s@%p: %s", level_name[level<LogLevel::Info], buf, (void*)pthread_self(), msg)+1, 0);
        std::snprintf(&ss[0], ss.size(),"%c %s@%p: %s", level_name[level<LogLevel::Info], buf, (void*)pthread_self(), msg);
        std::istringstream stream;
        stream.str(ss);
        for (std::string line; std::getline(stream, line);) {
            log = [log stringByAppendingFormat:@"%s\n", line.data()];
            static int lines = 0;
            if (lines++ > 16) {
                auto r = [log rangeOfString:@"\n"];
                log = [log substringFromIndex:r.location + r.length];
            }
        }
            NSString* main_log = [log copy];
            dispatch_async(dispatch_get_main_queue(), ^{
                logView.text = main_log; // if not in main thread, ui may not update
                //[main_log release];
            });
        }
    });
#endif
    NSLog(@"size: %fx%f", videoView.frame.size.width, videoView.frame.size.height),
    view = videoView;
    url_now = -1;
    pmp = new Player();
    MetalRenderAPI ra;
    //pmp->set(ColorSpaceUnknown); // FIXME: crash on macCatalyst
    pmp->setRenderAPI(&ra,(__bridge void *)videoView);
    pmp->set(ColorSpaceUnknown);
    pmp->setLoop(-1);
    pmp->setAspectRatio(IgnoreAspectRatio);
    pmp->updateNativeSurface((__bridge void *)videoView);
    NSString *path = [[NSBundle mainBundle] pathForResource:@"sample" ofType:@"mp4" inDirectory:nil];
    urls.push_back([[NSURL fileURLWithPath:path].path UTF8String]);

#if 0 // not available in macCatalyst
    // Enumerate just the photos and videos by using ALAssetsGroupSavedPhotos
    ALAssetsLibrary *library = [[ALAssetsLibrary alloc] init];
    [library enumerateGroupsWithTypes:ALAssetsGroupAll | ALAssetsGroupLibrary
                           usingBlock:^(ALAssetsGroup *group, BOOL *stop)
    {
        if (group != nil) {
            // Within the group enumeration block, filter to enumerate just videos.
            [group setAssetsFilter:[ALAssetsFilter allVideos]];
            [group enumerateAssetsUsingBlock:^(ALAsset *result, NSUInteger index, BOOL *stop) {
                if (result) {
                    // Do whatever you need to with `result`
                    //pmp->setMedia([[NSURL fileURLWithPath:path].path UTF8String]);
                    NSLog(@"asset %@", result.defaultRepresentation.url);
                    urls.push_back([result.defaultRepresentation.url.absoluteString UTF8String]);
                    //*stop = TRUE;
                }
            }];
        } else {
            // If group is nil, we're done enumerating
            // e.g. if you're using a UICollectionView reload it here
        }
    } failureBlock:^(NSError *error) {
        // If the user denied the authorization request
        NSLog(@"Authorization declined");
    }];
#endif
    //pmp->setMedia([[NSURL fileURLWithPath:path].path UTF8String]);
    //pmp->setState(PlayingState);
    //pmp->setVideoSurfaceSize(view.bounds.size.width, view.bounds.size.height);
#if 0
    fr = FrameReader::create();
    //fr->setMedia([[NSURL fileURLWithPath:path].path UTF8String]);
    fr->setActiveTracks(MediaType::Video, {0});
    fr->setActiveTracks(MediaType::Audio, {});
    fr->onRead<AudioFrame>([](const AudioFrame& a, int track){
        //std::cout << a.timestamp() << " got audio from " << track << ": " << a.format() << std::endl;
        return true;
    });

    fr->onRead<VideoFrame>([=](const VideoFrame& v, int track){
        if (!v)
            return true;
        const int64_t elapsed = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - t0).count();
        ++count;
        t.push(elapsed);
        if (t.size() > 32)
            t.pop();
        const float fps = double(t.size()*1000)/double(std::max<int64_t>(1LL, t.back() - t.front()));
        dispatch_async(dispatch_get_main_queue(), ^{
            fpsView.text = [[NSString alloc] initWithFormat:@"%d / %d", int(fps), int(count*1000.0/elapsed)];
        });
        return true;
    });
#endif
    [self hwdecSwitched:hwdecButton];
    if (true) {
        [[UIDevice currentDevice] setValue:[NSNumber numberWithInteger:UIDeviceOrientationLandscapeLeft] forKey:@"orientation"];
    } else {
        [[UIDevice currentDevice] setValue:[NSNumber numberWithInteger:UIDeviceOrientationPortrait] forKey:@"orientation"];
    }
    return YES;
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id <UIViewControllerTransitionCoordinator>)coordinator
{
    if (true) {
        [vc view].frame = CGRectMake(0, 0, [UIScreen mainScreen].bounds.size.height, [UIScreen mainScreen].bounds.size.width);
    } else {
        [vc view].frame = CGRectMake(0, 20, [UIScreen mainScreen].bounds.size.height, [UIScreen mainScreen].bounds.size.height * 9/16);
    }
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
    pmp->set(State::Paused);
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
    // Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:
    delete pmp;
    pmp = nullptr;
}

- (void)dealloc
{
    delete pmp;
    pmp = nullptr;
	[vc release];
    [window release];
    [super dealloc];
}
@end
