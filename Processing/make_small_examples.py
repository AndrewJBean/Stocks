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
import aggregate as ag
import sys
import time
from numba import jit
from multiprocessing import Pool

# how many samples to take at each time scale
NumSkips = 200

def main():
    AllDataFile = h5py.File("2min_aggregated_features.hdf5", "r")
    if 'FeatureParams' not in AllDataFile:
        sys.exit(0)
    grp = AllDataFile['FeatureParams']
    if 'StridesLPR' not in grp:
        sys.exit(0)

    FeatureParams = ag.FeatureParameters()
    FeatureParams.StridesLPR = grp['StridesLPR'][:]
    FeatureParams.StridesSMA = grp['StridesSMA'][:]
    FeatureParams.VolumeIntervals = grp['VolumeIntervals'][:]
    FeatureParams.IntervalsPerDay = AllDataFile['AAPL'].attrs['IntervalsPerDay']

    T0 = time.time()

    Gain = '0.5'
    Loss = '0.5'
    Features, Outcomes, Timestamps = GenerateExamples(
        AllDataFile,
        FeatureParams,
        Loss=1-float(Loss)/100.0,
        Gain=1+float(Gain)/100.0,
        Horizon=FeatureParams.IntervalsPerDay)
    print('going to make HDF5 file...')
    ExamplesFile = h5py.File("2min_examples_"+Gain+","+Loss+"_open.hdf5", "w")
    ExamplesFile.create_dataset('Features', data=Features)
    ExamplesFile.create_dataset('Outcomes', data=Outcomes)
    ExamplesFile.create_dataset('Timestamps', data=Timestamps)

    T1 = time.time()
    print('Seconds = ',(T1-T0))


def GenerateExamples(AllData,FeatureParams,Loss=0.99,Gain=1.02,Horizon=195):
    '''GenerateExamples(AllData,FeatureParams,Loss,Gain,Horizon)

    Arguments:
    - AllData -- h5py.File()
    - FeatureParams -- ag.FeatureParameters(), initiated from the HDF5 file
    - Loss -- acceptable loss factor (stop loss level for sell)
    - Gain -- gain factor (limit order for sell)
    - Horizon -- duration after buy for sell order to happen

    Return Value:
    - Features -- numpy array with all feature vectors
    - Outcomes -- numpy array with all corresponding investment outcomes
    - Timestamps -- numpy array with all timestamps (start of investment position)
    - Features[i,:],Outcomes[i],Timestamps[i] go together as one example
    - all examples are sorted according to Timestamps, increasing order
    '''
    # AllData - h5py.File()
    AllSymbols = [n.decode('UTF-8') for n in AllData['AllSymbols']]

    # generate all desired examples, then sort by time
    # MAY NEED TO REARCHITECT THIS, IF IT DOES NOT
    #   FIT IN RAM (QUITE LIKELY), as follows:
    #
    # Timestamps should all fit in ram, and can be sorted
    # save all single symbol Features,Outcomes,Timestamps arrays to
    #   HDF5 file - f[sym/data][j,:], f[sym/outcome], f[sym/times]
    # clear all data from RAM
    # create list of tuples -- (timestamp,sym,idx)
    # Determine rearrange indices from timestamp arrays
    # sort tuples by rearrange indices
    # create new HDF5 file with space allocated for all examples
    # go through tuples in sorted order, copying examples into HDF5
    # AllSymbols = AllSymbols[:5]

    if True:
        Features   = []
        Outcomes   = []
        Timestamps = []
        for sym in AllSymbols:
            print('symbol is',sym)
            F,O,T = GenerateExamplesForSymbol(
                sym,FeatureParams,Loss,Gain,Horizon)
            if F.shape[0] > 0:
                Features.append(F)
                Outcomes.append(O)
                Timestamps.append(T)
            print('added',len(F),' for symbol',sym)
    else:
        p = Pool(16)
        results = p.starmap(
            GenerateExamplesForSymbol,
            [(sym,FeatureParams,Loss,Gain,Horizon) for sym in AllSymbols]
            )
        Features = [results[i][0] for i in range(len(results))]
        Outcomes = [results[i][0] for i in range(len(results))]
        Timestamps = [results[i][0] for i in range(len(results))]

    print('going to concatenate...')
    print('...Features')
    Features = np.concatenate(Features)
    print('...Outcomes')
    Outcomes = np.concatenate(Outcomes)
    print('...Timestamps')
    Timestamps = np.concatenate(Timestamps)
    # timsort takes advantage of large contiguous sorted sections
    print('determining sort order...')
    Rearrange = Timestamps.argsort()
    print('rearranging data to be sorted...')
    return Features[Rearrange,:], Outcomes[Rearrange], Timestamps[Rearrange]


