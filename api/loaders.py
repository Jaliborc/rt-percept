from torchvision.transforms.functional import to_tensor
from torch.utils.data import Dataset
from PIL import Image
import torch, os

class GBufferDataset(Dataset):
    def __init__(self, *directories):
        self.directories = directories
        self.num_entries = []

        for directory in directories:
            count = 0
            buffer = next(iter(self.buffers.values()))[0][0]
            while os.path.exists(self.impath(directory, count, buffer)):
                count += 1

            self.num_entries += [count]

    def __getitem__(self, index):
        item = {}
        k = 0

        while self.num_entries[k] <= index:
            index -= self.num_entries[k]
            k += 1

        for key, group in self.buffers.items():
            images = []

            for entry in group:
                buffer, channels = entry
                indices = [i for i, c in enumerate('rgba') if channels.find(c) >= 0]
                images += [self.imread(self.impath(self.directories[k], index, buffer))[indices,...]]

            tensor = torch.cat(images, 0)
            tensor[tensor != tensor] = 0 # NaN hotfix
            item[key] = tensor

        return item

    def __len__(self): return sum(self.num_entries)
    def num_sources(self, group): return len(self.buffers[group])
    def num_channels(self, group): return sum(len(v[1]) for v in self.buffers[group])

class FalcorFrames(GBufferDataset):
    def impath(self, directory, index, buffer):
        return os.path.join(directory, '{:04d}'.format(index), buffer + '.png')

    def imread(self, path):
        return to_tensor(Image.open(path).convert('RGBA'))

class Falcor4to1(FalcorFrames):
    buffers = {
        'input': (('reproject_1x1', 'ra'), ('diffuse_1x1', 'r'), ('normals_1x1', 'b')),
        'modes': (('output_2x2', 'rgb'),),
        'gt': (('output_1x1', 'rgb'),),
    }

class FalcorVRS(FalcorFrames):
    buffers = {
        'input': (('reproject_1x1', 'ra'), ('diffuse_1x1', 'r'), ('normals_1x1', 'b')),
        'modes': (('output_1x2', 'rgb'), ('output_2x1', 'rgb'), ('output_2x4', 'rgb'), ('output_4x2', 'rgb')),
        'gt': (('output_1x1', 'rgb'),),
    }

class FalcorUnseenVRS(FalcorFrames):
    buffers = {
        'input': (('diffuse_1x1', 'r'), ('specular_1x1', 'r'), ('normals_1x1', 'b'), ('extra_1x1', 'r')),
        'modes': (('output_1x2', 'rgb'), ('output_2x1', 'rgb')),
        'gt': (('output_1x1', 'rgb'),),
    }

class FalcorReuse(FalcorFrames):
    buffers = {
        'input': (('reproject_1x1', 'r'), ('diffuse_1x1', 'r'), ('specular_1x1', 'r'), ('normals_1x1', 'b')),
        'modes': (('reproject_1x1', 'rgb'),),
        'mask': (('reproject_1x1', 'a'),),
        'gt': (('output_1x1', 'rgb'),),
    }
