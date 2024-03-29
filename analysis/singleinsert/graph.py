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
    sizes = (5000, 10000, 50000, 100000, 150000)
    files = (f'./analysis/singleinsert/result_{s}.txt' for s in sizes)
    matrix = np.zeros((6, len(sizes)))

    for i, (file, size) in enumerate(zip(files, sizes)):
        col = read_nums(file)
        matrix[:, i] = (col * 1000) / size # time for 1000 inserts

    plt.style.use("fivethirtyeight")
    plt.rcParams['figure.facecolor'] = 'white'
    plt.rcParams['axes.facecolor'] = 'white'
    plt.rcParams['axes.edgecolor'] = 'white'

    plt.rcParams.update({'font.size': 1.2 * plt.rcParams['font.size']})

    x_values = np.arange(len(sizes))
    fig, ax = plt.subplots(figsize=(10, 7))

    ax.plot(x_values, matrix[0], ':', label='Graphenix', marker='o', markersize=10, linewidth=2, dashes=(2, 7))
    ax.plot(x_values, matrix[5], ':', label='Graphenix množično', marker='o', markersize=10, linewidth=2, dashes=(2, 7))
    ax.plot(x_values, matrix[1], ':', label='SQLite ORM', marker='o', markersize=10, linewidth=2, dashes=(2, 7))
    ax.plot(x_values, matrix[2], ':', label='MySQL ORM', marker='o', markersize=10, linewidth=2, dashes=(2, 7))
    ax.plot(x_values, matrix[3], ':', label='SQLite', marker='o', markersize=10, linewidth=2, dashes=(2, 7))
    ax.plot(x_values, matrix[4], ':', label='MySQL', marker='o', markersize=10, linewidth=2, dashes=(2, 7))

    # Set labels and title
    ax.set_xlabel('Št. zapisov')
    ax.set_ylabel('Čas (ms)')
    ax.set_title('')
    ax.set_xticks(x_values)
    ax.set_xticklabels([str(s) for s in sizes], rotation=0)
    ax.legend(loc='upper center', bbox_to_anchor=(0.5, 1.2), ncol=3)
    plt.savefig(f'./analysis/graphs/singleinsert.png', bbox_inches='tight', facecolor='white')
    plt.close()


if __name__ == '__main__':
    main()