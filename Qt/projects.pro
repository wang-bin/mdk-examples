TEMPLATE = subdirs
CONFIG -= ordered
SUBDIRS = libqtmdk qtmultiwidgets qtmultiplayers qtpaintonvideo qmdkplay
qtConfig(vulkan): SUBDIRS += vkwindow
qtHaveModule(quick): SUBDIRS += qmdkqmlplay

libqtmdk.file = libqtmdk.pro

qtmultiwidgets.file = qtmultiwidgets.pro
qtmultiwidgets.depends = libqtmdk

qtmultiplayers.file = qtmultiplayers.pro
qtmultiplayers.depends = libqtmdk

qtpaintonvideo.file = qtpaintonvideo.pro
qtpaintonvideo.depends = libqtmdk

qmdkplay.file = qmdkplay.pro

qmdkqmlplay.file = qmdkqmlplay.pro
