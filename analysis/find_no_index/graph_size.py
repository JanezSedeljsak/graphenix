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

def main():
    num_users = int(sys.argv[1])
    file_indexed_1 = f'./analysis/find_index/result_{num_users}.txt'
    file_normal_1 =  f'./analysis/find_no_index/result_{num_users}.txt'
    
    no_index = read_nums(file_normal_1) 
    indexed = read_nums(file_indexed_1)

    plt.style.use("fivethirtyeight")
    plt.rcParams.update({'font.size': 2 * plt.rcParams['font.size']})

    x_values = np.arange(3)
    fig, ax = plt.subplots(figsize=(16, 12))
    width = 0.22
    
    ax.bar(x_values - width, no_index, width * 2, label='Brez indeksiranja', color="#D2CC7E")
    ax.bar(x_values + width, indexed, width * 2, label=f'Uporaba B+ drevesa', color="#68ADDF")

    # Set labels and title
    ax.set_xlabel('DBMS + faktor pohitritve')
    ax.set_ylabel('Čas (ms)')
    ax.set_title('')
    ax.set_xticks(x_values)
    speedups = no_index / indexed
    ax.set_xticklabels([
        f'Graphenix\n{speedups[0]:.2f}x\n',
        f'SQLite\n{speedups[1]:.2f}x\n',
        f'MySQL\n{speedups[2]:.2f}x\n'
    ])

    # for bar in bars:
    #     height = more.get_height()
    #     bottom_offset = 0.5
    #     ax.text(more.get_x() + more.get_width() / 2, height + bottom_offset, f'{height:.2f}x', 
    #             ha='center', va='bottom', fontweight='normal')
    #     
    #     iheight = less.get_height()
    #     ax.text(less.get_x() + less.get_width() / 2, iheight + bottom_offset, f'{(iheight):.2f}x', 
    #             ha='center', va='bottom', fontweight='normal')
        
    ax.legend(loc='upper left')
    plt.savefig(f'./analysis/graphs/indexing_speedup_{num_users}.png', bbox_inches='tight')
    plt.close()
    

if __name__ == '__main__':
    main()