//
//  VideoView.swift
//  UVideo
//
//  Created by WangBin on 2024/3/25.
//

import Metal
import swift_mdk
#if canImport(UIKit)
import UIKit
internal typealias VideoViewType = UIView
#endif
#if canImport(AppKit)
import AppKit
#if os(macOS)
internal typealias VideoViewType = NSView
#endif // os(macOS)
#endif


class VideoView : VideoViewType {
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
        super.init(frame: .zero)
        
        setLogHandler({ level, msg in
            print("mdk \(level): \(msg)")
        })
        setGlobalOption(name: "log", value: LogLevel.All)
        player.videoDecoders = ["VT:copy=0", "VideoToolbox", "BRAW:gpu=auto", "R3D", "hap", "FFmpeg", "dav1d"]
        player.updateNativeSurface(self, width: 0, height: 0);
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
}
