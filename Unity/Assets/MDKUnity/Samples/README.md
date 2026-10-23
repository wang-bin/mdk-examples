# Sample scene

`MDKPlayer.unity` contains a camera, a Canvas/RawImage display and an `MDKPlayer` component. Assign a file or URL to `Media Url` before entering Play Mode. `MDKPlayerControls` exposes `Open`, `Play`, `Pause` and `Stop` for wiring to Unity UI buttons.

In the Built-in pipeline, `MDKPlayer` automatically queues its render event on the tagged `Main Camera`; assign `Render Camera` only when a different camera should own the video command buffer.

`MDKMeshPlayer.unity` demonstrates the `MDKMeshDisplay` and
`MaterialPropertyBlock` path for a regular MeshRenderer. Its sample material is unlit, so the mesh does not require a scene light.
