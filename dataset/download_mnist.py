import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.datasets import fetch_openml
import time

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
    # X /= 256  # normalize
    y = y.astype(int)  # fetch_openml loads it as a str
    print("Dataset has : " + str(len(X)))
    # train_dev_X, train_dev_y = X[:60000], y[:60000]
    train_X, ev_X, train_y, ev_y = train_test_split(
        X, y, train_size=60000, test_size=10000, random_state=random_state
    )
    np.savetxt('y_test.csv', ev_y, fmt='%i', delimiter=',')
    np.savetxt('x_train.csv', train_X, fmt='%i', delimiter=',')
    np.savetxt('y_train.csv', train_y, fmt='%i', delimiter=',')
    np.savetxt('x_test.csv', ev_X, fmt='%i', delimiter=',')

fetch_classification_data()
