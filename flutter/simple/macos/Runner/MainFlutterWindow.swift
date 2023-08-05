import Cocoa
import FlutterMacOS

class MainFlutterWindow: NSWindow {
  open var currentFile: String? // add this variable

  override func awakeFromNib() {
    let flutterViewController = FlutterViewController()
    let windowFrame = self.frame
    self.contentViewController = flutterViewController
    self.setFrame(windowFrame, display: true)

    let channel = FlutterMethodChannel(name: "openFile", binaryMessenger: flutterViewController.engine.binaryMessenger)
    channel.setMethodCallHandler({
        (call: FlutterMethodCall, result: FlutterResult) -> Void in
        if (call.method == "getCurrentFile") {
            result(self.currentFile)
        } else {
            result(FlutterMethodNotImplemented)
        }
    })

    RegisterGeneratedPlugins(registry: flutterViewController)

    super.awakeFromNib()
  }
}
