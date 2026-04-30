/*
 * Copyright (c) 2024 WangBin <wbsecg1 at gmail.com>
 * MDK SDK Unity Plugin — Example Video Player Controller
 *
 * Demonstrates how to use MDKPlayerView for video playback in a Unity scene.
 *
 * Setup:
 *   1. Create a Quad (or Plane) in your scene.
 *   2. Attach this script to the Quad.
 *   3. Assign the MDKPlayerView component (on the same or a different GameObject).
 *   4. Set the videoUrl field in the Inspector.
 *   5. (Optional) assign UI elements (slider, buttons) in the Inspector.
 */
using System;
using UnityEngine;
using UnityEngine.UI;
using MDK;

[AddComponentMenu("MDK/Video Player Controller")]
public class VideoPlayerController : MonoBehaviour
{
    // ----------------------------------------------------------------
    // Inspector fields
    // ----------------------------------------------------------------

    [Header("Player")]
    [Tooltip("The MDKPlayerView component to control.")]
    public MDKPlayerView playerView;

    [Tooltip("Video URL or local file path.")]
    public string videoUrl = "https://www.w3schools.com/html/mov_bbb.mp4";

    [Header("UI (optional)")]
    public Slider  progressSlider;
    public Button  playPauseButton;
    public Button  stopButton;
    public Text    timeLabel;
    public Slider  volumeSlider;

    // ----------------------------------------------------------------
    // Private state
    // ----------------------------------------------------------------

    private bool _sliderDragging;

    // ----------------------------------------------------------------
    // Unity lifecycle
    // ----------------------------------------------------------------

    private void Start()
    {
        if (playerView == null)
            playerView = GetComponent<MDKPlayerView>();

        if (playerView == null)
        {
            Debug.LogError("[VideoPlayerController] No MDKPlayerView found.");
            return;
        }

        // Subscribe to player events
        playerView.OnStateChanged       += OnStateChanged;
        playerView.OnMediaStatusChanged += OnMediaStatusChanged;

        // Wire up UI
        if (playPauseButton != null)
            playPauseButton.onClick.AddListener(OnPlayPauseClicked);

        if (stopButton != null)
            stopButton.onClick.AddListener(OnStopClicked);

        if (progressSlider != null)
        {
            progressSlider.onValueChanged.AddListener(OnSliderValueChanged);
            // Detect when user starts/stops dragging
            var trigger = progressSlider.gameObject.AddComponent<UnityEngine.EventSystems.EventTrigger>();
            AddEventTrigger(trigger, UnityEngine.EventSystems.EventTriggerType.PointerDown,
                (_) => _sliderDragging = true);
            AddEventTrigger(trigger, UnityEngine.EventSystems.EventTriggerType.PointerUp,
                (_) => { _sliderDragging = false; SeekToSlider(); });
        }

        if (volumeSlider != null)
        {
            volumeSlider.value = 1f;
            volumeSlider.onValueChanged.AddListener(v => { if (playerView != null) playerView.Player.Volume = v; });
        }

        // Start playback
        if (!string.IsNullOrEmpty(videoUrl))
            playerView.Play(videoUrl);
    }

    private void Update()
    {
        if (playerView == null) return;

        // Update progress slider
        if (progressSlider != null && !_sliderDragging && playerView.Duration > 0)
            progressSlider.value = (float)playerView.Position / playerView.Duration;

        // Update time label
        if (timeLabel != null)
            timeLabel.text = $"{FormatTime(playerView.Position)} / {FormatTime(playerView.Duration)}";

        // Keyboard shortcuts
        if (Input.GetKeyDown(KeyCode.Space))
            playerView.Pause();
        if (Input.GetKeyDown(KeyCode.RightArrow))
            playerView.Seek(playerView.Position + 10000);
        if (Input.GetKeyDown(KeyCode.LeftArrow))
            playerView.Seek(playerView.Position - 10000);
    }

    private void OnDestroy()
    {
        if (playerView == null) return;
        playerView.OnStateChanged       -= OnStateChanged;
        playerView.OnMediaStatusChanged -= OnMediaStatusChanged;
    }

    // ----------------------------------------------------------------
    // Player event handlers
    // ----------------------------------------------------------------

    private void OnStateChanged(PlayerState state)
    {
        Debug.Log($"[VideoPlayerController] State changed: {state}");
        if (playPauseButton != null)
        {
            var label = playPauseButton.GetComponentInChildren<Text>();
            if (label != null)
                label.text = state == PlayerState.Playing ? "Pause" : "Play";
        }
    }

    private void OnMediaStatusChanged(MediaStatus status)
    {
        Debug.Log($"[VideoPlayerController] Media status: {status}");
        if ((status & MediaStatus.End) != 0)
            Debug.Log("[VideoPlayerController] Playback ended.");
        if ((status & MediaStatus.Invalid) != 0)
            Debug.LogError("[VideoPlayerController] Invalid media.");
    }

    // ----------------------------------------------------------------
    // UI event handlers
    // ----------------------------------------------------------------

    private void OnPlayPauseClicked()
    {
        playerView?.Pause();
    }

    private void OnStopClicked()
    {
        playerView?.Stop();
    }

    private void OnSliderValueChanged(float value)
    {
        // Only seek when the user is actively dragging
    }

    private void SeekToSlider()
    {
        if (playerView == null || playerView.Duration <= 0) return;
        long target = (long)(progressSlider.value * playerView.Duration);
        playerView.Seek(target);
    }

    // ----------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------

    private static string FormatTime(long ms)
    {
        var ts = TimeSpan.FromMilliseconds(ms);
        return ts.TotalHours >= 1
            ? $"{(int)ts.TotalHours:D2}:{ts.Minutes:D2}:{ts.Seconds:D2}"
            : $"{ts.Minutes:D2}:{ts.Seconds:D2}";
    }

    private static void AddEventTrigger(
        UnityEngine.EventSystems.EventTrigger trigger,
        UnityEngine.EventSystems.EventTriggerType type,
        UnityEngine.Events.UnityAction<UnityEngine.EventSystems.BaseEventData> action)
    {
        var entry = new UnityEngine.EventSystems.EventTrigger.Entry { eventID = type };
        entry.callback.AddListener(action);
        trigger.triggers.Add(entry);
    }
}
