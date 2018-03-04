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
from keras.models import Sequential
from keras.layers import Dense, Dropout
from keras.optimizers import SGD
import sys
from keras.callbacks import ModelCheckpoint
from keras.utils import plot_model

def main():
    Gain = '0.5'
    Loss = '0.5'
    MSE_Loss = False
    LinearActivation = False
    DropFrac = '0.0'
    NEpochs = 200
    NameRoot = 'my_model_'

    extension = Gain+','+Loss+'_open'
    print(extension)
    if MSE_Loss:
        print('using mean_squared_error loss')
    else:
        print('using binary_crossentropy loss')

    SaveName = NameRoot+extension
    if MSE_Loss:
        SaveName = SaveName + '_MSE'
    if LinearActivation:
        SaveName = SaveName + '_Lin'
    if DropFrac!='0.7':
        SaveName = SaveName + '_' + DropFrac
    # SaveName = SaveName + '_' + str(NEpochs) + '.h5'
    filepath = 'Trained/' + SaveName + "_{epoch:03d}.h5"
    print(filepath)

    # ExamplesFile = h5py.File("2min_aggregated_examples_"+extension+".hdf5", "r")
    ExamplesFile = h5py.File("2min_examples_"+extension+".hdf5", "r")
    print(ExamplesFile['Features'].shape)
    print(ExamplesFile['Outcomes'].shape)
    print(ExamplesFile['Timestamps'].shape)

    NumExamples = ExamplesFile['Features'].shape[0]
    FeatureSize = ExamplesFile['Features'].shape[1]

    StartTest = int(0.9*NumExamples)
    StartVal = int(0.8*NumExamples)

    print('reading data from HDF5...')
    x_train = ExamplesFile['Features'][:StartVal]
    y_train = ExamplesFile['Outcomes'][:StartVal]

    x_val = ExamplesFile['Features'][StartVal:StartTest]
    y_val = ExamplesFile['Outcomes'][StartVal:StartTest]

    x_test = ExamplesFile['Features'][StartTest:]
    y_test = ExamplesFile['Outcomes'][StartTest:]


    # DROPOUT - fraction set to zero
    # lower to drop out fewer
    print('beginning model generation...')
    model = Sequential()
    if NameRoot=='my_model_':
        # model.add(Dense(128, activation='relu', input_dim=FeatureSize))
        model.add(Dense(64, activation='relu', input_dim=FeatureSize))
        # model.add(Dropout(0.7))
        # model.add(Dense(64, activation='relu'))
        # model.add(Dropout(float(DropFrac)))
        model.add(Dense(32, activation='relu'))
        # model.add(Dropout(float(DropFrac)))
        model.add(Dense(16, activation='relu'))
        # model.add(Dropout(0.7))
    elif NameRoot=='my_model_2_':
        model.add(Dense(512, activation='relu', input_dim=FeatureSize))
        model.add(Dense(128, activation='relu'))
        model.add(Dense(64, activation='relu'))
        # model.add(Dropout(float(DropFrac)))
        model.add(Dense(32, activation='relu'))
        # model.add(Dropout(float(DropFrac)))
        model.add(Dense(16, activation='relu'))
    else:
        print('ERROR MODEL NOT SET......')
        sys.exit(0)
    if LinearActivation:
        model.add(Dense(1))
    else:
        model.add(Dense(1, activation='sigmoid'))

    # checkpoint
    checkpoint = ModelCheckpoint(filepath, monitor='val_loss', verbose=1, save_best_only=False, mode='auto',period=5)
    callbacks_list = [checkpoint]

    if not MSE_Loss:
        sgd = SGD(lr=0.01, decay=1e-6, momentum=0.9, nesterov=True)
        model.compile(loss='binary_crossentropy',
                      optimizer=sgd,
                      metrics=['accuracy','mse'])
    else:
        model.compile(loss='mean_squared_error',
                      optimizer='rmsprop',
                      metrics=['accuracy','mse'])

    plot_model(model, to_file='model.png')

    model.fit(x_train, y_train,
              validation_data = (x_val,y_val),
              validation_split = 0.15,
              epochs = NEpochs,
              batch_size = 1000,
              callbacks = callbacks_list,
              verbose = 1)
    # model.save(SaveName)

    score = model.evaluate(x_test, y_test, batch_size=128)
    print(score)

    y_predict = model.predict_on_batch(x_test)
    y_predict = np.array([y_predict[i][0] for i in range(len(y_predict))])
    Rearrange = y_predict.argsort()
    y_predict = y_predict[Rearrange]
    y_test = y_test[Rearrange]

    plt.plot(np.convolve(y_test,np.ones(NWindow)/NWindow,mode='same'))
    # plt.plot(y_test)
    plt.plot(y_predict)
    plt.show()


if __name__ == '__main__':
    main()
