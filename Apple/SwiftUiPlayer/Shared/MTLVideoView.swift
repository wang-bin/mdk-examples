//
//  MTLVideoView.swift
//  SwiftApp
//
//  Created by WangBin on 2021/3/15.
//

import MetalKit

// MARK: Metal Stuff

class MTLVideoView: MTKView {
    private var cmdQueue : MTLCommandQueue!
    private var player : Player!

    init(player : Player) {
        super.init(frame: .zero, device: MTLCreateSystemDefaultDevice())
        // Make sure we are on a device that can run metal!
        guard let defaultDevice = device else {
            fatalError("Device loading error")
        }
        self.player = player
        createRenderer(device: defaultDevice)
    }

    required init(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    func createRenderer(device: MTLDevice){
        logLevel = .All

        setLogHandler({ level, msg in
            print("mdk \(level): \(msg)")
        })

        cmdQueue = device.makeCommandQueue()
        delegate = self
        player.addRenderTarget(self, commandQueue: cmdQueue)

        enableSetNeedsDisplay = true
        if enableSetNeedsDisplay {
            player.setRenderCallback { [weak self] in
                DispatchQueue.main.async {
                    self?.setNeedsDisplay(.zero)
                }
            }
        }

        player.videoDecoders = ["VT:copy=0", "FFmpeg"]

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

        player.media = "https://devimages.apple.com.edgekey.net/streaming/examples/bipbop_4x3/gear1/prog_index.m3u8"
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

}

extension MTLVideoView: MTKViewDelegate {
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
