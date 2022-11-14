# import matplotlib.pyplot as plt
import numpy as np


def Data_Extract(data, type, layer):
    x_length = len(data)
    y_length = len(data[0])
    if layer == False:
        extract = np.empty([x_length, y_length])
        for i in range(x_length):
            for j in range(y_length):
                extract[i, j] = sum(data[i][j].__dict__[type])
    if layer == True:
        extract = []
        for i in range(x_length):
            extract_temp = []
            for j in range(y_length):
                extract_temp.append(data[i][j].__dict__[type])
            extract.append(extract_temp)
    return extract
