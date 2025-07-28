/*
 * Copyright (C) 2013-2015 Bilibili
 * Copyright (C) 2013-2015 Zhang Rui <bbcallen@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#import "IJKMoviePlayerViewController.h"
#import "IJKMediaControl.h"
#import "IJKCommon.h"
#import "IJKDemoHistory.h"

@implementation IJKVideoViewController {
    NSString *filePath;
}

- (void)dealloc
{
}

+ (void)presentFromViewController:(UIViewController *)viewController withTitle:(NSString *)title URL:(NSURL *)url completion:(void (^)())completion {
    IJKDemoHistoryItem *historyItem = [[IJKDemoHistoryItem alloc] init];
    
    historyItem.title = title;
    historyItem.url = url;
    [[IJKDemoHistory instance] add:historyItem];
    
    [viewController presentViewController:[[IJKVideoViewController alloc] initWithURL:url] animated:YES completion:completion];
}

- (instancetype)initWithManifest: (NSString*)manifest_string {
    self = [self initWithNibName:@"IJKMoviePlayerViewController" bundle:nil];
    if (self) {
        self.url = [NSURL URLWithString:@"ijklas:"];
        self.manifest = manifest_string;
        
    }
    return self;
}

- (instancetype)initWithURL:(NSURL *)url {
    self = [self initWithNibName:@"IJKMoviePlayerViewController" bundle:nil];
    if (self) {
        self.url = url;
    }
    return self;
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

#define EXPECTED_IJKPLAYER_VERSION (1 << 16) & 0xFF) |
- (void)viewDidLoad
{
    [super viewDidLoad];
    if (@available(iOS 14, *)) {
        
        [PHPhotoLibrary requestAuthorizationForAccessLevel:PHAccessLevelReadWrite handler:^(PHAuthorizationStatus status) {
            
        }];
    } else {
        // Fallback on earlier versions
        [PHPhotoLibrary requestAuthorization:^(PHAuthorizationStatus status) {
                        
        }];
    }
    // Do any additional setup after loading the view from its nib.

//    [[UIApplication sharedApplication] setStatusBarHidden:YES];
//    [[UIApplication sharedApplication] setStatusBarOrientation:UIInterfaceOrientationLandscapeLeft animated:NO];

#ifdef DEBUG
    [IJKFFMoviePlayerController setLogReport:YES];
    [IJKFFMoviePlayerController setLogLevel:k_IJK_LOG_DEBUG];
#else
    [IJKFFMoviePlayerController setLogReport:NO];
    [IJKFFMoviePlayerController setLogLevel:k_IJK_LOG_INFO];
#endif

    [IJKFFMoviePlayerController checkIfFFmpegVersionMatch:YES];
    // [IJKFFMoviePlayerController checkIfPlayerVersionMatch:YES major:1 minor:0 micro:0];

    IJKFFOptions *options = [IJKFFOptions optionsByDefault];

    if (self.manifest != nil){
        [options setPlayerOptionValue:@"ijklas"         forKey:@"iformat"];
        [options setPlayerOptionIntValue:0              forKey:@"find_stream_info"];
        [options setFormatOptionValue:self.manifest     forKey:@"manifest_string"];
        
    }
    // 启用 VideoToolbox 硬解
//    [options setPlayerOptionIntValue:1 forKey:@"videotoolbox"];
    
    [options setFormatOptionValue:@"Cookie: AWSALB=Ci6Qn35OrEtIaCRyWXCS9v4crleOQEK0eEIZsYqEkqfMJrXopC+Yr0XRZw7Xz+OovOdcqFsq1ZjE/T9kwZNJAqqj+AFdkgH70+TbMXhNARzj93lNQOnIRLNB0wPl; AWSALBCORS=Ci6Qn35OrEtIaCRyWXCS9v4crleOQEK0eEIZsYqEkqfMJrXopC+Yr0XRZw7Xz+OovOdcqFsq1ZjE/T9kwZNJAqqj+AFdkgH70+TbMXhNARzj93lNQOnIRLNB0wPl; CloudFront-Policy=eyJTdGF0ZW1lbnQiOlt7IlJlc291cmNlIjoiaHR0cHM6Ly9kb3duLWNuLmFvc3VsaWZlLmNvbS9jbG91ZC1zdG9yYWdlLTcvZ2xhemVyby9jbG91ZC9DMkUyREExMTAwMTk0NzAvMTc1Mjc1NTIwNzMyNi8qIiwiQ29uZGl0aW9uIjp7IkRhdGVMZXNzVGhhbiI6eyJBV1M6RXBvY2hUaW1lIjoxNzUyODA5MzQ3fX19XX0_; CloudFront-Signature=oF~AYRt0li2JuEsT8EFoe7Yx3vjZ~Ul045HIm6jJ1KIqim~4-WNnOHq2mbrBPfaCWiXuH2Pyn1dLTYl5jCo1zv6dKMraCjKnZECFEaFKUasetQ4FBwy3iTTAsXdjxCm2D0pO1PbLhjRdLK2f-QsyXLOosNe124tdH2atcFoL9LUoNYP-kh4cllIbfpgO6~DUy8WSdn7p3lR8KQoDxCI6vecgSiEy6h1ulSxhjRPxMpnZirQuKWYU6vSJDYuXYyIQogXwP5nEdReQHUhG7GnzE3SoMQM77g8amZeT1~1lUJi3bOL8xHC5C3dSftVPdoi0KE57SIAXKCeraW24Fcn2Og__; CloudFront-Key-Pair-Id=KQ613EGKVRRWL" forKey:@"headers"];
    
    self.url = [NSURL fileURLWithPath:[[NSBundle mainBundle] pathForResource:@"playlist" ofType:@"m3u8"]];
    
    self.player = [[IJKFFMoviePlayerController alloc] initWithContentURL:self.url withOptions:options];
    NSLog(@"%@FFP_MSG_初始化ijk播放器", [self currentDateString]);
    self.player.view.autoresizingMask = UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
    self.player.view.frame = self.view.bounds;
    self.player.scalingMode = IJKMPMovieScalingModeAspectFit;
    self.player.shouldAutoplay = YES;
//    self.player.playbackRate = 4.0;
    [self.player setPauseInBackground:YES];
    self.view.autoresizesSubviews = YES;
    [self.view addSubview:self.player.view];
    [self.view addSubview:self.mediaControl];
    self.mediaControl.delegatePlayer = self.player;
    
//    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
//        NSString *filePath = [[NSBundle mainBundle] pathForResource:@"demo/demo" ofType:@"m3u8"];
//        filePath = @"https://sf1-cdn-tos.huoshanstatic.com/obj/media-fe/xgplayer_doc_video/hls/xgplayer-demo.m3u8";
//        self -> filePath = filePath;
//        NSString *uuid = [[NSUUID UUID] UUIDString];
//        NSString *outfilePath = [NSTemporaryDirectory() stringByAppendingPathComponent:[NSString stringWithFormat:@"%@.mp4", uuid]];
//        NSString * cookieString = @"Cookie:AWSALB=1Q3kPTEO0Ge1TlO6nbWlRLod69wdqctCBYMhKQ/2OlCAfGHYNTJudb/AwDVBZDZzWZkE3wvUSLJgkSsvzeLa4+Un1bEjtAwn8qQJZgWjCzWGAf+9jdGaCU9fPGGp; AWSALBCORS=1Q3kPTEO0Ge1TlO6nbWlRLod69wdqctCBYMhKQ/2OlCAfGHYNTJudb/AwDVBZDZzWZkE3wvUSLJgkSsvzeLa4+Un1bEjtAwn8qQJZgWjCzWGAf+9jdGaCU9fPGGp; CloudFront-Policy=eyJTdGF0ZW1lbnQiOlt7IlJlc291cmNlIjoiaHR0cHM6Ly9kb3duLWNuLmFvc3VsaWZlLmNvbS9jbG91ZC1zdG9yYWdlLTcvZ2xhemVyby9jbG91ZC9DMkUyREExMTAwMTk0NzAvMTc1MTk3NjM4NDQxOC8qIiwiQ29uZGl0aW9uIjp7IkRhdGVMZXNzVGhhbiI6eyJBV1M6RXBvY2hUaW1lIjoxNzUxOTgwMDI3fX19XX0_; CloudFront-Signature=AxLYy4nurSCsVYP-P0kG3tSVQCZbfNz4Gy34E0wCdwIQLjpjIUvF9jEfbllRKOH2ke7-iGk7iHPfuKoLKusgaTLarXetokhQa8GhmUfMfxarttaU6y-aw6Jj~9evuzpDrx6HVy44Fz~cHGknyLM8-GB5ieSJ45mwPWDiCPTkINERD3F5KVeSaYEilnvsJwEI1D896h3-8uQlk9rVF8LdIAgoorpON0DuvFOAuN7ku49Yb6GXAdIh6Seg3uvLcQ8NNDtPWQg1VNJjCef1tk-FNFnA8F-7ZXkxssCc-aRArtmKlUqjdu40LCmFhF~scU6iJd9AvZN7XaRzprT3qdbBzw__; CloudFront-Key-Pair-Id=KQ613EGKVRRWL";
//        [IJKFFMoviePlayerController downloadVideoFromURL:filePath toFile:outfilePath cookieString:cookieString progress:^(int progress) {
//            NSLog(@"当前下载进度--->%d",progress);
//        }];
//        [self saveFileToPhotoLibrary:outfilePath];
//        NSLog(@"%@", outfilePath);
//    });
    
//    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(2.5 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
//        [IJKFFMoviePlayerController cleanAllDowloadTask];
//    });
   
}
- (NSString *)currentDateString {
    NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
    formatter.dateFormat = @"yyyy-MM-dd HH:mm:ss.SSS   ";
    return  [formatter stringFromDate:[NSDate date]];
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
    [self installMovieNotificationObservers];

    [self.player prepareToPlay];
}

- (void)viewDidDisappear:(BOOL)animated {
    [super viewDidDisappear:animated];
    
    [self.player shutdown];
    [self removeMovieNotificationObservers];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation{
    return UIInterfaceOrientationIsLandscape(toInterfaceOrientation);
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskLandscape;
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

#pragma mark IBAction

- (IBAction)onClickMediaControl:(id)sender
{
    [self.mediaControl showAndFade];
}

- (IBAction)onClickOverlay:(id)sender
{
    [self.mediaControl hide];
}

- (IBAction)onClickDone:(id)sender
{
    [self.presentingViewController dismissViewControllerAnimated:YES completion:nil];
}

- (IBAction)onClickHUD:(UIBarButtonItem *)sender
{
    if ([self.player isRecording]) {
        [self stopRecord];
    } else {
        [self beginRecord];
    }
    if ([self.player isKindOfClass:[IJKFFMoviePlayerController class]]) {
        IJKFFMoviePlayerController *player = self.player;
        sender.title = ([player isRecording] ? @"结束录制" : @"开始录制");
    }
}

- (void)beginRecord {
    NSString *uuid = [[NSUUID UUID] UUIDString];
    filePath = [NSTemporaryDirectory() stringByAppendingPathComponent:[NSString stringWithFormat:@"%@.mp4", uuid]];
    if (![self.player isRecording] && [self.player isPlaying]) {
        [self.player startRecordWithFileName:self->filePath recordFail:^(int errorCode) {
            NSLog(@"%d", errorCode);
            
            dispatch_async(dispatch_get_main_queue(), ^{
                self.beginRecordButton.title = @"开始录制";
            });
        }];
        }
}

- (void)stopRecord {
    [self.player stopRecord];
    [self saveFileToPhotoLibrary:filePath];
}

-(void)saveFileToPhotoLibrary:(NSString *)filePath {
    PHPhotoLibrary *photoLibrary = [PHPhotoLibrary sharedPhotoLibrary];
    
    [photoLibrary performChanges:^{
        [PHAssetChangeRequest creationRequestForAssetFromVideoAtFileURL:[NSURL
                                                                         fileURLWithPath:filePath]];
    } completionHandler:^(BOOL success, NSError * _Nullable error) {
        [[NSFileManager defaultManager] removeItemAtPath:filePath error:nil];
        if (success) {
            NSLog(@"已将视频保存至相册");
        } else {
            NSLog(@"未能保存视频到相册");
        }
    }];
}

- (IBAction)onClickPlay:(id)sender
{
    [self.player play];
    [self.mediaControl refreshMediaControl];
    
    
}

- (IBAction)onClickPause:(id)sender
{
    [self.player pause];
    [self.mediaControl refreshMediaControl];
    
}

- (IBAction)didSliderTouchDown
{
    [self.mediaControl beginDragMediaSlider];
}

- (IBAction)didSliderTouchCancel
{
    [self.mediaControl endDragMediaSlider];
}

- (IBAction)didSliderTouchUpOutside
{
    [self.mediaControl endDragMediaSlider];
}

- (IBAction)didSliderTouchUpInside
{
    self.player.currentPlaybackTime = self.mediaControl.mediaProgressSlider.value;
    [self.mediaControl endDragMediaSlider];
}

- (IBAction)didSliderValueChanged
{
    [self.mediaControl continueDragMediaSlider];
}

- (void)loadStateDidChange:(NSNotification*)notification
{
    //    MPMovieLoadStateUnknown        = 0,
    //    MPMovieLoadStatePlayable       = 1 << 0,
    //    MPMovieLoadStatePlaythroughOK  = 1 << 1, // Playback will be automatically started in this state when shouldAutoplay is YES
    //    MPMovieLoadStateStalled        = 1 << 2, // Playback will be automatically paused in this state, if started

    IJKMPMovieLoadState loadState = _player.loadState;

    if ((loadState & IJKMPMovieLoadStatePlaythroughOK) != 0) {
        NSLog(@"loadStateDidChange: IJKMPMovieLoadStatePlaythroughOK: %d\n", (int)loadState);
    } else if ((loadState & IJKMPMovieLoadStateStalled) != 0) {
        NSLog(@"loadStateDidChange: IJKMPMovieLoadStateStalled: %d\n", (int)loadState);
    } else {
        NSLog(@"loadStateDidChange: ???: %d\n", (int)loadState);
    }
}

- (void)moviePlayBackDidFinish:(NSNotification*)notification
{
    //    MPMovieFinishReasonPlaybackEnded,
    //    MPMovieFinishReasonPlaybackError,
    //    MPMovieFinishReasonUserExited
    int reason = [[[notification userInfo] valueForKey:IJKMPMoviePlayerPlaybackDidFinishReasonUserInfoKey] intValue];

    switch (reason)
    {
        case IJKMPMovieFinishReasonPlaybackEnded:
            NSLog(@"playbackStateDidChange: IJKMPMovieFinishReasonPlaybackEnded: %d\n", reason);
            break;

        case IJKMPMovieFinishReasonUserExited:
            NSLog(@"playbackStateDidChange: IJKMPMovieFinishReasonUserExited: %d\n", reason);
            break;

        case IJKMPMovieFinishReasonPlaybackError:
            NSLog(@"playbackStateDidChange: IJKMPMovieFinishReasonPlaybackError: %d\n", reason);
            break;

        default:
            NSLog(@"playbackPlayBackDidFinish: ???: %d\n", reason);
            break;
    }
}

- (void)mediaIsPreparedToPlayDidChange:(NSNotification*)notification
{
    NSLog(@"mediaIsPreparedToPlayDidChange\n");
}

- (void)moviePlayBackStateDidChange:(NSNotification*)notification
{
    //    MPMoviePlaybackStateStopped,
    //    MPMoviePlaybackStatePlaying,
    //    MPMoviePlaybackStatePaused,
    //    MPMoviePlaybackStateInterrupted,
    //    MPMoviePlaybackStateSeekingForward,
    //    MPMoviePlaybackStateSeekingBackward

    switch (_player.playbackState)
    {
        case IJKMPMoviePlaybackStateStopped: {
            NSLog(@"IJKMPMoviePlayBackStateDidChange %d: stoped", (int)_player.playbackState);
            break;
        }
        case IJKMPMoviePlaybackStatePlaying: {
            NSLog(@"IJKMPMoviePlayBackStateDidChange %d: playing", (int)_player.playbackState);
            break;
        }
        case IJKMPMoviePlaybackStatePaused: {
            NSLog(@"IJKMPMoviePlayBackStateDidChange %d: paused", (int)_player.playbackState);
            break;
        }
        case IJKMPMoviePlaybackStateInterrupted: {
            NSLog(@"IJKMPMoviePlayBackStateDidChange %d: interrupted", (int)_player.playbackState);
            break;
        }
        case IJKMPMoviePlaybackStateSeekingForward:
        case IJKMPMoviePlaybackStateSeekingBackward: {
            NSLog(@"IJKMPMoviePlayBackStateDidChange %d: seeking", (int)_player.playbackState);
            break;
        }
        default: {
            NSLog(@"IJKMPMoviePlayBackStateDidChange %d: unknown", (int)_player.playbackState);
            break;
        }
    }
}

#pragma mark Install Movie Notifications

/* Register observers for the various movie object notifications. */
-(void)installMovieNotificationObservers
{
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(loadStateDidChange:)
                                                 name:IJKMPMoviePlayerLoadStateDidChangeNotification
                                               object:_player];

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(moviePlayBackDidFinish:)
                                                 name:IJKMPMoviePlayerPlaybackDidFinishNotification
                                               object:_player];

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(mediaIsPreparedToPlayDidChange:)
                                                 name:IJKMPMediaPlaybackIsPreparedToPlayDidChangeNotification
                                               object:_player];

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(moviePlayBackStateDidChange:)
                                                 name:IJKMPMoviePlayerPlaybackStateDidChangeNotification
                                               object:_player];
}

#pragma mark Remove Movie Notification Handlers

/* Remove the movie notification observers from the movie object. */
-(void)removeMovieNotificationObservers
{
    [[NSNotificationCenter defaultCenter]removeObserver:self name:IJKMPMoviePlayerLoadStateDidChangeNotification object:_player];
    [[NSNotificationCenter defaultCenter]removeObserver:self name:IJKMPMoviePlayerPlaybackDidFinishNotification object:_player];
    [[NSNotificationCenter defaultCenter]removeObserver:self name:IJKMPMediaPlaybackIsPreparedToPlayDidChangeNotification object:_player];
    [[NSNotificationCenter defaultCenter]removeObserver:self name:IJKMPMoviePlayerPlaybackStateDidChangeNotification object:_player];
}

@end
