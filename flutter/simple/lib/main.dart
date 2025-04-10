// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file is used to extract code samples for the README.md file.
// Run update-excerpts if you modify this file.

// ignore_for_file: library_private_types_in_public_api, public_member_api_docs

// #docregion basic-example
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:video_player/video_player.dart';
import 'package:fvp/fvp.dart';
import 'package:logging/logging.dart';
import 'package:intl/intl.dart';
import 'package:file_selector/file_selector.dart';
import 'package:image/image.dart' as img;

String? source;

void main(List<String> args) async {
// set logger before registerWith()
  Logger.root.level = Level.ALL;
  final df = DateFormat("HH:mm:ss.SSS");
  Logger.root.onRecord.listen((record) {
    debugPrint(
        '${record.loggerName}.${record.level.name}: ${df.format(record.time)}: ${record.message}',
        wrapWidth: 0x7FFFFFFFFFFFFFFF);
  });

  final opts = <String, Object>{};
  final globalOpts = <String, Object>{};
  int i = 0;
  bool useFvp = true;
  opts['subtitleFontFile'] =
      'https://github.com/mpv-android/mpv-android/raw/master/app/src/main/assets/subfont.ttf';
  for (; i < args.length; i++) {
    if (args[i] == '-c:v') {
      opts['video.decoders'] = [args[++i]];
    } else if (args[i] == '-maxSize') {
      // ${w}x${h}
      final size = args[++i].split('x');
      opts['maxWidth'] = int.parse(size[0]);
      opts['maxHeight'] = int.parse(size[1]);
    } else if (args[i] == '-fvp') {
      useFvp = int.parse(args[++i]) > 0;
    } else if (args[i].startsWith('-')) {
      globalOpts[args[i].substring(1)] = args[++i];
    } else {
      break;
    }
  }
  if (globalOpts.isNotEmpty) {
    opts['global'] = globalOpts;
  }
  opts['lowLatency'] = 0;

  if (i <= args.length - 1) source = args[args.length - 1];
  source ??= await getStartFile();

  if (useFvp) {
    registerWith(options: opts);
  }

  runApp(const MaterialApp(
      localizationsDelegates: [DefaultMaterialLocalizations.delegate],
      title: 'Video Demo',
      home: VideoApp()));
}

Future<String?> getStartFile() async {
  if (Platform.isMacOS) {
    WidgetsFlutterBinding.ensureInitialized();
    const hostApi = MethodChannel("openFile");
    return await hostApi.invokeMethod("getCurrentFile");
  }
  return null;
}

Future<String?> showURIPicker(BuildContext context) async {
  final key = GlobalKey<FormState>();
  final src = TextEditingController();
  String? uri;
  await showModalBottomSheet(
    context: context,
    builder: (context) => Container(
      alignment: Alignment.center,
      child: Form(
        key: key,
        child: Padding(
          padding: const EdgeInsets.all(16.0),
          child: Column(
            mainAxisSize: MainAxisSize.max,
            mainAxisAlignment: MainAxisAlignment.start,
            crossAxisAlignment: CrossAxisAlignment.center,
            children: [
              TextFormField(
                controller: src,
                style: const TextStyle(fontSize: 14.0),
                decoration: const InputDecoration(
                  border: UnderlineInputBorder(),
                  labelText: 'Video URI',
                ),
                validator: (value) {
                  if (value == null || value.isEmpty) {
                    return 'Please enter a URI';
                  }
                  return null;
                },
              ),
              Padding(
                padding: const EdgeInsets.symmetric(vertical: 16.0),
                child: ElevatedButton(
                  onPressed: () {
                    if (key.currentState!.validate()) {
                      uri = src.text;
                      Navigator.of(context).maybePop();
                    }
                  },
                  child: const Text('Play'),
                ),
              ),
            ],
          ),
        ),
      ),
    ),
  );
  return uri;
}

/// Stateful widget to fetch and then display video content.
class VideoApp extends StatefulWidget {
  const VideoApp({super.key});

  @override
  _VideoAppState createState() => _VideoAppState();
}

class _VideoAppState extends State<VideoApp> {
  late VideoPlayerController _controller;

  @override
  void initState() {
    super.initState();
    if (source != null) {
      if (File(source!).existsSync()) {
        playFile(source!);
      } else {
        playUri(source!);
      }
    } else {
      playUri(
          'https://flutter.github.io/assets-for-api-docs/assets/videos/bee.mp4');
    }
  }

  void playFile(String path) {
    _controller.dispose();
    _controller = VideoPlayerController.file(File(path));
    _controller.addListener(() {
      setState(() {});
    });
    _controller.initialize().then((_) {
      // Ensure the first frame is shown after the video is initialized, even before the play button has been pressed.
      setState(() {});
      _controller.play();
    });
  }

