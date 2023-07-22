import numpy as np
import matplotlib.pyplot as plt

def read_nums(file_path): 
    try:
        with open(file_path, 'r') as file:
            lines = file.readlines()
            integers = [int(line.strip()) for line in lines]
        return np.array(integers)
    except:
        return []
    
def main():
    nums = read_nums("./stats5.txt")
    
    sort_insert_vec = nums[::3]
    sort_insert_queue = nums[1::3]
    sort_end = nums[2::3]

    plt.rcParams.update({'font.size': 2.5 * plt.rcParams['font.size']})

    plt.style.use("fast")
    x_values = np.arange(500, 25001, 500)
    fig, ax = plt.subplots(figsize=(16, 10))
    

    ax.plot(x_values, sort_insert_queue, label='Uporaba prioritetne vrste')
    ax.plot(x_values, sort_insert_vec, label='Sprotno urejanje')
    ax.plot(x_values, sort_end, label='Urejanje na koncu')

    # Set the Y axis limits from 0 to 2000
    ax.set_ylim(0, 13000)

    # Set labels and title
    ax.set_xlabel('K')
    ax.set_ylabel('Čas (ms)')
    ax.set_title('')
    ax.legend()
    plt.savefig('plot_image.png', bbox_inches='tight')
    plt.close()
    
if __name__ == '__main__':
    main()
