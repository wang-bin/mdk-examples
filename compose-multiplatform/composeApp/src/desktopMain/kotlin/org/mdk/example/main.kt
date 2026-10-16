package org.mdk.example

import androidx.compose.ui.unit.dp
import androidx.compose.ui.window.Window
import androidx.compose.ui.window.application
import androidx.compose.ui.window.rememberWindowState

fun main() = application {
    Window(
        onCloseRequest = ::exitApplication,
        title = "MDK Compose Multiplatform",
        state = rememberWindowState(width = 960.dp, height = 600.dp),
    ) {
        App()
    }
}
