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
import matplotlib.pyplot as plt
from keras.models import load_model

def main():
    Gain = '0.5'
    Loss = '0.5'
    MSE_Loss = False
    LinearActivation = False
    DropFrac = '0.0'
    NEpochs = '200'
    NameRoot = 'my_model_'

    whichplot = 1

    extension = Gain+','+Loss +'_open'
    ExamplesFile = h5py.File("2min_examples_"+extension+".hdf5", "r")
    # ExamplesFile = h5py.File("2min_aggregated_examples_"+extension+".hdf5", "r")

    SaveName = NameRoot+extension
    if MSE_Loss:
        SaveName = SaveName + '_MSE'
    if LinearActivation:
        SaveName = SaveName + '_Lin'
    if DropFrac!='0.7':
        SaveName = SaveName + '_' + DropFrac
    # SaveName = 'Trained/' + SaveName + "_"+NEpochs+"_"+val_loss+".h5"
    SaveName = 'Trained/' + SaveName + "_"+NEpochs+".h5"
    print(SaveName)
    model = load_model(SaveName)

    print(extension)
    if MSE_Loss:
        print('using mean_squared_error loss')
    else:
        print('using binary_crossentropy loss')
    print(ExamplesFile['Features'].shape)
    print(ExamplesFile['Outcomes'].shape)
    print(ExamplesFile['Timestamps'].shape)

    NumExamples = ExamplesFile['Features'].shape[0]
    FeatureSize = ExamplesFile['Features'].shape[1]

    SampleShift = 0.0
    StartTest = int((0.9 - SampleShift )*NumExamples)
    EndTest = int((1.0 - SampleShift )*NumExamples)

    print('reading data from HDF5...')
    # x_train = ExamplesFile['Features'][:StartTest]
    # y_train = ExamplesFile['Outcomes'][:StartTest]

    x_test = ExamplesFile['Features'][StartTest:EndTest]
    y_test = ExamplesFile['Outcomes'][StartTest:EndTest]
    t_test = ExamplesFile['Timestamps'][StartTest:EndTest]

    y_predict = model.predict_on_batch(x_test)
    y_predict = np.array([y_predict[i][0] for i in range(len(y_predict))])
    Rearrange = y_predict.argsort()
    # flip so losses are at right of plot
    # don't flip to keep losses at the left
    # Rearrange = np.flip(Rearrange,0)
    y_predict = y_predict[Rearrange]
    y_test = y_test[Rearrange]
    t_test = t_test[Rearrange]
    y_test = y_test*(float(Gain)+float(Loss))/100.0 - float(Loss)/100.0 + 1.0

    if whichplot==0:
        y_test = np.log(y_test)
        y_test = np.cumsum(y_test)
        for i in range(len(y_test)):
            y_test[i] = y_test[i]/(i+1.0)
        plt.plot(np.exp(y_test)*100-100)
        plt.plot((t_test-min(t_test))/(max(t_test)-min(t_test)),'.')
        plt.show()
    elif whichplot==1:
        NWindow = 100
        plt.plot((t_test-min(t_test))/(max(t_test)-min(t_test)),'.')
        plt.plot(np.convolve(y_test*100-100,np.ones(NWindow)/NWindow,mode='same'))
        plt.plot(y_predict)
        plt.show()




if __name__ == '__main__':
    main()

