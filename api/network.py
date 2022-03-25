###########################################################################
# Copyright (c) 2022, Joao Cardoso. All rights reserved.
#
# This work is licensed under the Creative Commons Attribution-NonCommercial
# 4.0 International License. To view a copy of this license, visit
# http://creativecommons.org/licenses/by-nc/4.0/ or send a letter to
# Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
###########################################################################

from torch.nn import Sequential, Conv2d, BatchNorm2d, MaxPool2d, ReLU, Sigmoid
from pytorch_lightning import LightningModule
from torchvision import utils
from torch import optim, no_grad

class GeneralizedRTMetric(LightningModule):
    def __init__(self, metric, transform, **args):
        super().__init__()
        self.save_hyperparameters(ignore=['metric', 'transform'])
        self.model = Predictor(stride=metric.stride, **args)
        self.metric, self.transform = metric, transform

    def configure_optimizers(self):
        return optim.RMSprop(self.model.parameters(), lr=0.0001, weight_decay=0.0001)

    def forward(self, inputs, safe=True):
        return self.transform.inverse(self.model(inputs) + (self.safety if safe else 0))

    def training_step(self, batch, *args):
        gt, y = self.compare(batch)
        loss = (gt - y).abs().mean()

        self.log('training/loss', loss)
        return loss

    def validation_step(self, batch, *args):
        gt, y = self.compare(batch)
        d = (gt - y).abs()
        n = y.numel()

        for i in range(y.size(1)):
            self.logger.experiment.add_image(f'mode {i}', utils.make_grid(y[:,i:(i+1),:,:], 4), self.global_step)
        self.safety = Parameter(d[y < gt].sum() / n, requires_grad=False)

        self.log('validation/r2', 1 - (d ** 2).sum() / ((gt - gt.mean()) ** 2).sum())
        self.log('validation/overestimation', d[y > gt].sum() / n)
        self.log('validation/underestimation', self.safety)
        self.log('validation/std', d.var().sqrt())
        self.log('validation/loss', d.mean())

    def compare(self, batch):
        with no_grad():
            gt = self.transform(self.metric(batch['gt'], batch['modes']))
        return gt, self.model(batch['input'])

class Predictor(Sequential):
    def __init__(self, num_channels=4, num_guesses=1, stride=8, width=16):
        p1 = min(int(stride), 2)
        p2 = min(int(stride/2), 2)
        p3 = min(int(stride/4), 2)

        super().__init__(
            PoolDown(num_channels, width),
            PoolDown(width, width, pool=p1),
            PoolDown(width, width, pool=p2, groups=p1),
            PoolDown(width, width, pool=p3, groups=p2*p1),
            PoolDown(width, num_guesses, pool=int(stride/8)),
            Sigmoid())

class PoolDown(Sequential):
    def __init__(self, input, output, kernel=3, stride=1, padding=1, dilation=1, groups=1, pool=1):
        super().__init__(
            Conv2d(input, output, kernel, stride, padding, dilation, groups),
            ReLU(), BatchNorm2d(output))

        if pool > 1:
            self.add_module('3', MaxPool2d(pool))
