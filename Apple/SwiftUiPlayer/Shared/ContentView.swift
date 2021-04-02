//
//  ContentView.swift
//  Shared
//
//  Created by WangBin on 2020/11/16.
//
//

import SwiftUI

struct ContentView: View {
    private let player = Player()
    var body: some View {
    
        ZStack(alignment: .topTrailing) {
            SwiftUIView {
                MTLVideoView(player: player)
            }
            Button("Stop") {
                player.state = .Stopped
            }.border(Color.red, width: 1)
        }
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}


