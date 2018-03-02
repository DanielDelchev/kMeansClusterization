import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import matplotlib.patches as mpatches

if __name__ == '__main__':
    data = pd.read_csv('./output.csv',index_col='ID')
    data['Cluster'] = pd.Categorical(data['Cluster'])
    clusters = data['Cluster'].cat.categories
    clustersCount = (len(clusters))
    colors = cm.rainbow(np.linspace(0, 1, clustersCount))
    info = []
    for _label,col in zip(clusters,range(0,clustersCount)):
        plt.scatter(data[data['Cluster']==_label]['coordinate1'], data[data['Cluster']==_label]['coordinate2'],color = colors[col])
        info.append(mpatches.Patch(color=colors[col], label=_label))
    plt.legend(handles=info)
    plt.savefig('kMeans.png')
    plt.close()
