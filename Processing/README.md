# Decription of files

## aggregate.py
This code can take composite data, in the form of double precision binary files with tuples (timestamp,close,high,low,open,volume) and generate cleaned data, aligned in time.

This also can be used to generate time series of potentially informative features.

## allfeatures.py
Uses aggregate.py to generate a time series for each stock in a list (see the code). If the feature vector time series has already been generated (as indicated by existence of the corresponding file in /features/), the features will not be generated again.

## combine_data.py
Creates a single HDF5 file with all the data generated by allfeatures.py.

## make_examples.py
Uses the data generated by combine_data.py to create a single HDF5 file with feature vectors (X values), investment outcomes (Y values), and timestamps for each (X,Y) pair. Initial experimentation is with basic feedforward neural networks (no LSTMs or other recurrent architectures), so feature vectors contain data from some specified window in time. Investment outcomes take values in the closed interval [0,1], where 0 is indicated if a stop loss order at (1-a) fraction of the purchase price is executed before a time limit T. The value 1 is indicated if a sell limit order at (1+b) fraction of the purchase price is executed before time limit T. The stop loss and limit order are assumed submitted as a one-cancels-other conditional order. The position is exited at time limit T, and if this time is reached, the outcome in [0,1] is indicative of the relative gain achieved between (1-a) and (1+b).

## make_small_examples.py
Does the same as make_examples.py, but generates smaller feature vectors.

## feed_forward.py
Use examples generated by make_examples.py or make_small_examples.py to train a dense ANN. Model generation and training utilize the Keras neural network API.

## play_model.py
Examine the performance of the trained model on a test set.

## General Notes
Training set is the first (oldest) 80% of data, sorted in time. Validation set is the next 10% of data. Test set is the most recent 10% of data.

Data consists of 1min price data for over 500 stocks, with data starting Wednesday, April 2, 2014. Data is complete up to the present day, minus a couple days or so if i have not run the data collection program. Recently, I have added many mid-cap and small-cap stocks to my data collection list (e.g. Russell 3000 stocks), but I only have a small amount of this data (a couple weeks or so).
