from torch.nn import functional as f
from torchvision.transforms import functional as ft
from api.perceptual import *
from api.network import *
from api.loaders import *
import torch, json

if __name__ == '__main__':
    config = json.load(open('config.json'))
    data = FalcorVRS(*config['data'])
    batch = {b[0]: b[1].unsqueeze(0) for b in data[config['preview']].items()}

    metric = Flip(stride=16, weber=True)
    model = GeneralizedRTMetric.load_from_checkpoint(config['vrs']['checkpoint'], metric=metric, transform=LinearTransform(0.25)).eval()
    predictions = {
        'gt' : metric(batch['gt'], batch['modes']),
        'net': model(batch['input'])
    }

    for name, prediction in predictions.items():
        e = ExtrapolatePerceptuals()(prediction)
        e = f.interpolate(e, batch['gt'].size()[-2:]) < config['vrs']['threshold']

        r4x4 = e[:,5,:,:]
        r2x4 = e[:,4,:,:] * ~r4x4
        r4x2 = e[:,3,:,:] * ~r2x4 * ~r4x4
        r2x2 = e[:,2,:,:] * ~r4x2 * ~r2x4 * ~r4x4
        r1x2 = e[:,1,:,:] * ~r2x2 * ~r4x2 * ~r2x4 * ~r4x4
        r2x1 = e[:,0,:,:] * ~r1x2 * ~r2x2 * ~r4x2 * ~r2x4 * ~r4x4
        r1x1 = ~r2x1 * ~r1x2 * ~r2x2 * ~r4x2 * ~r2x4 * ~r4x4

        reduction = (r1x2 + r2x1 + r2x2 * 3 + r2x4 * 7 + r4x2 * 7 + r4x4 * 15).sum() / (r1x1.numel() * 16)
        render =\
            batch['gt'] * r1x1 +\
            batch['modes'][:,0:3,...] * r1x2 +\
            batch['modes'][:,3:6,...] * r2x1 +\
            batch['modes'][:,6:9,...] * r2x2 +\
            batch['modes'][:,9:12,...] * r2x4 +\
            batch['modes'][:,12:15,...] * r4x2 +\
            batch['modes'][:,15:18,...] * r4x4

        debug =\
            color(0,1,0) * r1x2 +\
            color(0,1,0) * r2x1 +\
            color(1,1,0) * r2x2 +\
            color(1,.5,0) * r2x4 +\
            color(1,.5,0) * r4x2 +\
            color(1,0,0) * r4x4

        debug = r1x1 * render + ~r1x1 * (render * 0.5 + debug * 0.5)
        debug[...,::8,:] = 0
        debug[...,:,::8] = 0

        save(render, f'{name}-render')
        save(debug, f'{name}-debug')

def save(img, path): return ft.to_pil_image(img).save(os.path.expanduser('~/Desktop') + f'/{path}.png', 'png')
def color(*rgb): return torch.tensor(rgb).view(-1,1,1)
