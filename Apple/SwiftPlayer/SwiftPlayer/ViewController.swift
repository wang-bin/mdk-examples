//
//  ViewController.swift
//  SwiftPlayer
//
//  Created by WangBin on 2020/11/6.
//

import Cocoa
import mdk
import MetalKit
// https://stackoverflow.com/questions/43880839/swift-unable-to-cast-function-pointer-to-void-for-use-in-c-style-third-party
// https://stackoverflow.com/questions/37401959/how-can-i-get-the-memory-address-of-a-value-type-or-a-custom-struct-in-swift
// https://stackoverflow.com/questions/33294620/how-to-cast-self-to-unsafemutablepointervoid-type-in-swift
import Foundation // needed for strdup and free

public enum State : UInt32 {
    case Stopped
    case Playing
    case Paused
}

public func withArrayOfCStrings<R>(
    _ args: [String],
    _ body: ([UnsafeMutablePointer<CChar>?]) -> R
) -> R {
    var cStrings = args.map { strdup($0) }
    cStrings.append(nil)
    defer {
        cStrings.forEach { free($0) }
    }
    return body(cStrings)
}

func bridge<T : AnyObject>(obj : T) -> UnsafeRawPointer {
    return UnsafeRawPointer(Unmanaged.passUnretained(obj).toOpaque())
    // return unsafeAddressOf(obj) // ***
}

func bridge<T : AnyObject>(obj : T) -> UnsafeMutableRawPointer {
    return UnsafeMutableRawPointer(Unmanaged.passUnretained(obj).toOpaque())
    // return unsafeAddressOf(obj) // ***
}

func bridge<T : AnyObject>(ptr : UnsafeRawPointer) -> T {
    return Unmanaged<T>.fromOpaque(ptr).takeUnretainedValue()
    // return unsafeBitCast(ptr, T.self) // ***
}

func address(o: UnsafeRawPointer) -> OpaquePointer {
    return unsafeBitCast(o, to: OpaquePointer.self)
}

class CPlayer {
    public var mute = false {
        didSet {
            player.pointee.setMute(player.pointee.object, mute)
        }
    }
    
    public var volume:Float = 1.0 {
        didSet {
            player.pointee.setVolume(player.pointee.object, volume)
        }
    }
    
    public var media = "" {
        didSet {
            player.pointee.setMedia(player.pointee.object, media)
        }
    }
    
    // audioDecoders
    /*public var audioDecoders = ["FFmpeg"] {
        didSet {
            withArrayOfCStrings(audioDecoders) {
                player.pointee.setVideoDecoders(player.pointee.object, $0)
            }
        }
    }
    
    public var videoDecoders = ["FFmpeg"] {
        didSet {
            withArrayOfCStrings(videoDecoders) {
                player.pointee.setVideoDecoders(player.pointee.object, $0)
            }
        }
    }*/
    
    
    public var state:State = .Stopped {
        didSet {
            player.pointee.setState(player.pointee.object, MDK_State(state.rawValue))
        }
    }
    
    public var loop:Int32 = 0 {
        didSet {
            player.pointee.setLoop(player.pointee.object, loop)
        }
    }
    
    public var preloadImmediately = true {
        didSet {
            player.pointee.setPreloadImmediately(player.pointee.object, preloadImmediately)
        }
    }
    
    public var nextMedia = "" {
        didSet {
        }
    }
        
    private var player : UnsafePointer<mdkPlayerAPI>! = mdkPlayerAPI_new()
   
    deinit {
        mdkPlayerAPI_delete(&player)
    }

    func setRendAPI(_ api :  UnsafePointer<mdkMetalRenderAPI>) ->Void {
        player.pointee.setRenderAPI(player.pointee.object, OpaquePointer(api), nil)
    }

    func setRenderTarget(_ mkv : MTKView, commandQueue cmdQueue: MTLCommandQueue) ->Void {
        func currentRt(_ opaque: UnsafeRawPointer?)->UnsafeRawPointer? {
            guard let p = opaque else {
                return nil
            }
            let v : MTKView = bridge(ptr: p)
            guard let drawable = v.currentDrawable else {
                return nil
            }
            return bridge(obj: drawable.texture)
        }

        var ra = mdkMetalRenderAPI()
        ra.type = MDK_RenderAPI_Metal
        ra.device = bridge(obj: mkv.device.unsafelyUnwrapped)
        ra.cmdQueue = bridge(obj: cmdQueue)
        ra.opaque = bridge(obj: mkv)
        ra.currentRenderTarget = currentRt
        setRendAPI(&ra)
    }

