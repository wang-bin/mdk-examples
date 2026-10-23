/* Copyright (c) 2026 WangBin <wbsecg1 at gmail.com>
 * AI participated
 */
using UnityEngine;
using UnityEngine.UI;

namespace MDK.Unity
{
    [RequireComponent(typeof(RawImage))]
    public sealed class MDKRawImageDisplay : MonoBehaviour
    {
        [SerializeField] private MDKPlayer player;
        [SerializeField] private bool flipY;
        [SerializeField] private bool autoFlipY = true;
        [SerializeField] private bool applyRotation = true;
        [SerializeField] private bool preserveAspect = true;
        private RawImage image;
        private AspectRatioFitter aspectFitter;
        private Material previewMaterial;
        private Material originalMaterial;

        private void Awake()
        {
            image = GetComponent<RawImage>();
            originalMaterial = image.material;
            if (preserveAspect) {
                aspectFitter = GetComponent<AspectRatioFitter>();
                if (aspectFitter == null)
                    aspectFitter = gameObject.AddComponent<AspectRatioFitter>();
                aspectFitter.aspectMode = AspectRatioFitter.AspectMode.FitInParent;
            }
        }
        private void OnEnable()
        {
            if (player != null) {
                player.TextureChanged += ApplyTexture;
                player.InfoChanged += ApplyInfo;
                ApplyTexture(player.VideoTexture);
                ApplyInfo(default);
            }
        }
        private void OnDisable()
        {
            if (player != null) {
                player.TextureChanged -= ApplyTexture;
                player.InfoChanged -= ApplyInfo;
            }
            image.texture = null;
            image.material = originalMaterial;
        }
        private void OnDestroy()
        {
            if (previewMaterial != null)
                Destroy(previewMaterial);
        }
        private void ApplyTexture(RenderTexture texture)
        {
            image.texture = texture;
            if (texture != null && texture.format == RenderTextureFormat.ARGBHalf) {
                if (previewMaterial == null)
                    previewMaterial = new Material(Resources.Load<Shader>("MDKUIHDRPreview"));
                image.material = previewMaterial;
            } else {
                image.material = originalMaterial;
            }
            var flipped = flipY ^ (autoFlipY && SystemInfo.graphicsUVStartsAtTop);
            image.uvRect = flipped ? new Rect(0, 1, 1, -1) : new Rect(0, 0, 1, 1);
        }

        private void ApplyInfo(MDKNative.PlayerInfo info)
        {
            if (image == null)
                return;
            if (applyRotation)
                image.rectTransform.localRotation = Quaternion.Euler(0, 0, -(((info.rotation % 360) + 360) % 360));
            if (aspectFitter == null || info.width <= 0 || info.height <= 0)
                return;
            var width = info.width;
            var height = info.height;
            var rotation = ((info.rotation % 360) + 360) % 360;
            if (rotation == 90 || rotation == 270) {
                var swap = width;
                width = height;
                height = swap;
            }
            aspectFitter.aspectRatio = width * Mathf.Max(0.0001f, info.pixelAspect) / height;
        }
    }
}
