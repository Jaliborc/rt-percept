###########################################################################
# Copyright (c) 2022, Joao Cardoso. All rights reserved.
#
# This work is licensed under the Creative Commons Attribution-NonCommercial
# 4.0 International License. To view a copy of this license, visit
# http://creativecommons.org/licenses/by-nc/4.0/ or send a letter to
# Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
###########################################################################

from torch.nn import Module
from lpips import LPIPS as LpipsNet
from api.external.flip import FLIP as FlipBase
import torch.nn.functional as f
import torch

# Transforms
class ExtrapolatePerceptuals(Module):
    def forward(self, e, channels=6):
        if channels > 2 and e.size()[1] == 2:
            e = torch.cat((e,
                torch.maximum(e[:,0:1,:,:]*2.13, e[:,1:2,:,:]),
                torch.maximum(e[:,0:1,:,:], e[:,1:2,:,:]*2.13),
            ),1)
        if channels > 4 and e.size()[1] == 4:
            e = torch.cat((
                e[:,0:2,:,:],
                torch.maximum(e[:,0:1,:,:], e[:,1:2,:,:]),
                e[:,2:4,:,:],
                torch.maximum(e[:,2:3,:,:], e[:,3:4,:,:])
            ),1)

        return e

class LinearTransform(Module):
    def __init__(self, factor=1, off=0):
        super().__init__()
        self.factor, self.off = torch.tensor(factor), torch.tensor(off)
    def forward(self, e):
        return (e * self.factor + self.off).clamp(0, 1)
    def inverse(self, e):
        return (e - self.off) / self.factor

class LogitTransform(Module):
    def __init__(self, midpoint, growth=10):
        super().__init__()
        self.mid = torch.tensor(midpoint)
        self.growth = torch.tensor(growth)
        self.low = torch.sigmoid(-self.mid*self.growth)
        self.scale = torch.sigmoid((1-self.mid)*self.growth) - self.low
    def forward(self, e):
        return (torch.sigmoid((e-self.mid)*self.growth) - self.low) / self.scale
    def inverse(self, e):
        p = e * self.scale + self.low
        return torch.log(p/(1-p)) / self.growth + self.mid

# Metrics
class Yang(Module):
    def __init__(self, stride=16):
        super().__init__()
        self.stride = stride
    def forward(self, gt, modes=None):
        s = gt.size()
        luma = rgb2luma(gt)
        mask = f.max_pool2d(1 - gt[:,3,...] if s[1] > 3 else torch.zeros(s[0], 1, s[2], s[3], device=gt.device), self.stride)

        x = (luma[...,0::2,:] - luma[...,1::2,:]).abs()
        x = f.max_pool2d(x, (self.stride//2, self.stride)) + mask

        y = (luma[...,0::2] - luma[...,1::2]).abs()
        y = f.max_pool2d(y, (self.stride, self.stride//2)) + mask

        return torch.cat((x,y), 1)

class MetricBase(Module):
    def __init__(self, stride=16, pool='max', weber=False):
        super().__init__()
        self.stride = stride
        self.weber = weber
        self.pool = pool

    def forward(self, gt, modes):
        v = torch.cat([self.compare(gt, modes[:,i:(i+3),...]) for i in range(0, modes.size(1), 3)], 1)

        if self.weber:
            v /= f.interpolate(f.avg_pool2d(rgb2luma(gt), 4) + 0.0000000001, scale_factor=(4,4))
        if self.pool == 'max':
            return f.max_pool2d(v, self.stride)
        elif self.pool == 'mean':
            return f.avg_pool2d(v, self.stride)
        else:
            return v

class YangPlus(MetricBase):
    def compare(self, a, b):
        return (rgb2luma(a) - rgb2luma(b)).abs()

class Flip(MetricBase, FlipBase):
    def compare(self, a, b):
        return self.compute_flip(a, b)

class Psnr(MetricBase):
    def compare(self, a, b):
        mse = f.avg_pool2d(f.mse_loss(a, b, reduction='none').sum(1).unsqueeze(1) / 3., self.stride)
        return f.interpolate(-10. * torch.log10(mse), scale_factor=self.stride)

class Lpips(MetricBase):
    def __init__(self, **args):
        super().__init__(**args)
        self.net = LpipsNet(spatial=True, net='alex')
        self.eval()

    def compare(self, a, b):
        return self.net.forward(a, b)

# Utils
def rgb2luma(rgb):
    return (0.299*rgb[:,0,...] + 0.587*rgb[:,1,...] + 0.114*rgb[:,2,...]).unsqueeze(1)
