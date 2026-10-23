/* Copyright (c) 2026 WangBin <wbsecg1 at gmail.com>
 * AI participated
 */
using UnityEngine;

namespace MDK.Unity
{
    [RequireComponent(typeof(Renderer))]
    public sealed class MDKMeshDisplay : MonoBehaviour
    {
        [SerializeField] private MDKPlayer player;
        [SerializeField] private string textureProperty = "_MainTex";
        [SerializeField] private bool flipY;
        [SerializeField] private bool autoFlipY = true;
        private Renderer targetRenderer;
        private MaterialPropertyBlock propertyBlock;
        private int textureId;
        private int textureTransformId;

        private void Awake()
        {
            targetRenderer = GetComponent<Renderer>();
            propertyBlock = new MaterialPropertyBlock();
            textureId = Shader.PropertyToID(textureProperty);
            textureTransformId = Shader.PropertyToID(textureProperty + "_ST");
        }
        private void OnEnable()
        {
            if (player != null) {
                player.TextureChanged += ApplyTexture;
                ApplyTexture(player.VideoTexture);
            }
        }
        private void OnDisable()
        {
            if (player != null)
                player.TextureChanged -= ApplyTexture;
            ApplyTexture(null);
        }
        private void ApplyTexture(RenderTexture texture)
        {
            targetRenderer.GetPropertyBlock(propertyBlock);
            // SetTexture rejects null, including during player/display destruction.
            propertyBlock.SetTexture(textureId, texture != null ? (Texture)texture : Texture2D.blackTexture);
            propertyBlock.SetFloat("_HDRPreview", texture != null && texture.format == RenderTextureFormat.ARGBHalf ? 1 : 0);
            var flipped = flipY ^ (autoFlipY && SystemInfo.graphicsUVStartsAtTop);
            propertyBlock.SetVector(textureTransformId, flipped ? new Vector4(1, -1, 0, 1) : new Vector4(1, 1, 0, 0));
            targetRenderer.SetPropertyBlock(propertyBlock);
        }
    }
}
