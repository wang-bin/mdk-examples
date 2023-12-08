using System;
using System.Runtime.InteropServices;
using Avalonia;
using Avalonia.OpenGL;
using Avalonia.OpenGL.Controls;
using Avalonia.Threading;
using MDK.SDK.NET;

namespace Example;

internal class Mdkgl : OpenGlControlBase
{
    private GLRenderAPI.GetProcAddressCallback? _getProcAddressCallback;
    private PixelSize _pixelSize;
    private readonly MDKPlayer _player = new();
    private GLRenderAPI _ra = new();
    private string _url = "";

    public Mdkgl()
    {
        // the options is only for Windows，if you want to use it on other platforms, please refer to the following url
        // https://github.com/wang-bin/mdk-sdk/wiki/Player-APIs#void-setdecodersmediatype-type-const-stdvectorstdstring-names
        _player.SetAudioBackends(["OpenAL", "XAudio2"]);
        _player.SetDecoders(MediaType.Video, ["MFT:d3d=11", "hap", "D3D11", "DXVA", "FFmpeg"]);
        _player.SetRenderCallback(_ =>
        {
            Dispatcher.UIThread.InvokeAsync(RequestNextFrameRendering, DispatcherPriority.Background);
        });
    }

    public State MdkState
    {
        get => (State) (_player.State ?? PlaybackState.NotRunning);
        set => _player.Set(value);
    }

    public string MediaPath
    {
        get => _url;
        set
        {
            _url = value;
            _player.SetMedia(_url);
        }
    }

    protected override void OnOpenGlRender(GlInterface gl, int fb)
    {
        if (_ra.Fbo != fb)
        {
            _ra.Fbo = fb;
            _getProcAddressCallback = (n, o) => { return gl.GetProcAddress(n); };
            _ra.GetProcAddress = Marshal.GetFunctionPointerForDelegate(_getProcAddressCallback);
            _player.SetRenderAPI(_ra.GetPtr());
        }

        var pixelSize = GetPixelSize();
        if (pixelSize != _pixelSize)
        {
            _pixelSize = pixelSize;
            _player.SetVideoSurfaceSize(pixelSize.Width, pixelSize.Height);
        }

        _player.RenderVideo();
    }

    private PixelSize GetPixelSize()
    {
        var scaling = VisualRoot.RenderScaling;
        return new PixelSize(Math.Max(1, (int) (Bounds.Width * scaling)),
            Math.Max(1, (int) (Bounds.Height * scaling)));
    }

    protected override void OnOpenGlDeinit(GlInterface gl)
    {
        _player.Set(State.Stopped);
        _player.SetVideoSurfaceSize(-1, -1);
        _player.Dispose();
        base.OnOpenGlDeinit(gl);
    }
}