/*
 * Copyright (c) 2024 WangBin <wbsecg1 at gmail.com>
 * MDK SDK OHOS example - main player page
 */
import playerNapi from 'libplayer_napi.so';
import picker from '@ohos.file.picker';
import fs from '@ohos.file.fs';
import promptAction from '@ohos.promptAction';

const XCOMPONENT_ID = 'mdkPlayer';
const SPEED_OPTIONS: number[] = [0.25, 0.5, 0.75, 1.0, 1.25, 1.5, 2.0];
const DEFAULT_SPEED_INDEX = 3; // 1.0x

@Entry
@Component
struct Index {
  @State isPlaying: boolean = false;
  @State progress: number = 0;       // 0–100
  @State currentTime: number = 0;    // ms
  @State duration: number = 0;       // ms
  @State volume: number = 100;       // 0–100
  @State speedIndex: number = DEFAULT_SPEED_INDEX;
  @State urlInput: string = '';

  private timerID: number = -1;
  private isSeeking: boolean = false;
  private openedFd: number = -1;

  aboutToAppear() {
    // Poll position every 500ms
    this.timerID = setInterval(() => {
      this.refreshProgress();
    }, 500);
  }

  aboutToDisappear() {
    if (this.timerID !== -1) {
      clearInterval(this.timerID);
      this.timerID = -1;
    }
    this.closeFd();
    playerNapi.stop();
  }

  private closeFd() {
    if (this.openedFd !== -1) {
      try {
        fs.closeSync(this.openedFd);
      } catch (_e) {
        // Best-effort close: fd may already be invalid if the process recycled it
      }
      this.openedFd = -1;
    }
  }

  private refreshProgress() {
    if (this.isSeeking) return;
    const pos: number = playerNapi.getPosition() as number;
    const dur: number = playerNapi.getDuration() as number;
    this.currentTime = pos;
    this.duration = dur;
    if (dur > 0) {
      this.progress = (pos / dur) * 100;
    }
    this.isPlaying = playerNapi.isPlaying() as boolean;
  }

  private formatTime(ms: number): string {
    const total = Math.floor(ms / 1000);
    const h = Math.floor(total / 3600);
    const m = Math.floor((total % 3600) / 60);
    const s = total % 60;
    const mm = m.toString().padStart(2, '0');
    const ss = s.toString().padStart(2, '0');
    return h > 0 ? `${h}:${mm}:${ss}` : `${mm}:${ss}`;
  }

  private openFile() {
    const docPicker = new picker.DocumentViewPicker();
    docPicker.select({ maxSelectNumber: 1 })
      .then((uris: Array<string>) => {
        if (!uris || uris.length === 0) return;
        const uri = uris[0];
        try {
          this.closeFd();
          const file = fs.openSync(uri, fs.OpenMode.READ_ONLY);
          this.openedFd = file.fd;
          const fdUrl = `fd://${file.fd}`;
          this.urlInput = fdUrl;
          this.startMedia(fdUrl);
        } catch (e) {
          promptAction.showToast({ message: `Cannot open file: ${(e as Error).message}` });
        }
      })
      .catch((err: Error) => {
        promptAction.showToast({ message: `File picker error: ${err.message}` });
      });
  }

  private startMedia(url: string) {
    if (!url || url.trim() === '') {
      promptAction.showToast({ message: 'Please enter a URL or pick a file' });
      return;
    }
    playerNapi.stop();
    playerNapi.setMedia(url.trim());
    playerNapi.play();
    this.isPlaying = true;
    this.progress = 0;
    this.currentTime = 0;
    this.duration = 0;
  }

