//
//  ContentView.swift
//  UVideo
//
//  Created by WangBin on 2024/3/24.
//

import SwiftUI

import MetalKit

struct ContentView: View {
    var body: some View {
        VStack {
            Image(systemName: "globe")
                .imageScale(.large)
                //.foregroundStyle(.tint)
            Text("Hello, world!")
            NativeView {
                #if os(visionOS)
                VideoView(url: URL(string: "https://devimages.apple.com.edgekey.net/streaming/examples/bipbop_4x3/gear1/prog_index.m3u8"))
                #else
                MTKVideoView(url: URL(string: "https://devimages.apple.com.edgekey.net/streaming/examples/bipbop_4x3/gear1/prog_index.m3u8"))
                #endif
            }
        }
        .padding()
    }
}

#Preview {
    ContentView()
}
