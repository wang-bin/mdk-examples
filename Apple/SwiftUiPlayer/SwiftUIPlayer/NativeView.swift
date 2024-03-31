//
//  ContentView.swift
//  Shared
//
//  Created by WangBin on 2020/11/16.
//
//

import SwiftUI

#if canImport(UIKit)
import UIKit
internal typealias NSViewRepresentable = UIViewRepresentable
internal typealias NSView = UIView
internal typealias NSViewRepresentableContext = UIViewRepresentableContext
#endif
#if canImport(AppKit)
import AppKit
#if os(macOS)
internal typealias UIViewRepresentable = NSViewRepresentable
internal typealias UIView = NSView
internal typealias UIViewRepresentableContext = NSViewRepresentableContext
#endif // os(macOS)
#endif

// https://forums.developer.apple.com/forums/thread/119112
// MARK: SwiftUI + NS/UIView
struct NativeView: UIViewRepresentable {
    //public typealias NSViewType = UIView
    
    var wrappedView: UIView
    
    private var handleUpdateUIView: ((UIView, Context) -> Void)?
    private var handleMakeUIView: ((Context) -> UIView)?
    
    init(closure: () -> UIView) {
        wrappedView = closure()
    }
    
    //func makeCoordinator() -> Coordinator {
    //    Coordinator(self)
    //}
    func makeUIView(context: Context) -> UIView {
        guard let handler = handleMakeUIView else {
            return wrappedView
        }
        
        return handler(context)
    }
    
    func updateUIView(_ uiView: UIView, context: Context) {
        handleUpdateUIView?(uiView, context)
    }
    
    
    func makeNSView(context: Context) -> UIView {
        guard let handler = handleMakeUIView else {
            return wrappedView
        }
        
        return handler(context)
    }
    
    func updateNSView(_ nsView: UIView, context: Context) {
        handleUpdateUIView?(nsView, context)
    }
}

extension NativeView {
    mutating func setMakeUIView(handler: @escaping (Context) -> UIView) -> Self {
        handleMakeUIView = handler
        
        return self
    }
    
    mutating func setUpdateUIView(handler: @escaping (UIView, Context) -> Void) -> Self {
        handleUpdateUIView = handler
        
        return self
    }
}

