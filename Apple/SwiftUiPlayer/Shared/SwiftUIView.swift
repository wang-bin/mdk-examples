//
//  SwiftUIView.swift
//  SwiftUiPlayer
//
//  Created by WangBin on 2021/3/16.
//


import SwiftUI

#if os(macOS)
typealias UIViewRepresentable = NSViewRepresentable
typealias UIView = NSView
#endif

struct SwiftUIView: UIViewRepresentable {

    public typealias NSViewType = UIView

    var wrappedView: UIView

    private var handleUpdateUIView: ((UIView, Context) -> Void)?
    private var handleMakeUIView: ((Context) -> UIView)?

    init(closure: () -> UIView) {
        wrappedView = closure()
    }

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

extension SwiftUIView {
    mutating func setMakeUIView(handler: @escaping (Context) -> UIView) -> Self {
        handleMakeUIView = handler

        return self
    }

    mutating func setUpdateUIView(handler: @escaping (UIView, Context) -> Void) -> Self {
        handleUpdateUIView = handler

        return self
    }
}
