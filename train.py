###########################################################################
# Copyright (c) 2022, Joao Cardoso. All rights reserved.
#
# This work is licensed under the Creative Commons Attribution-NonCommercial
# 4.0 International License. To view a copy of this license, visit
# http://creativecommons.org/licenses/by-nc/4.0/ or send a letter to
# Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
###########################################################################

from api.loaders import *
from api.perceptual import *
from api.network import GeneralizedRTMetric
from torch.utils.data import DataLoader, random_split
from pytorch_lightning import Trainer
import json

if __name__ == '__main__':
    config = json.load(open('config.json'))
    data = FalcorVRS(*config['data'])
    stride = config['stride']

    tdata, vdata = random_split(data, [len(data)-12, 12])
    tloader = DataLoader(tdata, batch_size=8, num_workers=16, shuffle=True, drop_last=True)
    vloader = DataLoader(vdata, batch_size=len(vdata), num_workers=12)

    model = GeneralizedRTMetric(Flip(stride=stride), LinearTransform(2.0), width=stride,
                                num_channels = data.num_channels('input'),
                                num_guesses = data.num_sources('modes'))

    trainer = Trainer(gpus=1, val_check_interval=100)
    trainer.fit(model, tloader, vloader)
