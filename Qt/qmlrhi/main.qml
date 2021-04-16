
import QtQuick 2.8 // GraphicsInfo
//! [1]
import MDKTextureItem 1.0
//! [1]

Rectangle {
    property string url

    //! [2]
    VideoTextureItem {
        id: renderer
        anchors.fill: parent
        anchors.margins: 10
        source: url
    }

    Rectangle {
        id: labelFrame
        anchors.margins: -10
        radius: 5
        color: "white"
        border.color: "black"
        opacity: 0.5
        anchors.fill: label
    }

    Text {
        id: label
        anchors.bottom: renderer.bottom
        anchors.left: renderer.left
        anchors.right: renderer.right
        anchors.margins: 20
        wrapMode: Text.WordWrap
        text: "mdk renders video in a " + gfxApi() + " texture. The texture is then imported in a QtQuick item."
        color: "red" // FIXME: video color is the same as text color in opengl rhi
    }

    function gfxApi() {
        switch (GraphicsInfo.api) {
        case GraphicsInfo.OpenGLRhi: return "OpenGL Rhi"
        case GraphicsInfo.MetalRhi: return "Metal Rhi"
        case GraphicsInfo.Direct3D11Rhi: return "D3D11 Rhi"
        case GraphicsInfo.VulkanRhi: return "Vulkan Rhi"
        case GraphicsInfo.OpenGL: return "OpenGL"
        case GraphicsInfo.Direct3D12: return "D3D12"
        }
    }
}
