package org.mdk.example

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp

@Composable
fun App() {
    var url by remember { mutableStateOf("") }
    val player = remember { MdkPlayer() }

    DisposableEffect(Unit) {
        onDispose { player.release() }
    }

    MaterialTheme {
        Column(
            modifier = Modifier.fillMaxSize().background(Color.Black),
        ) {
            // Native video surface
            VideoSurface(
                player = player,
                modifier = Modifier
                    .fillMaxWidth()
                    .weight(1f),
            )

            // Controls
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .background(Color(0xFF1E1E1E))
                    .padding(horizontal = 12.dp, vertical = 8.dp),
                verticalArrangement = Arrangement.spacedBy(8.dp),
            ) {
                // URL input row
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.spacedBy(8.dp),
                ) {
                    OutlinedTextField(
                        value = url,
                        onValueChange = { url = it },
                        modifier = Modifier.weight(1f),
                        placeholder = { Text("Enter URL (http / rtsp / rtmp / file …)") },
                        colors = OutlinedTextFieldDefaults.colors(
                            unfocusedTextColor = Color.White,
                            focusedTextColor = Color.White,
                            unfocusedPlaceholderColor = Color(0xFF888888),
                            focusedPlaceholderColor = Color(0xFF888888),
                        ),
                        singleLine = true,
                    )
                    Button(
                        onClick = {
                            val trimmed = url.trim()
                            if (trimmed.isNotEmpty()) {
                                player.setMedia(trimmed)
                                player.play()
                            }
                        },
                        colors = ButtonDefaults.buttonColors(
                            containerColor = Color(0xFF1698CE),
                        ),
                    ) {
                        Text("Open")
                    }
                }

                // Playback controls
                Row(
                    modifier = Modifier.fillMaxWidth().padding(bottom = 4.dp),
                    horizontalArrangement = Arrangement.spacedBy(8.dp),
                    verticalAlignment = Alignment.CenterVertically,
                ) {
                    PlaybackButton(label = "▶", onClick = { player.play() })
                    PlaybackButton(label = "⏸", onClick = { player.pause() })
                    PlaybackButton(label = "⏹", onClick = { player.stop() })
                }
            }
        }
    }
}

@Composable
private fun PlaybackButton(label: String, onClick: () -> Unit) {
    Button(
        onClick = onClick,
        colors = ButtonDefaults.buttonColors(containerColor = Color(0xFF444444)),
        contentPadding = PaddingValues(horizontal = 16.dp, vertical = 8.dp),
    ) {
        Text(text = label, color = Color.White)
    }
}
