#!/usr/bin/python3

import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.datasets import fetch_openml
import time
import sys

def fetch_classification_data(dataset="mnist_784", random_state=42):
    """
    Loads the dataset from openml, normalizes feature values (by dividing
    everything by 256), and saves to an npz file.

    dataset: the name of the dataset (accepted: "mnist_784", "Fashion-MNIST")
    """
    assert dataset in {"mnist_784", "Fashion-MNIST", "Kuzushiji-MNIST"}
    start_time = time.time()
    X, y = fetch_openml(dataset, version=1, return_X_y=True, as_frame=False)
    print("Downloaded data in {:.4f} seconds".format(time.time() - start_time))
    X /= 255  # normalize
    y = y.astype(int)  # fetch_openml loads it as a str
    print("Dataset has : " + str(len(X)))
    train_X, ev_X, train_y, ev_y = train_test_split(
        X, y, train_size=60000, test_size=10000, random_state=int(random_state)
    )
    return (train_X, ev_X, train_y, ev_y )    

def main():

    if len(sys.argv) == 2:
        print(sys.argv)
        train_X, ev_X, train_y, ev_y = fetch_classification_data(random_state=int(sys.argv[1]))
        np.savetxt('../dataset/test.txt',np.concatenate((ev_y.reshape((10000, 1)), ev_X), axis=1), fmt='%f', delimiter='\t')
        np.savetxt('../dataset/test_features.txt', np.transpose(ev_X), fmt='%f', delimiter='\t')
        np.savetxt('../dataset/test_labels.txt', ev_y, fmt='%i', delimiter='\t')
    else:
        train_X, ev_X, train_y, ev_y = fetch_classification_data()
        np.savetxt('../dataset/train.txt',np.concatenate((train_y.reshape(60000, 1), train_X), axis=1), fmt='%f', delimiter='\t')
        np.savetxt('../dataset/test.txt',np.concatenate((ev_y.reshape((10000, 1)), ev_X), axis=1), fmt='%f', delimiter='\t')
        np.savetxt('../dataset/test_features.txt', np.transpose(ev_X), fmt='%f', delimiter='\t')
        np.savetxt('../dataset/test_labels.txt', ev_y, fmt='%i', delimiter='\t')


if __name__=='__main__':
    main()
