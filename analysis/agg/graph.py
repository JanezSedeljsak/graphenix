import numpy as np
import matplotlib.pyplot as plt

def read_nums(file_path): 
    try:
        with open(file_path, 'r') as file:
            lines = file.readlines()
            times = [float(line.strip()) for line in lines]
        return np.array(times)
    except Exception as ex:
        print("err", ex)
        return []
    

def main():
    sizes = (1000, 5000, 10000, 20000)
    files = (f'./analysis/agg/result_{s}.txt' for s in sizes)
    matrix = np.zeros((5, len(sizes)))

    for i, (file, size) in enumerate(zip(files, sizes)):
        col = read_nums(file)
        matrix[:, i] = col

    plt.style.use("fivethirtyeight")
    plt.rcParams['figure.facecolor'] = 'white'
    plt.rcParams['axes.facecolor'] = 'white'
    plt.rcParams['axes.edgecolor'] = 'white'
    plt.rcParams.update({'font.size': 2 * plt.rcParams['font.size']})

    x_values = np.arange(len(sizes))
    fig, ax = plt.subplots(figsize=(16, 12))
    width = 0.15

    ax.bar(x_values - 2*width, matrix[0], width, label='Graphenix', color="#F5D65D")
    ax.bar(x_values - width, matrix[3], width, label='SQLite', color="#D2CC7E")
    ax.bar(x_values, matrix[4], width, label='MySQL', color="#AFC29E")
    ax.bar(x_values + width, matrix[1], width, label='SQLite ORM', color="#8BB7BF")
    ax.bar(x_values + 2*width, matrix[2], width, label='MySQL ORM', color="#68ADDF")

    # Set labels and title
    ax.set_xlabel('Št. zapisov')
    ax.set_ylabel('Čas (ms)')
    ax.set_title('')
    ax.set_xticks(x_values)
    ax.set_xticklabels([str(s) for s in sizes], rotation=0)
    ax.legend()
    plt.savefig(f'./analysis/graphs/agg.png', bbox_inches='tight', facecolor='white')
    plt.close()


if __name__ == '__main__':
    main()