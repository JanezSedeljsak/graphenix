import matplotlib.pyplot as plt
import sys
import numpy as np

def read_nums(file_path): 
    try:
        with open(file_path, 'r') as file:
            lines = file.readlines()
            times = [float(line.strip()) for line in lines]
        return np.array(times)
    except Exception as ex:
        print("err", ex)
        return []
    
def bytes_array_to_megabytes(bytes_array):
    megabytes_array = bytes_array / (1024 ** 2)
    return np.round(megabytes_array, 2)

def main():
    num_users = int(sys.argv[1])
    sizes = read_nums(f'analysis/singleinsert/size_{num_users}.txt')
    mb_sizes = bytes_array_to_megabytes(sizes)

    indexed_sizes = read_nums(f'analysis/singleinsert/indexed_size_{num_users}.txt')
    mb_indexed_sizes = bytes_array_to_megabytes(indexed_sizes)
    
    plt.rcParams.update({'font.size': 3 * plt.rcParams['font.size']})
    plt.style.use("fast")

    fig, ax = plt.subplots(figsize=(16, 12))

    ibars = ax.bar(np.arange(3), mb_indexed_sizes, color=['#EEEC7B', '#92D6B4', '#35BFEC'])
    bars = ax.bar(np.arange(3), mb_sizes, color=['#B1B04F', '#649A7C', '#1784A9'])

    

    # Set labels and title
    ax.set_xlabel('Podatkovni sistem')
    ax.set_ylabel('Velikost (MB)')
    ax.set_title('')
    ax.set_xticks(np.arange(3))
    ax.set_xticklabels(['Graphenix', 'SQLite', 'MySQL'], rotation=0)
    for ibar, bar in zip(ibars, bars):
        height = bar.get_height()
        bottom_offset = 2
        ax.text(bar.get_x() + bar.get_width() / 2, bottom_offset, f'{height:.2f} MB', 
                ha='center', va='bottom', fontweight='normal', color="white")
        
        iheight = ibar.get_height()
        ax.text(bar.get_x() + bar.get_width() / 2, 7, f'{(iheight - height):.2f} MB', 
                ha='center', va='bottom', fontweight='normal', color="white")

    plt.savefig(f'./analysis/singleinsert/sizes.png', bbox_inches='tight')
    plt.close()


    

if __name__ == '__main__':
    main()



    import numpy as np


def read_nums(file_path): 
    try:
        with open(file_path, 'r') as file:
            lines = file.readlines()
            times = [float(line.strip()) for line in lines]
        return np.array(times)
    except Exception as ex:
        print("err", ex)
        return []