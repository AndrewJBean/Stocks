#!/usr/bin/env python3

# MIT License

# Copyright (c) 2018 Andrew J. Bean

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import h5py
import numpy as np
import glob
import aggregate as ag


def MakeFeatureParams(IntervalsPerDay):
    FeatureParams = ag.FeatureParameters(IntervalsPerDay)
    # interval sizes for aggregated log price relatives
    FeatureParams.StridesLPR = [
        1,
        2,
        5,
        10,
        30,
        60,
        IntervalsPerDay,
        IntervalsPerDay*2,
        IntervalsPerDay*5,
        IntervalsPerDay*10,
        # IntervalsPerDay*20,
        # IntervalsPerDay*50,
        # IntervalsPerDay*100,
    ]
    # window sizes for Simple Moving Average
    FeatureParams.StridesSMA = [
        10,
        20,
        IntervalsPerDay,
        IntervalsPerDay*2,
        IntervalsPerDay*5,
        IntervalsPerDay*10,
        IntervalsPerDay*20,
        IntervalsPerDay*50,
        IntervalsPerDay*100,
    ]
    FeatureParams.VolumeIntervals = [
        10,
        20,
        IntervalsPerDay,
        IntervalsPerDay*5,
        IntervalsPerDay*20,
    ]
    return FeatureParams




def main():
    FileNames = glob.glob("features/*.float32")

    period = 2
    IntervalsPerDay = int(6.5 * 60 / 2)
    FeatureParams = MakeFeatureParams(IntervalsPerDay)

    AllDataFile = h5py.File("2min_aggregated_features.hdf5", "w")

    grp = AllDataFile.create_group("FeatureParams")
    dset = grp.create_dataset('StridesLPR', data=FeatureParams.StridesLPR)
    dset = grp.create_dataset('StridesSMA', data=FeatureParams.StridesSMA)
    dset = grp.create_dataset('VolumeIntervals', data=FeatureParams.VolumeIntervals)

    AllSymbols = [x[9:] for x in FileNames]
    AllSymbols = [x[:-8] for x in AllSymbols]
    AllSymbols = [n.encode("ascii", "ignore") for n in AllSymbols]
    AllDataFile.create_dataset('AllSymbols', (len(AllSymbols),),'S15', AllSymbols)

    for FName in FileNames:
        # extract just the stock symbol from the file name
        SymbolString = FName[9:len(FName)-8]
        print('stock symbol = ' + SymbolString)
        # read in the data from the file
        data = np.fromfile(FName,dtype=np.float32)
        # 81 features total
        TotalFeatures = (
            6 + 4*len(FeatureParams.StridesLPR)
            + len(FeatureParams.StridesSMA)
            + len(FeatureParams.VolumeIntervals))
        data.shape = (data.shape[0]//TotalFeatures,TotalFeatures)
        # put into HDF5 database
        # compression doesn't help much
        # but does make dataset creation much slower
        # dset = AllDataFile.create_dataset(SymbolString, data=data, compression="gzip", compression_opts=9)
        dset = AllDataFile.create_dataset(SymbolString, data=data)
        dset.attrs['period'] = period
        dset.attrs['IntervalsPerDay'] = IntervalsPerDay
        dset.attrs['NumDays'] = data.shape[0]//IntervalsPerDay








if __name__ == '__main__':
    main()

