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
func bridge<T : AnyObject>(obj : T) -> UnsafeRawPointer {
    return UnsafeRawPointer(Unmanaged.passUnretained(obj).toOpaque())
    // return unsafeAddressOf(obj) // ***
}

func bridge<T : AnyObject>(ptr : UnsafeRawPointer) -> T {
    return Unmanaged<T>.fromOpaque(ptr).takeUnretainedValue()
    // return unsafeBitCast(ptr, T.self) // ***
}

func address(o: UnsafeRawPointer) -> OpaquePointer {
    return unsafeBitCast(o, to: OpaquePointer.self)
}

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

class CPlayer {
    private var player : UnsafePointer<mdkPlayerAPI>?
    init() {
        player = mdkPlayerAPI_new()
    }

    deinit {
        mdkPlayerAPI_delete(&player)
    }

    func setVideoDecoders(_ names : [String]) {
        //    public var setVideoDecoders: (@convention(c) (OpaquePointer?, UnsafeMutablePointer<UnsafePointer<Int8>?>?) -> Void)!
    }

    func setMedia(url : String) ->Void {
        player?.pointee.setMedia(player?.pointee.object, url)
    }

    func setState(state: MDK_State) ->Void {
        player?.pointee.setState(player?.pointee.object, state)
    }

    func setRendAPI(_ api :  OpaquePointer/*mdkMetalRenderAPI*/) ->Void {
        //let a = address(o: &api)
        player?.pointee.setRenderAPI(player?.pointee.object, api, nil)
    }

    func setRenderTarget(_ mkv : MTKView, commandQueue cmdQueue: MTLCommandQueue) ->Void {
        var ra = mdkMetalRenderAPI()//type: MDK_RenderAPI_Metal, device: &dev, cmdQueue: &cmdQueue, texture: nil, opaque: &mkv, currentRenderTarget: <#T##((UnsafeMutableRawPointer?) -> UnsafeRawPointer?)?##((UnsafeMutableRawPointer?) -> UnsafeRawPointer?)?##(UnsafeMutableRawPointer?) -> UnsafeRawPointer?#>);
        ra.type = MDK_RenderAPI_Metal
        ra.device = bridge(obj: mkv.device.unsafelyUnwrapped)
        ra.cmdQueue = bridge(obj: cmdQueue)
        ra.opaque = bridge(obj: mkv)

        ra.currentRenderTarget = currentRt
        setRendAPI(address(o: &ra))
    }

    func setLoop(_ value : Int32) -> Void {
        player?.pointee.setLoop(player?.pointee.object, value)
    }

    func setVideoSurfaceSize(_ width : CGFloat, _ height : CGFloat)->Void {
        player?.pointee.setVideoSurfaceSize(player?.pointee.object, Int32(width), Int32(height), nil)
    }

    func renderVideo() -> Double {
        return player!.pointee.renderVideo(player?.pointee.object, nil)
    }
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
        cmdQueue = dev?.makeCommandQueue()

        mkv = MTKView(frame:NSMakeRect(0, 0, 100, 100))
        //mkv.autoResizeDrawable = true
        mkv.device = dev
        super.init(coder: coder)
    }

    override func viewDidLoad() {
        super.viewDidLoad()
/*
        var ra = mdkMetalRenderAPI()//type: MDK_RenderAPI_Metal, device: &dev, cmdQueue: &cmdQueue, texture: nil, opaque: &mkv, currentRenderTarget: <#T##((UnsafeMutableRawPointer?) -> UnsafeRawPointer?)?##((UnsafeMutableRawPointer?) -> UnsafeRawPointer?)?##(UnsafeMutableRawPointer?) -> UnsafeRawPointer?#>);
        ra.type = MDK_RenderAPI_Metal
        ra.device = bridge(obj: dev)
        ra.cmdQueue = bridge(obj: cmdQueue)
        ra.opaque = bridge(obj: mkv)

        ra.currentRenderTarget = currentRt
        player.setRendAPI(address(o: &ra))
 */
        player.setRenderTarget(mkv, commandQueue: cmdQueue)
        mkv.delegate = self
        mkv.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(mkv)

        mkv.leftAnchor.constraint(equalTo: view.leftAnchor).isActive = true
        mkv.rightAnchor.constraint(equalTo: view.rightAnchor).isActive = true
        mkv.topAnchor.constraint(equalTo: view.topAnchor).isActive = true
        mkv.bottomAnchor.constraint(equalTo: view.bottomAnchor).isActive = true

        // Do any additional setup after loading the view.
        //player.setVideoDecoders({"VT", "FFmpeg"});
        player.setLoop(-1);
        player.setMedia(url: "https://devimages.apple.com.edgekey.net/streaming/examples/bipbop_4x3/gear1/prog_index.m3u8")
        player.setState(state: MDK_State_Playing)
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
        let cmdBuf = cmdQueue?.makeCommandBuffer()
        cmdBuf?.present(d)
        cmdBuf?.commit()
    }
}

