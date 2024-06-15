TEMPLATE = subdirs
CONFIG -= ordered
SUBDIRS = libqtmdk qtmultiwidgets qtmultiplayers qtpaintonvideo qmdkplay graphicsview
qtConfig(vulkan): SUBDIRS += vkwindow
qtHaveModule(quick): {
    SUBDIRS += qmdkqmlplay
}

greaterThan(QT_MAJOR_VERSION, 5)|greaterThan(QT_MINOR_VERSION, 9): {
  qtHaveModule(quick): SUBDIRS += qmlrhi qmlrhi0
  SUBDIRS += rhiwindow
}
greaterThan(QT_MAJOR_VERSION, 5){
  greaterThan(QT_MAJOR_VERSION, 6)|greaterThan(QT_MINOR_VERSION, 6): SUBDIRS += rhiwidget
}

libqtmdk.file = libqtmdk.pro

qtmultiwidgets.file = qtmultiwidgets.pro
qtmultiwidgets.depends = libqtmdk

qtmultiplayers.file = qtmultiplayers.pro
qtmultiplayers.depends = libqtmdk

qtpaintonvideo.file = qtpaintonvideo.pro
qtpaintonvideo.depends = libqtmdk

qmdkplay.file = qmdkplay.pro

qmdkqmlplay.file = qmdkqmlplay.pro
