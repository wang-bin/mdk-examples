TEMPLATE = subdirs
CONFIG -= ordered
SUBDIRS = libqtmdk qtmultiwidgets qmdkplay
libqtmdk.file = libqtmdk.pro
qtmultiwidgets.file = qtmultiwidgets.pro
qtmultiwidgets.depends = libqtmdk
qmdkplay.file = qmdkplay.pro