from torchvision.transforms import functional as ft
from matplotlib import pyplot
from api.perceptual import *
from api.loaders import *
import torch, json

if __name__ == '__main__':
    config = json.load(open('config.json'))
    data = Falcor4to1(*config['data'])
    batch = {b[0]: b[1].unsqueeze(0) for b in data[config['preview']].items()}
    stride = config['stride']

    metrics = {
        'yang':   (YangPlus(stride=stride), LinearTransform(1.0)),
        'jnyang': (YangPlus(stride=stride, weber=True), LinearTransform(0.7)),
        'flip':   (Flip(stride=stride), LinearTransform(2.0)),
        'jnflip': (Flip(stride=stride, weber=True), LinearTransform(0.25)),
        'lpips':  (Lpips(stride=stride), LinearTransform(1.0)),
        'psnr':   (Psnr(stride=stride), LinearTransform(-0.015, 1.0)),
    }

    with torch.no_grad():
        for name, entry in metrics.items():
            metric, transform = entry
            result = transform(metric(batch['gt'], batch['modes'])).squeeze()
            colored = pyplot.get_cmap()(result)

            img = ft.to_pil_image(torch.tensor(colored).permute(2,0,1))
            img.save(name + '.png', 'png')
