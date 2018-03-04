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

import numpy as np
import matplotlib.pyplot as plt
import glob
import sys
from numba import jit

class CleanedData:
    """Holds cleaned data and some extra info"""
    # IntervalsPerDay
    # NumDays
    # period -- integer number of minutes per aggregated trading period
    # data -- cleaned data array
    pass

class FeatureData:
    """Holds sequential feature vectors generated from cleaned data"""
    # IntervalsPerDay
    # NumDays
    # period
    # data
    pass

class FeatureParameters:
    def __init__(self,IntervalsPerDay=0):
        # interval sizes for aggregated log price relatives
        self.StridesLPR = [
            1,
            2,
            5,
            10,
            20,
            IntervalsPerDay,
            # IntervalsPerDay*2,
            # IntervalsPerDay*5,
            # IntervalsPerDay*10,
            # IntervalsPerDay*20,
            # IntervalsPerDay*50,
            # IntervalsPerDay*100,
        ]
        # window sizes for Simple Moving Average
        self.StridesSMA = [
            10,
            20,
            # IntervalsPerDay,
            # IntervalsPerDay*2,
            # IntervalsPerDay*5,
            # IntervalsPerDay*10,
            IntervalsPerDay*20,
            # IntervalsPerDay*50,
            # IntervalsPerDay*100,
        ]
        self.VolumeIntervals = [
            10,
            20,
            IntervalsPerDay,
        ]

def GetDayStartIndices(data):
    """Determine indices for each start of a trading day

    Arguments:
     - data -- np.array of np.float64, size (N,6)
       * passing full raw dataset to avoid data copy (i think)
       data[:,0] is unix times (seconds), start of the interval
       data[:,1] is close price, dollars
       data[:,2] is high price, dollars
       data[:,3] is low price, dollars
       data[:,4] is open price, dollars
       data[:,5] is share unit volume

    Return Value:
     - np.array of np.int32, size (NumDays)
    """

    # points at the beginning of an open market period
    CheckPnt = 0

    # return values
    DayStarts = np.array([],dtype=np.int32)

    while CheckPnt < data.shape[0]:
        DayStarts = np.append(DayStarts,CheckPnt)
        # this will point to the next market open time
        EndPnt = CheckPnt+1
        # compare with prev point, stop searching if time diff is big
        while EndPnt < data.shape[0] and (data[EndPnt,0]-data[EndPnt-1,0])<8*60*60:
            EndPnt += 1
        # EndPnt now points to next start of market period
        CheckPnt = EndPnt

    return DayStarts






