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
    sizes = (100000, 1000000, 10000000)
    files = (f'./analysis/find_no_index/speedup_{s}.txt' for s in sizes)
    matrix = np.zeros((3, len(sizes)))

    for i, (file, size) in enumerate(zip(files, sizes)):
        col = read_nums(file)
        matrix[:, i] = col

    plt.rcParams.update({'font.size': 2.5 * plt.rcParams['font.size']})
    plt.style.use("fast")

    x_values = np.arange(len(sizes))
    fig, ax = plt.subplots(figsize=(16, 12))
    width = 0.125
    
    ax.bar(x_values - width, matrix[0], width, label='Graphenix', color="#EEEC7B")
    ax.bar(x_values, matrix[1], width, label='SQLite', color="#92D6B4")
    ax.bar(x_values + width, matrix[2], width, label='MySQL', color="#35BFEC")

    # Set labels and title
    ax.set_xlabel('Št. zapisov')
    ax.set_ylabel('Faktor pohitritve')
    ax.set_title('')
    ax.set_xticks(x_values)
    ax.set_xticklabels([str(s) for s in sizes])
    ax.legend()
    plt.savefig(f'./analysis/graphs/indexing_speedup.png', bbox_inches='tight')
    plt.close()
    

if __name__ == '__main__':
    main()