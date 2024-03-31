//
//  MTKVideoView.swift
//  SwiftApp
//
//  Created by WangBin on 2021/3/15.
//
import MetalKit
import swift_mdk

// MARK: Metal Stuff
@available(visionOS, unavailable)
class MTKVideoView: MTKView {
    private var cmdQueue : MTLCommandQueue!
    private var player = Player()

    var hdr : Bool = false {
        didSet {
            if hdr {
                player.set(colorSpace: .BT2020_PQ, vid: self)
            } else {
                player.set(colorSpace: .BT709, vid: self)
            }
        }
    }

    init(url : URL? = nil) {
        super.init(frame: .zero, device: MTLCreateSystemDefaultDevice())
        // Make sure we are on a device that can run metal!
        guard let defaultDevice = device else {
            fatalError("Device loading error")
        }
        
        setLogHandler({ level, msg in
            print("mdk \(level): \(msg)")
        })
        setGlobalOption(name: "log", value: LogLevel.All)
        createRenderer(device: defaultDevice)
#if os(macOS)
        registerForDraggedTypes([.fileURL, .URL])
#endif // os(macOS)
        if let s = url?.absoluteString {
            player.media = s
            player.state = .Playing
        }
    }

    required init(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

#if os(macOS)
    override func draggingEntered(_ sender: NSDraggingInfo) -> NSDragOperation {
        return .copy
    }

    override func prepareForDragOperation(_ sender: NSDraggingInfo) -> Bool {
        return true
    }

    override func performDragOperation(_ sender: NSDraggingInfo) -> Bool {
        let pb = sender.draggingPasteboard
        if let urls = pb.readObjects(forClasses: [NSURL.self]) as? [URL] {
            player.media = urls.first!.path
            return true
        }
        return false
    }
#endif // os(macOS)
    func removeRenderer() {
        player.setVideoSurfaceSize(-1.0, -1.0, vid: self)
    }

    func createRenderer(device: MTLDevice){
        cmdQueue = device.makeCommandQueue()
        delegate = self
        player.addRenderTarget(self, commandQueue: cmdQueue)

        enableSetNeedsDisplay = false
        if enableSetNeedsDisplay {
            player.setRenderCallback { [weak self] in
                DispatchQueue.main.async {
                    self?.setNeedsDisplay(.zero)
                }
            }
        }

        //let scrW = UIScreen.main.bounds.width //, "BRAW:gpu=auto:scale=\(scrW)", "R3D:scale=\(scrW)"
        player.videoDecoders = ["VT:copy=0", "VideoToolbox", "BRAW:gpu=auto", "R3D", "hap", "FFmpeg", "dav1d"]

        player.setTimeout(0, callback: { timeout in
            print("timeout detected \(timeout)!!!!")
            return true
        })
        player.onMediaStatusChanged {
            print(".....Status changed to \($0)....")
            return true
        }
        setGlobalOption(name: "videoout.clear_on_stop", value: 1)
    }

}

@available(visionOS, unavailable)
extension MTKVideoView: MTKViewDelegate {
    func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
        player.setVideoSurfaceSize(size.width, size.height, vid: view)
    }

    func draw(in view: MTKView) {
        let _ = player.renderVideo(vid: view)
        //print("draw view: \(view)")
        //NSLog("@ buffered: %d", player.buffered())
        guard let d = view.currentDrawable else {
            return
        }
        let cmdBuf = cmdQueue.makeCommandBuffer()
        cmdBuf?.present(d)
        cmdBuf?.commit()
    }
}