    func setVideoSurfaceSize(_ width : CGFloat, _ height : CGFloat)->Void {
        player.pointee.setVideoSurfaceSize(player.pointee.object, Int32(width), Int32(height), nil)
    }

    func renderVideo() -> Double {
        return player.pointee.renderVideo(player.pointee.object, nil)
    }
    
    func set(media:String, forType type:MDK_MediaType) {
        player.pointee.setMediaForType(player.pointee.object, media, type)
    }
    
    func setNext(media:String, from:Int64 = 0, withSeekFlag flag:MDK_SeekFlag = MDK_SeekFlag_Default) {
        player.pointee.setNextMedia(player.pointee.object, nextMedia, from, flag)
    }
    
    func prepare(from:Int64, complete:((Int64, inout Bool)->Bool)?) {
        func _f(pos:Int64, boost:UnsafeMutablePointer<Bool>?, opaque:UnsafeMutableRawPointer?)->Bool {
            let f = opaque?.load(as: ((Int64, inout Bool)->Bool).self)
            var _boost = true
            let ret = f!(pos, &_boost)
            boost?.assign(from: &_boost, count: 1)
            return ret
        }
        var cb = mdkPrepareCallback()
        if complete != nil {
            if prepare_cb == nil {
                prepare_cb = UnsafeMutableRawPointer.allocate(byteCount: MemoryLayout<()>.stride, alignment: MemoryLayout<()>.alignment)
            }
            //cb.opaque = UnsafeMutableRawPointer.allocate(byteCount: MemoryLayout<(Int64, inout Bool)->Bool>.stride, alignment: MemoryLayout<(Int64, inout Bool)->Bool>.alignment)
            cb.opaque = prepare_cb
            var tmp = complete
            cb.opaque.initializeMemory(as: type(of: complete), from: &tmp, count: 1)
        }
        cb.cb = _f
        player.pointee.prepare(player.pointee.object, from, cb, MDK_SeekFlag_Default)
    }
    
    func waitFor(_ state:State, timeout:Int) -> Bool {
        return player.pointee.waitFor(player.pointee.object, MDK_State(state.rawValue), timeout)
    }
    
    private var prepare_cb : UnsafeMutableRawPointer? //((Int64, inout Bool)->Bool)?
}

class ViewController: NSViewController {
    private var player = CPlayer()
    private var cmdQueue : MTLCommandQueue!
    private var dev : MTLDevice!
    private var mkv : MTKView

    required init?(coder: NSCoder) {
        // Make sure we are on a device that can run metal!
        guard let defaultDevice = MTLCreateSystemDefaultDevice() else {
            fatalError("Device loading error")
        }
        dev = defaultDevice
        cmdQueue = dev.makeCommandQueue()

        mkv = MTKView(frame:NSMakeRect(0, 0, 100, 100))
        //mkv.autoResizeDrawable = true
        mkv.device = dev
        super.init(coder: coder)
    }

    override func viewDidLoad() {
        super.viewDidLoad()
        player.setRenderTarget(mkv, commandQueue: cmdQueue)
        mkv.delegate = self
        mkv.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(mkv)

        mkv.leftAnchor.constraint(equalTo: view.leftAnchor).isActive = true
        mkv.rightAnchor.constraint(equalTo: view.rightAnchor).isActive = true
        mkv.topAnchor.constraint(equalTo: view.topAnchor).isActive = true
        mkv.bottomAnchor.constraint(equalTo: view.bottomAnchor).isActive = true

        // Do any additional setup after loading the view.
        player.media = "https://devimages.apple.com.edgekey.net/streaming/examples/bipbop_4x3/gear1/prog_index.m3u8"
        //player.videoDecoders = ["VT", "FFmpeg"]
        player.loop = -1;
        /*player.prepare(from: 10000, complete: {
            from, boost in
            boost = true
            return true
        })*/
        player.state = .Playing
    }

    override var representedObject: Any? {
        didSet {
        // Update the view, if already loaded.
        }
    }


}

extension ViewController : MTKViewDelegate {
    func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
        player.setVideoSurfaceSize(size.width, size.height)
    }

    func draw(in view: MTKView) {
        player.renderVideo()
        guard let d = mkv.currentDrawable else {
            return
        }
        let cmdBuf = cmdQueue.makeCommandBuffer()
        cmdBuf?.present(d)
        cmdBuf?.commit()
    }
}

