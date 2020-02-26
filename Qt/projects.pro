TEMPLATE = subdirs
CONFIG -= ordered
SUBDIRS = libqtmdk qtmultiwidgets qtpaintonvideo qmdkplay
qtHaveModule(quick): SUBDIRS += qmdkqmlplay

libqtmdk.file = libqtmdk.pro

qtmultiwidgets.file = qtmultiwidgets.pro
qtmultiwidgets.depends = libqtmdk

qtpaintonvideo.file = qtpaintonvideo.pro
qtpaintonvideo.depends = libqtmdk

qmdkplay.file = qmdkplay.pro

qmdkqmlplay.file = qmdkqmlplay.pro
