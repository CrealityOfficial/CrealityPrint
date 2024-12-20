#include "macwindow.h"
#include "mycppobject.h"
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

//重构appdelegate
@interface  AppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, assign) MyCppObject *wrapped;
@end
@implementation  AppDelegate
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
    //这个控制cmd+q 是否被直接响应退出。NSTerminateNow
    //NSTerminateNow 表示马上退出，这里我们要给弹窗自己控制退出。所以这里要是NSTerminateCancel
    NSLog(@"applicationShouldTerminate");
    self.wrapped->closeFunction();
    return NSTerminateCancel;
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    NSLog(@"applicationShouldTerminateAfterLastWindowClosed");
    return NO;
}
- (BOOL)applicationShouldHandleReopen:(NSApplication *)sender hasVisibleWindows:(BOOL)flag
{
    NSLog(@"applicationShouldHandleReopen =%d",flag);
    return YES;
}

- (void)applicationWillFinishLaunching:(NSNotification *)notification
{
   NSLog(@"applicationWillFinishLaunching");
}
- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
   NSLog(@"applicationDidFinishLaunching");

}
- (void)applicationWillHide:(NSNotification *)notification
{
    NSLog(@"applicationWillHide");
}
- (void)applicationDidHide:(NSNotification *)notification
{
    NSLog(@"applicationDidHide");
}

- (void)applicationWillBecomeActive:(NSNotification *)notification
{
   NSLog(@"applicationWillBecomeActive");
}

- (void)applicationDidBecomeActive:(NSNotification *)notification
{
   NSLog(@"applicationDidBecomeActive");
}
@end

@interface WindowDelegate : NSObject <NSWindowDelegate>
@property (nonatomic, assign) float topY;
@property (nonatomic, assign) MyCppObject *wrapped;

@property (nonatomic,assign) void (^windowShouldClosecCallback)();
@end

@implementation WindowDelegate

- (void)exampleMethodWithString:(NSString *)str
{
    std::string cpp_str([str UTF8String],[str lengthOfBytesUsingEncoding:NSUTF8StringEncoding]);

    self.wrapped->ExampleMethod(cpp_str);
    NSLog(@"exampleMethodWithString");

}
- (void)useselfClose:(NSInteger) size
{
    self.wrapped->closeFunction();

}

- (id)init
{
    if(self =[super init])
    {
        //初始化对象
        self.wrapped = new MyCppObject();
    }
    return self;
}

- (void)dealloc
{
    delete self.wrapped;
    [super dealloc];
}

- (void)windowDidExitFullScreen:(NSNotification *)notification
{
    //重构退出全屏操作
    NSLog(@"windowDidExitFullScreen");
    NSWindow *window = notification.object;
    window.styleMask |= NSWindowStyleMaskFullSizeContentView;
}

- (BOOL)windowShouldClose:(NSWindow *)sender
{
    //重构退出
    NSLog(@"windowShouldClose");
    // [self exampleMethodWithString:@"window close example"];
    [self useselfClose:0];
    return NO;
}

- (void)windowWillClose:(NSNotification *)notification
{
    //重构退出
    NSLog(@"windowWillClose");
}
@end


MacWindow::MacWindow()
{


}
void MacWindow::setContentWindow(QWindow *contentWindow)
{
    m_window = contentWindow;
}

void MacWindow::createNSWindow()
{
    qDebug() << "createNSWindow";
    if (m_nsWindow)
        return;

    NSView *view = (__bridge NSView *)reinterpret_cast<void *>(m_window->winId());
    NSWindow *window = [view window];


    WindowDelegate *delegate = [[WindowDelegate alloc] init];
    delegate.topY = 10.0;
    m_macAppObject =  delegate.wrapped;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1.0 * NSEC_PER_SEC), dispatch_get_main_queue(), ^(){
    // 启动一秒之后，重新设置NSAPP的代理。
    // 代理主要是解决 nsapp 自带的关闭程序，需要弹窗
    // 这么处理的原因是，直接设置的话，程序打开的时候不是展开的，会被隐藏在任务栏
                       NSLog(@" 1s");
                       AppDelegate *myAppDelegate = [[AppDelegate alloc]init];
                       myAppDelegate.wrapped = delegate.wrapped;
                       NSApp.delegate = myAppDelegate;
                   });
    window.delegate = delegate;
    m_nsWindow = window;
    m_nsWindow.titleVisibility = m_titleVisibility ? NSWindowTitleVisible : NSWindowTitleHidden;
    m_nsWindow.titlebarAppearsTransparent = m_titlebarAppearsTransparent;
    m_nsWindow.styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable| NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
                         | NSWindowStyleMaskFullSizeContentView;

  
}

void MacWindow::destroyNSWindow()
{
     @autoreleasepool {
         [m_nsWindow close];
         m_nsWindow = nil;
     }
}

void MacWindow::recreateNSWindow()
{
    if (!m_nsWindow)
        return;

    destroyNSWindow();
    createNSWindow();

    if (m_visible)
        [m_nsWindow makeKeyAndOrderFront:nil];
}

void MacWindow::scheduleRecreateNSWindow()
{
}

void MacWindow::setFullSizeContentView(bool enable)
{
    if (m_fullSizeContentView == enable)
        return;
    m_fullSizeContentView = enable;
    scheduleRecreateNSWindow();
}

bool MacWindow::fullSizeContentView() const
{
    return m_fullSizeContentView;
}

void MacWindow::setTitlebarAppearsTransparent(bool enable)
{
    if (m_titlebarAppearsTransparent == enable)
        return;

    m_titlebarAppearsTransparent = enable ? YES : NO;
}

bool MacWindow::titlebarAppearsTransparent() const
{
    return m_titlebarAppearsTransparent;
}



void MacWindow::setTitleVisibility(bool enable)
{
    if (m_titleVisibility == enable)
        return;
    m_titleVisibility = enable;
}

bool MacWindow::titleVisibility() const
{
    return m_titleVisibility;
}

void MacWindow::setVisible(bool visible)
{
    qDebug() << "setVisible" << visible;
    m_visible = visible;
    if (visible) {
        createNSWindow();
        [m_nsWindow makeKeyAndOrderFront:nil];
    } else {

    }
}

void MacWindow::show()
{
    setVisible(true);
}
// void MacWindow::setMacObject(MyCppObject *cppObject)
// {
//     m_macAppObject = cppObject;
// }

MyCppObject *MacWindow::getMacAppObject()
{
    return m_macAppObject;
}
