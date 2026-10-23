/* Copyright (c) 2026 WangBin <wbsecg1 at gmail.com>
 * AI participated
 */
using UnityEngine;

public sealed class MDKHDROutputStatus : MonoBehaviour
{
    public string requestedMode;
    public MDK.Unity.MDKPlayer player;
    private string status;
    private float nextPoll;
    private float exitAfter;
    private void Awake()
    {
        float.TryParse(System.Environment.GetEnvironmentVariable("MDK_HDR_EXIT_AFTER_SECONDS"),
            System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture, out exitAfter);
    }
    private void Update()
    {
        if (Time.unscaledTime < nextPoll) return;
        nextPoll = Time.unscaledTime + 1;
        var hdr = HDROutputSettings.main;
        var current = $"Requested={requestedMode} HDR available={hdr.available} active={hdr.active} format={hdr.graphicsFormat} gamut={hdr.displayColorGamut} paperWhite={hdr.paperWhiteNits} max={hdr.maxToneMapLuminance}nit";
        if (!hdr.active) current += " [SDR FALLBACK: NOT HDR DISPLAY VALIDATED]";
        if (current != status) { status = current; Debug.Log("MDK_HDR_OUTPUT " + status); }
        if (exitAfter > 0 && player != null) {
            Debug.Log($"MDK_HDR_VIDEO position={player.Position} format={player.VideoTexture?.graphicsFormat} applied={player.AppliedTextureVersion}/{player.TargetVersion} error={player.ErrorMessage}");
            if (player.Position > 1000 && player.VideoTexture != null) {
                var previous = RenderTexture.active;
                var sample = new Texture2D(1, 1, TextureFormat.RGBAFloat, false, true);
                var rt = player.VideoTexture;
                RenderTexture.active = rt;
                sample.ReadPixels(new Rect(rt.width * 0.9f, rt.height * 0.5f, 1, 1), 0, 0);
                sample.Apply();
                RenderTexture.active = previous;
                Debug.Log($"MDK_HDR_VIDEO highlight={sample.GetPixel(0, 0):R}");
                Destroy(sample);
            }
        }
        if (exitAfter > 0 && Time.unscaledTime >= exitAfter)
            Application.Quit();
    }
    private void OnGUI() { GUI.Label(new Rect(10, 10, Screen.width - 20, 60), status); }
}