def GenerateExamplesForSymbol(Data,FeatureParams,Loss=0.99,Gain=1.02,Horizon=195):
    '''GenerateExamplesForSymbol(Data,FeatureParams,Loss,Gain,Horizon)

    Arguments:
    - Data -- h5py dataset for a symbol
    - FeatureParams -- ag.FeatureParameters(), initiated from the HDF5 file
    - Loss -- acceptable loss factor (stop loss level for sell)
    - Gain -- gain factor (limit order for sell)
    - Horizon -- duration after buy for sell order to happen

    Return Value:
    - Features -- numpy array with all feature vectors
    - Outcomes -- numpy array with all corresponding investment outcomes
    - Timestamps -- numpy array with all timestamps (start of investment position)
    - Features[i,:],Outcomes[i],Timestamps[i] go together as one example
    - all examples are sorted according to Timestamps, increasing order
    '''
    AllDataFile = h5py.File("2min_aggregated_features.hdf5", "r")
    Data = AllDataFile[Data][:]
    # print(type(Data))
    IntervalsPerDay = FeatureParams.IntervalsPerDay
    MinTimeIdx = FeatureParams.StridesLPR[0]*NumSkips
    MaxTimeIdx = Data.shape[0] - Horizon
    if MinTimeIdx>MaxTimeIdx:
        return np.array([]),np.array([]),np.array([])
    IdxList = []
    TestTime = MinTimeIdx
    # basically a do while loop
    while True:
        IdxList.append(TestTime)
        TestTime += np.random.randint(1,1*IntervalsPerDay,1)[0]
        # TestTime += IntervalsPerDay
        if TestTime > MaxTimeIdx:
            break

    FirstF,FirstO = MakeExample(Data,FeatureParams,IdxList[0],Loss,Gain,Horizon)
    Features = np.zeros((len(IdxList),len(FirstF)))
    Features[0,:] = FirstF
    Outcomes = np.zeros(len(IdxList))
    Outcomes[0] = FirstO
    Timestamps = np.zeros(len(IdxList))
    Timestamps[0] = Data[IdxList[0],0]
    for i in range(1,len(IdxList)):
        Features[i,:],Outcomes[i] = MakeExample(Data,FeatureParams,IdxList[i],Loss,Gain,Horizon)
        Timestamps[i] = Data[IdxList[i],0]

    return Features, Outcomes, Timestamps


def MakeExample(Data,FeatureParams,Time=0,Loss=0.99,Gain=1.02,Horizon=195):
    '''MakeExample(Data,FeatureParams,Time,Loss,Gain,Horizon)
    Arguments:
    - Data -- h5py dataset for a symbol
    - FeatureParams -- ag.FeatureParameters(), initiated from the HDF5 file
    - Time -- time index around which to compute the example
    - Loss -- acceptable loss factor (stop loss level for sell)
    - Gain -- gain factor (limit order for sell)
    - Horizon -- duration after buy for sell order to happen

    Return Values:
    - numpy array of features
    - outcome of the investment (stop loss, limit order, or closed at period end)
    '''
    # feature inputs are:
    #   - timestamp
    #   - close, high, low, open, volume
    #   - LPRs, close, each stride in StridesLPR
    #   - LPRs, high, each stride in StridesLPR
    #   - LPRs, low, each stride in StridesLPR
    #   - LPRs, open, each stride in StridesLPR
    #   - SMAs of close, each stride in StridesSMA
    #   - moving average relative volume, each stride in VolumeIntervals

    N = NumSkips

    IntervalsPerDay = FeatureParams.IntervalsPerDay

    NumStridesLPR = len(FeatureParams.StridesLPR)
    NumStridesSMA = len(FeatureParams.StridesSMA)
    NumVolumeIntervals = len(FeatureParams.VolumeIntervals)

    LPRsStart = 6
    SMAsStart = LPRsStart + 4*NumStridesLPR
    VolStart  = SMAsStart + NumStridesSMA

    Features = np.concatenate([]
        # time of day, from opening bell, in [0,1)
        +[np.array([(Time%IntervalsPerDay)/IntervalsPerDay])]
        # log price relatives w.r.t. previous aggregate interval
        # contains duplicates, probably fine though
        +[Data[Time - FeatureParams.StridesLPR[i]*N:Time:FeatureParams.StridesLPR[i],LPRsStart+IDX]
        for i in [0]
        for IDX in range(i,4*NumStridesLPR,NumStridesLPR) ]
        # moving averages, lined up with price relative samples
        # contains duplicates, probably fine though
        +[Data[Time - FeatureParams.StridesLPR[i]*N:Time:FeatureParams.StridesLPR[i],SMAsStart+IDX]
        for i in [0]
        for IDX in [0,1,6]]
        # smoothed volume, lined up with price relative samples
        # contains duplicates, probably fine though
        +[Data[Time - FeatureParams.StridesLPR[i]*N:Time:FeatureParams.StridesLPR[i],VolStart+IDX]
        for i in [0]
        for IDX in [0]]
        )
    return Features, Success(Data,Time,Loss,Gain,Horizon)


@jit
def Success(Data,Time,Loss,Gain,Horizon):
    '''Success(Data,Time,Loss,Gain,Horizon)
    determine the result of in investment

    Arguments:
    - Data -- h5py dataset for a symbol
    - Time -- time index around which to compute the example
    - Loss -- acceptable loss factor (stop loss level for sell)
    - Gain -- gain factor (limit order for sell)
    - Horizon -- duration after buy for sell order to happen

    Return Value:
    - outcome of the investment (stop loss, limit order, or closed at period end)
      - 1 means gain, 0 means loss, between is likely closed at period end
      - value guaranteed in closed interval [0,1]
    '''
    # Pessimistic:
    #   - assuming purchase is at the high of the starting interval
    # BuyIdx = 2
    # Neutral:
    #   - purchase made at openning price of the interval
    BuyIdx = 4

    # check filling of limit order sell (success)
    Idx = Time
    while Idx-Time < Horizon and Data[Idx,2]/Data[Time,BuyIdx] < Gain:
        Idx += 1

    # check filling of stop loss (fail)
    Idx2 = Time
    while Idx2-Time < Horizon and Data[Idx2,3]/Data[Time,BuyIdx] > Loss:
        Idx2 += 1

    if Idx2>Idx:
        return 1.0
    elif Idx2<Idx:
        return 0.0
    else:
        if Idx-Time < Horizon:
            ClosePrice = Data[Idx,1]
        else:
            ClosePrice = Data[Idx-1,1]
        Relative = ClosePrice / Data[Time,BuyIdx]
        Relative -= Loss
        Relative /= (Gain-Loss)
        return max(min(Relative,1.0),0.0)


if __name__ == '__main__':
    main()




