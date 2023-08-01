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
    megabytes_array = bytes_array / (1024 * 1024)
    return np.round(megabytes_array, 2)

def main():
    num_users = int(sys.argv[1])
    sizes = read_nums(f'analysis/singleinsert/size_{num_users}.txt')
    mb_sizes = bytes_array_to_megabytes(sizes)
    
    plt.rcParams.update({'font.size': 3 * plt.rcParams['font.size']})
    plt.style.use("fast")

    fig, ax = plt.subplots(figsize=(16, 12))

    bars = ax.bar(np.arange(3), mb_sizes, color=['#F5D65D', '#AFC29E', '#68ADDF'])

    # Set labels and title
    ax.set_xlabel('Podatkovni sistem')
    ax.set_ylabel('Velikost (MB)')
    ax.set_title('')
    ax.set_xticks(np.arange(3))
    ax.set_xticklabels(['Graphenix', 'SQLite', 'MySQL'], rotation=0)
    for bar in bars:
        height = bar.get_height()
        bottom_offset = .5
        ax.text(bar.get_x() + bar.get_width() / 2, bottom_offset, f'{height:.2f} MB', 
                ha='center', va='bottom', fontweight='normal')

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