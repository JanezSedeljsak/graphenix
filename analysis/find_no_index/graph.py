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
    sizes = (10000, 100000, 1000000)
    ifiles = (f'./analysis/find_no_index/iresult_{s}.txt' for s in sizes)
    files = (f'./analysis/find_no_index/result_{s}.txt' for s in sizes)
    matrix = np.zeros((6, len(sizes)))

    for i, (file, ifile) in enumerate(zip(files, ifiles)):
        col = read_nums(file)
        icol = read_nums(ifile)
        vals = np.concatenate((col, icol), axis=0)
        matrix[:, i] = vals

    plt.rcParams.update({'font.size': 2.5 * plt.rcParams['font.size']})
    plt.style.use("fast")

    x_values = np.arange(len(sizes))
    fig, ax = plt.subplots(figsize=(16, 12))
    width = 0.125
    hw = width / 2

    ax.bar(x_values - hw - width * 2, matrix[0], width, label='Graphenix', color="#F5D65D")
    ax.bar(x_values - hw - width, matrix[3], width, label='Graphenix & index', color="#D9CE77")
    ax.bar(x_values - hw, matrix[1], width, label='SQLite', color="#BDC691")
    ax.bar(x_values + hw, matrix[4], width, label='SQLite & index', color="#A0BDAB")
    ax.bar(x_values + hw + width, matrix[2], width, label='MySQL', color="#84B5C5")
    ax.bar(x_values + hw + width * 2, matrix[5], width, label='MySQL & index', color="#68ADDF")

    # Set labels and title
    ax.set_xlabel('Št. zapisov')
    ax.set_ylabel('Čas (ms)')
    ax.set_title('')
    ax.set_xticks(x_values)
    ax.set_xticklabels([str(s) for s in sizes])
    ax.legend()
    plt.savefig(f'./analysis/find_no_index/indexing.png', bbox_inches='tight')
    plt.close()
    

if __name__ == '__main__':
    main()