  build() {
    Column() {
      // ── Video surface ──────────────────────────────────────────────
      XComponent({
        id: XCOMPONENT_ID,
        type: XComponentType.SURFACE,
        libraryname: 'player_napi'
      })
        .width('100%')
        .aspectRatio(16 / 9)
        .backgroundColor(Color.Black)

      // ── Controls ───────────────────────────────────────────────────
      Column({ space: 4 }) {
        // Progress bar row
        Row({ space: 4 }) {
          Text(this.formatTime(this.currentTime))
            .fontSize(12)
            .fontColor(Color.White)
            .width(48)
            .textAlign(TextAlign.Start)

          Slider({
            value: this.progress,
            min: 0,
            max: 100,
            step: 0.1,
            style: SliderStyle.OutSet
          })
            .layoutWeight(1)
            .selectedColor('#1698CE')
            .onChange((value: number, mode: SliderChangeMode) => {
              this.progress = value;
              if (mode === SliderChangeMode.Begin) {
                this.isSeeking = true;
              }
              if (mode === SliderChangeMode.End || mode === SliderChangeMode.Click) {
                if (this.duration > 0) {
                  playerNapi.seek(Math.floor((value / 100) * this.duration));
                }
                this.isSeeking = false;
              }
            })

          Text(this.formatTime(this.duration))
            .fontSize(12)
            .fontColor(Color.White)
            .width(48)
            .textAlign(TextAlign.End)
        }
        .width('100%')
        .padding({ left: 8, right: 8 })

        // Button row
        Row({ space: 8 }) {
          // Play / Pause
          Button({ type: ButtonType.Circle }) {
            Text(this.isPlaying ? '⏸' : '▶')
              .fontSize(22)
              .fontColor(Color.White)
          }
          .width(48)
          .height(48)
          .backgroundColor('#1698CE')
          .onClick(() => {
            if (this.isPlaying) {
              playerNapi.pause();
              this.isPlaying = false;
            } else {
              playerNapi.play();
              this.isPlaying = true;
            }
          })

          // Stop
          Button({ type: ButtonType.Circle }) {
            Text('⏹')
              .fontSize(22)
              .fontColor(Color.White)
          }
          .width(48)
          .height(48)
          .backgroundColor('#555')
          .onClick(() => {
            playerNapi.stop();
            this.isPlaying = false;
            this.progress = 0;
            this.currentTime = 0;
          })

          // Speed selector
          Select(SPEED_OPTIONS.map((s: number): SelectOption => ({ value: `${s}x` })))
            .selected(this.speedIndex)
            .value(`${SPEED_OPTIONS[this.speedIndex]}x`)
            .fontColor(Color.White)
            .selectedOptionFontColor('#1698CE')
            .optionBgColor('#333')
            .backgroundColor('#444')
            .onSelect((index: number) => {
              this.speedIndex = index;
              playerNapi.setPlaybackRate(SPEED_OPTIONS[index]);
            })

          Blank().layoutWeight(1)

          // Volume icon
          Text('🔊')
            .fontSize(18)
            .fontColor(Color.White)

          // Volume slider
          Slider({
            value: this.volume,
            min: 0,
            max: 100,
            step: 1,
            style: SliderStyle.InSet
          })
            .width(100)
            .selectedColor('#1698CE')
            .onChange((value: number) => {
              this.volume = value;
              playerNapi.setVolume(value / 100);
            })
        }
        .width('100%')
        .padding({ left: 8, right: 8 })

        // URL input + file picker + play button row
        Row({ space: 6 }) {
          TextInput({ placeholder: 'Enter URL (http / rtsp / rtmp …)', text: this.urlInput })
            .layoutWeight(1)
            .placeholderColor('#888')
            .fontColor(Color.White)
            .caretColor('#1698CE')
            .backgroundColor('#333')
            .onChange((value: string) => {
              this.urlInput = value;
            })
            .onSubmit(() => {
              this.startMedia(this.urlInput);
            })

          Button('File')
            .width(56)
            .height(40)
            .fontSize(14)
            .backgroundColor('#555')
            .onClick(() => { this.openFile(); })

          Button('Open')
            .width(56)
            .height(40)
            .fontSize(14)
            .backgroundColor('#1698CE')
            .onClick(() => { this.startMedia(this.urlInput); })
        }
        .width('100%')
        .padding({ left: 8, right: 8, bottom: 8 })
      }
      .width('100%')
      .backgroundColor('#222')
    }
    .width('100%')
    .height('100%')
    .backgroundColor(Color.Black)
  }
}