  void playUri(String uri) {
    _controller = VideoPlayerController.network(uri);
    _controller.addListener(() {
      setState(() {});
    });
    _controller.initialize().then((_) {
      // Ensure the first frame is shown after the video is initialized, even before the play button has been pressed.
      setState(() {});
      _controller.play();
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      floatingActionButton: Row(
        children: [
          FloatingActionButton(
            heroTag: 'file',
            tooltip: 'Open [File]',
            onPressed: () async {
              const XTypeGroup typeGroup = XTypeGroup(
                label: 'videos',
                //extensions: <String>['*'],
              );
              final XFile? file =
                  await openFile(acceptedTypeGroups: <XTypeGroup>[typeGroup]);

              if (file != null) {
                playFile(file.path);
              }
            },
            child: const Icon(Icons.file_open),
          ),
          const SizedBox(width: 16.0),
          FloatingActionButton(
            heroTag: 'uri',
            tooltip: 'Open [Uri]',
            onPressed: () {
              showURIPicker(context).then((value) {
                if (value != null) {
                  playUri(value);
                }
              });
            },
            child: const Icon(Icons.link),
          ),
          const SizedBox(width: 16.0),
          FloatingActionButton(
            heroTag: 'snapshot',
            tooltip: 'Snapshot',
            onPressed: () {
              if (_controller.value.isInitialized) {
                final info = _controller.getMediaInfo()?.video?[0].codec;
                if (info == null) {
                  debugPrint('No video codec info');
                  return;
                }
                final width = info.width;
                final height = info.height;
                _controller.snapshot().then((value) {
                  if (value != null) {
                    // value is rgba data, must encode to png image and save as a file
                    final i = img.Image.fromBytes(
                        width: width,
                        height: height,
                        bytes: value.buffer,
                        numChannels: 4,
                        rowStride: width * 4);
                    final savePath =
                        '${Directory.systemTemp.path}/snapshot.jpg';
                    img.encodeJpgFile(savePath, i, quality: 70).then((value) {
                      final msg = value
                          ? 'Snapshot saved to $savePath'
                          : 'Failed to save snapshot';
                      debugPrint(msg);
                      // show a toast
                      if (context.mounted) {
                        ScaffoldMessenger.of(context).showSnackBar(
                          SnackBar(
                            content: Text(msg),
                            duration: const Duration(seconds: 2),
                          ),
                        );
                      }
                    });
                  }
                });
              }
            },
            child: const Icon(Icons.screenshot),
          ),
        ],
      ),
      body: Center(
        child: _controller.value.isInitialized
            ? AspectRatio(
                aspectRatio: _controller.value.aspectRatio,
                child: Stack(
                  alignment: Alignment.bottomCenter,
                  children: <Widget>[
                    VideoPlayer(_controller),
                    _ControlsOverlay(controller: _controller),
                    VideoProgressIndicator(_controller, allowScrubbing: true),
                  ],
                ),
              )
            : Container(),
      ),
    );
  }

  @override
  void dispose() {
    super.dispose();
    _controller.dispose();
  }
}

class _ControlsOverlay extends StatelessWidget {
  const _ControlsOverlay({required this.controller});

  static const List<double> _examplePlaybackRates = <double>[
    0.25,
    0.5,
    1.0,
    1.5,
    2.0,
    3.0,
    5.0,
    10.0,
  ];

  final VideoPlayerController controller;

  @override
  Widget build(BuildContext context) {
    return Stack(
      children: <Widget>[
        AnimatedSwitcher(
          duration: const Duration(milliseconds: 50),
          reverseDuration: const Duration(milliseconds: 200),
          child: controller.value.isPlaying
              ? const SizedBox.shrink()
              : Container(
                  color: Colors.black26,
                  child: const Center(
                    child: Icon(
                      Icons.play_arrow,
                      color: Colors.white,
                      size: 100.0,
                      semanticLabel: 'Play',
                    ),
                  ),
                ),
        ),
        GestureDetector(
          onTap: () {
            controller.value.isPlaying ? controller.pause() : controller.play();
          },
        ),
        Align(
          alignment: Alignment.topRight,
          child: PopupMenuButton<double>(
            initialValue: controller.value.playbackSpeed,
            tooltip: 'Playback speed',
            onSelected: (double speed) {
              controller.setPlaybackSpeed(speed);
            },
            itemBuilder: (BuildContext context) {
              return <PopupMenuItem<double>>[
                for (final double speed in _examplePlaybackRates)
                  PopupMenuItem<double>(
                    value: speed,
                    child: Text('${speed}x'),
                  )
              ];
            },
            child: Padding(
              padding: const EdgeInsets.symmetric(
                // Using less vertical padding as the text is also longer
                // horizontally, so it feels like it would need more spacing
                // horizontally (matching the aspect ratio of the video).
                vertical: 12,
                horizontal: 16,
              ),
              child: Text('${controller.value.playbackSpeed}x'),
            ),
          ),
        ),
      ],
    );
  }
}

// #enddocregion basic-example
