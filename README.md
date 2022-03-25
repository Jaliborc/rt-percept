# RT-Percept Renderer
Modified build of NVidia [Falcor 4.3](https://github.com/NVIDIAGameWorks/Falcor/tree/4.3) that can be used to generate new datasets or run metric prediction and VRS in real time (example trained networks already included). Example scenes [available separately](https://researchdata.tuwien.ac.at/records/py0ks-zzv95).

## Prerequisites
- `Windows 10 version 2004 (May 2020 Update)` or newer
- `Visual Studio 2019`
- `Windows 10 SDK (10.0.19041.0)` for Windows 10, version 2004
- `CUDA 11.1`, `CUDNN 8.1.1` and `TensorRT 7.2.3` for network usage

Refer to Falcor documentation for installation and usage instructions.

## Mainline Differences
Features minor modifications to the Falcor core and multiple new rendering passes:
- `AdaptiveVRS`
- `CapturePass`
- `DeferredMultiresPass`
- `DeferredPass`
- `FakeInput`
- `PredictMetric`
- `RemoteRenderPass`
- `SceneWritePass`
- `TemporalReproject`

Finally, contains two additional python scripts:
- `Mogwai\Data\RTPerceptGraphs`: implements the rendering graphs used in our demos (to be imported by other scripts)
- `BlenderGenerateViewpoints`: independent from Falcor, run inside blender to obtain UI for automatic viewpoint selection in a scene

## Citation
If you use this code for your research, please cite our paper:

```bibtex
TBD
```
