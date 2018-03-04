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
import aggregate as ag
from multiprocessing import Process
from multiprocessing import Pool
import time

def ProcessFile(FName):
    print(FName)
    data = np.fromfile(FName,dtype=np.float64)
    data.shape = (data.shape[0]//6,6)

    # clean up the data set
    Cleaned = ag.AggregateData(data,2)
    # construct parameters for feature generation
    FeatureParams = MakeFeatureParams(Cleaned.IntervalsPerDay)

    SequentialFeatures = ag.GenerateFeatureSeries(Cleaned,FeatureParams)
    # take off 'composite'
    OutName = FName[9:]
    # add 'features', take off '.dat', add '.float32'
    OutName = 'features' + OutName[:-4] + '.float32'
    print(OutName)
    SequentialFeatures.data.astype(np.float32).tofile(OutName)


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



def NeedToDo():
    ExistingFileNames = glob.glob("features/*.float32")
    ExistingFileNames = [ (x[9:]) for x in ExistingFileNames]
    ExistingFileNames = [ (x[:-8]) for x in ExistingFileNames]

    SymbolsList = [
        # Large Cap Stock Symbols (over $10B)
        "DJI","INX","IXIC","A","AAPL","ABB","ABBV","ABC","ABT","ACN","ADBE","ADI","ADM","ADP","ADS","ADSK",
        "AEE","AEP","AES","AET","AFL","AIG","AIV","AIZ","AKAM","ALB","ALL","ALLE","ALXN","AMAT","AMD","AME","AMG",
        "AMGN","AMP","AMT","AMZN","AN","AON","APA","APC","APD","APH","ATI","ATVI","AVB","AVGO","AVP","AVY",
        "AXP","AZO","BA","BABA","BAC","BAX","BB","BBBY","BBD","BBT","BBVA","BBY","BCS","BDX","BEN","BF.B",
        "BIIB","BK","BLK","BLL","BMS","BMY","BOX","BRK.B","BSMX","BSX","BUD","BWA","BXP","C","CA","CAG","CAH",
        "CAT","CB","CBG","CBS","CCE","CCI","CCL","CELG","CEO","CERN","CF","CHK","CHRW","CHU","CI","CINF",
        "CL","CLF","CLX","CMA","CMCSA","CME","CMG","CMI","CMS","CNP","CNX","COF","COG","COH","COL","COP","CORE","COST",
        "CPB","CRM","CSCO","CSLT","CSX","CTAS","CTL","CTSH","CTXS","CVS","CVX","D","DAL","DDD","DE","DFS",
        "DG","DGX","DHI","DHR","DIS","DISCA","DLPH","DLTR","DNB","DNKN","DNR","DO","DOV","DPS","DPZ","DRI","DTE",
        "DUK","DVA","DVN","DWDP","EA","EBAY","ECL","ED","EFX","EIX","EL","EMN","EMR","EOG","EQR","EQT","ESRX","ESV","ETFC",
        "ETN","ETR","EW","EXC","EXPD","EXPE","F","FAST","FB","FCX","FDX","FE","FFIV","FIS","FISV","FITB","FLIR","FLR",
        "FLS","FMC","FOSL","FOXA","FSLR","FTI","FTR","GD","GE","GGP","GHC","GILD","GIS","GLW","GM",
        "GME","GNW","GOOG","GOOGL","GPC","GPS","GRA","GRMN","GRPN","GS","GSK","GT","GWW","HAL","HAS","HBAN","HCN",
        "HCP","HD","HES","HIG","HOG","HON","HP","HPQ","HRB","HRL","HRS","HSBC","HST","HSY","HUM","IBM","ICE","IFF",
        "IGT","ILMN","INTC","INTU","IP","IPG","IR","IRM","ISRG","ITW","IVZ","JBL","JCI","JCP","JD","JEC","JNJ","JNPR","JPM",
        "JWN","K","KEY","KHC","KIM","KLAC","KMB","KMX","KO","KORS","KR","KSS","KSU","L","LB","LEG","LEN","LH",
        "LLL","LLY","LM","LMT","LNC","LOW","LRCX","LUK","LUV","LYB","M","MA","MAC","MAR","MAS","MAT",
        "MCD","MCHP","MCK","MCO","MDLZ","MDT","MEET","MET","MHK","MKC","MLM","MMC","MMM","MNST","MO","MON","MOS","MPC",
        "MRK","MRO","MS","MSFT","MSI","MTB","MU","MUR","MYL","NATI","NBL","NBR","NDAQ","NDXT","NEE","NEM","NFG","NFLX","NFX",
        "NGG","NI","NKE","NLSN","NOC","NOV","NRG","NSC","NTAP","NTRS","NUE","NVDA","NVS","NWL","NWSA","OI","OII","OKE",
        "OMC","ORCL","ORLY","OXY","P","PAYX","PBCT","PBI","PBPB","PCAR","PCG","PCLN","PDCO","PEG","PEP",
        "PFE","PFG","PG","PGR","PH","PHM","PKI","PLD","PM","PNC","PNR","PNW","PPG","PPL","PRGO","PRU",
        "PSA","PSX","PTR","PUK","PVH","PWR","PX","PXD","PZZA","QCOM","QEP","QQQ","R","RDC","REGN","RF","RHI","RHT","RIG","RL",
        "ROK","ROP","ROST","RRC","RSG","RST","RTN","S","SBUX","SCG","SCHW","SEE","SHLD","SHW",
        "SIG","SINA","SJM","SLAB","SLB","SNA","SNI","SO","SP500-10","SP500-10102020","SP500-15","SP500-20","SP500-25",
        "SP500-30","SP500-35","SP500-40","SP500-45","SP500-50","SP500-55","SPG","SPGI","SPWR","SPY","SRCL","SRE","STI","STM","STT",
        "STX","STZ","SWK","SWN","SYK","SYMC","SYY","T","TAP","TDC","TEL","TGT","THC","TIF","TJX","TM",
        "TMK","TMO","TMUS","TOT","TRIP","TROW","TRV","TSCO","TSLA","TSN","TSS","TWTR","TWX","TXN","TXT",
        "UA","UL","UNFI","UNH","UNM","UNP","UPS","URBN","USB","UTX","V","VAR","VFC","VIAB","VLO","VMC","VMW",
        "VNO","VOO","VRSN","VRTX","VTR","VZ","WBA","WAT","WDC","WEC","WFC","WHR","WM","WMB","WMT","WRB","WU","WY",
        "WYN","WYNN","X","XEC","XEL","XL","XLNX","XOM","XRAY","XRX","XYL","YELP","YUM","Z","ZION","ZNGA","ZTS",
    ]
    print('len(SymbolsList) =',len(SymbolsList))
    return sorted(list(set(SymbolsList)-set(ExistingFileNames)))






def main():

    # FileNames = glob.glob("composite/*.dat")
    SymbolsList = NeedToDo()

    T0 = time.time()

    FileNames = []
    for sym in SymbolsList:
        FileName = 'composite/' + sym + '.dat'
        if(sym[0]=='.'):
            FileName = 'composite/' + sym[1:] + '.dat'
        FileNames.append(FileName)


    p = Pool(16)
    p.map(ProcessFile, FileNames)

    # for a in FileNames:
    #     ProcessFile(a)

    T1 = time.time()
    print('Seconds = ',(T1-T0))


if __name__ == '__main__':
    main()


