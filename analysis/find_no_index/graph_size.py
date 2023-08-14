import numpy as np
import matplotlib.pyplot as plt
import sys

def read_nums(file_path): 
    try:
        with open(file_path, 'r') as file:
            lines = file.readlines()
            times = [float(line.strip()) for line in lines]
        return np.array(times)
    except Exception as ex:
        print("err", ex)
        return []

sizes = {
    1000000: [131, 14_197],
    10000000: [1308, 140_758],
}

def main():
    num_users = int(sys.argv[1])
    file_indexed_1 = f'./analysis/find_index/result_{num_users}.txt'
    file_normal_1 =  f'./analysis/find_no_index/result_{num_users}.txt'
    
    speedups = read_nums(file_normal_1) / read_nums(file_indexed_1)
    plt.rcParams.update({'font.size': 2.5 * plt.rcParams['font.size']})
    plt.style.use("fast")

    x_values = np.arange(3)
    fig, ax = plt.subplots(figsize=(16, 12))
    width = 0.45
    
    bars = ax.bar(x_values, speedups, width, label=f'{more_size} - 1.25 %', color="#EEEC7B")

    # Set labels and title
    ax.set_xlabel('DBMS')
    ax.set_ylabel('Faktor pohitritve')
    ax.set_title('')
    ax.set_xticks(x_values)
    ax.set_xticklabels(['Graphenix', 'SQLite', 'MySQL'])

    for bar in bars:
        height = more.get_height()
        bottom_offset = 0.5
        ax.text(more.get_x() + more.get_width() / 2, height + bottom_offset, f'{height:.2f}x', 
                ha='center', va='bottom', fontweight='normal')
        
        iheight = less.get_height()
        ax.text(less.get_x() + less.get_width() / 2, iheight + bottom_offset, f'{(iheight):.2f}x', 
                ha='center', va='bottom', fontweight='normal')
        
    ax.legend(loc='upper left')
    plt.savefig(f'./analysis/graphs/indexing_speedup_{num_users}.png', bbox_inches='tight')
    plt.close()
    

if __name__ == '__main__':
    main()