# RT Percept
<div align="center">
<h4><a href="https://jaliborc.github.io/rt-percept/">Website</a> • <a href="https://jaliborc.github.io/rt-percept/paper.pdf">Paper</a> • <a href="">Video (soon)</a> • <a href="">Dataset</a> • <a href="https://github.com/Jaliborc/rt-percept/tree/renderer">Renderer</a> • <a href="">Scenes</a></h4>
</div>

![teaser](https://github.com/Jaliborc/rt-percept/blob/website/images/teaser.png?raw=true)

**Training and Predicting Visual Error for Real-Time Applications**  
[Joao Liborio Cardoso](https://www.jaliborc.com), [Bernhard Kerbl](https://scholar.google.com/citations?user=jeasMB0AAAAJ), [Lei Yang](https://www.leiy.cc/), [Yury Uralsky](), [Michael Wimmer](https://www.cg.tuwien.ac.at/staff/MichaelWimmer)  
In Proceedings of the ACM in Computer Graphics and Interactive Techniques

*Visual error metrics play a fundamental role in the quantification of perceived image similarity. Most recently, use cases for them in real-time applications have emerged, such as content-adaptive shading and shading reuse to increase performance and improve efficiency. A wide range of different metrics has been established, with the most sophisticated being capable of capturing the perceptual characteristics of the human visual system. However, their complexity, computational expense, and reliance on reference images to compare against prevent their generalized use in real-time, restricting such applications to using only the simplest available metrics.*

*In this work, we explore the abilities of convolutional neural networks to predict a variety of visual metrics without requiring either reference or rendered images. Specifically, we train and deploy a neural network to estimate the visual error resulting from reusing shading or using reduced shading rates. The resulting models account for 70%--90% of the variance while achieving up to an order of magnitude faster computation times. Our solution combines image-space information that is readily available in most state-of-the-art deferred shading pipelines with reprojection from previous frames to enable an adequate estimate of visual errors, even in previously unseen regions. We describe a suitable convolutional network architecture and considerations for data preparation for training. We demonstrate the capability of our network to predict complex error metrics at interactive rates in a real-time application that implements content-adaptive shading in a deferred pipeline. Depending on the portion of unseen image regions, our approach can achieve up to 2x performance compared to state-of-the-art methods.*

## Installation
Code was tested using [Python 3.9](https://www.python.org/downloads/) and [CUDA 11.1.0](https://developer.nvidia.com/cuda-toolkit-archive). Having those two installed, this repo comes with a shell script for easy setup using a bash terminal:

```shell
cd REPOSITORY_DIRECTORY
source install.sh
```

This utility builds a `venv` virtual environment and installs all required python libraries, including binaries compatible with CUDA for GPU use. Alternatively, look into `install.sh` to install dependencies manually.

Finally, running programs only requires activating the virtual environment:

```shell
source activate.sh
python train.py
```

## Usage
We are releasing both our dataset and renderer + scenes used to generate it. The dataset consists of renderings in multiple resolutions of different scenes from randomly selected (valid) viewpoints, and corresponding G-buffer data used in deferred shading.

To use this code, the location of the dataset and other parameters can be controlled by modifying the `config.json` file. This repository comes with 3 example python scripts:
* `train.py` - trains a new FLIP predictive model
* `preview-metrics.py` - renders ground truth of all the tested metrics in transformed space for a given view
* `preview-vrs.py` - compares VRS render decisions using JNFLIP with using a trained JNFLIP  model on a given view

## Citation
If you use this code or dataset for your research, please cite our paper:

```bibtex
  TBD
```
