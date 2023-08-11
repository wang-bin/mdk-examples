import 'package:flutter/material.dart';
import 'package:fvp/mdk.dart';

void main(List<String> args) async {
  runApp(const SinglePlayerMultipleVideoWidget());
}
class SinglePlayerMultipleVideoWidget extends StatefulWidget {
  const SinglePlayerMultipleVideoWidget({Key? key}) : super(key: key);

  @override
  State<SinglePlayerMultipleVideoWidget> createState() =>
      _SinglePlayerMultipleVideoWidgetState();
}

class _SinglePlayerMultipleVideoWidgetState
    extends State<SinglePlayerMultipleVideoWidget> {
  late final player = Player();

  @override
  void initState() {
    super.initState();

    player.media = 'https://flutter.github.io/assets-for-api-docs/assets/videos/bee.mp4';
    player.loop = -1;
    player.state = PlaybackState.playing;
    player.updateTexture();
  }

  @override
  void dispose() {
    player.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'package:fvp',
      home: Scaffold(
        body: Center(
          child: Row(
                crossAxisAlignment: CrossAxisAlignment.stretch,
                children: [
                  Expanded(
                    flex: 3,
                    child: Column(
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        Expanded(
                          child: Card(
                            elevation: 8.0,
                            color: Colors.black,
                            clipBehavior: Clip.antiAlias,
                            margin: const EdgeInsets.all(32.0),
                            child: Column(
                              mainAxisSize: MainAxisSize.max,
                              children: [
                                Expanded(
                                  child: Row(
                                    children: [
                                      Expanded(
                                        child: ValueListenableBuilder<int?>(
                                          valueListenable: player.textureId,
                                          builder: (context, id, _) => id == null
                                            ? const SizedBox.shrink()
                                            : Texture(textureId: id),
                                        ),
                                      ),
                                      Expanded(
                                        child:  ValueListenableBuilder<int?>(
                                          valueListenable: player.textureId,
                                          builder: (context, id, _) => id == null
                                            ? const SizedBox.shrink()
                                            : Texture(textureId: id),
                                        ),
                                      ),
                                    ],
                                  ),
                                ),
                                Expanded(
                                  child: Row(
                                    children: [
                                      Expanded(
                                        child: ValueListenableBuilder<int?>(
                                          valueListenable: player.textureId,
                                          builder: (context, id, _) => id == null
                                            ? const SizedBox.shrink()
                                            : Texture(textureId: id),
                                        ),
                                      ),
                                      Expanded(
                                        child:  ValueListenableBuilder<int?>(
                                          valueListenable: player.textureId,
                                          builder: (context, id, _) => id == null
                                            ? const SizedBox.shrink()
                                            : Texture(textureId: id),
                                        ),
                                      ),
                                    ],
                                  ),
                                ),
                              ],
                            ),
                          ),
                        ),
                        const SizedBox(height: 32.0),
                      ],
                    ),
                  ),
                ],
              ),
        ),
      ),
    );
  }
}