def AlignStartTime(DayStartTime,TimeReference):
    """ Map DayStartTime to the correct unix
     start time for that trading day

    Arguments:
    - DayStartTime -- unix time during a trading day
      should be one of the following:
      - exactly N*TimeReference+9.5*60*60 or slightly after
      - exactly N*TimeReference+10.5*60*60 or slightly after
      (indicative of daylight saving time)
    - TimeReference -- unix time 1396411200
      equal to 2014-04-02 00:00:00 GMT-04:00 DST

    Return Value:
    - a "floored" value for the start of the trading day
    - should be exactly 9:30 AM eastern time
    """
    # align start time with 9:30am
    # account for daylight saving time
    # check that seconds after midnight less than 10.5*3600
    # i.e. before 10:30am
    if np.mod( DayStartTime - TimeReference , 3600*24 ) < (105*360):
        # eastern time is GMT-04:00
        DayStartTime -= TimeReference
        # start time at exactly 9:30am
        DayStartTime = (DayStartTime//(24*3600))*(24*3600) + 95*360
    else:
        # eastern time is GMT-05:00
        DayStartTime -= TimeReference
        # start time at exactly 9:30am
        DayStartTime = (DayStartTime//(24*3600))*(24*3600) + 105*360
    return DayStartTime + TimeReference






def AggregateData(data,period):
    """Create a clean dataset

    Arguments:
    - data -- np.array of np.float64, size (N,6)
      data[:,0] is unix times (seconds), start of the interval
      data[:,1] is close price, dollars
      data[:,2] is high price, dollars
      data[:,3] is low price, dollars
      data[:,4] is open price, dollars
      data[:,5] is share unit volume
    - period -- integer number of minutes per aggregated trading period

    Return Value:
    - np.array of np.float32, size (K,6)
      retval[:,0] is start of the interval,
        days after unix_time(1396411200)
        = Wednesday, April 2, 2014 12:00:00 AM GMT-04:00 DST
        (start of the earliest day in my dataset)
      retval[:,1] is close price, dollars
      retval[:,2] is high price, dollars
      retval[:,3] is low price, dollars
      retval[:,4] is open price, dollars
      retval[:,5] is share unit volume
      * each day will have the same number of data points
      * day i starts at retval[i*ceil(9.5*60/period),:]

    Notes:
    - intervals may not line up perfectly
      with the indicated trading period
    - allow a decision making time delay (nonzero
      response time) to avoid anticausal strategies
    - aggregates a period from t0 to t1 using raw periods
      that have end time in the (half-open) interval (t0,t1]
    - missing data will be filled in appropriately,
      carrying over from previous raw data period
    - short trading days will be lengthened to 9.5hrs
      with filler data
    - periods in daylight time vs standard time will
      be appropriately aligned
    """

    # 2014-04-02 00:00:00 GMT-04:00 DST
    TimeReference = 1396411200.0
    # trading hours are 09:30 to 16:00 = 6.5 hours
    IntervalsPerDay = int(np.ceil(6.5*60/period))
    DayStarts = GetDayStartIndices(data)
    NumDays = len(DayStarts)
    ReturnData = np.zeros([NumDays*IntervalsPerDay, 6],dtype=np.float32)

    # include IntervalStart in the aggregated period
    IntervalStart = 0
    # do not include IntervalEnd in the aggregated period
    IntervalEnd = 0
    for WhichDay in range(NumDays):
        # align start time with 9:30am
        # account for daylight saving time
        # this is a unix time value
        DayStartTime = AlignStartTime(data[DayStarts[WhichDay],0],TimeReference)
        for Interval in range(IntervalsPerDay):
            INDEX = Interval+WhichDay*IntervalsPerDay
            # period start
            ReturnData[INDEX,0] = \
                (DayStartTime + Interval*60*period - TimeReference) / (24*3600)
            while IntervalEnd<data.shape[0] and data[IntervalEnd,0] <= DayStartTime+(Interval+1)*60*period:
                IntervalEnd += 1
            # IntervalEnd is now just past the desired aggregate interval
            # aggregate the data [IntervalStart,IntervalEnd)
            if IntervalEnd > IntervalStart:
                # close, high, low, open, volume
                ReturnData[INDEX,1] = data[IntervalEnd-1,1]
                ReturnData[INDEX,2] = np.max(data[IntervalStart:IntervalEnd,2])
                ReturnData[INDEX,3] = np.min(data[IntervalStart:IntervalEnd,3])
                ReturnData[INDEX,4] = data[IntervalStart,4]
                ReturnData[INDEX,5] = np.sum(data[IntervalStart:IntervalEnd,5])
            elif IntervalStart>0:
                # close, high, low, open
                # use close of previous
                ReturnData[INDEX,1] = data[IntervalStart-1,1]
                ReturnData[INDEX,2] = data[IntervalStart-1,1]
                ReturnData[INDEX,3] = data[IntervalStart-1,1]
                ReturnData[INDEX,4] = data[IntervalStart-1,1]
                # volume
                ReturnData[INDEX,5] = 0
            else:
                # close, high, low, open, volume
                ReturnData[INDEX,1] = data[0,4]
                ReturnData[INDEX,2] = data[0,4]
                ReturnData[INDEX,3] = data[0,4]
                ReturnData[INDEX,4] = data[0,4]
                ReturnData[INDEX,5] = 0
            IntervalStart = IntervalEnd

    # construct return value
    RetVal = CleanedData()
    RetVal.IntervalsPerDay = IntervalsPerDay
    RetVal.NumDays = NumDays
    RetVal.period = period
    RetVal.data = ReturnData
    return RetVal



FeatureIndex = 1



def FeatureLPR(data,PriceIndex,Interval):
    """ get time series of price relatives
    Arguments:
    - data -- cleaned data, np.array of np.float32, size (K,6)
      data[:,0] is start of the interval,
        days after unix_time(1396411200)
        = Wednesday, April 2, 2014 12:00:00 AM GMT-04:00 DST
        (start of the earliest day in my dataset)
      data[:,1] is close price, dollars
      data[:,2] is high price, dollars
      data[:,3] is low price, dollars
      data[:,4] is open price, dollars
      data[:,5] is share unit volume
      * each day will have the same number of data points
      * day i starts at retval[i*ceil(9.5*60/period),:]
    - PriceIndex -- 1 for close, 2 for high, 3 for low, 4 for open
    - Interval -- how far back for relative close value
    """
    global FeatureIndex
    print(str(FeatureIndex)+'  '+str(PriceIndex)+'  '+str(Interval))
    FeatureIndex += 1
    RetVal = np.zeros(data.shape[0])
    for i in range(len(RetVal)):
        # will fill in element i of RetVal
        # will use close from element i-Interval of data
        TimeIndex = max(0,i-Interval)
        # close prices are data[:,1]
        if PriceIndex in [1,4]:
            RetVal[i] = np.log(data[i,PriceIndex]/data[TimeIndex,1])
        elif PriceIndex==2:
            # find high in [max(i-Interval+1,0),i+1)
            RetVal[i] = np.log(max(data[max(i-Interval+1,0):(i+1),2])/data[TimeIndex,1])
        elif PriceIndex==3:
            RetVal[i] = np.log(min(data[max(i-Interval+1,0):(i+1),3])/data[TimeIndex,1])
    return RetVal






def FeatureAverageLPR(data,PriceIndex,Interval):
    """ get time series of price relatives w.r.t. SMA
    Arguments:
    - data -- cleaned data, np.array of np.float32, size (K,6)
      data[:,0] is start of the interval,
        days after unix_time(1396411200)
        = Wednesday, April 2, 2014 12:00:00 AM GMT-04:00 DST
        (start of the earliest day in my dataset)
      data[:,1] is close price, dollars
      data[:,2] is high price, dollars
      data[:,3] is low price, dollars
      data[:,4] is open price, dollars
      data[:,5] is share unit volume
      * each day will have the same number of data points
      * day i starts at retval[i*ceil(9.5*60/period),:]
    - PriceIndex -- 1 for close, 2 for high, 3 for low, 4 for open
    - Interval -- how far back for SMA
    """
    global FeatureIndex
    print(str(FeatureIndex)+'  '+str(PriceIndex)+'  '+str(Interval))
    FeatureIndex += 1
    RetVal = np.zeros(data.shape[0])
    # First calculate moving average
    Normalizer = 0.0
    RunningSum = 0.0
    for i in range(len(RetVal)):
        # moving average from close prices
        RunningSum += data[i,1]
        Normalizer += 1
        if Normalizer > Interval:
            Normalizer -= 1
            RunningSum -= data[i-Interval,1]
        RetVal[i] = np.log(data[i,PriceIndex] / (RunningSum / Normalizer))

    return RetVal






def FeatureVolume(data,Interval,LongInterval):
    """ get time series of normalized trading volume
    Arguments:
    - data -- cleaned data, np.array of np.float32, size (K,6)
      data[:,0] is start of the interval,
        days after unix_time(1396411200)
        = Wednesday, April 2, 2014 12:00:00 AM GMT-04:00 DST
        (start of the earliest day in my dataset)
      data[:,1] is close price, dollars
      data[:,2] is high price, dollars
      data[:,3] is low price, dollars
      data[:,4] is open price, dollars
      data[:,5] is share unit volume
      * each day will have the same number of data points
      * day i starts at retval[i*ceil(9.5*60/period),:]
    - Interval -- how far back for short EMA
    - LongInterval -- how far back for long EMA
        - calculating volume relative to longer term average
    """
    global FeatureIndex
    print(str(FeatureIndex)+'  '+str(Interval))
    FeatureIndex += 1
    RetVal = np.zeros(data.shape[0])
    # First calculate moving averages
    # then, ratio of short MA to long MA
    ShortFilter = data[0,5]
    LongFilter = data[0,5]
    for i in range(len(RetVal)):
        ShortFilter += (1/Interval)*(data[i,5]-ShortFilter)
        LongFilter += (1/LongInterval)*(data[i,5]-LongFilter)
        RetVal[i] = ShortFilter/(LongFilter+1)

    return RetVal






def GenerateFeatureSeries(Cleaned,FeatureParams=None):
    """Create a time series of features from a cleaned dataset

    Arguments:
    - Cleaned -- instance of CleanedData, including:
      - data -- np.array of np.float32, size (K,6)
        data[:,0] is start of the interval,
          days after unix_time(1396411200)
          = Wednesday, April 2, 2014 12:00:00 AM GMT-04:00 DST
          (start of the earliest day in my dataset)
        data[:,1] is close price, dollars
        data[:,2] is high price, dollars
        data[:,3] is low price, dollars
        data[:,4] is open price, dollars
        data[:,5] is share unit volume
        * each day will have the same number of data points
        * day i starts at retval[i*ceil(9.5*60/period),:]
      - period -- integer number of minutes per aggregated trading period
      - IntervalsPerDay
      - NumDays

    Return Value:

    List of Features:
    - note - LPR = log price relative
    - note - SMA = simple moving average
    - note - EMA = exponential moving average
    - untouched clean data
    - single interval and aggregated LPRs, at varying strides
      - w.r.t. prev close
      - e.g. 1,2,5,10,20,1day,2day,5day,10day,20day,50day,100day
      - finer aggregation choices would depend on Cleaned.period
    - close price LPRs w.r.t moving averages
      - same intervals as aggregated LPRs for SMAs
    - volume, ratio with moving average volume
      - maybe same moving average intervals as SMAs?
    """
    if FeatureParams is None:
        FeatureParams = FeatureParameters(Cleaned.IntervalsPerDay);

    RetVal = FeatureData()
    RetVal.IntervalsPerDay = Cleaned.IntervalsPerDay
    RetVal.NumDays = Cleaned.NumDays
    RetVal.period = Cleaned.period
    RetVal.data = np.transpose(np.array([]
        +[Cleaned.data[:,i]
            for i in range(6)]
        +[FeatureLPR(Cleaned.data,PriceIndex,Interval)
            for PriceIndex in range(1,5)
            for Interval in FeatureParams.StridesLPR]
        +[FeatureAverageLPR(Cleaned.data,PriceIndex,Interval)
            for PriceIndex in [1]
            for Interval in FeatureParams.StridesSMA]
        +[FeatureVolume(Cleaned.data,Interval,max(Interval*10,Cleaned.IntervalsPerDay*20))
            for Interval in FeatureParams.VolumeIntervals]
        ))
    return RetVal











# This is just for debugging the functionality provide in this file
def main():
    data = np.fromfile('composite/NFLX.dat',dtype=np.float64)
    data.shape = (data.shape[0]//6,6)
    # data[:,0] is unix times, start of the interval
    # data[:,1] is close price, dollars
    # data[:,2] is high price, dollars
    # data[:,3] is low price, dollars
    # data[:,4] is open price, dollars
    # data[:,5] is share unit volume

    AggregatedData = AggregateData(data,2)
    StockFeatures = GenerateFeatureSeries(AggregatedData)

    print("IntervalsPerDay = " + str(AggregatedData.IntervalsPerDay))
    print("NumDays = " + str(AggregatedData.NumDays))
    print("Period (minutes) = " + str(AggregatedData.period))

    WhichPlot = 2

    # verify price relatives
    if WhichPlot==1:
        # reconstruct growth from price relatives
        # reconstruct moving average from price relatives and
        #    relative moving average
        plt.plot(np.exp(np.cumsum(StockFeatures.data[:,6])))
        plt.plot(np.exp(np.cumsum(StockFeatures.data[:,6]) - StockFeatures.data[:,32]))
        plt.show()

    # plot unsmoothed volume, smoothed volume, and closing prices
    elif WhichPlot==2:
        # plt.subplot2grid:
        # first tuple
        #   (vertical divisions,horizontal divisions)
        # next tuple
        #   (idx from top, idx from left) for top left of plot
        ax1 = plt.subplot2grid((3,1),(0,0),colspan=1,rowspan=1)
        ax2 = plt.subplot2grid((3,1),(1,0),colspan=1,rowspan=1,sharex=ax1)
        ax3 = plt.subplot2grid((3,1),(2,0),colspan=1,rowspan=1,sharex=ax1)
        ax1.plot(StockFeatures.data[:,5])
        ax2.plot(StockFeatures.data[:,35])
        ax3.plot(StockFeatures.data[:,1])
        plt.show()

    # plot (open,high,low,close) and volume underneath
    elif WhichPlot==3:
        ax1 = plt.subplot2grid((3,1),(0,0),colspan=1,rowspan=2)
        ax2 = plt.subplot2grid((3,1),(2,0),colspan=1,rowspan=1,sharex=ax1)

        if True:
            ax1.plot(AggregatedData.data[:,1],'b')
            ax1.plot(AggregatedData.data[:,2],'g',)
            ax1.plot(AggregatedData.data[:,3],'r')
            ax1.plot(AggregatedData.data[:,4],'k')
        else:
            ax1.semilogy(AggregatedData.data[:,1],'b')
            ax1.semilogy(AggregatedData.data[:,2],'g')
            ax1.semilogy(AggregatedData.data[:,3],'r')
            ax1.semilogy(AggregatedData.data[:,4],'k')
        ax2.plot(AggregatedData.data[:,5])
        plt.show()

if __name__ == '__main__':
    main()



