//
//  GameViewController.swift
//  SwiftMdkExample macOS, iOS and tvOS
//
//  Created by Wang Bin on 2021/1/24.
//

#if os(macOS)
import Cocoa
typealias UIViewController = NSViewController
#endif
#if canImport(UIKit)
import UIKit
#endif
import MetalKit

class GameViewController: UIViewController {

    private var player = Player()
    private var cmdQueue : MTLCommandQueue!
    private var cmdQueue2 : MTLCommandQueue!
    private var dev : MTLDevice!
    private var mkv : MTKView
    private var mkv2 : MTKView

    required init?(coder: NSCoder) {
        // Make sure we are on a device that can run metal!
        guard let defaultDevice = MTLCreateSystemDefaultDevice() else {
            fatalError("Device loading error")
        }
        dev = defaultDevice
        cmdQueue = dev.makeCommandQueue()

        mkv = MTKView(frame:CGRect(x: 0, y: 0, width: 100, height: 100))
        //mkv.autoResizeDrawable = true
        mkv.device = dev

        cmdQueue2 = cmdQueue //dev.makeCommandQueue()
        mkv2 = MTKView(frame:CGRect(x: 0, y: 0, width: 100, height: 100))
        //mkv.autoResizeDrawable = true
        mkv2.device = dev
        super.init(coder: coder)
    }

    override func viewDidLoad() {
        super.viewDidLoad()
        player.addRenderTarget(mkv, commandQueue: cmdQueue)
        mkv.delegate = self
        mkv.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(mkv)

        mkv.leftAnchor.constraint(equalTo: view.leftAnchor).isActive = true
        //mkv.rightAnchor.constraint(equalTo: view.rightAnchor).isActive = true
        mkv.topAnchor.constraint(equalTo: view.topAnchor).isActive = true
        mkv.bottomAnchor.constraint(equalTo: view.bottomAnchor).isActive = true
        mkv.widthAnchor.constraint(equalTo: view.widthAnchor, multiplier: 0.5).isActive = true
        
        player.addRenderTarget(mkv2, commandQueue: cmdQueue2)
        mkv2.delegate = self
        mkv2.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(mkv2)

        //mkv2.leftAnchor.constraint(equalTo: view.leftAnchor).isActive = true
        mkv2.rightAnchor.constraint(equalTo: view.rightAnchor).isActive = true
        mkv2.topAnchor.constraint(equalTo: view.topAnchor).isActive = true
        mkv2.bottomAnchor.constraint(equalTo: view.bottomAnchor).isActive = true
        mkv2.widthAnchor.constraint(equalTo: view.widthAnchor, multiplier: 0.5).isActive = true

        // we support rendering driven internally and externally
        mkv.enableSetNeedsDisplay = true
        mkv2.enableSetNeedsDisplay = false

        if mkv.enableSetNeedsDisplay || mkv2.enableSetNeedsDisplay {
            player.setRenderCallback { [weak mkv2, weak mkv] in
                DispatchQueue.main.async {
                    mkv?.setNeedsDisplay(.zero)
                    mkv2?.setNeedsDisplay(.zero)
                }
            }
        }

        logLevel = .Warning
        
        setLogHandler({ level, msg in
            print("mdk \(level): \(msg)")
        })
        player.currentMediaChanged({
            print("++++++++++currentMediaChanged: \(self.player.media)+++++++")
        })
        player.setTimeout(0, callback: { timeout in
            print("timeout detected \(timeout)!!!!")
            return true
        })
        player.onStateChanged { (state) in
            print(".....State changed to \(state)....")
        }
        player.onMediaStatusChanged {
            print(".....Status changed to \($0)....")
            return true
        }
        // Do any additional setup after loading the view.
        player.media = "https://devimages.apple.com.edgekey.net/streaming/examples/bipbop_4x3/gear1/prog_index.m3u8"

        player.videoDecoders = ["VT:copy=0", "FFmpeg"]
        player.setRange(from: 1000)
        player.setBufferRange(msMin: 1000, msMax: 4000)
        //player.loop = -1;
        player.prepare(from: 1000, complete: {
            [weak player] from, boost in
            boost = true
            guard let info = player?.mediaInfo else {
                return true
            }
            print("MediaInfo: \(info)")
            return true
        })
        //setGlobalOption(name: "MDK_KEY", value: "123")
        setGlobalOption(name: "videoout.clear_on_stop", value: 1)
        player.state = .Playing
    }
/*
    override var representedObject: Any? {
        didSet {
        // Update the view, if already loaded.
        }
    }
*/
}


extension GameViewController : MTKViewDelegate {
    func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
        player.setVideoSurfaceSize(size.width, size.height, vid: view)
    }

    func draw(in view: MTKView) {
        player.renderVideo(vid: view)
        //print("draw view: \(view)")
        //NSLog("@ buffered: %d", player.buffered())
        guard let d = view.currentDrawable else {
            return
        }
        let cmdBuf = view == mkv ? cmdQueue.makeCommandBuffer() : cmdQueue2.makeCommandBuffer()
        cmdBuf?.present(d)
        cmdBuf?.commit()
    }
}
