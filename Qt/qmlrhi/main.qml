
import QtQuick 2.8 // GraphicsInfo
//! [1]
import MDKTextureItem 1.0
//! [1]
import QtQuick.Dialogs
import QtQuick.Controls 2.0

Item {
    property string url

    //! [2]
    /*Repeater{
            id:rep
            model:1*/
    VideoTextureItem {
        id: renderer
        anchors.fill: parent
        anchors.margins: 10
        source: url
        autoHDR: true
    }
    /*}
    Rectangle{
        width: parent.width
        height: 60
        anchors.bottom: parent.bottom
        color: "green"
        opacity: 0.8
        Text {
            text: qsTr("重新加载")
            anchors.centerIn: parent
            font.bold: true
            color: "white"
        }
        MouseArea{
            anchors.fill: parent
            onClicked: {
                rep.model=0
                rep.model=1
            }
        }
    }*/

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

    Button {
        text: "Open"
        anchors.top: parent.top
        anchors.right: parent.right
        width: 100
        height: 80

        onClicked: {
            fileDalog.visible = true;
        }
    }

    FileDialog {
        id: fileDalog
        title: "Choose a file"
        onAccepted: {
            renderer.source = selectedFiles[0]
        }
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
