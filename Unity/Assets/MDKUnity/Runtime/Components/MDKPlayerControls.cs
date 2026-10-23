/* Copyright (c) 2026 WangBin <wbsecg1 at gmail.com>
 * AI participated
 */
using UnityEngine;
using UnityEngine.UI;

namespace MDK.Unity
{
    public sealed class MDKPlayerControls : MonoBehaviour
    {
        [SerializeField] private MDKPlayer player;
        [SerializeField] private InputField urlField;
        [SerializeField] private Text statusText;

        public void Open() => player.Open(urlField != null ? urlField.text : player.MediaUrl);
        public void Play() => player.Play();
        public void Pause() => player.Pause();
        public void Stop() => player.Stop();

        private void OnEnable()
        {
            if (player != null)
                player.InfoChanged += UpdateStatus;
        }
        private void OnDisable()
        {
            if (player != null)
                player.InfoChanged -= UpdateStatus;
        }
        private void UpdateStatus(MDKNative.PlayerInfo info)
        {
            if (statusText != null)
                statusText.text = $"{info.width}x{info.height}  {info.position / 1000.0:0.0}s  {info.state}";
        }
    }
}
